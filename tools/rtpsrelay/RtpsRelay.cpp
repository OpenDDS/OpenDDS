/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "PublicationListener.h"
#include "ReaderListener.h"
#include "RelayHandler.h"
#include "StatisticsHandler.h"
#include "SubscriptionListener.h"
#include "WriterListener.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>

#include <dds/DCPS/transport/framework/NetworkAddress.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/Reactor.h>
#include <ace/Select_Reactor.h>

#include <cstdlib>
#include <iostream>

#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/security/framework/SecurityRegistry.h>

namespace {
  void append(DDS::PropertySeq& props, const char* name, const std::string& value, bool propagate = false)
  {
    const DDS::Property_t prop = {name, value.c_str(), propagate};
    const unsigned int len = props.length();
    props.length(len + 1);
    try {
      props[len] = prop;
    } catch (const CORBA::BAD_PARAM& ex) {
      ACE_ERROR((LM_ERROR, "Exception caught when appending parameter\n"));
    }
  }
}

#endif

using namespace RtpsRelay;

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
  DDS::DomainId_t relay_domain = 0;
  DDS::DomainId_t application_domain = 1;
  ACE_INET_Addr nic_horizontal, nic_vertical;
  OpenDDS::DCPS::TimeDuration lifespan(60);   // 1 minute

#ifdef OPENDDS_SECURITY
  std::string identity_ca_file;
  std::string permissions_ca_file;
  std::string identity_certificate_file;
  std::string identity_key_file;
  std::string governance_file;
  std::string permissions_file;
  bool secure = false;
  const std::string file = "file:";
#endif

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
    } else if ((arg = args.get_the_parameter("-RelayDomain"))) {
      relay_domain = ACE_OS::atoi(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-ApplicationDomain"))) {
      application_domain = ACE_OS::atoi(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Lifespan"))) {
      lifespan = OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg));
      args.consume_arg();
#ifdef OPENDDS_SECURITY
    } else if ((arg = args.get_the_parameter("-IdentityCA"))) {
      identity_ca_file = file + arg;
      secure = true;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PermissionsCA"))) {
      permissions_ca_file = file + arg;
      secure = true;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-IdentityCertificate"))) {
      identity_certificate_file = file + arg;
      secure = true;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-IdentityKey"))) {
      identity_key_file = file + arg;
      secure = true;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Goverance"))) {
      governance_file = file + arg;
      secure = true;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Permissions"))) {
      permissions_file = file + arg;
      secure = true;
      args.consume_arg();
#endif
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

#ifdef OPENDDS_SECURITY
  if (secure) {
    if (identity_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -IdentityCA is empty\n"));
      return EXIT_FAILURE;
    }
    if (permissions_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -PermissionsCA is empty\n"));
      return EXIT_FAILURE;
    }
    if (identity_certificate_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -IdentityCertificate is empty\n"));
      return EXIT_FAILURE;
    }
    if (identity_key_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -IdentityKey is empty\n"));
      return EXIT_FAILURE;
    }
    if (governance_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -Governance is empty\n"));
      return EXIT_FAILURE;
    }
    if (permissions_file.empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: -Permissions is empty\n"));
      return EXIT_FAILURE;
    }
  }
#endif

  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  if (!factory) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to initialize participant factory\n"));
    return EXIT_FAILURE;
  }

#ifdef OPENDDS_SECURITY
  if (secure && !TheServiceParticipant->get_security()) {
    ACE_ERROR((LM_ERROR, "ERROR: Security documents provided but security is not enabled\n"));
    return EXIT_FAILURE;
  }
#endif

  DDS::DomainParticipantQos participant_qos;
  factory->get_default_participant_qos(participant_qos);

#ifdef OPENDDS_SECURITY
  if (secure) {
    DDS::PropertySeq& properties = participant_qos.property.value;
    append(properties, DDS::Security::Properties::AuthIdentityCA, identity_ca_file);
    append(properties, DDS::Security::Properties::AccessPermissionsCA, permissions_ca_file);
    append(properties, DDS::Security::Properties::AuthIdentityCertificate, identity_certificate_file);
    append(properties, DDS::Security::Properties::AuthPrivateKey, identity_key_file);
    append(properties, DDS::Security::Properties::AccessGovernance, governance_file);
    append(properties, DDS::Security::Properties::AccessPermissions, permissions_file);
  }
