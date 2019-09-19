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
#pragma GCC diagnostic pop

#include <util.h>
#include <json_conversion.h>

using namespace Bench::NodeController;
using Bench::get_option_argument_int;
using Bench::get_option_argument_uint;
using Bench::join_path;
using Bench::json_2_idl;

struct Node {
  NodeId node_id;
  std::string name;
  StateEnum state;
};
typedef std::map<NodeId, Node, OpenDDS::DCPS::GUID_tKeyLessThan> Nodes;
std::ostream& operator<<(std::ostream& os, const Node& node)
{
  os << node.node_id;
  if (node.name.size()) {
    os << " \"" << node.name << '\"';
  }
  return os;
}

std::string bench_root;

std::string read_file(const std::string& name)
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

class Scenario {
public:
  class NodeConfig {
  public:
    void add_worker_config(const std::string& config_contents, size_t count = 1) {
      if (config_contents.empty() || count == 0) {
        throw std::runtime_error("Invalid arguments to add_worker_config.");
      }
      WorkerConfig worker;
      worker.worker_id = worker_count_ + 1;
      worker.config = config_contents.c_str();
      worker.count = count;
      worker_configs_.push_back(worker);
      worker_count_ += count;
    };

    unsigned copy_to(Config& config) {
      config.workers.length(worker_configs_.size());
      unsigned i = 0;
      for (auto& worker : worker_configs_) {
        config.workers[i++] = worker;
      }
      return worker_count_;
    }

  private:
    std::vector<WorkerConfig> worker_configs_;
    unsigned worker_count_ = 0;
  };

  void add_node_config(NodeConfig& config) {
    node_configs_.push_back(config);
  }

  void add_any_node_config(NodeConfig& config) {
    if (got_any_) {
      throw std::runtime_error("Already defined a \"Any Node\" config for the scenario!");
    }
    got_any_ = true;
    any_node_config_ = config;
  }

  size_t distribute(ConfigDataWriter_var config_writer, Nodes& available_nodes) {
    const size_t minimum_nodes = node_configs_.size() + got_any_ ? 1 : 0;
    if (available_nodes.size() < minimum_nodes) {
      std::stringstream ss;
      ss << "At least " << minimum_nodes << " available nodes are required for this scenario";
      throw std::runtime_error(ss.str());
    }

    size_t expected_reports = 0;
    Nodes::iterator ni = available_nodes.begin();
    Config config;
    for (auto& node_config : node_configs_) {
      config.node_id = ni->first;
      expected_reports += node_config.copy_to(config);
      if (config_writer->write(config, DDS::HANDLE_NIL)) {
        throw std::runtime_error("Config write failed!");
      }
      ++ni;
    }
    if (got_any_) {
      for (; ni != available_nodes.end(); ++ni) {
        config.node_id = ni->first;
        expected_reports += any_node_config_.copy_to(config);
        if (config_writer->write(config, DDS::HANDLE_NIL)) {
          throw std::runtime_error("Config write failed!");
        }
      }
    }
    return expected_reports;
  }

private:
  std::vector<NodeConfig> node_configs_;
  NodeConfig any_node_config_;
  bool got_any_ = false;
};

class StatusListener : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  StatusListener()
  {
  }

  virtual ~StatusListener()
  {
  }

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedDeadlineMissedStatus& /* status */)
  {
  }

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr /* reader */,
    const DDS::RequestedIncompatibleQosStatus& /* status */)
  {
  }

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr /* reader */,
    const DDS::LivelinessChangedStatus& /* status */)
  {
  }

  virtual void on_subscription_matched(
    DDS::DataReader_ptr /* reader */,
    const DDS::SubscriptionMatchedStatus& /* status */)
  {
  }

  virtual void on_sample_rejected(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleRejectedStatus& /* status */)
  {
  }

  virtual void on_data_available(DDS::DataReader_ptr reader)
  {
    std::lock_guard<std::mutex> lock(nodes_mutex_);

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
      bool already_discovered = iter != nodes_.end();
      if (info[i].valid_data) {
        Node node;
        node.node_id = statuses[i].node_id;
        node.name = statuses[i].name;
        node.state = statuses[i].state;
        if (in_discovery_period_) {
          std::cout << "Discovered Node " << node << std::endl;
          nodes_[node.node_id] = node;
        } else if (!already_discovered) {
          std::cerr << "Warning: Discovered node " << node << " outside discovery period" << std::endl;
        } else {
          nodes_[node.node_id] = node;
        }
      } else if (info[i].instance_state & DDS::NOT_ALIVE_INSTANCE_STATE && already_discovered) {
        nodes_.erase(iter);
      }
    }
  }

  virtual void on_sample_lost(
    DDS::DataReader_ptr /* reader */,
    const DDS::SampleLostStatus& /* status */)
  {
  }

  Nodes get_available_nodes() {
    std::lock_guard<std::mutex> lock(nodes_mutex_);
    in_discovery_period_ = false;
    Nodes rv;
    for (auto i : nodes_) {
      if (i.second.state == AVAILABLE) {
        rv[i.first] = i.second;
      }
    }
    return rv;
  }

