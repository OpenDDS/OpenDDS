/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "GuidPartitionTable.h"
#include "ParticipantListener.h"
#include "ParticipantStatisticsReporter.h"
#include "PublicationListener.h"
#include "RelayAddressListener.h"
#include "RelayConfigControlListener.h"
#include "RelayHandler.h"
#include "RelayHttpMetaDiscovery.h"
#include "RelayPartitionTable.h"
#include "RelayPartitionsListener.h"
#include "RelayStatisticsReporter.h"
#include "RelayStatusReporter.h"
#include "RelayThreadMonitor.h"
#include "StatisticsWriterListener.h"
#include "SubscriptionListener.h"

#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/NetworkResource.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/security/BuiltInPlugins.h>
#endif

#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/security/framework/SecurityRegistry.h>

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/Reactor.h>
#include <ace/Select_Reactor.h>
#include <ace/TP_Reactor.h>

#include <cstdlib>
#include <algorithm>

using namespace RtpsRelay;

namespace {
  void append(DDS::PropertySeq& props, const char* name, const std::string& value, bool propagate = false)
  {
    const DDS::Property_t prop = {name, value.c_str(), propagate};
    const auto len = props.length();
    props.length(len + 1);
    try {
      props[len] = prop;
    } catch (const CORBA::BAD_PARAM& /*ex*/) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Exception caught when appending parameter\n"));
    }
  }

  ACE_INET_Addr get_bind_addr(unsigned short port)
  {
    std::vector<ACE_INET_Addr> nics;
    OpenDDS::DCPS::get_interface_addrs(nics);

    for (auto& nic : nics) {
      if (nic.is_loopback()) {
        continue;
      }

      nic.set_port_number(port);
      return nic;
    }
    return ACE_INET_Addr();
  }

  const unsigned short DEFAULT_HORIZONTAL = 17400;
  const unsigned short DEFAULT_VERTICAL = 7400;
  const unsigned short DEFAULT_META = 8080;

  struct InternalConfigListener : OpenDDS::DCPS::ConfigListener {
    InternalConfigListener(const RelayConfigDataWriter_var& writer, const std::string& id)
      : InternalDataReaderListener{TheServiceParticipant->job_queue()}
      , writer_{writer}
      , config_{id, {}}
    {}

    void on_data_available(InternalDataReader_rch reader) override
    {
      OpenDDS::DCPS::ConfigReader::SampleSequence samples;
      OpenDDS::DCPS::InternalSampleInfoSequence infos;
      reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
                   DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
      for (size_t idx = 0; idx != samples.size(); ++idx) {
        const auto& info = infos[idx];
        const auto& pair = samples[idx];
        if (info.valid_data) {
          config_.config()[pair.key()] = pair.value();
        } else if (info.instance_state != DDS::ALIVE_INSTANCE_STATE) {
          config_.config().erase(pair.key());
        }
      }
      writer_->write(config_, DDS::HANDLE_NIL);
    }

    RelayConfigDataWriter_var writer_;
    RelayConfig config_;
  };
}

