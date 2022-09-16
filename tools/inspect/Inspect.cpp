/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Recorder.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
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
#include <vector>
#include <string>
#include <utility>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

typedef std::vector<std::string> Args;
typedef Args::iterator ArgsIt;
typedef std::pair<bool, ArgsIt> AfterOpt;

AfterOpt get_option(Args& args, const std::string& opt)
{
  for (ArgsIt args_it = args.begin(); args_it != args.end(); ++args_it) {
    if (*args_it == "--") {
      // Remaining arguments should be interpreted as positional arguments
      return AfterOpt(false, args.end());
    }
    if (opt.length() <= args_it->length()) {
      const bool starts_with = args_it->substr(0, opt.length()) == opt;
      const bool same_size = args_it->length() == opt.length();
      if (starts_with && !same_size && args_it->at(opt.length()) == '=') {
        // Form is OPT=VALUE, replace current with VALUE
        *args_it = args_it->substr(opt.length() + 1);
        return AfterOpt(true, args_it);
      } else if (same_size && starts_with) {
        // Form is OPT VALUE?, return next argument
        return AfterOpt(true, args.erase(args_it));
      }
    }
  }
  return AfterOpt(false, args.end());
}

bool has_option(Args& args, const std::string& opt)
{
  return get_option(args, opt).first;
}

std::string get_option_argument(Args& args, AfterOpt& after_opt, const std::string& opt)
{
  ArgsIt& args_it = after_opt.second;
  if (args_it == args.end()) {
    std::cerr << "ERROR: Option " << opt << " requires an argument" << std::endl;
    throw 1;
  }
  const std::string value = *args_it;
  args_it = args.erase(args_it);
  return value;
}

template <typename UintType>
UintType get_option_argument_uint(Args& args, AfterOpt& after_opt, const std::string& opt)
{
  UintType value;
  if (!convertToInteger(get_option_argument(args, after_opt, opt), value)) {
    std::cerr << "ERROR: Option " << opt
      << " requires an argument that's a valid non-negative number" << std::endl;
    throw 1;
  }
  return value;
}

void check_for_unknown_options(Args& args)
{
  for (ArgsIt args_it = args.begin(); args_it != args.end(); ++args_it) {
    if (*args_it == "--") {
      // Remaining arguments should be interpreted as positional arguments
      return;
    }
    if (args_it->length() && args_it->at(0) == '-') {
      std::cerr << "ERROR: Option " << *args_it << " is invalid" << std::endl;
      throw 1;
    }
  }
}

template <typename IntType>
IntType get_pos_argument_int(Args& args, size_t pos, const std::string& what)
{
  IntType value;
  if (!convertToInteger(args[pos], value)) {
    std::cerr << "ERROR: positional argument " << what
      << " should be a valid number" << std::endl;
    throw 1;
  }
  return value;
}

std::string prog_name;

std::string topic_name;
std::string type_name;
DDS::DomainId_t domainid = 0;
unsigned num_samples = 0;
unsigned num_seconds = 0;
bool help = false;
bool writer_count = false;

void print_usage(bool for_error = false)
{
  std::ostream& os = for_error ? std::cerr : std::cout;
  os <<
    "usage: " << prog_name << " [OPTIONS] TOPIC_NAME TYPE_NAME DOMAIN_ID\n"
    "usage: " << prog_name << " --help|-h|--version|-v\n";
  if (for_error) {
    os << "See -h for more details\n";
  }
}

void print_help()
{
  print_usage();
  std::cout <<
    "\n"
    "Print samples written to the given topic when the remotes support DDS XTypes\n"
    "complete TypeObjects.\n"
    "\n"
    "Positional Arguments:\n"
    "  TOPIC_NAME              The name of the topic to listen for.\n"
    "  TYPE_NAME               The full name (including any modules) of the topic\n"
    "                          type. This should NOT include a leading ::.\n"
    "  DOMAIN_ID               The DDS Domain to participant in.\n"
    "\n"
    "OPTIONS:\n"
    "  All OpenDDS command line options listed in section 7.2 of the OpenDDS\n"
    "  Developer's Guide are also available.\n"
    "  -h | --help             Displays this message.\n"
    "  -v | --version          Displays the version. This is the same as OpenDDS's.\n"
    "  -w | --writer-count     Print number of associated writers when they change.\n"
    "                          Default is to not to.\n"
    "  --samples COUNT         Wait for at least this number of samples and exit.\n"
    "                          May actually print more. Default is to print samples\n"
    "                          forever.\n"
    "  --time SECONDS          Print samples for the given number of seconds and\n"
    "                          exit. Default is to print samples forever.\n";
}

// parse the command line arguments
int parse_args(int argc, ACE_TCHAR* argv[])
{
  unsigned long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);

  prog_name = ACE_TEXT_ALWAYS_CHAR(argv[0]);
  Args args;
  for (int i = 1; i < argc; ++i) {
    args.push_back(ACE_TEXT_ALWAYS_CHAR(argv[i]));
  }

  try {
    // Parse options
    if (has_option(args, "-h") || has_option(args, "--help")) {
      print_help();
      std::exit(0);
    }
    if (has_option(args, "-v") || has_option(args, "--version")) {
      std::cout << "Version " OPENDDS_VERSION << std::endl;
      std::exit(0);
    }
    writer_count = has_option(args, "-w") || has_option(args, "--writer-count");
    {
      const std::string opt = "--samples";
      AfterOpt after_opt = get_option(args, opt);
      if (after_opt.first) {
        num_samples = get_option_argument_uint<unsigned>(args, after_opt, opt);
      }
    }
    {
      const std::string opt = "--time";
      AfterOpt after_opt = get_option(args, opt);
      if (after_opt.first) {
        num_seconds = get_option_argument_uint<unsigned>(args, after_opt, opt);
      }
    }
    check_for_unknown_options(args);

    // Parse positional arguments
    if (args.size() != 3) {
      std::cerr << "ERROR: incorrect number of arguments: expected 3, received " << args.size() << "\n";
      throw 1;
    }
    topic_name = args[0];
    type_name = args[1];
    domainid = get_pos_argument_int<DDS::DomainId_t>(args, 2, "DOMAIN_ID");
  } catch(int value) {
    print_usage(true);
    return value;
  }

  return 0;
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
    ret_val = parse_args(argc, argv);
    if (ret_val) {
      return ret_val;
    }

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