private:
  Nodes nodes_;
  std::mutex nodes_mutex_;
  bool in_discovery_period_ = true;
};

void handle_reports(std::vector<Bench::WorkerReport> parsed_reports);

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  const char* cstr = ACE_OS::getenv("BENCH_ROOT");
  bench_root = cstr ? cstr : "";
  if (bench_root.empty()) {
    cstr = ACE_OS::getenv("DDS_ROOT");
    const std::string dds_root{cstr ? cstr : ""};
    if (dds_root.empty()) {
      std::cerr << "ERROR: BENCH_ROOT or DDS_ROOT must be defined" << std::endl;
      return 1;
    }
    bench_root = join_path(dds_root, "performance-tests", "bench");
  }

  TheServiceParticipant->default_configuration_file(
    ACE_TEXT_CHAR_TO_TCHAR(join_path(bench_root, "control_opendds_config.ini").c_str()));

  // Parse Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  int domain = default_control_domain;
  /// How much time for the node discovery period.
  unsigned wait_for_nodes = 10;
  /// Max time to wait inbetween reports comming in.
  unsigned wait_for_reports = 120;

  try {
    for (int i = 1; i < argc; i++) {
      const char* argument = ACE_TEXT_ALWAYS_CHAR(argv[i]);
      if (!ACE_OS::strcmp(argument, "--domain")) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--wait-for-nodes")) {
        wait_for_nodes = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--wait-for-reports")) {
        wait_for_reports = get_option_argument_uint(i, argc, argv);
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
    domain, PARTICIPANT_QOS_DEFAULT, nullptr,
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
    status_topic_name, status_ts->get_type_name(), TOPIC_QOS_DEFAULT, nullptr,
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
    config_topic_name, config_ts->get_type_name(), TOPIC_QOS_DEFAULT, nullptr,
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
    report_topic_name, report_ts->get_type_name(), TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_topic) {
    std::cerr << "create_topic report failed" << std::endl;
    return 1;
  }

  // Create DataWriters
  DDS::Publisher_var publisher = participant->create_publisher(
    PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher) {
    std::cerr << "create_publisher failed" << std::endl;
    return 1;
  }
  DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  DDS::DataWriter_var config_writer = publisher->create_datawriter(
    config_topic, dw_qos, nullptr,
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
    SUBSCRIBER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber) {
    std::cerr << "create_subscriber failed" << std::endl;
    return 1;
  }
  DDS::DataReaderQos dr_qos;
  subscriber->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  StatusListener status_listener;
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
    report_topic, dr_qos, nullptr,
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
  Nodes available_nodes = status_listener.get_available_nodes();
  if (available_nodes.empty()) {
    std::cerr << "Error: no available nodes discovered during wait" << std::endl;
    return 1;
  }
  std::cout << "Discovered " << available_nodes.size() << " nodes, continuing with scenario..." << std::endl;

  // Distrubute Configs
  Scenario scenario;
  size_t expected_reports;

  try {
    // Begin Hardcoded Scenario
    const std::string configs = join_path(bench_root, "worker", "configs");

    Scenario::NodeConfig master_node;
    master_node.add_worker_config(read_file(join_path(configs, "jfti_sim_master_config.json")));
    scenario.add_node_config(master_node);

    Scenario::NodeConfig site_node;
    site_node.add_worker_config(read_file(join_path(configs, "jfti_sim_daemon_config.json")));
    site_node.add_worker_config(read_file(join_path(configs, "jfti_sim_worker_config.json")));
    scenario.add_any_node_config(site_node);
    // End Hardcoded Scenario

    expected_reports = scenario.distribute(config_writer_impl, available_nodes);
  } catch (const std::runtime_error& r) {
    std::cerr << "Error Compiling and Distributing Scenario: " << r.what() << std::endl;
    return 1;
  }

  // Wait for reports
  std::cout << "Distributed node configs, waiting for " << expected_reports << " reports..." << std::endl;

  std::vector<Bench::WorkerReport> parsed_reports;
  while (parsed_reports.size() < expected_reports) {
    DDS::ReturnCode_t rc;

    DDS::ReadCondition_var read_condition = report_reader_impl->create_readcondition(
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(read_condition);
    DDS::ConditionSeq active;
    rc = ws->wait(active, {static_cast<int>(wait_for_reports), 0});
    ws->detach_condition(read_condition);
    report_reader_impl->delete_readcondition(read_condition);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Waiting for " << wait_for_reports << " seconds between reports failed" << std::endl;
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
        if (reports[r].failed) {
          std::cerr << "Worker " << reports[r].worker_id << " of node "
            << reports[r].node_id << " failed" << std::endl;
          return 1;
        } else {
          Bench::WorkerReport report;
          std::stringstream ss;
          ss << reports[r].details << std::flush;
          if (json_2_idl(ss, report)) {
            parsed_reports.push_back(report);
          } else {
            std::cerr << "Error parsing report details for node "
              << reports[r].node_id << ", worker " << reports[r].worker_id << std::endl;
            return 1;
          }
        }
        std::cout << "Got " << parsed_reports.size() << " out of "
          << expected_reports << " expected reports" << std::endl;
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

void handle_reports(std::vector<Bench::WorkerReport> parsed_reports)
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
