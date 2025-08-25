/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/Atomic.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/XTypes/DynamicDataXcdrReadImpl.h>
#include <dds/DCPS/XTypes/DynamicTypeSupport.h>
#include <dds/DCPS/XTypes/Utils.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/RTPS/RtpsDiscoveryConfig.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportConfig.h>
#include <dds/DCPS/transport/framework/TransportInst.h>
#if defined ACE_AS_STATIC_LIBS && !OPENDDS_CONFIG_SAFETY_PROFILE
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
Atomic<unsigned> current_sample_count;
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

bool check_rc(DDS::ReturnCode_t rc, const std::string& what)
{
  if (rc != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %C: %C\n", what.c_str(), retcode_to_string(rc)));
    return false;
  }
  return true;
}

struct Topic : public virtual LocalObject<DDS::DataReaderListener> {
  std::string name;
  std::string type_name;
  std::string path;
  DDS::DynamicType_var type;
  DDS::TypeSupport_var ts;
  DDS::Topic_var topic;
  DDS::DataReader_var reader;
  DDS::GuardCondition_var gc;

  Topic(const std::string& name, const std::string& type_name, DDS::GuardCondition_var gc)
  : name(name)
  , type_name(type_name)
  , gc(gc)
  {
  }

  bool matches_topic_queries()
  {
    return name == topic_name && this->type_name == type_name;
  }

  void print_dynamic_data(DDS::DynamicData_ptr dd)
  {
    std::string repr;
    std::string indent;
    if (OpenDDS::XTypes::print_dynamic_data(dd, repr, indent)) {
      std::cout << repr;
      std::cout.flush();
    } else {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Failed to print dynamic data\n"));
    }
  }

  virtual void print() = 0;

  void on_requested_deadline_missed(DDS::DataReader_ptr, const DDS::RequestedDeadlineMissedStatus&)
  {
  }

  void on_requested_incompatible_qos(
    DDS::DataReader_ptr, const DDS::RequestedIncompatibleQosStatus&)
  {
  }

  void on_sample_rejected(DDS::DataReader_ptr, const DDS::SampleRejectedStatus&)
  {
  }

  void on_liveliness_changed(DDS::DataReader_ptr, const DDS::LivelinessChangedStatus&)
  {
  }

  void on_data_available(DDS::DataReader_ptr reader)
  {
    ACE_DEBUG((LM_DEBUG, "Sample %C - %C\n", name.c_str(), type_name.c_str()));
    print();
    ++current_sample_count;
    if (num_samples > 0 && current_sample_count >= num_samples) {
      gc->set_trigger_value(true);
    }
  }

  void on_subscription_matched(DDS::DataReader_ptr, const DDS::SubscriptionMatchedStatus& status)
  {
    if (writer_count) {
      std::cout << "Listening to " << status.current_count << " writer(s)\n";
    }
  }

  void on_sample_lost(DDS::DataReader_ptr, const DDS::SampleLostStatus&)
  {
  }
};

struct DynamicTopic : public Topic {
  DDS::DynamicDataReader_var ddreader;

  DynamicTopic(const std::string& name, const std::string& type_name, DDS::GuardCondition_var gc)
  : Topic(name, type_name, gc)
  {
  }

  bool init(DDS::DomainParticipant_var& dp, DDS::Subscriber_var& sub,
    DDS::PublicationBuiltinTopicData& pb)
  {
    if (!check_rc(TheServiceParticipant->get_dynamic_type(type, dp, pb.key), "get_dynamic_type")) {
      return false;
    }

    ts = new DDS::DynamicTypeSupport(type);
    if (!check_rc(ts->register_type(dp, ""), "register_type")) {
      return false;
    }

    CORBA::String_var type_name = ts->get_type_name();
    topic = dp->create_topic(name.c_str(), type_name, TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!topic) {
      ACE_ERROR((LM_ERROR, "ERROR: create_topic \"%C\" with type \"%C\" failed!\n",
        name.c_str(), type_name.in()));
      return false;
    }

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.representation.value.length(2);
    dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
    dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    reader = sub->create_datareader(topic, dr_qos, this, DEFAULT_STATUS_MASK);
    ddreader = DDS::DynamicDataReader::_narrow(reader);
    if (!ddreader) {
      ACE_ERROR((LM_ERROR, "ERROR: create_datareader failed!\n"));
      return false;
    }

    return true;
  }