#endif

  DDS::DomainParticipant_var application_participant = factory->create_participant(application_domain, participant_qos, nullptr,
                                                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!application_participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to create application participant\n"));
    return EXIT_FAILURE;
  }

  OpenDDS::DCPS::DomainParticipantImpl* application_participant_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(application_participant.in());
  if (application_participant_impl == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to get application participant impl\n"));
    return EXIT_FAILURE;
  }

  factory->get_default_participant_qos(participant_qos);
  DDS::DomainParticipant_var relay_participant = factory->create_participant(relay_domain, participant_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to create relay participant\n"));
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

  ACE_Reactor reactor_(new ACE_Select_Reactor, true);
  const auto reactor = &reactor_;

  RelayAddresses relay_addresses {
    addr_to_string(spdp_horizontal_addr),
    addr_to_string(sedp_horizontal_addr),
    addr_to_string(data_horizontal_addr)
  };

  AssociationTable association_table(relay_addresses);

  HorizontalHandler spdp_horizontal_handler(reactor, association_table);
  HorizontalHandler sedp_horizontal_handler(reactor, association_table);
  HorizontalHandler data_horizontal_handler(reactor, association_table);

  const auto application_participant_id = application_participant_impl->get_id();

  const auto discovery = TheServiceParticipant->get_discovery(application_domain);
  const auto rtps_discovery = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(discovery);

  ACE_INET_Addr spdp(rtps_discovery->get_spdp_port(application_domain, application_participant_id), "127.0.0.1");
  ACE_INET_Addr sedp(rtps_discovery->get_sedp_port(application_domain, application_participant_id), "127.0.0.1");

#ifdef OPENDDS_SECURITY
  OpenDDS::Security::SecurityConfig_rch conf = TheSecurityRegistry->default_config();
  DDS::Security::CryptoTransform_var crypto = conf->get_crypto_transform();
#else
  const int crypto = 0;
#endif

  SpdpHandler spdp_vertical_handler(reactor, relay_addresses, association_table, lifespan, rtps_discovery, application_domain, application_participant_id, crypto, spdp);
  SedpHandler sedp_vertical_handler(reactor, relay_addresses, association_table, lifespan, rtps_discovery, application_domain, application_participant_id, crypto, sedp);
  DataHandler data_vertical_handler(reactor, relay_addresses, association_table, lifespan, rtps_discovery, application_domain, application_participant_id, crypto);

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

  DDS::Subscriber_var bit_subscriber = application_participant->get_builtin_subscriber();
  {
    ReaderEntryTypeSupportImpl::_var_type reader_entry_ts =
      new ReaderEntryTypeSupportImpl;
    if (reader_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to register ReaderEntry type\n"));
      return EXIT_FAILURE;
    }

    CORBA::String_var type_name = reader_entry_ts->get_type_name();
    DDS::Topic_var topic = relay_participant->create_topic("Readers", type_name,
                                                           TOPIC_QOS_DEFAULT, nullptr,
                                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Readers topic\n"));
      return EXIT_FAILURE;
    }

    DDS::Publisher_var publisher = relay_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader publisher\n"));
      return EXIT_FAILURE;
    }

    DDS::Subscriber_var subscriber = relay_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader subscriber\n"));
      return EXIT_FAILURE;
    }

    DDS::DataWriterQos writer_qos;
    publisher->get_default_datawriter_qos(writer_qos);

    writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::DataWriter_var writer = publisher->create_datawriter(topic, writer_qos, nullptr,
                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader data writer\n"));
      return EXIT_FAILURE;
    }

    ReaderEntryDataWriter_ptr reader_writer = ReaderEntryDataWriter::_narrow(writer);

    if (!reader_writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Reader data writer\n"));
      return EXIT_FAILURE;
    }

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);

    reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader = subscriber->create_datareader(topic, reader_qos,
                                                               new ReaderListener(association_table, spdp_vertical_handler),
                                                               DDS::DATA_AVAILABLE_STATUS);

    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader data reader\n"));
      return EXIT_FAILURE;
    }

    DDS::DataReader_var dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
    DDS::DataReaderListener_var subscription_listener(new SubscriptionListener(application_participant_impl,
                                                                               reader_writer,
                                                                               relay_addresses));
    DDS::ReturnCode_t ret = dr->set_listener(subscription_listener, DDS::DATA_AVAILABLE_STATUS);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to set listener on SubscriptionBuiltinTopicDataDataReader\n"));
      return EXIT_FAILURE;
    }
  }

  {
    WriterEntryTypeSupportImpl::_var_type writer_entry_ts =
      new WriterEntryTypeSupportImpl;
    if (writer_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to register WriterEntry type\n"));
      return EXIT_FAILURE;
    }

    CORBA::String_var type_name = writer_entry_ts->get_type_name();
    DDS::Topic_var topic = relay_participant->create_topic("Writers", type_name,
                                                           TOPIC_QOS_DEFAULT, nullptr,
                                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!topic) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writers topic\n"));
      return EXIT_FAILURE;
    }

    DDS::Publisher_var publisher = relay_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!publisher) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer publisher\n"));
      return EXIT_FAILURE;
    }

    DDS::Subscriber_var subscriber = relay_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!subscriber) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer subscriber\n"));
      return EXIT_FAILURE;
    }

    DDS::DataWriterQos writer_qos;
    publisher->get_default_datawriter_qos(writer_qos);

    writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    DDS::DataWriter_var writer = publisher->create_datawriter(topic, writer_qos, nullptr,
                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (!writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer data writer\n"));
      return EXIT_FAILURE;
    }

    WriterEntryDataWriter_ptr writer_writer = WriterEntryDataWriter::_narrow(writer);

    if (!writer_writer) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Writer data writer\n"));
      return EXIT_FAILURE;
    }

    DDS::DataReaderQos reader_qos;
    subscriber->get_default_datareader_qos(reader_qos);

    reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    DDS::DataReader_var reader = subscriber->create_datareader(topic, reader_qos,
                                                               new WriterListener(association_table, spdp_vertical_handler),
                                                               DDS::DATA_AVAILABLE_STATUS);
    if (!reader) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer data reader\n"));
      return EXIT_FAILURE;
    }

    DDS::DataReader_var dr = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    DDS::DataReaderListener_var publication_listener(new PublicationListener(application_participant_impl,
                                                                             writer_writer,
                                                                             relay_addresses));
    DDS::ReturnCode_t ret = dr->set_listener(publication_listener, DDS::DATA_AVAILABLE_STATUS);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to set listener on PublicationBuiltinTopicDataDataReader\n"));
      return EXIT_FAILURE;
    }
  }

  StatisticsHandler statistics_h(reactor,
                                 &spdp_vertical_handler, &spdp_horizontal_handler,
                                 &sedp_vertical_handler, &sedp_horizontal_handler,
                                 &data_vertical_handler, &data_horizontal_handler);
  statistics_h.open();

  rtps_discovery->schedule_send(application_domain, application_participant_id, OpenDDS::DCPS::TimeDuration(1));

  reactor->run_reactor_event_loop();

  return EXIT_SUCCESS;
}
