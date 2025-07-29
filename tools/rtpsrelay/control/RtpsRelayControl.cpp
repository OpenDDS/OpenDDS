/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>

#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#  include <dds/DCPS/security/BuiltInPlugins.h>
#endif

#include <ace/Arg_Shifter.h>
#include <ace/Argv_Type_Converter.h>
#include <ace/Event_Handler.h>
#include <ace/Sig_Handler.h>

#include <cstdlib>
#include <string>

using namespace RtpsRelay;

bool keep_running{false}, show_all{false};
static constexpr char prefix[]{"RTPS_RELAY_"};

void filter(RelayConfig& config)
{
  if (show_all) {
    return;
  }
  for (auto iter{config.config().begin()}; iter != config.config().end();) {
    if (iter->first.substr(0, sizeof(prefix) - 1) == prefix) {
      ++iter;
    } else {
      config.config().erase(iter++);
    }
  }
}

struct ShutdownHandler : ACE_Event_Handler {
  explicit ShutdownHandler(const DDS::GuardCondition_var& guard)
    : guard_{guard}
  {
    signals_.register_handler(SIGTERM, this);
    signals_.register_handler(SIGINT, this);
  }

  int handle_signal(int, siginfo_t*, ucontext_t*) override
  {
    guard_->set_trigger_value(true);
    return 0;
  }

  ACE_Sig_Handler signals_;
  const DDS::GuardCondition_var guard_;
};

