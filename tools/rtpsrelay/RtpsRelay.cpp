/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DomainStatisticsReporter.h"
#include "ParticipantEntryListener.h"
#include "ParticipantListener.h"
#include "ParticipantStatisticsReporter.h"
#include "PublicationListener.h"
#include "ReaderListener.h"
#include "RelayHandler.h"
#include "RelayStatisticsReporter.h"
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
#include <algorithm>

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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Exception caught when appending parameter\n")));
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
  ACE_INET_Addr nic_horizontal, nic_vertical;
  std::string user_data;
  Config config;

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

  int argc_copy = argc;
  ACE_Argv_Type_Converter atc(argc_copy, argv);
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
      config.application_domain(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-Lifespan"))) {
      config.lifespan(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-UserData"))) {
      user_data = arg;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-StatisticsInterval"))) {
      config.statistics_interval(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishStatistics"))) {
      config.publish_relay_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogEntries"))) {
      config.log_entries(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogDiscovery"))) {
      config.log_discovery(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogActivity"))) {
      config.log_activity(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogRelayStatistics"))) {
      config.log_relay_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogHandlerStatistics"))) {
      config.log_handler_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogParticipantStatistics"))) {
      config.log_participant_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogDomainStatistics"))) {
      config.log_domain_statistics(ACE_OS::atoi(arg));
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
    nic_horizontal = get_bind_addr(17400);
  }

  if (nic_vertical == ACE_INET_Addr()) {
    nic_vertical = ACE_INET_Addr(7400);
  }

#ifdef OPENDDS_SECURITY
  if (secure) {
    if (identity_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -IdentityCA is empty\n")));
      return EXIT_FAILURE;
    }
    if (permissions_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -PermissionsCA is empty\n")));
      return EXIT_FAILURE;
    }
    if (identity_certificate_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -IdentityCertificate is empty\n")));
      return EXIT_FAILURE;
    }
    if (identity_key_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -IdentityKey is empty\n")));
      return EXIT_FAILURE;
    }
    if (governance_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -Governance is empty\n")));
      return EXIT_FAILURE;
    }
    if (permissions_file.empty()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: -Permissions is empty\n")));
      return EXIT_FAILURE;
    }
  }
#endif

  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  if (!factory) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to initialize participant factory\n")));
    return EXIT_FAILURE;
  }

#ifdef OPENDDS_SECURITY
  if (secure && !TheServiceParticipant->get_security()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Security documents provided but security is not enabled\n")));
    return EXIT_FAILURE;
  }
#endif

  const DDS::Duration_t one_minute = { 60, 0 };

  TheServiceParticipant->bit_autopurge_nowriter_samples_delay(one_minute);
  TheServiceParticipant->bit_autopurge_disposed_samples_delay(one_minute);

  // Set up the relay participant.
  DDS::DomainParticipantQos participant_qos;
  factory->get_default_participant_qos(participant_qos);
  DDS::PropertySeq& relay_properties = participant_qos.property.value;
  append(relay_properties, OpenDDS::RTPS::RTPS_REFLECT_HEARTBEAT_COUNT, "true");

  DDS::DomainParticipant_var relay_participant = factory->create_participant(relay_domain, participant_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_participant) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to create relay participant\n")));
    return EXIT_FAILURE;
  }

  // Set up relay topics.
  ReaderEntryTypeSupport_var reader_entry_ts = new ReaderEntryTypeSupportImpl;
  if (reader_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register ReaderEntry type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var reader_entry_type_name = reader_entry_ts->get_type_name();

  DDS::Topic_var readers_topic = relay_participant->create_topic(RtpsRelay::READERS_TOPIC_NAME.c_str(),
                                                                 reader_entry_type_name,
                                                                 TOPIC_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!readers_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Readers topic\n")));
    return EXIT_FAILURE;
  }

  WriterEntryTypeSupport_var writer_entry_ts = new WriterEntryTypeSupportImpl;
  if (writer_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register WriterEntry type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var writer_entry_type_name = writer_entry_ts->get_type_name();

  DDS::Topic_var writers_topic = relay_participant->create_topic(RtpsRelay::WRITERS_TOPIC_NAME.c_str(),
                                                                 writer_entry_type_name,
                                                                 TOPIC_QOS_DEFAULT, nullptr,
                                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!writers_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Writers topic\n")));
    return EXIT_FAILURE;
  }

  GuidNameAddressTypeSupport_var guid_relay_addresses_ts = new GuidNameAddressTypeSupportImpl;
  if (guid_relay_addresses_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register GuidNameAddress type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var guid_relay_addresses_type_name = guid_relay_addresses_ts->get_type_name();

  DDS::Topic_var responsible_relay_topic =
    relay_participant->create_topic(RtpsRelay::RESPONSIBLE_RELAY_TOPIC_NAME.c_str(),
                                    guid_relay_addresses_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!responsible_relay_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Responsible Relay topic\n")));
    return EXIT_FAILURE;
  }

  HandlerStatisticsTypeSupport_var handler_statistics_ts = new HandlerStatisticsTypeSupportImpl;
  if (handler_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register HandlerStatistics type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var handler_statistics_type_name = handler_statistics_ts->get_type_name();

  DDS::Topic_var handler_statistics_topic =
    relay_participant->create_topic(RtpsRelay::HANDLER_STATISTICS_TOPIC_NAME.c_str(),
                                    handler_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!handler_statistics_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Handler Statistics topic\n")));
    return EXIT_FAILURE;
  }

  RelayStatisticsTypeSupport_var relay_statistics_ts = new RelayStatisticsTypeSupportImpl;
  if (relay_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register RelayStatistics type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_statistics_type_name = relay_statistics_ts->get_type_name();

  DDS::Topic_var relay_statistics_topic =
    relay_participant->create_topic(RtpsRelay::RELAY_STATISTICS_TOPIC_NAME.c_str(),
                                    relay_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_statistics_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay Statistics topic\n")));
    return EXIT_FAILURE;
  }

  DomainStatisticsTypeSupport_var domain_statistics_ts = new DomainStatisticsTypeSupportImpl;
  if (domain_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register DomainStatistics type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var domain_statistics_type_name = domain_statistics_ts->get_type_name();

  DDS::Topic_var domain_statistics_topic =
    relay_participant->create_topic(RtpsRelay::DOMAIN_STATISTICS_TOPIC_NAME.c_str(),
                                    domain_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!domain_statistics_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Domain Statistics topic\n")));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsTypeSupport_var participant_statistics_ts = new ParticipantStatisticsTypeSupportImpl;
  if (participant_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register ParticipantStatistics type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var participant_statistics_type_name = participant_statistics_ts->get_type_name();

  DDS::Topic_var participant_statistics_topic =
    relay_participant->create_topic(RtpsRelay::PARTICIPANT_STATISTICS_TOPIC_NAME.c_str(),
                                    participant_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!participant_statistics_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Participant Statistics topic\n")));
    return EXIT_FAILURE;
  }

  ParticipantEntryTypeSupport_var participant_entry_ts = new ParticipantEntryTypeSupportImpl;
  if (participant_entry_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to register ParticipantEntry type\n")));
    return EXIT_FAILURE;
  }
  CORBA::String_var participant_entry_type_name = participant_entry_ts->get_type_name();

  DDS::Topic_var participant_entry_topic =
    relay_participant->create_topic(RtpsRelay::PARTICIPANTS_TOPIC_NAME.c_str(),
                                    participant_entry_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!participant_entry_topic) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Particpant Entry topic\n")));
    return EXIT_FAILURE;
  }

  // Setup relay publisher and subscriber.
  DDS::Publisher_var relay_publisher = relay_participant->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr,
                                                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_publisher) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay publisher\n")));
    return EXIT_FAILURE;
  }

  DDS::Subscriber_var relay_subscriber = relay_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr,
                                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_subscriber) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay subscriber\n")));
    return EXIT_FAILURE;
  }

  // Setup relay reader/writer qos.
  DDS::DataWriterQos writer_qos;
  relay_publisher->get_default_datawriter_qos(writer_qos);

  writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderQos reader_qos;
  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = one_minute;
  reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = one_minute;

  DDS::DataWriter_var participant_entry_writer_var = relay_publisher->create_datawriter(participant_entry_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant_entry_writer_var) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Participant Entry data writer\n")));
      return EXIT_FAILURE;
  }

  ParticipantEntryDataWriter_var participant_entry_writer = ParticipantEntryDataWriter::_narrow(participant_entry_writer_var);
  if (!participant_entry_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Participant Entry data writer\n")));
    return EXIT_FAILURE;
  }

  // Setup statistics publishing.
  DDS::DataWriter_var handler_statistics_writer_var = relay_publisher->create_datawriter(handler_statistics_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!handler_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Handler Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  HandlerStatisticsDataWriter_var handler_statistics_writer = HandlerStatisticsDataWriter::_narrow(handler_statistics_writer_var);
  if (!handler_statistics_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Handler Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  DDS::DataWriter_var relay_statistics_writer_var = relay_publisher->create_datawriter(relay_statistics_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Relay Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  RelayStatisticsDataWriter_var relay_statistics_writer = RelayStatisticsDataWriter::_narrow(relay_statistics_writer_var);
  if (!relay_statistics_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Relay Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  DDS::DataWriter_var domain_statistics_writer_var = relay_publisher->create_datawriter(domain_statistics_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!domain_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Domain Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  DomainStatisticsDataWriter_var domain_statistics_writer = DomainStatisticsDataWriter::_narrow(domain_statistics_writer_var);
  if (!domain_statistics_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Domain Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  auto participant_statistics_writer_var = relay_publisher->create_datawriter(participant_statistics_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Participant Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsDataWriter_var participant_statistics_writer = ParticipantStatisticsDataWriter::_narrow(participant_statistics_writer_var);
  if (!participant_statistics_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Participant Statistics data writer\n")));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsReporter::config = &config;
  ParticipantStatisticsReporter::writer = participant_statistics_writer;
  DDS::Topic_var t = participant_statistics_writer->get_topic();
  ParticipantStatisticsReporter::topic_name = t->get_name();

  // Configure ports and addresses.
  auto port_horizontal = nic_horizontal.get_port_number();
  ACE_INET_Addr spdp_horizontal_addr(nic_horizontal);
  spdp_horizontal_addr.set_port_number(port_horizontal++);
  ACE_INET_Addr sedp_horizontal_addr(nic_horizontal);
  sedp_horizontal_addr.set_port_number(port_horizontal++);
  ACE_INET_Addr data_horizontal_addr(nic_horizontal);
  data_horizontal_addr.set_port_number(port_horizontal++);

  auto port_vertical = nic_vertical.get_port_number();
  ACE_INET_Addr spdp_vertical_addr(nic_vertical);
  spdp_vertical_addr.set_port_number(port_vertical++);
  ACE_INET_Addr sedp_vertical_addr(nic_vertical);
  sedp_vertical_addr.set_port_number(port_vertical++);
  ACE_INET_Addr data_vertical_addr(nic_vertical);
  data_vertical_addr.set_port_number(port_vertical++);

  // Set up the application participant.
  factory->get_default_participant_qos(participant_qos);
  participant_qos.user_data.value.length(static_cast<CORBA::ULong>(user_data.length()));
  std::memcpy(participant_qos.user_data.value.get_buffer(), user_data.data(), user_data.length());

  DDS::PropertySeq& application_properties = participant_qos.property.value;
  append(application_properties, OpenDDS::RTPS::RTPS_DISCOVERY_ENDPOINT_ANNOUNCEMENTS, "false");
  append(application_properties, OpenDDS::RTPS::RTPS_DISCOVERY_TYPE_LOOKUP_SERVICE, "false");
  append(application_properties, OpenDDS::RTPS::RTPS_REFLECT_HEARTBEAT_COUNT, "true");

#ifdef OPENDDS_SECURITY
  if (secure) {
    append(application_properties, DDS::Security::Properties::AuthIdentityCA, identity_ca_file);
    append(application_properties, DDS::Security::Properties::AccessPermissionsCA, permissions_ca_file);
    append(application_properties, DDS::Security::Properties::AuthIdentityCertificate, identity_certificate_file);
    append(application_properties, DDS::Security::Properties::AuthPrivateKey, identity_key_file);
    append(application_properties, DDS::Security::Properties::AccessGovernance, governance_file);
    append(application_properties, DDS::Security::Properties::AccessPermissions, permissions_file);
  }
#endif

  DDS::DomainParticipant_var application_participant = factory->create_participant(config.application_domain(), participant_qos, nullptr,
                                                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!application_participant) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to create application participant\n")));
    return EXIT_FAILURE;
  }

  OpenDDS::DCPS::DomainParticipantImpl* application_participant_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(application_participant.in());
  if (application_participant_impl == 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to get application participant impl\n")));
    return EXIT_FAILURE;
  }

  config.application_participant_guid(application_participant_impl->get_id());

  const auto discovery = TheServiceParticipant->get_discovery(config.application_domain());
  const auto rtps_discovery = OpenDDS::DCPS::dynamic_rchandle_cast<OpenDDS::RTPS::RtpsDiscovery>(discovery);

  ACE_INET_Addr spdp;
  if (spdp_vertical_addr.get_type() == AF_INET) {
    spdp = ACE_INET_Addr(rtps_discovery->get_spdp_port(config.application_domain(), config.application_participant_guid()), "127.0.0.1");
#ifdef ACE_HAS_IPV6
  } else if (spdp_vertical_addr.get_type() == AF_INET6) {
    spdp = ACE_INET_Addr(rtps_discovery->get_ipv6_spdp_port(config.application_domain(), config.application_participant_guid()), "::1");
#endif
  }

  ACE_INET_Addr sedp;
  if (sedp_vertical_addr.get_type() == AF_INET) {
    sedp = ACE_INET_Addr(rtps_discovery->get_sedp_port(config.application_domain(), config.application_participant_guid()), "127.0.0.1");
#ifdef ACE_HAS_IPV6
  } else if (sedp_vertical_addr.get_type() == AF_INET6) {
    sedp = ACE_INET_Addr(rtps_discovery->get_ipv6_sedp_port(config.application_domain(), config.application_participant_guid()), "::1");
#endif
  }

#ifdef OPENDDS_SECURITY
  OpenDDS::Security::SecurityConfig_rch conf = TheSecurityRegistry->default_config();
  DDS::Security::CryptoTransform_var crypto = conf->get_crypto_transform();
#else
  const int crypto = 0;
#endif

  ACE_Reactor reactor_(new ACE_Select_Reactor, true);
  const auto reactor = &reactor_;
  AssociationTable association_table;

  // Setup readers and writers for managing the association table.
  DDS::DataWriter_var responsible_relay_writer_var =
    relay_publisher->create_datawriter(responsible_relay_topic, writer_qos, nullptr,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!responsible_relay_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Responsible Relay data writer\n")));
    return EXIT_FAILURE;
  }

  GuidNameAddressDataWriter_var responsible_relay_writer = GuidNameAddressDataWriter::_narrow(responsible_relay_writer_var);

  if (!responsible_relay_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Responsible Relay data writer\n")));
    return EXIT_FAILURE;
  }

  DDS::DataReader_var responsible_relay_reader_var =
    relay_subscriber->create_datareader(responsible_relay_topic, reader_qos, nullptr, 0);
  if (!responsible_relay_reader_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Reponsible Relay data reader\n")));
    return EXIT_FAILURE;
  }

  GuidNameAddressDataReader_var responsible_relay_reader = GuidNameAddressDataReader::_narrow(responsible_relay_reader_var);

  if (!responsible_relay_reader) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Responsible Relay data reader\n")));
    return EXIT_FAILURE;
  }

  RelayStatisticsReporter relay_statistics_reporter(config, relay_statistics_writer);

  HandlerStatisticsReporter spdp_vertical_reporter(config, "VSPDP", handler_statistics_writer, relay_statistics_reporter);
  SpdpHandler spdp_vertical_handler(config, "VSPDP", spdp_horizontal_addr, reactor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto, spdp, spdp_vertical_reporter);
  HandlerStatisticsReporter sedp_vertical_reporter(config, "VSEDP", handler_statistics_writer, relay_statistics_reporter);
  SedpHandler sedp_vertical_handler(config, "VSEDP", sedp_horizontal_addr, reactor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto, sedp, sedp_vertical_reporter);
  HandlerStatisticsReporter data_vertical_reporter(config, "VDATA", handler_statistics_writer, relay_statistics_reporter);
  DataHandler data_vertical_handler(config, "VDATA", data_horizontal_addr, reactor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto, data_vertical_reporter);

  HandlerStatisticsReporter spdp_horizontal_reporter(config, "HSPDP", handler_statistics_writer, relay_statistics_reporter);
  HorizontalHandler spdp_horizontal_handler(config, "HSPDP", reactor, spdp_horizontal_reporter);
  HandlerStatisticsReporter sedp_horizontal_reporter(config, "HSEDP", handler_statistics_writer, relay_statistics_reporter);
  HorizontalHandler sedp_horizontal_handler(config, "HSEDP", reactor, sedp_horizontal_reporter);
  HandlerStatisticsReporter data_horizontal_reporter(config, "HDATA", handler_statistics_writer, relay_statistics_reporter);
  HorizontalHandler data_horizontal_handler(config, "HDATA", reactor, data_horizontal_reporter);

  spdp_horizontal_handler.vertical_handler(&spdp_vertical_handler);
  sedp_horizontal_handler.vertical_handler(&sedp_vertical_handler);
  data_horizontal_handler.vertical_handler(&data_vertical_handler);

  spdp_vertical_handler.horizontal_handler(&spdp_horizontal_handler);
  sedp_vertical_handler.horizontal_handler(&sedp_horizontal_handler);
  data_vertical_handler.horizontal_handler(&data_horizontal_handler);

  DDS::Subscriber_var bit_subscriber = application_participant->get_builtin_subscriber();

  DDS::DataReader_var participant_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  DomainStatisticsReporter domain_statistics_reporter(config, domain_statistics_writer);

  DDS::DataReaderListener_var participant_listener = new ParticipantListener(config,
                                                                             application_participant_impl,
                                                                             domain_statistics_reporter,
                                                                             participant_entry_writer);
  DDS::ReturnCode_t ret = participant_reader->set_listener(participant_listener, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to set listener on ParticipantBuiltinTopicDataDataReader\n")));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var participant_entry_listener =
    new ParticipantEntryListener(config, domain_statistics_reporter);
  DDS::DataReader_var participant_entry_reader = relay_subscriber->create_datareader(participant_entry_topic, reader_qos,
                                                                                     participant_entry_listener,
                                                                                     DDS::DATA_AVAILABLE_STATUS);
  if (!participant_entry_reader) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Participant data reader\n")));
    return EXIT_FAILURE;
  }

  // Setup the reader writer and reader reader.
  DDS::DataWriter_var reader_writer_var = relay_publisher->create_datawriter(readers_topic, writer_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!reader_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Reader data writer\n")));
    return EXIT_FAILURE;
  }

  ReaderEntryDataWriter_var reader_writer = ReaderEntryDataWriter::_narrow(reader_writer_var);

  if (!reader_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Reader data writer\n")));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var reader_listener =
    new ReaderListener(config, association_table, spdp_vertical_handler, domain_statistics_reporter);
  DDS::DataReader_var reader_reader_var = relay_subscriber->create_datareader(readers_topic, reader_qos,
                                                                              reader_listener,
                                                                              DDS::DATA_AVAILABLE_STATUS);

  if (!reader_reader_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Reader data reader\n")));
    return EXIT_FAILURE;
  }

  // Setup a subscription listener to feed the reader writer.
  DDS::DataReader_var subscription_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  DDS::DataReaderListener_var subscription_listener =
    new SubscriptionListener(config, application_participant_impl, reader_writer, domain_statistics_reporter);
  ret = subscription_reader->set_listener(subscription_listener, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to set listener on SubscriptionBuiltinTopicDataDataReader\n")));
    return EXIT_FAILURE;
  }

  // Setup the writer writer and writer reader.
  DDS::DataWriter_var writer_writer_var = relay_publisher->create_datawriter(writers_topic, writer_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!writer_writer_var) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Writer data writer\n")));
    return EXIT_FAILURE;
  }

  WriterEntryDataWriter_var writer_writer = WriterEntryDataWriter::_narrow(writer_writer_var);

  if (!writer_writer) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to narrow Writer data writer\n")));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var writer_listener =
    new WriterListener(config, association_table, spdp_vertical_handler, domain_statistics_reporter);
  DDS::DataReader_var writer_reader = relay_subscriber->create_datareader(writers_topic, reader_qos,
                                                                          writer_listener,
                                                                          DDS::DATA_AVAILABLE_STATUS);
  if (!writer_reader) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to create Writer data reader\n")));
    return EXIT_FAILURE;
  }

  // Setup a publication listener to feed the writer writer.
  DDS::DataReader_var publication_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
  DDS::DataReaderListener_var publication_listener(new PublicationListener(config,
                                                                           application_participant_impl,
                                                                           writer_writer,
                                                                           domain_statistics_reporter));
  ret = publication_reader->set_listener(publication_listener, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Failed to set listener on PublicationBuiltinTopicDataDataReader\n")));
    return EXIT_FAILURE;
  }

  if (spdp_horizontal_handler.open(spdp_horizontal_addr) == -1 ||
      sedp_horizontal_handler.open(sedp_horizontal_addr) == -1 ||
      data_horizontal_handler.open(data_horizontal_addr) == -1 ||
      spdp_vertical_handler.open(spdp_vertical_addr) == -1 ||
      sedp_vertical_handler.open(sedp_vertical_addr) == -1 ||
      data_vertical_handler.open(data_vertical_addr) == -1) {
    return EXIT_FAILURE;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SPDP Horizontal listening on %C\n"), addr_to_string(spdp_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SEDP Horizontal listening on %C\n"), addr_to_string(sedp_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: Data Horizontal listening on %C\n"), addr_to_string(data_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SPDP Vertical listening on %C\n"), addr_to_string(spdp_vertical_addr).c_str()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: SEDP Vertical listening on %C\n"), addr_to_string(sedp_vertical_addr).c_str()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: Data Vertical listening on %C\n"), addr_to_string(data_vertical_addr).c_str()));

  reactor->run_reactor_event_loop();

  return EXIT_SUCCESS;
}
