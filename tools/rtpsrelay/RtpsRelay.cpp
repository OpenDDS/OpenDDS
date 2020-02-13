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
#endif

using namespace RtpsRelay;

namespace {
  void append(DDS::PropertySeq& props, const char* name, const std::string& value, bool propagate = false)
  {
    const DDS::Property_t prop = {name, value.c_str(), propagate};
    const unsigned int len = props.length();
    props.length(len + 1);
    try {
      props[len] = prop;
    } catch (const CORBA::BAD_PARAM& /*ex*/) {
      ACE_ERROR((LM_ERROR, "Exception caught when appending parameter\n"));
    }
  }

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
  unsigned short stun_port = 3478;
  std::string user_data;

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
    } else if ((arg = args.get_the_parameter("-StunPort"))) {
      stun_port = ACE_OS::atoi(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-UserData"))) {
      user_data = arg;
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
    } else if ((arg = args.get_the_parameter("-Governance"))) {
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
  participant_qos.user_data.value.length(static_cast<CORBA::ULong>(user_data.length()));
  std::memcpy(participant_qos.user_data.value.get_buffer(), user_data.data(), user_data.length());

  DDS::PropertySeq& properties = participant_qos.property.value;
  append(properties, OpenDDS::RTPS::RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS, "false");

#ifdef OPENDDS_SECURITY
  if (secure) {
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

  ACE_INET_Addr stun_addr = nic_vertical;
  stun_addr.set_port_number(stun_port);

  ACE_Reactor reactor_(new ACE_Select_Reactor, true);
  const auto reactor = &reactor_;

  RelayAddresses relay_addresses {
    addr_to_string(spdp_horizontal_addr),
    addr_to_string(sedp_horizontal_addr),
    addr_to_string(data_horizontal_addr)
  };

  AssociationTable association_table;

  HorizontalHandler spdp_horizontal_handler(reactor);
  HorizontalHandler sedp_horizontal_handler(reactor);
  HorizontalHandler data_horizontal_handler(reactor);

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

  DDS::Subscriber_var bit_subscriber = application_participant->get_builtin_subscriber();

  GuidRelayAddressesTypeSupport_var guid_relay_addresses_ts =
    new GuidRelayAddressesTypeSupportImpl;
  if (guid_relay_addresses_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to register GuidRelayAddresses type\n"));
    return EXIT_FAILURE;
  }

  DDS::Topic_var responsible_relay_topic =
    relay_participant->create_topic("Responsible Relay",
                                    guid_relay_addresses_ts->get_type_name(),
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!responsible_relay_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Responsible Relay topic\n"));
    return EXIT_FAILURE;
  }

  ReaderEntryTypeSupport_var reader_entry_ts =
    new ReaderEntryTypeSupportImpl;
  if (reader_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to register ReaderEntry type\n"));
    return EXIT_FAILURE;
  }

  DDS::Topic_var readers_topic = relay_participant->create_topic("Readers", reader_entry_ts->get_type_name(),
                                                                 TOPIC_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!readers_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Readers topic\n"));
    return EXIT_FAILURE;
  }

  WriterEntryTypeSupport_var writer_entry_ts =
    new WriterEntryTypeSupportImpl;
  if (writer_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to register WriterEntry type\n"));
    return EXIT_FAILURE;
  }

  DDS::Topic_var writers_topic = relay_participant->create_topic("Writers", writer_entry_ts->get_type_name(),
                                                                 TOPIC_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!writers_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writers topic\n"));
    return EXIT_FAILURE;
  }

  DDS::Publisher_var relay_publisher = relay_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_publisher) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Relay publisher\n"));
    return EXIT_FAILURE;
  }

  DDS::Subscriber_var relay_subscriber = relay_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_subscriber) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Relay subscriber\n"));
    return EXIT_FAILURE;
  }

  DDS::DataWriterQos writer_qos;
  relay_publisher->get_default_datawriter_qos(writer_qos);

  writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderQos reader_qos;
  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataWriter_var responsible_relay_writer_var =
    relay_publisher->create_datawriter(responsible_relay_topic, writer_qos, nullptr,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!responsible_relay_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Responsible Relay data writer\n"));
    return EXIT_FAILURE;
  }

  GuidRelayAddressesDataWriter_ptr responsible_relay_writer = GuidRelayAddressesDataWriter::_narrow(responsible_relay_writer_var);

  if (!responsible_relay_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Responsible Relay data writer\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReader_var responsible_relay_reader_var =
    relay_subscriber->create_datareader(responsible_relay_topic, reader_qos, nullptr, 0);
  if (!responsible_relay_reader_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reponsible Relay data reader\n"));
    return EXIT_FAILURE;
  }

  GuidRelayAddressesDataReader_ptr responsible_relay_reader = GuidRelayAddressesDataReader::_narrow(responsible_relay_reader_var);

  if (!responsible_relay_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Responsible Relay data reader\n"));
    return EXIT_FAILURE;
  }

  SpdpHandler spdp_vertical_handler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_id, crypto, spdp);
  SedpHandler sedp_vertical_handler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_id, crypto, sedp);
  DataHandler data_vertical_handler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_id, crypto);