int run(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var factory = TheParticipantFactoryWithArgs(argc, argv);
  if (!factory) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to initialize participant factory\n"));
    return EXIT_FAILURE;
  }

  DDS::DomainId_t relay_domain = 0;
  ACE_INET_Addr nic_horizontal, nic_vertical, meta_discovery_addr;
  std::string meta_discovery_content_type = "application/json";
  std::string meta_discovery_content = "{}";
  std::string meta_discovery_content_path;
  std::string user_data;
  Config config;

  std::string identity_ca_file;
  std::string permissions_ca_file;
  std::string identity_certificate_file;
  std::string identity_key_file;
  std::string governance_file;
  std::string permissions_file;
  bool secure = false;
  const std::string file = "file:";

  int argc_copy = argc;
  ACE_Argv_Type_Converter atc(argc_copy, argv);
  ACE_Arg_Shifter_T<char> args(atc.get_argc(), atc.get_ASCII_argv());
  args.ignore_arg(); // argv[0] is the program name
  while (args.is_anything_left()) {
    const char* arg = nullptr;
    if ((arg = args.get_the_parameter("-HorizontalAddress"))) {
      nic_horizontal = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-VerticalAddress"))) {
      nic_vertical = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-MetaDiscoveryAddress"))) {
      meta_discovery_addr = ACE_INET_Addr(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-MetaDiscoveryContentType"))) {
      meta_discovery_content_type = arg;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-MetaDiscoveryContentPath"))) {
      meta_discovery_content_path = arg;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-MetaDiscoveryContent"))) {
      meta_discovery_content = arg;
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
    } else if ((arg = args.get_the_parameter("-InactivePeriod"))) {
      config.inactive_period(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-BufferSize"))) {
      config.buffer_size(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-UserData"))) {
      user_data = arg;
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-AllowEmptyPartition"))) {
      config.allow_empty_partition(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogWarnings"))) {
      config.log_warnings(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogDiscovery"))) {
      config.log_discovery(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogActivity"))) {
      config.log_activity(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogHttp"))) {
      config.log_http(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogThreadStatus"))) {
      config.log_thread_status(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-ThreadStatusSafetyFactor"))) {
      config.thread_status_safety_factor(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-UtilizationLimit"))) {
      config.utilization_limit(ACE_OS::atof(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogUtilizationChanges"))) {
      config.log_utilization_changes(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogRelayStatistics"))) {
      config.log_relay_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogHandlerStatistics"))) {
      config.log_handler_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-LogParticipantStatistics"))) {
      config.log_participant_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishRelayStatistics"))) {
      config.publish_relay_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishHandlerStatistics"))) {
      config.publish_handler_statistics(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishParticipantStatistics"))) {
      config.publish_participant_statistics(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishRelayStatusLiveliness"))) {
      config.publish_relay_status_liveliness(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-PublishRelayStatus"))) {
      config.publish_relay_status(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-RestartDetection"))) {
      config.restart_detection(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-AdmissionControlQueueSize"))) {
      config.admission_control_queue_size(static_cast<size_t>(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-AdmissionControlQueueDuration"))) {
      config.admission_control_queue_duration(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-AdmissionMaxParticipantsRange"))) {
      auto conv = std::atoi(arg);
      if (conv <= 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must be positive: %d\n", conv));
        return EXIT_FAILURE;
      }
      config.admission_max_participants_low_water(static_cast<size_t>(conv));
      const auto dash = std::strchr(arg, '-');
      if (!dash) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must contain a '-': %C\n", arg));
        return EXIT_FAILURE;
      }
      conv = std::atoi(dash + 1);
      if (conv <= 0) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Argument for -AdmissionMaxParticipantsRange option must be positive: %d\n", conv));
        return EXIT_FAILURE;
      }
      if (static_cast<size_t>(conv) < config.admission_max_participants_low_water()) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: High value for -AdmissionMaxParticipantsRange option must be greater than or equal to low value\n"));
        return EXIT_FAILURE;
      }
      config.admission_max_participants_high_water(static_cast<size_t>(conv));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-RunTime"))) {
      config.run_time(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-HandlerThreads"))) {
      config.handler_threads(static_cast<size_t>(std::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-SynchronousOutput"))) {
      config.synchronous_output(ACE_OS::atoi(arg));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-MaxIpsPerClient"))) {
      config.max_ips_per_client(static_cast<size_t>(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-RejectedAddressDuration"))) {
      config.rejected_address_duration(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
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
    } else if ((arg = args.get_the_parameter("-Id"))) {
      config.relay_id(arg);
      args.consume_arg();
    } else if ((arg = args.get_the_parameter("-DrainInterval"))) {
      config.drain_interval(OpenDDS::DCPS::TimeDuration(ACE_OS::atoi(arg)));
      args.consume_arg();
    } else {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Invalid option: %C\n", args.get_current()));
      return EXIT_FAILURE;
    }
  }

  if (!meta_discovery_content_path.empty()) {
    std::ifstream in(meta_discovery_content_path);
    if (!in) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Could not open %C\n", meta_discovery_content_path.c_str()));
      return EXIT_FAILURE;
    }
    std::stringstream x;
    x << in.rdbuf();
    meta_discovery_content = x.str();
  }

  if (nic_horizontal == ACE_INET_Addr()) {
    nic_horizontal = get_bind_addr(DEFAULT_HORIZONTAL);
  }

  if (nic_vertical == ACE_INET_Addr()) {
    nic_vertical = ACE_INET_Addr(DEFAULT_VERTICAL);
  }

  if (meta_discovery_addr == ACE_INET_Addr()) {
    meta_discovery_addr = ACE_INET_Addr(DEFAULT_META);
  }

  if (config.relay_id().empty()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -Id is empty\n"));
    return EXIT_FAILURE;
  }

  if (config.publish_relay_status() != OpenDDS::DCPS::TimeDuration::zero_value &&
      config.publish_relay_status() > config.publish_relay_status_liveliness()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -PublishRelayStatus is greater than -PublishRelayStatusLiveliness\n"));
    return EXIT_FAILURE;
  }

  if (secure) {
    if (identity_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -IdentityCA is empty\n"));
      return EXIT_FAILURE;
    }
    if (permissions_ca_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -PermissionsCA is empty\n"));
      return EXIT_FAILURE;
    }
    if (identity_certificate_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -IdentityCertificate is empty\n"));
      return EXIT_FAILURE;
    }
    if (identity_key_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -IdentityKey is empty\n"));
      return EXIT_FAILURE;
    }
    if (governance_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -Governance is empty\n"));
      return EXIT_FAILURE;
    }
    if (permissions_file.empty()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: -Permissions is empty\n"));
      return EXIT_FAILURE;
    }
  }
  if (secure && !TheServiceParticipant->get_security()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Security documents provided but security is not enabled\n"));
    return EXIT_FAILURE;
  }

  const DDS::Duration_t one_minute = { 60, 0 };

  TheServiceParticipant->bit_autopurge_nowriter_samples_delay(one_minute);
  TheServiceParticipant->bit_autopurge_disposed_samples_delay(one_minute);

  const bool either = config.log_relay_statistics() || config.publish_relay_statistics(),
    both = config.log_relay_statistics() && config.publish_relay_statistics();
  const auto interval = both ? std::min(config.log_relay_statistics(), config.publish_relay_statistics())
    : (config.log_relay_statistics() ? config.log_relay_statistics() : config.publish_relay_statistics());
  if (either) {
    TheServiceParticipant->statistics_period(interval);
  }

  // Set up the relay participant.
  DDS::DomainParticipantQos participant_qos;
  factory->get_default_participant_qos(participant_qos);
  append(participant_qos.property.value, OpenDDS::RTPS::RTPS_REFLECT_HEARTBEAT_COUNT, "true");

  DDS::DomainParticipant_var relay_participant = factory->create_participant(relay_domain, participant_qos, nullptr,
                                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to create relay participant\n"));
    return EXIT_FAILURE;
  }

  // Set up relay topics.
  RelayPartitionsTypeSupport_var relay_partitions_ts = new RelayPartitionsTypeSupportImpl;
  if (relay_partitions_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayPartitions type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_partitions_type_name = relay_partitions_ts->get_type_name();

  DDS::Topic_var relay_partitions_topic =
    relay_participant->create_topic(RELAY_PARTITIONS_TOPIC_NAME.c_str(),
                                    relay_partitions_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_partitions_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Partitions topic\n"));
    return EXIT_FAILURE;
  }

  RelayParticipantStatusTypeSupport_var relay_participant_status_ts = new RelayParticipantStatusTypeSupportImpl;
  if (relay_participant_status_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayParticipantStatus type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_participant_status_type_name = relay_participant_status_ts->get_type_name();

  DDS::Topic_var relay_participant_status_topic =
    relay_participant->create_topic(RELAY_PARTICIPANT_STATUS_TOPIC_NAME.c_str(),
                                    relay_participant_status_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_participant_status_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Participant Status topic\n"));
    return EXIT_FAILURE;
  }

  RelayAddressTypeSupport_var relay_address_ts = new RelayAddressTypeSupportImpl;
  if (relay_address_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayAddress type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_address_type_name = relay_address_ts->get_type_name();

  DDS::Topic_var relay_addresses_topic =
    relay_participant->create_topic(RELAY_ADDRESSES_TOPIC_NAME.c_str(),
                                    relay_address_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_addresses_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Addresses topic\n"));
    return EXIT_FAILURE;
  }

  RelayStatusTypeSupport_var relay_status_ts = new RelayStatusTypeSupportImpl;
  if (relay_status_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayStatus type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_status_type_name = relay_status_ts->get_type_name();

  DDS::Topic_var relay_status_topic =
    relay_participant->create_topic(RELAY_STATUS_TOPIC_NAME.c_str(),
                                    relay_status_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_status_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Instances topic\n"));
    return EXIT_FAILURE;
  }

  HandlerStatisticsTypeSupport_var handler_statistics_ts = new HandlerStatisticsTypeSupportImpl;
  if (handler_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register HandlerStatistics type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var handler_statistics_type_name = handler_statistics_ts->get_type_name();

  DDS::Topic_var handler_statistics_topic =
    relay_participant->create_topic(HANDLER_STATISTICS_TOPIC_NAME.c_str(),
                                    handler_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!handler_statistics_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Handler Statistics topic\n"));
    return EXIT_FAILURE;
  }

  RelayStatisticsTypeSupport_var relay_statistics_ts = new RelayStatisticsTypeSupportImpl;
  if (relay_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayStatistics type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_statistics_type_name = relay_statistics_ts->get_type_name();

  DDS::Topic_var relay_statistics_topic =
    relay_participant->create_topic(RELAY_STATISTICS_TOPIC_NAME.c_str(),
                                    relay_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_statistics_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Statistics topic\n"));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsTypeSupport_var participant_statistics_ts = new ParticipantStatisticsTypeSupportImpl;
  if (participant_statistics_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register ParticipantStatistics type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var participant_statistics_type_name = participant_statistics_ts->get_type_name();

  DDS::Topic_var participant_statistics_topic =
    relay_participant->create_topic(PARTICIPANT_STATISTICS_TOPIC_NAME.c_str(),
                                    participant_statistics_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr,
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!participant_statistics_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Participant Statistics topic\n"));
    return EXIT_FAILURE;
  }

  // Setup relay publisher and subscriber.
  DDS::PublisherQos publisher_qos;
  relay_participant->get_default_publisher_qos(publisher_qos);
  publisher_qos.partition.name.length(1);
  publisher_qos.partition.name[0] = config.relay_id().c_str(); // Publish to dedicated partition.

  DDS::Publisher_var relay_publisher = relay_participant->create_publisher(publisher_qos, nullptr,
                                                                           OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_publisher) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay publisher\n"));
    return EXIT_FAILURE;
  }

  DDS::SubscriberQos subscriber_qos;
  relay_participant->get_default_subscriber_qos(subscriber_qos);
  subscriber_qos.partition.name.length(1);
  subscriber_qos.partition.name[0] = "*"; // Subscribe to all partitions.

  DDS::Subscriber_var relay_subscriber = relay_participant->create_subscriber(subscriber_qos, nullptr,
                                                                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_subscriber) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay subscriber\n"));
    return EXIT_FAILURE;
  }

  // Setup relay reader/writer qos.
  DDS::DataWriterQos writer_qos;
  relay_publisher->get_default_datawriter_qos(writer_qos);

  writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  writer_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  writer_qos.history.depth = 1;

  DDS::DataReaderQos reader_qos;
  relay_subscriber->get_default_datareader_qos(reader_qos);

  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  reader_qos.history.depth = 1;
  reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay = one_minute;
  reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay = one_minute;

  // Setup statistics publishing.
  DDS::DataWriter_var relay_statistics_writer_var = relay_publisher->create_datawriter(relay_statistics_topic, writer_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  RelayStatisticsDataWriter_var relay_statistics_writer = RelayStatisticsDataWriter::_narrow(relay_statistics_writer_var);
  if (!relay_statistics_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  RelayStatisticsReporter relay_statistics_reporter(config, relay_statistics_writer);

  DDS::DataWriterListener_var relay_statistics_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::relay_statistics_sub_count);
  relay_statistics_writer->set_listener(relay_statistics_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataWriterListener_var handler_statistics_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::handler_statistics_sub_count);
  DDS::DataWriter_var handler_statistics_writer_var = relay_publisher->create_datawriter(handler_statistics_topic, writer_qos, handler_statistics_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!handler_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Handler Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  HandlerStatisticsDataWriter_var handler_statistics_writer = HandlerStatisticsDataWriter::_narrow(handler_statistics_writer_var);
  if (!handler_statistics_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Handler Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  DDS::DataWriterQos participant_statistics_writer_qos = writer_qos;
  participant_statistics_writer_qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
  DDS::DataWriterListener_var participant_statistics_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::participant_statistics_sub_count);
  DDS::DataWriter_var participant_statistics_writer_var = relay_publisher->create_datawriter(participant_statistics_topic, participant_statistics_writer_qos, participant_statistics_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant_statistics_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Participant Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsDataWriter_var participant_statistics_writer = ParticipantStatisticsDataWriter::_narrow(participant_statistics_writer_var);
  if (!participant_statistics_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Participant Statistics data writer\n"));
    return EXIT_FAILURE;
  }

  ParticipantStatisticsReporter::config = &config;
  ParticipantStatisticsReporter::writer = participant_statistics_writer;
  DDS::Topic_var participant_stats_topic = participant_statistics_writer->get_topic();
  ParticipantStatisticsReporter::topic_name = participant_stats_topic->get_name();

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
  append(application_properties, OpenDDS::RTPS::RTPS_RELAY_APPLICATION_PARTICIPANT, "true");
  append(application_properties, OpenDDS::RTPS::RTPS_HARVEST_THREAD_STATUS, "true");

  if (secure) {
    append(application_properties, DDS::Security::Properties::AuthIdentityCA, identity_ca_file);
    append(application_properties, DDS::Security::Properties::AccessPermissionsCA, permissions_ca_file);
    append(application_properties, DDS::Security::Properties::AuthIdentityCertificate, identity_certificate_file);
    append(application_properties, DDS::Security::Properties::AuthPrivateKey, identity_key_file);
    append(application_properties, DDS::Security::Properties::AccessGovernance, governance_file);
    append(application_properties, DDS::Security::Properties::AccessPermissions, permissions_file);
  }

  DDS::DomainParticipant_var application_participant = factory->create_participant(config.application_domain(), participant_qos, nullptr,
                                                                                   OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!application_participant) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to create application participant\n"));
    return EXIT_FAILURE;
  }

  OpenDDS::DCPS::DomainParticipantImpl* application_participant_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(application_participant.in());
  if (application_participant_impl == 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to get application participant impl\n"));
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

  OpenDDS::Security::SecurityConfig_rch conf = TheSecurityRegistry->default_config();
  DDS::Security::CryptoTransform_var crypto = conf->get_crypto_transform();

  DDS::DataWriterListener_var relay_partitions_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::relay_partitions_sub_count);
  DDS::DataWriter_var relay_partitions_writer_var =
    relay_publisher->create_datawriter(relay_partitions_topic, writer_qos, relay_partitions_writer_listener,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_partitions_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Partitions data writer\n"));
    return EXIT_FAILURE;
  }

  RelayPartitionsDataWriter_var relay_partitions_writer = RelayPartitionsDataWriter::_narrow(relay_partitions_writer_var);

  if (!relay_partitions_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Partitions data writer\n"));
    return EXIT_FAILURE;
  }

  auto relay_participant_status_writer_qos = writer_qos;
  relay_participant_status_writer_qos.writer_data_lifecycle.autodispose_unregistered_instances = false;
  DDS::DataWriterListener_var relay_participant_status_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::relay_participant_status_sub_count);
  DDS::DataWriter_var relay_participant_status_writer_var =
    relay_publisher->create_datawriter(relay_participant_status_topic, relay_participant_status_writer_qos, relay_participant_status_writer_listener,
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (!relay_participant_status_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Participant Status data writer\n"));
    return EXIT_FAILURE;
  }

  RelayParticipantStatusDataWriter_var relay_participant_status_writer = RelayParticipantStatusDataWriter::_narrow(relay_participant_status_writer_var);

  if (!relay_participant_status_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Participant Status data writer\n"));
    return EXIT_FAILURE;
  }

  RelayParticipantStatusReporter relay_participant_status_reporter(config, relay_participant_status_writer, relay_statistics_reporter);

  RelayThreadMonitor* relay_thread_monitor = new RelayThreadMonitor(config);
  const auto reactor = new ACE_Reactor(config.handler_threads() == 1 ? new ACE_Select_Reactor : new ACE_TP_Reactor, true); // deleted by ReactorTask
  const auto reactor_task = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ReactorTask>();
  reactor_task->init_reactor_task(&TheServiceParticipant->get_thread_status_manager(), "RtpsRelay Main", reactor);

  const auto guid_addr_set = OpenDDS::DCPS::make_rch<GuidAddrSet>(config, reactor_task, rtps_discovery,
                                                                  OpenDDS::DCPS::ref(relay_participant_status_reporter), OpenDDS::DCPS::ref(relay_statistics_reporter), OpenDDS::DCPS::ref(*relay_thread_monitor));
  GuidPartitionTable guid_partition_table(config, spdp_horizontal_addr, relay_partitions_writer, relay_statistics_reporter);
  RelayPartitionTable relay_partition_table(relay_statistics_reporter);
  relay_statistics_reporter.report();

  HandlerStatisticsReporter spdp_vertical_reporter(config, VSPDP, handler_statistics_writer, relay_statistics_reporter);
  spdp_vertical_reporter.report();
  SpdpHandler spdp_vertical_handler(config, VSPDP, spdp_horizontal_addr, reactor, guid_partition_table, relay_partition_table, *guid_addr_set, rtps_discovery, crypto, spdp, spdp_vertical_reporter);
  HandlerStatisticsReporter sedp_vertical_reporter(config, VSEDP, handler_statistics_writer, relay_statistics_reporter);
  sedp_vertical_reporter.report();
  SedpHandler sedp_vertical_handler(config, VSEDP, sedp_horizontal_addr, reactor, guid_partition_table, relay_partition_table, *guid_addr_set, rtps_discovery, crypto, sedp, sedp_vertical_reporter);
  HandlerStatisticsReporter data_vertical_reporter(config, VDATA, handler_statistics_writer, relay_statistics_reporter);
  data_vertical_reporter.report();
  DataHandler data_vertical_handler(config, VDATA, data_horizontal_addr, reactor, guid_partition_table, relay_partition_table, *guid_addr_set, rtps_discovery, crypto, data_vertical_reporter);

  HandlerStatisticsReporter spdp_horizontal_reporter(config, HSPDP, handler_statistics_writer, relay_statistics_reporter);
  spdp_horizontal_reporter.report();
  HorizontalHandler spdp_horizontal_handler(config, HSPDP, SPDP, reactor, guid_partition_table, spdp_horizontal_reporter);
  HandlerStatisticsReporter sedp_horizontal_reporter(config, HSEDP, handler_statistics_writer, relay_statistics_reporter);
  sedp_horizontal_reporter.report();
  HorizontalHandler sedp_horizontal_handler(config, HSEDP, SEDP, reactor, guid_partition_table, sedp_horizontal_reporter);
  HandlerStatisticsReporter data_horizontal_reporter(config, HDATA, handler_statistics_writer, relay_statistics_reporter);
  data_horizontal_reporter.report();
  HorizontalHandler data_horizontal_handler(config, HDATA, DATA, reactor, guid_partition_table, data_horizontal_reporter);

  spdp_horizontal_handler.vertical_handler(&spdp_vertical_handler);
  sedp_horizontal_handler.vertical_handler(&sedp_vertical_handler);
  data_horizontal_handler.vertical_handler(&data_vertical_handler);

  spdp_vertical_handler.horizontal_handler(&spdp_horizontal_handler);
  sedp_vertical_handler.horizontal_handler(&sedp_horizontal_handler);
  data_vertical_handler.horizontal_handler(&data_horizontal_handler);

  spdp_vertical_handler.spdp_handler(&spdp_vertical_handler);
  sedp_vertical_handler.spdp_handler(&spdp_vertical_handler);

  DDS::Subscriber_var bit_subscriber = application_participant->get_builtin_subscriber();

  DDS::DataReader_var thread_status_reader_var = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_INTERNAL_THREAD_TOPIC);
  OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader_var thread_status_reader = OpenDDS::DCPS::InternalThreadBuiltinTopicDataDataReader::_narrow(thread_status_reader_var);
  if (!thread_status_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to narrow InternalThreadBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }

  relay_thread_monitor->set_reader(thread_status_reader);
  DDS::DataReaderListener_var relay_thread_monitor_var(relay_thread_monitor);
  DDS::ReturnCode_t ret = thread_status_reader->set_listener(relay_thread_monitor_var, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to set listener on InternalThreadBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }
  // Don't need to invoke listener for existing samples because additional samples are forthcoming.

  DDS::DataReaderListener_var relay_partition_listener =
    new RelayPartitionsListener(relay_partition_table, relay_statistics_reporter);
  DDS::DataReader_var relay_partition_reader_var =
    relay_subscriber->create_datareader(relay_partitions_topic, reader_qos,
                                        relay_partition_listener,
                                        DDS::DATA_AVAILABLE_STATUS | DDS::SUBSCRIPTION_MATCHED_STATUS);

  if (!relay_partition_reader_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Partition data reader\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReaderListener_var relay_address_listener =
    new RelayAddressListener(relay_partition_table, relay_statistics_reporter);
  DDS::DataReader_var relay_address_reader_var =
    relay_subscriber->create_datareader(relay_addresses_topic, reader_qos,
                                        relay_address_listener,
                                        DDS::DATA_AVAILABLE_STATUS | DDS::SUBSCRIPTION_MATCHED_STATUS);

  if (!relay_address_reader_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Address data reader\n"));
    return EXIT_FAILURE;
  }

  DDS::DataReader_var participant_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC);
  ParticipantListener* participant_listener =
    new ParticipantListener(application_participant_impl, guid_addr_set, relay_participant_status_reporter);
  DDS::DataReaderListener_var participant_listener_var(participant_listener);
  ret = participant_reader->set_listener(participant_listener_var, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to set listener on ParticipantBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }
  // Don't need to invoke listener for existing samples because no remote participants could be discovered yet.

  DDS::DataReader_var subscription_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC);
  SubscriptionListener* subscription_listener =
    new SubscriptionListener(config, guid_addr_set, application_participant_impl, guid_partition_table, relay_statistics_reporter);
  DDS::DataReaderListener_var subscription_listener_var(subscription_listener);
  ret = subscription_reader->set_listener(subscription_listener_var, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to set listener on SubscriptionBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }
  // Don't need to invoke listener for existing samples because no remote participants could be discovered yet.

  DDS::DataReader_var publication_reader = bit_subscriber->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
  PublicationListener* publication_listener =
    new PublicationListener(config, guid_addr_set, application_participant_impl, guid_partition_table, relay_statistics_reporter);
  DDS::DataReaderListener_var publication_listener_var(publication_listener);
  ret = publication_reader->set_listener(publication_listener_var, DDS::DATA_AVAILABLE_STATUS);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to set listener on PublicationBuiltinTopicDataDataReader\n"));
    return EXIT_FAILURE;
  }
  // Don't need to invoke listener for existing samples because no remote participants could be discovered yet.

  if (spdp_horizontal_handler.open(spdp_horizontal_addr) == -1 ||
      sedp_horizontal_handler.open(sedp_horizontal_addr) == -1 ||
      data_horizontal_handler.open(data_horizontal_addr) == -1 ||
      spdp_vertical_handler.open(spdp_vertical_addr) == -1 ||
      sedp_vertical_handler.open(sedp_vertical_addr) == -1 ||
      data_vertical_handler.open(data_vertical_addr) == -1) {
    return EXIT_FAILURE;
  }

  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Application Participant GUID %C\n",
    OpenDDS::DCPS::LogGuid(config.application_participant_guid()).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SPDP Horizontal %d listening on %C\n",
    handle_to_int(spdp_horizontal_handler.get_handle()), OpenDDS::DCPS::LogAddr(spdp_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SEDP Horizontal %d listening on %C\n",
    handle_to_int(sedp_horizontal_handler.get_handle()), OpenDDS::DCPS::LogAddr(sedp_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Data Horizontal %d listening on %C\n",
    handle_to_int(data_horizontal_handler.get_handle()), OpenDDS::DCPS::LogAddr(data_horizontal_addr).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SPDP Vertical %d listening on %C\n",
    handle_to_int(spdp_vertical_handler.get_handle()), OpenDDS::DCPS::LogAddr(spdp_vertical_addr).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SEDP Vertical %d listening on %C\n",
    handle_to_int(sedp_vertical_handler.get_handle()), OpenDDS::DCPS::LogAddr(sedp_vertical_addr).c_str()));
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Data Vertical %d listening on %C\n",
    handle_to_int(data_vertical_handler.get_handle()), OpenDDS::DCPS::LogAddr(data_vertical_addr).c_str()));

  // Write about the relay.
  DDS::DataWriterListener_var relay_address_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::relay_address_sub_count);
  DDS::DataWriter_var relay_address_writer_var = relay_publisher->create_datawriter(relay_addresses_topic, writer_qos, relay_address_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_address_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Address data writer\n"));
    return EXIT_FAILURE;
  }

  RelayAddressDataWriter_var relay_address_writer = RelayAddressDataWriter::_narrow(relay_address_writer_var);
  if (!relay_address_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Address data writer\n"));
    return EXIT_FAILURE;
  }

  RelayAddress relay_address;
  relay_address.relay_id(config.relay_id());
  relay_address.name(HSPDP);
  relay_address.address(OpenDDS::DCPS::LogAddr(spdp_horizontal_addr).str());
  ret = relay_address_writer->write(relay_address, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to write Relay Address\n"));
    return EXIT_FAILURE;
  }
  relay_address.name(HSEDP);
  relay_address.address(OpenDDS::DCPS::LogAddr(sedp_horizontal_addr).str());
  ret = relay_address_writer->write(relay_address, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to write Relay Address\n"));
    return EXIT_FAILURE;
  }
  relay_address.name(HDATA);
  relay_address.address(OpenDDS::DCPS::LogAddr(data_horizontal_addr).str());
  ret = relay_address_writer->write(relay_address, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to write Relay Address\n"));
    return EXIT_FAILURE;
  }

  DDS::DataWriterQos relay_status_writer_qos;
  relay_publisher->get_default_datawriter_qos(relay_status_writer_qos);

  relay_status_writer_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  relay_status_writer_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  relay_status_writer_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  relay_status_writer_qos.history.depth = 1;
  if (config.publish_relay_status_liveliness() != OpenDDS::DCPS::TimeDuration::zero_value) {
    relay_status_writer_qos.liveliness.lease_duration = config.publish_relay_status_liveliness().to_dds_duration();
    relay_status_writer_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
  }

  DDS::DataWriterListener_var relay_status_writer_listener =
    new StatisticsWriterListener(relay_statistics_reporter, &RelayStatisticsReporter::relay_status_sub_count);
  DDS::DataWriter_var relay_status_writer_var = relay_publisher->create_datawriter(relay_status_topic, relay_status_writer_qos, relay_status_writer_listener, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!relay_status_writer_var) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Relay Status data writer\n"));
    return EXIT_FAILURE;
  }

  RelayStatusDataWriter_var relay_status_writer = RelayStatusDataWriter::_narrow(relay_status_writer_var);
  if (!relay_status_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow Relay Status data writer\n"));
    return EXIT_FAILURE;
  }

  RelayConfigTypeSupport_var relay_config_ts = new RelayConfigTypeSupportImpl;
  if (relay_config_ts->register_type(relay_participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to register RelayConfig type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_config_type_name = relay_config_ts->get_type_name();

  DDS::Topic_var relay_config_control_topic =
    relay_participant->create_topic(RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str(), relay_config_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr, 0);
  if (!relay_config_control_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create %C\n", RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }

  DDS::StringSeq filter_params(1);
  filter_params.length(1);
  filter_params[0] = config.relay_id().c_str();
  DDS::ContentFilteredTopic_var cft_config_ctrl =
    relay_participant->create_contentfilteredtopic("Filtered Relay Config Control", relay_config_control_topic,
                                                   "relay_id = %0", filter_params);
  if (!cft_config_ctrl) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create Content-Filtered Topic for %C\n",
               RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }

  DDS::DataReaderQos relay_config_control_qos;
  relay_subscriber->get_default_datareader_qos(relay_config_control_qos);
  relay_config_control_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataReaderListener_var relay_config_control_listener(new RelayConfigControlListener);
  DDS::DataReader_var relay_config_control_reader =
    relay_subscriber->create_datareader(cft_config_ctrl, relay_config_control_qos,
                                        relay_config_control_listener, DDS::DATA_AVAILABLE_STATUS);
  if (!relay_config_control_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create %C data reader\n", RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }

  DDS::Topic_var relay_config_status_topic =
    relay_participant->create_topic(RELAY_CONFIG_STATUS_TOPIC_NAME.c_str(), relay_config_type_name,
                                    TOPIC_QOS_DEFAULT, nullptr, 0);
  if (!relay_config_status_topic) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create %C\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }

  DDS::DataWriter_var relay_config_status_writer =
    relay_publisher->create_datawriter(relay_config_status_topic, writer_qos, nullptr, 0);
  if (!relay_config_status_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to create %C data writer\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }
  RelayConfigDataWriter_var relay_config_status_data_writer = RelayConfigDataWriter::_narrow(relay_config_status_writer);
  if (!relay_config_status_data_writer) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to narrow %C data writer\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }
  const auto internal_config_listener =
    OpenDDS::DCPS::make_rch<InternalConfigListener>(relay_config_status_data_writer, config.relay_id());
  const auto internal_config_reader =
    OpenDDS::DCPS::make_rch<OpenDDS::DCPS::ConfigReader>(TheServiceParticipant->config_store()->datareader_qos(),
                                                         internal_config_listener);
  TheServiceParticipant->config_topic()->connect(internal_config_reader);

  RelayStatusReporter relay_status_reporter(config, *guid_addr_set, relay_status_writer, reactor);

  RelayHttpMetaDiscovery relay_http_meta_discovery(config, meta_discovery_content_type, meta_discovery_content, *guid_addr_set);
  if (relay_http_meta_discovery.open(meta_discovery_addr, reactor) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: could not open RelayHttpMetaDiscovery\n"));
    return EXIT_FAILURE;
  }
  ACE_DEBUG((LM_INFO, "(%P|%t) INFO: Meta Discovery listening on %C\n", OpenDDS::DCPS::LogAddr(meta_discovery_addr).c_str()));

  const auto run_thread_mon = TheServiceParticipant->get_thread_status_manager().update_thread_status();
  if (run_thread_mon && relay_thread_monitor->start() != EXIT_SUCCESS) {
    ACE_ERROR((LM_ERROR, "(%P:%t) ERROR: Failed to start Relay Thread Monitor\n"));
    return EXIT_FAILURE;
  }

  const auto status = reactor_task->run_reactor(config.handler_threads(), config.run_time());
  if (status != EXIT_SUCCESS) {
    ACE_ERROR((LM_ERROR, "(%P:%t) ERROR: Failed to run reactor task: %m\n"));
    return EXIT_FAILURE;
  }

  if (run_thread_mon) {
    relay_thread_monitor->stop();
  }

  TheServiceParticipant->config_topic()->disconnect(internal_config_reader);

  application_participant->delete_contained_entities();
  factory->delete_participant(application_participant);

  relay_participant->delete_contained_entities();
  factory->delete_participant(relay_participant);

  TheServiceParticipant->shutdown();

  spdp_vertical_handler.stop();
  sedp_vertical_handler.stop();
  data_vertical_handler.stop();

  return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    return run(argc, argv);
  } catch (const CORBA::Exception& e) { // IDL-to-C++ classic sequences
    e._tao_print_exception("RtpsRelay.cpp run():", stderr);
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: exception thrown from run(): %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: unknown exception thrown from run()\n"));
  }
  return EXIT_FAILURE;
}
