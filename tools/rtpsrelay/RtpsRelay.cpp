/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "GroupTable.h"
#include "RelayHandler.h"
#include "RoutingTable.h"
#include "StatisticsHandler.h"

#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/Reactor.h>

#include <cstdlib>
#include <iostream>

namespace {
  ACE_INET_Addr get_bind_addr(unsigned short port)
  {
    std::vector<ACE_INET_Addr> nics;
    OpenDDS::DCPS::get_interface_addrs(nics);

    for (auto nic : nics) {
      if (nic.is_loopback()) {
        continue;
      }

      nic.set_port_number(port);
      return nic;
    }
    return ACE_INET_Addr();
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainId_t domain = 0;
  ACE_INET_Addr nic_horizontal, nic_vertical;
  ACE_Time_Value renew_after(60); // 1 minute
  ACE_Time_Value lifespan(300);   // 5 minutes

  ACE_Argv_Type_Converter atc(argc, argv);
  ACE_Arg_Shifter_T<char> args(atc.get_argc(), atc.get_ASCII_argv());
  while (args.is_anything_left()) {
    const char* arg = nullptr;

    if ((arg = args.get_the_parameter("-HorizontalAddress"))) {
      nic_horizontal = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-VerticalAddress"))) {
      nic_vertical = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Domain"))) {
      domain = ACE_OS::atoi(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-RenewAfter"))) {
      renew_after = ACE_Time_Value(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Lifespan"))) {
      lifespan = ACE_Time_Value(ACE_OS::atoi(arg));
      args.consume_arg();
    } else {
      args.ignore_arg();
    }
  }

  if (nic_horizontal == ACE_INET_Addr()) {
    nic_horizontal = get_bind_addr(11444);
  }

  if (nic_vertical == ACE_INET_Addr()) {
    nic_vertical = ACE_INET_Addr(4444);
  }

  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipantQos qos;
  factory->get_default_participant_qos(qos);

  if (!factory) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize participant factory\n"));
    return EXIT_FAILURE;
  }

  DDS::DomainParticipant_var participant = factory->create_participant(domain, qos, nullptr,
                                                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to create participant\n"));
    return EXIT_FAILURE;
  }

  GroupTable group_table(renew_after, lifespan);
  RoutingTable spdp_table(renew_after, lifespan);
  RoutingTable sedp_table(renew_after, lifespan);
  RoutingTable data_table(renew_after, lifespan);

  if (!group_table.initialize(participant, "Group Info")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize group table\n"));
    return EXIT_FAILURE;
  }

  if (!spdp_table.initialize(participant, "Spdp Routing Info")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize SPDP routing table\n"));
    return EXIT_FAILURE;
  }

  if (!sedp_table.initialize(participant, "Sedp Routing Info")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize SEDP routing table\n"));
    return EXIT_FAILURE;
  }

  if (!data_table.initialize(participant, "Data Routing Info")) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize data routing table\n"));
    return EXIT_FAILURE;
  }

  const auto addr_horizontal = nic_horizontal.get_ip_address();
  auto port_horizontal = nic_horizontal.get_port_number();
  const ACE_INET_Addr spdp_horizontal_addr(port_horizontal++, addr_horizontal);
  const ACE_INET_Addr sedp_horizontal_addr(port_horizontal++, addr_horizontal);
  const ACE_INET_Addr data_horizontal_addr(port_horizontal++, addr_horizontal);

  const auto addr_vertical = nic_vertical.get_ip_address();
  auto port_vertical = nic_vertical.get_port_number();
  const ACE_INET_Addr spdp_vertical_addr(port_vertical++, addr_vertical);
  const ACE_INET_Addr sedp_vertical_addr(port_vertical++, addr_vertical);
  const ACE_INET_Addr data_vertical_addr(port_vertical++, addr_vertical);

  const auto reactor = ACE_Reactor::instance();

  HorizontalHandler spdp_horizontal_handler(reactor, group_table, spdp_table);
  HorizontalHandler sedp_horizontal_handler(reactor, group_table, sedp_table);
  HorizontalHandler data_horizontal_handler(reactor, group_table, data_table);

  SpdpHandler spdp_vertical_handler(reactor, group_table, spdp_table);
  VerticalHandler sedp_vertical_handler(reactor, group_table, sedp_table);
  VerticalHandler data_vertical_handler(reactor, group_table, data_table);

  spdp_horizontal_handler.vertical_handler(&spdp_vertical_handler);
  sedp_horizontal_handler.vertical_handler(&sedp_vertical_handler);
  data_horizontal_handler.vertical_handler(&data_vertical_handler);

  spdp_vertical_handler.horizontal_handler(&spdp_horizontal_handler);
  sedp_vertical_handler.horizontal_handler(&sedp_horizontal_handler);
  data_vertical_handler.horizontal_handler(&data_horizontal_handler);

  spdp_horizontal_handler.open(spdp_horizontal_addr);
  sedp_horizontal_handler.open(sedp_horizontal_addr);
  data_horizontal_handler.open(data_horizontal_addr);

  spdp_vertical_handler.open(spdp_vertical_addr);
  sedp_vertical_handler.open(sedp_vertical_addr);
  data_vertical_handler.open(data_vertical_addr);

  std::cout << "SPDP Horizontal listening on " << spdp_horizontal_handler.relay_address() << '\n'
    << "SEDP Horizontal listening on " << sedp_horizontal_handler.relay_address() << '\n'
    << "Data Horizontal listening on " << data_horizontal_handler.relay_address() << '\n'
    << "SPDP Vertical listening on " << spdp_vertical_handler.relay_address() << '\n'
    << "SEDP Vertical listening on " << sedp_vertical_handler.relay_address() << '\n'
    << "Data Vertical listening on " << data_vertical_handler.relay_address() << std::endl;

  StatisticsHandler statistics_h(reactor,
                                 &spdp_vertical_handler, &spdp_horizontal_handler,
                                 &sedp_vertical_handler, &sedp_horizontal_handler,
                                 &data_vertical_handler, &data_horizontal_handler);
  statistics_h.open();

  reactor->run_reactor_event_loop();

  return EXIT_SUCCESS;
}
