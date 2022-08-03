/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Recorder.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/ArgParsing.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <iostream>
#include <string>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

std::string prog_name;

std::string topic_name;
std::string type_name;
DDS::DomainId_t domainid = 0;
unsigned num_samples = 0;
unsigned num_seconds = 0;
bool writer_count = false;

void parse_args(int argc, ACE_TCHAR* argv[])
{
  using namespace OpenDDS::DCPS::ArgParsing;

  ArgParser arg_parser(
    "Print samples written to the given topic when the remotes support DDS XTypes "
    "complete TypeObjects.");

  Positional topic_name_arg(arg_parser, "TOPIC_NAME",
    "The name of the topic to listen for.",
    topic_name);
  Positional type_name_arg(arg_parser, "TYPE_NAME",
    "The full name (including any modules) of the topic type. "
    "This should NOT include a leading ::.",
    type_name);
  PositionalAs<IntValue<DDS::DomainId_t> > domainid_arg(arg_parser, "DOMAIN_ID",
    "The DDS Domain to participant in.",
    domainid);

  Option write_count_opt(arg_parser, "writer-count",
    "Print number of associated writers when they change. "
    "Default is to not to.",
    writer_count);
  write_count_opt.add_alias("w");
  OptionAs<IntValue<unsigned> > samples_opt(arg_parser, "samples",
    "Wait for at least this number of samples and exit. "
    "May actually print more. Default is to print samples forever.",
    num_samples, "COUNT");
  OptionAs<IntValue<unsigned> > time_opt(arg_parser, "time",
    "Print samples for the given number of seconds and exit. "
    "Default is to print samples forever.",
    num_seconds, "SECONDS");

  arg_parser.parse(argc, argv);
}

class RecorderListenerImpl : public RecorderListener {
public:
  explicit RecorderListenerImpl(DDS::GuardCondition_var gc)
    : sample_count_(0)
    , gc_(gc)
  {
  }

  virtual void on_sample_data_received(Recorder* rec,
                                       const RawDataSample& sample)
  {
    DDS::DynamicData_var dd = rec->get_dynamic_data(sample);
    String my_type;
    String indent;
    if (!OpenDDS::XTypes::print_dynamic_data(dd, my_type, indent)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RecorderListenerImpl::on_sample_data_received: "
        "Failed to read dynamic data\n"));
    }
    std::cout << my_type;
    ++sample_count_;
    if (num_samples > 0 && sample_count_ >= num_samples) {
      gc_->set_trigger_value(true);
    }
  }

  virtual void on_recorder_matched(Recorder*,
                                   const ::DDS::SubscriptionMatchedStatus& status)
  {
    if (writer_count) {
      std::cout << "Listening to " << status.current_count << " writer(s)\n";
    }
  }

  unsigned sample_count() const
  {
    return sample_count_;
  }

private:
  unsigned sample_count_;
  DDS::GuardCondition_var gc_;
};

int run(int argc, ACE_TCHAR* argv[])
{
  unsigned long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);

  int ret_val = 0;
  try {
    TransportConfig_rch transport_config =
      TheTransportRegistry->create_config("default_rtps_transport_config");
    TransportInst_rch transport_inst =
      TheTransportRegistry->create_inst("default_rtps_transport", "rtps_udp");
    transport_config->instances_.push_back(transport_inst);
    TheTransportRegistry->global_config(transport_config);

    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    parse_args(argc, argv);

    OpenDDS::RTPS::RtpsDiscovery_rch disc =
      make_rch<OpenDDS::RTPS::RtpsDiscovery>("rtps_disc");
    disc->use_xtypes(OpenDDS::RTPS::RtpsDiscoveryConfig::XTYPES_COMPLETE);
    Service_Participant* const service = TheServiceParticipant;
    service->add_discovery(static_rchandle_cast<Discovery>(disc));
    service->set_repo_domain(domainid, disc->key());

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(domainid,
                              PARTICIPANT_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    if (!participant) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_participant failed!\n"));
      }
      return 1;
    }

    DDS::GuardCondition_var gc = new DDS::GuardCondition;
    DDS::WaitSet_var ws = new DDS::WaitSet;
    DDS::ReturnCode_t ret = ws->attach_condition(gc);
    {
      DDS::Topic_var topic =
        service->create_typeless_topic(participant,
                                       topic_name.c_str(),
                                       type_name.c_str(),
                                       true,
                                       TOPIC_QOS_DEFAULT,
                                       0,
                                       DEFAULT_STATUS_MASK);

      if (!topic) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_topic failed!\n"));
        }
        return 1;
      }

      RcHandle<RecorderListenerImpl> recorder_listener = make_rch<RecorderListenerImpl>(gc);

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos(sub_qos);

      DDS::DataReaderQos dr_qos = service->initial_DataReaderQos();
      dr_qos.representation.value.length(2);
      dr_qos.representation.value[0] = DDS::XCDR_DATA_REPRESENTATION;
      dr_qos.representation.value[1] = DDS::XCDR2_DATA_REPRESENTATION;
      dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

      // Create Recorder
      Recorder_var recorder =
        service->create_recorder(participant,
                                 topic.in(),
                                 sub_qos,
                                 dr_qos,
                                 recorder_listener);
      if (!recorder.in()) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): create_recorder failed!\n"));
        }
        return 1;
      }

      DDS::Duration_t timeout = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
      if (num_seconds > 0) {
        timeout.sec = num_seconds;
        timeout.nanosec = 0;
      }
      DDS::ConditionSeq conditions;
      ret = ws->wait(conditions, timeout);
      if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_TIMEOUT) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): wait failed!\n"));
        }
        return 1;
      }

      ws->detach_condition(gc);
      service->delete_recorder(recorder);
    }
    participant->delete_contained_entities();
    dpf->delete_participant(participant);
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return 1;
  }
  if (ret_val == 1) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: main(): failed to properly analyze sample!\n"));
    }
    return 1;
  }
  return ret_val;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int ret = run(argc, argv);
  if (!ret) {
    ACE_Thread_Manager::instance()->wait();
  }
  return ret;
}
