#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "BenchTypeSupportImpl.h"
#include "PropertyStatBlock.h"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#pragma GCC diagnostic pop

using namespace Bench::NodeController;

struct Node {
  NodeId node_id;
  std::string name;
};
typedef std::map<NodeId, Node, OpenDDS::DCPS::GUID_tKeyLessThan> Nodes;
std::ostream& operator<<(std::ostream& os, const Node& node)
{
  os << node.node_id;
  if (node.name.size()) {
    os << " \"" << node.name << '\'';
  }
  return os;
}

std::string dds_root;

std::string
read_file(const std::string& name)
{
  std::stringstream ss;
  std::ifstream file(name);
  if (file.is_open()) {
    ss << file.rdbuf();
  } else {
    std::cerr << "Couldn't open " << name << std::endl;
  }
  return ss.str();
}

bool
json_2_report(std::istream& is, Bench::WorkerReport& report)
{
  rapidjson::Document document;
  rapidjson::IStreamWrapper isw(is);
  document.ParseStream(isw);
  if (!document.IsObject()) {
    std::cerr << "Expected report file to contain JSON document object" << std::endl;
    return false;
  }

  OpenDDS::DCPS::copyFromRapidJson(document, report);

  return true;
}

class StatusListener : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  StatusListener(Nodes& nodes)
  : nodes_(nodes)
  {
  }

  virtual ~StatusListener()
  {
  }

  virtual void
  on_requested_deadline_missed(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedDeadlineMissedStatus& /* status */)
  {
  }

  virtual void
  on_requested_incompatible_qos(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedIncompatibleQosStatus& /* status */)
  {
  }

  virtual void
  on_liveliness_changed(
    DDS::DataReader_ptr /* reader */,
    const DDS::LivelinessChangedStatus& /* status */)
  {
  }

  virtual void
  on_subscription_matched(
    DDS::DataReader_ptr /* reader */,
    const DDS::SubscriptionMatchedStatus& /* status */)
  {
  }

  virtual void
  on_sample_rejected(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleRejectedStatus& /* status */)
  {
  }

  virtual void
  on_data_available(DDS::DataReader_ptr reader)
  {
    std::lock_guard<std::mutex> lock(discovery_mutex);

    StatusDataReader_var status_reader_impl = StatusDataReader::_narrow(reader);
    if (!status_reader_impl) {
      std::cerr << "narrow status reader failed" << std::endl;
      ACE_OS::exit(1);
    }

    StatusSeq statuses;
    DDS::SampleInfoSeq info;
    DDS::ReturnCode_t rc = status_reader_impl->take(
      statuses, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Take failed" << std::endl;
      ACE_OS::exit(1);
    }

    for (size_t i = 0; i < statuses.length(); i++) {
      Nodes::iterator iter = nodes_.find(statuses[i].node_id);
      bool discovered = iter != nodes_.end();
      if (info[i].valid_data && statuses[i].state == AVAILABLE) {
        Node node;
        node.node_id = statuses[i].node_id;
        node.name = statuses[i].name;
        if (!discovered) {
          if (in_discovery_period_) {
            std::cout << "Discovered Node " << node << std::endl;
            nodes_[node.node_id] = node;
          } else {
            std::cerr << "Warning: Discovered node " << node << " outside discovery period" << std::endl;
          }
        }
      } else if (info[i].instance_state & DDS::NOT_ALIVE_INSTANCE_STATE && discovered) {
        nodes_.erase(iter);
      }
    }
  }

  virtual void
  on_sample_lost(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleLostStatus& /* status */)
  {
  }

  void
  end_discovery()
  {
    std::lock_guard<std::mutex> lock(discovery_mutex);
    in_discovery_period_ = false;
  }

private:
  Nodes& nodes_;
  std::mutex discovery_mutex;
  bool in_discovery_period_ = true;
};

void handle_reports(std::vector<Bench::WorkerReport> parsed_reports);

inline std::string
get_option_argument(int& i, int argc, ACE_TCHAR* argv[])
{
  if (i == argc - 1) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << " requires an argument" << std::endl;
    throw int{1};
  }
  return ACE_TEXT_ALWAYS_CHAR(argv[++i]);
}