int write_if_ready(const RelayConfig& config,
                   RelayConfigDataWriter_var& relay_config_data_writer,
                   const DDS::WaitSet_var& waiter)
{
  DDS::PublicationMatchedStatus matches{};
  if (relay_config_data_writer->get_publication_matched_status(matches) != ::DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: get_publication_matched_status failed\n"));
    return EXIT_FAILURE;
  }
  if (matches.current_count) {
    if (relay_config_data_writer->write(config, DDS::HANDLE_NIL) != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: write failed\n"));
      return EXIT_FAILURE;
    }
    constexpr DDS::Duration_t timeout{DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    if (relay_config_data_writer->wait_for_acknowledgments(timeout) != ::DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: wait_for_acknowledgments failed\n"));
      return EXIT_FAILURE;
    }
    std::cerr << "Sent control" << std::endl;
    if (!keep_running) {
      std::cerr << "Running until status contains requested changes" << std::endl;
    }
    DDS::StatusCondition_var status_cond{relay_config_data_writer->get_statuscondition()};
    DDS::ConditionSeq conditions;
    waiter->get_conditions(conditions);
    for (DDS::UInt32 i{0}; i < conditions.length(); ++i) {
      if (status_cond == conditions[i]) {
        waiter->detach_condition(status_cond);
      }
    }
    DDS::Publisher_var pub{relay_config_data_writer->get_publisher()};
    pub->delete_datawriter(relay_config_data_writer);
    relay_config_data_writer = nullptr;
  }
  return EXIT_SUCCESS;
}

int read(const RelayConfigDataReader_var& reader, RelayConfig& config, bool& done)
{
  RelayConfig sample;
  DDS::SampleInfo info;
  DDS::ReturnCode_t ret;
  while ((ret = reader->take_next_sample(sample, info)) == DDS::RETCODE_OK) {
    if (info.valid_data) {
      filter(sample);
      std::cout << OpenDDS::DCPS::to_json(sample) << std::endl;
      if (sample.relay_id() == config.relay_id()) {
        for (auto it{config.config().begin()}; it != config.config().end();) {
          const auto sample_it{sample.config().find(it->first)};
          if (sample_it != sample.config().end() && sample_it->second == it->second) {
            config.config().erase(it++);
          } else {
            ++it;
          }
        }
        if (config.config().empty() && !keep_running) {
          done = true;
        }
      }
    } else {
      if (info.instance_state != DDS::ALIVE_INSTANCE_STATE) {
        std::cerr << "Relay no longer available: " << sample.relay_id() << std::endl;
        if (sample.relay_id() == config.relay_id() && !keep_running) {
          done = true;
        }
      }
    }
  }

  if (ret != DDS::RETCODE_NO_DATA) {
    ACE_ERROR((LM_ERROR, "ERROR: read returned %C\n", OpenDDS::DCPS::retcode_to_string(ret)));
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int run(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var factory{TheParticipantFactoryWithArgs(argc, argv)};
  if (!factory) {
    ACE_ERROR((LM_ERROR, "ERROR: Failed to get participant factory\n"));
    return EXIT_FAILURE;
  }

  DDS::DomainId_t domain{0};
  RelayConfig config;

  ACE_Argv_Type_Converter atc{argc, argv};
  ACE_Arg_Shifter_T<char> args{atc.get_argc(), atc.get_ASCII_argv()};
  for (args.ignore_arg(); args.is_anything_left(); args.consume_arg()) {
    const char* arg{nullptr};
    if ((arg = args.get_the_parameter("-Domain"))) {
      domain = std::atoi(arg);
    } else if ((arg = args.get_the_parameter("-RelayId"))) {
      config.relay_id(arg);
    } else if ((arg = args.get_the_parameter("-Set"))) {
      const std::string arg_str{arg};
      const auto pos{arg_str.find_first_of('=')};
      if (pos && pos != std::string::npos) {
        const auto key{OpenDDS::DCPS::ConfigPair::canonicalize(arg_str.substr(0, pos))};
        config.config()[key] = arg_str.substr(pos + 1);
        if (key.substr(0, sizeof(prefix) - 1) != prefix) {
          show_all = true;
        }
      } else {
        throw std::runtime_error{"Argument to -Set must be NAME=Value: " + arg_str};
      }
    } else if (args.cur_arg_strncasecmp("-ShowAll") == 0) {
      show_all = true;
    } else if (args.cur_arg_strncasecmp("-KeepRunning") == 0) {
      keep_running = true;
    } else {
      ACE_ERROR((LM_ERROR, "ERROR: Invalid argument: %C\n", args.get_current()));
      return EXIT_FAILURE;
    }
  }

  if (keep_running || config.relay_id().empty()) {
    std::cerr << "Running until Ctrl-C" << std::endl;
  }

  DDS::DomainParticipantQos participant_qos;
  factory->get_default_participant_qos(participant_qos);
  {
    const auto idx{participant_qos.property.value.length()};
    participant_qos.property.value.length(idx + 1);
    const DDS::Property_t p{OpenDDS::RTPS::RTPS_REFLECT_HEARTBEAT_COUNT, "true", false};
    participant_qos.property.value[idx] = p;
  }
  DDS::DomainParticipant_var participant{factory->create_participant(domain, participant_qos, nullptr, 0)};
  if (!participant) {
    ACE_ERROR((LM_ERROR, "ERROR: Failed to create relay participant\n"));
    return EXIT_FAILURE;
  }

  RelayConfigTypeSupport_var relay_config_ts{new RelayConfigTypeSupportImpl};
  if (relay_config_ts->register_type(participant, "") != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: failed to register RelayConfig type\n"));
    return EXIT_FAILURE;
  }
  CORBA::String_var relay_config_type_name{relay_config_ts->get_type_name()};

  DDS::PublisherQos publisher_qos;
  participant->get_default_publisher_qos(publisher_qos);
  publisher_qos.partition.name.length(1);
  publisher_qos.partition.name[0] = config.relay_id().empty() ? "*" : config.relay_id().c_str();
  DDS::SubscriberQos subscriber_qos;
  participant->get_default_subscriber_qos(subscriber_qos);
  subscriber_qos.partition.name = publisher_qos.partition.name;

  DDS::WaitSet_var waiter = new DDS::WaitSet;

  RelayConfigDataWriter_var relay_config_data_writer;
  if (!config.config().empty()) {
    if (config.relay_id().empty()) {
      ACE_ERROR((LM_ERROR, "ERROR: Attempting to change configuration without a -RelayId\n"));
      return EXIT_FAILURE;
    }

    DDS::Publisher_var publisher{participant->create_publisher(publisher_qos, nullptr, 0)};
    if (!publisher) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to create publisher\n"));
      return EXIT_FAILURE;
    }

    DDS::Topic_var relay_config_control_topic{
      participant->create_topic(RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str(), relay_config_type_name,
                                TOPIC_QOS_DEFAULT, nullptr, 0)};
    if (!relay_config_control_topic) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to create %C\n", RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
      return EXIT_FAILURE;
    }

    DDS::DataWriter_var dw{publisher->create_datawriter(relay_config_control_topic, DATAWRITER_QOS_DEFAULT, nullptr, 0)};
    if (!dw) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to create %C data writer\n", RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
      return EXIT_FAILURE;
    }
    relay_config_data_writer = RelayConfigDataWriter::_narrow(dw);
    if (!relay_config_data_writer) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to narrow %C data writer\n", RELAY_CONFIG_CONTROL_TOPIC_NAME.c_str()));
      return EXIT_FAILURE;
    }

    DDS::StatusCondition_var cond = relay_config_data_writer->get_statuscondition();
    cond->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
    waiter->attach_condition(cond);
  }

  DDS::Subscriber_var subscriber{participant->create_subscriber(subscriber_qos, nullptr, 0)};
  if (!subscriber) {
    ACE_ERROR((LM_ERROR, "ERROR: failed to create subscriber\n"));
    return EXIT_FAILURE;
  }

  DDS::Topic_var relay_config_status_topic{
    participant->create_topic(RELAY_CONFIG_STATUS_TOPIC_NAME.c_str(), relay_config_type_name,
                              TOPIC_QOS_DEFAULT, nullptr, 0)};
  if (!relay_config_status_topic) {
    ACE_ERROR((LM_ERROR, "ERROR: failed to create %C\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }

  DDS::DataReaderQos reader_qos;
  subscriber->get_default_datareader_qos(reader_qos);
  reader_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  reader_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  DDS::DataReader_var dr{subscriber->create_datareader(relay_config_status_topic, reader_qos, nullptr, 0)};
  if (!dr) {
    ACE_ERROR((LM_ERROR, "ERROR: failed to create %C data reader\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }
  RelayConfigDataReader_var relay_config_data_reader{RelayConfigDataReader::_narrow(dr)};
  if (!relay_config_data_reader) {
    ACE_ERROR((LM_ERROR, "ERROR: failed to narrow %C data reader\n", RELAY_CONFIG_STATUS_TOPIC_NAME.c_str()));
    return EXIT_FAILURE;
  }
  DDS::ReadCondition_var read_cond{
    dr->create_readcondition(DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE)};
  waiter->attach_condition(read_cond);

  DDS::GuardCondition_var shutdown_guard{new DDS::GuardCondition};
  ShutdownHandler handler{shutdown_guard};
  waiter->attach_condition(shutdown_guard);

  for (auto done{false}; !done;) {
    if (relay_config_data_writer) {
      if (write_if_ready(config, relay_config_data_writer, waiter) != EXIT_SUCCESS) {
        return EXIT_FAILURE;
      }
    }
    constexpr DDS::Duration_t timeout{DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    DDS::ConditionSeq active_conditions;
    if (waiter->wait(active_conditions, timeout) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to wait()\n"));
      return EXIT_FAILURE;
    }
    for (DDS::UInt32 i{0}; i < active_conditions.length(); ++i) {
      if (read_cond == active_conditions[i]) {
        if (read(relay_config_data_reader, config, done) != EXIT_SUCCESS) {
          return EXIT_FAILURE;
        }
      } else if (shutdown_guard == active_conditions[i]) {
        done = true;
        break;
      }
    }
  }

  DDS::ConditionSeq conditions;
  waiter->get_conditions(conditions);
  waiter->detach_conditions(conditions);

  participant->delete_contained_entities();
  factory->delete_participant(participant);
  participant = nullptr;
  factory = nullptr;
  TheServiceParticipant->shutdown();
  return EXIT_SUCCESS;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    return run(argc, argv);
  } catch (const CORBA::Exception& e) { // IDL-to-C++ classic sequences
    e._tao_print_exception("RtpsRelayControl.cpp run():", stderr);
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "ERROR: exception thrown from run(): %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "ERROR: unknown exception thrown from run()\n"));
  }
  return EXIT_FAILURE;
}
