/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <iostream>

#include <ace/Arg_Shifter.h>
#include <ace/Reactor.h>
#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include "GroupTable.h"
#include "RelayHandler.h"
#include "RoutingTable.h"
#include "StatisticsHandler.h"

namespace {
  ACE_INET_Addr get_bind_addr(unsigned short port) {
    OPENDDS_VECTOR(ACE_INET_Addr) NICs;
    OpenDDS::DCPS::get_interface_addrs(NICs);

    for (auto NIC : NICs) {
      if (NIC.is_loopback()) {
        continue;
      }

      NIC.set_port_number(port);
      return NIC;
    }
    return ACE_INET_Addr();
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[]) {
  DDS::DomainId_t domain = 0;
  ACE_INET_Addr NIC_horizontal, NIC_vertical;
  ACE_Time_Value renew_after(60); // 1 minute
  ACE_Time_Value lifespan(300);   // 5 minutes

  ACE_Arg_Shifter args(argc, argv);
  while (args.is_anything_left()) {
    const char* arg = nullptr;

    if ((arg = args.get_the_parameter("-HorizontalAddress"))) {
      NIC_horizontal = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-VerticalAddress"))) {
      NIC_vertical = ACE_INET_Addr(arg);
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

  if (NIC_horizontal == ACE_INET_Addr()) {
    NIC_horizontal = ACE_INET_Addr(get_bind_addr(11444));
  }

  if (NIC_vertical == ACE_INET_Addr()) {
    NIC_vertical = ACE_INET_Addr(get_bind_addr(4444));
  }

  DDS::DomainParticipantQos qos;

  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  factory->get_default_participant_qos(qos);

  if (! factory) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize participant factory\n"));
    return -1;
  }

  DDS::DomainParticipant_var participant = factory->create_participant(
                                                                       domain,
                                                                       qos,
                                                                       nullptr,
                                                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (! participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to create participant\n"));
    return -1;
  }

  GroupTable group_table(renew_after, lifespan);
  RoutingTable spdp_table(renew_after, lifespan);
  RoutingTable sedp_table(renew_after, lifespan);
  RoutingTable data_table(renew_after, lifespan);

  int err;

  err = group_table.initialize(participant, "Group Info");

  if (err) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize group table\n"));
    return err;
  }

  err = spdp_table.initialize(participant, "Spdp Routing Info");

  if (err) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize SPDP routing table\n"));
    return err;
  }

  err = sedp_table.initialize(participant, "Sedp Routing Info");

  if (err) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize SEDP routing table\n"));
    return err;
  }

  err = data_table.initialize(participant, "Data Routing Info");

  if (err) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize data routing table\n"));
    return err;
  }

  auto addr_horizontal = NIC_horizontal.get_ip_address();
  auto port_horizontal = NIC_horizontal.get_port_number();
  ACE_INET_Addr spdp_horizontal_addr(port_horizontal++, addr_horizontal);
  ACE_INET_Addr sedp_horizontal_addr(port_horizontal++, addr_horizontal);
  ACE_INET_Addr data_horizontal_addr(port_horizontal++, addr_horizontal);

  auto addr_vertical = NIC_vertical.get_ip_address();
  auto port_vertical = NIC_vertical.get_port_number();
  ACE_INET_Addr spdp_vertical_addr(port_vertical++, addr_vertical);
  ACE_INET_Addr sedp_vertical_addr(port_vertical++, addr_vertical);
  ACE_INET_Addr data_vertical_addr(port_vertical++, addr_vertical);

  ACE_Reactor* reactor = ACE_Reactor::instance();

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

  std::cout << "SPDP Horizontal listening on " << spdp_horizontal_handler.relay_address() << '\n';
  std::cout << "SEDP Horizontal listening on " << sedp_horizontal_handler.relay_address() << '\n';
  std::cout << "Data Horizontal listening on " << data_horizontal_handler.relay_address() << '\n';
  std::cout << "SPDP Vertical listening on " << spdp_vertical_handler.relay_address() << '\n';
  std::cout << "SEDP Vertical listening on " << sedp_vertical_handler.relay_address() << '\n';
  std::cout << "Data Vertical listening on " << data_vertical_handler.relay_address() << std::endl;

  StatisticsHandler statistics_h(reactor,
                                 &spdp_vertical_handler, &spdp_horizontal_handler,
                                 &sedp_vertical_handler, &sedp_horizontal_handler,
                                 &data_vertical_handler, &data_horizontal_handler);
  statistics_h.open();

  reactor->run_reactor_event_loop();

  return 0;
}