inline int
get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[])
{
  int value;
  try {
    value = std::stoll(get_option_argument(i, argc, argv));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << " requires an argument that's a valid number" << std::endl;
    throw 1;
  }
  return value;
}

inline unsigned
get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[])
{
  unsigned value;
  try {
    value = std::stoull(get_option_argument(i, argc, argv));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << " requires an argument that's a valid positive number"
      << std::endl;
    throw 1;
  }
  return value;
}

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  dds_root = ACE_OS::getenv("DDS_ROOT");
  if (dds_root.empty()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("DDS_ROOT isn't defined\n")));
    return 1;
  }

  Nodes nodes;

  TheServiceParticipant->default_configuration_file("rtps_disc.ini");

  // Parse Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  unsigned wait_for_nodes = 3;
  int domain = default_control_domain;

  try {
    for (int i = 1; i < argc; i++) {
      const char* argument = ACE_TEXT_ALWAYS_CHAR(argv[i]);
      if (!ACE_OS::strcmp(argument, "--domain")) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--wait-for-nodes")) {
        wait_for_nodes = get_option_argument_uint(i, argc, argv);
      } else {
        std::cerr << "Invalid Option: " << argument << std::endl;
        return 1;
      }
    }
  } catch(int value) {
    return value;
  }

  // Create Participant
  DDS::DomainParticipant_var participant = dpf->create_participant(
    domain, PARTICIPANT_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant) {
    std::cerr << "create_participant failed" << std::endl;
    return 1;
  }

  // Create Topics
  StatusTypeSupport_var status_ts = new StatusTypeSupportImpl;
  if (status_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Status" << std::endl;
    return 1;
  }
  DDS::Topic_var status_topic = participant->create_topic(
    status_topic_name, status_ts->get_type_name(), TOPIC_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_topic) {
    std::cerr << "create_topic status failed" << std::endl;
    return 1;
  }
  ConfigTypeSupport_var config_ts = new ConfigTypeSupportImpl;
  if (config_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Config" << std::endl;
    return 1;
  }
  DDS::Topic_var config_topic = participant->create_topic(
    config_topic_name, config_ts->get_type_name(), TOPIC_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!config_topic) {
    std::cerr << "create_topic config failed" << std::endl;
    return 1;
  }
  ReportTypeSupport_var report_ts = new ReportTypeSupportImpl;
  if (report_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Report" << std::endl;
    return 1;
  }
  DDS::Topic_var report_topic = participant->create_topic(
    report_topic_name, report_ts->get_type_name(), TOPIC_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_topic) {
    std::cerr << "create_topic report failed" << std::endl;
    return 1;
  }

  // Create DataWriters
  DDS::Publisher_var publisher = participant->create_publisher(
    PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher) {
    std::cerr << "create_publisher failed" << std::endl;
    return 1;
  }
  DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  DDS::DataWriter_var config_writer = publisher->create_datawriter(
    config_topic, dw_qos, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!config_writer) {
    std::cerr << "create_datawriter config failed" << std::endl;
    return 1;
  }
  ConfigDataWriter_var config_writer_impl = ConfigDataWriter::_narrow(config_writer);
  if (!config_writer_impl) {
    std::cerr << "narrow writer config failed" << std::endl;
    return 1;
  }

  // Create DataReaders
  DDS::Subscriber_var subscriber = participant->create_subscriber(
    SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber) {
    std::cerr << "create_subscriber failed" << std::endl;
    return 1;
  }
  DDS::DataReaderQos dr_qos;
  subscriber->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  StatusListener status_listener(nodes);
  DDS::DataReader_var status_reader = subscriber->create_datareader(
    status_topic, dr_qos, &status_listener,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_reader) {
    std::cerr << "create_datareader status failed" << std::endl;
    return 1;
  }
  StatusDataReader_var status_reader_impl = StatusDataReader::_narrow(status_reader);
  if (!status_reader_impl) {
    std::cerr << "narrow status reader failed" << std::endl;
    return 1;
  }
  DDS::DataReader_var report_reader = subscriber->create_datareader(
    report_topic, dr_qos, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_reader) {
    std::cerr << "create_datareader report failed" << std::endl;
    return 1;
  }
  ReportDataReader_var report_reader_impl = ReportDataReader::_narrow(report_reader);
  if (!report_reader_impl) {
    std::cerr << "narrow report reader failed" << std::endl;
    return 1;
  }

  // Discovery Period: wait a period of time while nodes are discovered
  std::cout << "Waiting for nodes for " << wait_for_nodes << " seconds..." << std::endl;
  ACE_OS::sleep(wait_for_nodes);
  status_listener.end_discovery();
  size_t node_count = nodes.size();
  if (node_count == 0) {
    std::cerr << "Error: no nodes discovered during wait" << std::endl;
    return 1;
  }
  std::cout << "Discovered " << node_count << " nodes, continuing with scenario..." << std::endl;

  // Begin Hardcoded Scenario
  if (node_count < 2) {
    std::cerr << "Error: Expecting at least 2 Nodes" << std::endl;
    return 1;
  }

  WorkerConfig worker;
  Config config;
  const std::string configs = dds_root + "/performance-tests/bench/worker/configs/";
  const size_t expected_workers = 3;

  Nodes::iterator ni = nodes.begin();
  config.node_id = ni->first;
  config.workers.length(2);

  worker.worker_id = 1;
  const std::string daemon_config = read_file(configs + "jfti_sim_daemon_config.json");
  worker.config = daemon_config.c_str();
  config.workers[0] = worker;

  worker.worker_id = 2;
  const std::string worker_config = read_file(configs + "jfti_sim_worker_config.json");
  worker.config = worker_config.c_str();
  config.workers[1] = worker;

  if (config_writer_impl->write(config, DDS::HANDLE_NIL)) {
    std::cerr << "1st write failed" << std::endl;
    return 1;
  }

  ++ni;

  config.node_id = ni->first;
  config.workers.length(1);
  worker.worker_id = 1;
  std::string master_config = read_file(configs + "jfti_sim_master_config.json");
  worker.config = master_config.c_str();
  config.workers[0] = worker;
  if (config_writer_impl->write(config, DDS::HANDLE_NIL)) {
    std::cerr << "2nd write failed" << std::endl;
    return 1;
  }
  // End Hardcoded Scenario

  // Wait for reports
  std::cout << "Distributed node configs, waiting for reports..." << std::endl;

  size_t reports_received = 0;
  std::vector<Bench::WorkerReport> parsed_reports;
  while (reports_received < expected_workers) {
    DDS::ReturnCode_t rc;

    DDS::ReadCondition_var read_condition = report_reader_impl->create_readcondition(
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(read_condition);
    DDS::ConditionSeq active;
    rc = ws->wait(active, {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC});
    ws->detach_condition(read_condition);
    report_reader_impl->delete_readcondition(read_condition);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Wait failed" << std::endl;
      return 1;
    }

    ReportSeq reports;
    DDS::SampleInfoSeq info;
    rc = report_reader_impl->take(
      reports, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Take failed" << std::endl;
      return 1;
    }

    for (size_t r = 0; r < reports.length(); r++) {
      if (info[r].valid_data) {
        reports_received += 1;
        if (reports[r].failed) {
          std::cerr << "Worker " << reports[r].worker_id << " of node "
            << reports[r].node_id << " failed" << std::endl;
          return 1;
        } else {
          Bench::WorkerReport report;
          std::stringstream ss;
          ss << reports[r].details << std::flush;
          if (json_2_report(ss, report)) {
            parsed_reports.push_back(report);
          } else {
            std::cerr << "Error parsing report details for node "
              << reports[r].node_id << ", worker " << reports[r].worker_id << std::endl;
            return 1;
          }
        }
      }
    }
  }

  handle_reports(parsed_reports);

  // Clean up OpenDDS
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}