  void print()
  {
    DDS::DynamicDataSeq seq;
    DDS::SampleInfoSeq info;
    ddreader->read(seq, info, DDS::LENGTH_UNLIMITED,
      DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    for (unsigned i = 0; i < seq.length(); ++i) {
      print_dynamic_data(seq[i]);
    }
  }
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

    RtpsDiscovery_rch disc = make_rch<RtpsDiscovery>("rtps_disc");
    disc->use_xtypes(RtpsDiscoveryConfig::XTYPES_COMPLETE);
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
      ACE_ERROR((LM_ERROR, "ERROR: create_participant failed!\n"));
      return 1;
    }

    DDS::Subscriber_var sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    if (!sub) {
      ACE_ERROR((LM_ERROR, "ERROR: create_subscriber failed!\n"));
      return false;
    }

    DDS::GuardCondition_var gc = new DDS::GuardCondition;

    DDS::Subscriber_var bit_subscriber = participant->get_builtin_subscriber();
    DDS::DataReader_var pub_reader = bit_subscriber->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
    DDS::PublicationBuiltinTopicDataDataReader_var pub_reader_i =
      DDS::PublicationBuiltinTopicDataDataReader::_narrow(pub_reader);
    if (!pub_reader_i) {
      ACE_ERROR((LM_ERROR, "ERROR: failed to get BIT Publication DataReader\n"));
      return 1;
    }

    DDS::ReadCondition_var dr_rc = pub_reader->create_readcondition(
      DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    if (!check_rc(ws->attach_condition(gc), "attach guard condition")) {
      return 1;
    }
    if (!check_rc(ws->attach_condition(dr_rc), "attach read condition")) {
      return 1;
    }

    const DDS::Duration_t infinite = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};
    const bool use_timeout = num_seconds > 0;
    const SystemTimePoint end = SystemTimePoint::now() + TimeDuration(num_seconds);
    while (true) {
      DDS::ConditionSeq active;
      const DDS::Duration_t timeout = use_timeout ? (end - SystemTimePoint::now()).to_dds_duration() : infinite;
      if (!check_rc(ws->wait(active, timeout), "wait on publication BIT")) {
        break;
      }
      if (gc->get_trigger_value() || (use_timeout && SystemTimePoint::now() >= end)) {
        ACE_DEBUG((LM_DEBUG, "Reached stop condition\n"));
        break;
      }

      DDS::PublicationBuiltinTopicDataSeq pub_bit_seq;
      DDS::SampleInfoSeq info;
      pub_reader_i->read(pub_bit_seq, info, DDS::LENGTH_UNLIMITED,
        DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
      DynamicTopic* topic = 0;
      for (unsigned i = 0; i < pub_bit_seq.length(); ++i) {
        DDS::PublicationBuiltinTopicData& pb = pub_bit_seq[i];
        const std::string name = pb.topic_name.in();
        const std::string type_name = pb.type_name.in();
        if (topic) {
          continue;
        }
        ACE_DEBUG((LM_DEBUG, "Learned about topic \"%C\" type \"%C\"\n",
          name.c_str(), type_name.c_str()));
        topic = new DynamicTopic(name, type_name, gc);
        if (topic->matches_topic_queries()) {
          if (topic->init(participant, sub, pb)) {
            continue;
          }
        } else {
          ACE_DEBUG((LM_DEBUG, "Ignoring\n"));
        }
        delete topic;
        topic = 0;
      }
    }

    ws->detach_condition(gc);
    ws->detach_condition(dr_rc);
    pub_reader->delete_readcondition(dr_rc);
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