#ifdef OPENDDS_SECURITY
  StunHandler stun_handler(reactor);
#endif

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

#ifdef OPENDDS_SECURITY
  stun_handler.open(stun_addr);
#endif

  std::cout << "SPDP Horizontal listening on " << addr_to_string(spdp_horizontal_handler.relay_address()) << '\n'
            << "SEDP Horizontal listening on " << addr_to_string(sedp_horizontal_handler.relay_address()) << '\n'
            << "Data Horizontal listening on " << addr_to_string(data_horizontal_handler.relay_address()) << '\n'
            << "SPDP Vertical listening on " << addr_to_string(spdp_vertical_handler.relay_address()) << '\n'
            << "SEDP Vertical listening on " << addr_to_string(sedp_vertical_handler.relay_address()) << '\n'
            << "Data Vertical listening on " << addr_to_string(data_vertical_handler.relay_address()) << '\n'
#ifdef OPENDDS_SECURITY
            << "STUN listening on " << addr_to_string(stun_handler.relay_address()) << std::endl;
#else
    ;
#endif

  DDS::DataWriter_var reader_writer_var = relay_publisher->create_datawriter(readers_topic, writer_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!reader_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader data writer\n"));
    return EXIT_FAILURE;
  }

  ReaderEntryDataWriter_ptr reader_writer = ReaderEntryDataWriter::_narrow(reader_writer_var);

  if (!reader_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Reader data writer\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var reader_listener =
    new ReaderListener(association_table, spdp_vertical_handler);
  DDS::DataReader_var reader_reader_var = relay_subscriber->create_datareader(readers_topic, reader_qos,
                                                                              reader_listener,
                                                                              DDS::DATA_AVAILABLE_STATUS);

  if (!reader_reader_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Reader data reader\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReader_var subscription_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  DDS::DataReaderListener_var subscription_listener =
    new SubscriptionListener(application_participant_impl, reader_writer);
  DDS::ReturnCode_t ret = subscription_reader->set_listener(subscription_listener, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to set listener on SubscriptionBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }

  DDS::DataWriter_var writer_writer_var = relay_publisher->create_datawriter(writers_topic, writer_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!writer_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer data writer\n"));
    return EXIT_FAILURE;
  }

  WriterEntryDataWriter_ptr writer_writer = WriterEntryDataWriter::_narrow(writer_writer_var);

  if (!writer_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to narrow Writer data writer\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var writer_listener =
    new WriterListener(association_table, spdp_vertical_handler);
  DDS::DataReader_var writer_reader = relay_subscriber->create_datareader(writers_topic, reader_qos,
                                                                          writer_listener,
                                                                          DDS::DATA_AVAILABLE_STATUS);
  if (!writer_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: failed to create Writer data reader\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReader_var publication_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
  DDS::DataReaderListener_var publication_listener(new PublicationListener(application_participant_impl,
                                                                           writer_writer));
  ret = publication_reader->set_listener(publication_listener, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Failed to set listener on PublicationBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }

  StatisticsHandler statistics_h(reactor,
                                 &spdp_vertical_handler, &spdp_horizontal_handler,
                                 &sedp_vertical_handler, &sedp_horizontal_handler,
                                 &data_vertical_handler, &data_horizontal_handler);
  statistics_h.open();

  reactor->run_reactor_event_loop();

  return EXIT_SUCCESS;
}