void
handle_reports(std::vector<Bench::WorkerReport> parsed_reports)
{
  using Builder::ZERO;

  Builder::TimeStamp max_construction_time = ZERO;
  Builder::TimeStamp max_enable_time = ZERO;
  Builder::TimeStamp max_start_time = ZERO;
  Builder::TimeStamp max_stop_time = ZERO;
  Builder::TimeStamp max_destruction_time = ZERO;

  size_t total_undermatched_readers = 0;
  size_t total_undermatched_writers = 0;
  Builder::TimeStamp max_discovery_time_delta = ZERO;

  Bench::SimpleStatBlock consolidated_latency_stats;
  Bench::SimpleStatBlock consolidated_jitter_stats;
  Bench::SimpleStatBlock consolidated_round_trip_latency_stats;
  Bench::SimpleStatBlock consolidated_round_trip_jitter_stats;

  for (size_t r = 0; r < parsed_reports.size(); ++r) {

    const Bench::WorkerReport& worker_report = parsed_reports[r];

    max_construction_time = std::max(max_construction_time, worker_report.construction_time);
    max_enable_time = std::max(max_enable_time, worker_report.enable_time);
    max_start_time = std::max(max_start_time, worker_report.start_time);
    max_stop_time = std::max(max_stop_time, worker_report.stop_time);
    max_destruction_time = std::max(max_destruction_time, worker_report.destruction_time);

    total_undermatched_readers += worker_report.undermatched_readers;
    total_undermatched_writers += worker_report.undermatched_writers;
    max_discovery_time_delta = std::max(max_discovery_time_delta, worker_report.max_discovery_time_delta);

    const Builder::ProcessReport& process_report = worker_report.process_report;

    for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
      for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
        for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {

          const Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];

          Bench::ConstPropertyStatBlock dr_latency(dr_report.properties, "latency");
          Bench::ConstPropertyStatBlock dr_jitter(dr_report.properties, "jitter");
          Bench::ConstPropertyStatBlock dr_round_trip_latency(dr_report.properties, "round_trip_latency");
          Bench::ConstPropertyStatBlock dr_round_trip_jitter(dr_report.properties, "round_trip_jitter");

          consolidated_latency_stats = consolidate(consolidated_latency_stats, dr_latency.to_simple_stat_block());
          consolidated_jitter_stats = consolidate(consolidated_jitter_stats, dr_jitter.to_simple_stat_block());
          consolidated_round_trip_latency_stats = consolidate(consolidated_round_trip_latency_stats, dr_round_trip_latency.to_simple_stat_block());
          consolidated_round_trip_jitter_stats = consolidate(consolidated_round_trip_jitter_stats, dr_round_trip_jitter.to_simple_stat_block());
        }
      }
    }
  }

  std::cout << std::endl;

  std::cout << "Test Timing Stats:" << std::endl;
  std::cout << "  Max Construction Time: " << max_construction_time << " seconds" << std::endl;
  std::cout << "  Max Enable Time: " << max_enable_time << " seconds" << std::endl;
  std::cout << "  Max Start Time: " << max_start_time << " seconds" << std::endl;
  std::cout << "  Max Stop Time: " << max_stop_time << " seconds" << std::endl;
  std::cout << "  Max Destruction Time: " << max_destruction_time << " seconds" << std::endl;

  std::cout << std::endl;

  std::cout << "Discovery Stats:" << std::endl;
  std::cout << "  Total Undermatched Readers: " << total_undermatched_readers << ", Total Undermatched Writers: " << total_undermatched_writers << std::endl;
  std::cout << "  Max Discovery Time Delta: " << max_discovery_time_delta << " seconds" << std::endl;

  std::cout << std::endl;

  consolidated_latency_stats.pretty_print(std::cout, "latency");

  std::cout << std::endl;

  consolidated_jitter_stats.pretty_print(std::cout, "jitter");

  std::cout << std::endl;

  consolidated_round_trip_latency_stats.pretty_print(std::cout, "round trip latency");

  std::cout << std::endl;

  consolidated_round_trip_jitter_stats.pretty_print(std::cout, "round trip jitter");
}
