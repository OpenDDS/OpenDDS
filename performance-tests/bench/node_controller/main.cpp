#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cstdint>

#include "ace/Process_Manager.h"
#include "ace/OS_NS_stdlib.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#include "BenchTypeSupportImpl.h"

using namespace Bench::NodeController;

const int DOMAIN = 89;
const std::string CONFIG_TOPIC_NAME = "Node_Controller_Config";
const std::string REPORT_TOPIC_NAME = "Node_Controller_Report";

ACE_Process_Manager* process_manager = ACE_Process_Manager::instance();
std::string dds_root;
ReportDataWriter_var report_writer_impl;

std::string
create_config(const std::string& file_base_name, const char* contents)
{
  const std::string filename = file_base_name + "_config.json";
  std::ofstream file(filename);
  if (file.is_open()) {
    file << contents;
  } else {
    std::cerr << "Could not write " << filename << std::endl;
  }
  return filename;
}

/**
 * Manager for Worker
 */
class Worker {
private:
  NodeId node_id_;
  WorkerId worker_id_;
  pid_t pid_;
  std::string file_base_name_;
  std::string config_filename_;

public:
  Worker(NodeId node_id, const WorkerConfig& config)
  : node_id_(node_id)
  , worker_id_(config.worker_id)
  , pid_(ACE_INVALID_PID)
  {
    std::stringstream ss;
    ss << 'n' << node_id_ << 'w' << worker_id_;
    file_base_name_ = ss.str();
    config_filename_ = create_config(file_base_name_, config.config.in());
  }

  WorkerId
  id()
  {
    return worker_id_;
  }

  void
  write_report(bool failed)
  {
    Report report;
    report.node_id = node_id_;
    report.worker_id = worker_id_;
    report.failed = failed;
    report.details = "";
    if (!failed) {
      std::string filename = file_base_name_ + "_report.json";
      std::ifstream file(filename);
      if (file.good()) {
        std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        report.details = str.c_str();
      }
    }
    if (report_writer_impl->write(report, DDS::HANDLE_NIL)) {
      std::cerr << "Write report failed" << std::endl;
    }
  }

  void
  run()
  {
    ACE_Process_Options proc_opts;
    std::stringstream ss;
    ss << dds_root << "/performance-tests/bench/worker/worker " << config_filename_
      << " --report " << file_base_name_ << "_report.json"
      << " --log " << file_base_name_ << "_log.txt";
    std::cerr << ss.str() << std::endl;
    proc_opts.command_line(ss.str().c_str());
    pid_ = process_manager->spawn(proc_opts);
    bool failed = pid_ == ACE_INVALID_PID;
    if (failed) {
      std::cerr << "Failed to run worker " << id() << std::endl;
      write_report(true);
    }
  }

  void
  check()
  {
    if (pid_ == ACE_INVALID_PID) {
      return;
    }
    ACE_exitcode status;
    bool failed = process_manager->wait(pid_, &status) == ACE_INVALID_PID || status != 0;
    if (failed) {
      std::cerr << "Worker " << id() << " returned a " << status << " status code" << std::endl;
    }
    write_report(failed);
  }
};

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  dds_root = ACE_OS::getenv("DDS_ROOT");
  if (dds_root.empty()) {
    std::cerr << "ERROR: DDS_ROOT isn't defined" << std::endl;
    return 1;
  }
  NodeId this_node_id;
  std::vector<Worker> workers;
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  for (int i = 1; i < argc; i++) {
    const char* argument = ACE_TEXT_ALWAYS_CHAR(argv[i]);
    if (!ACE_OS::strcmp(argument, "--id")) {
      argument = ACE_TEXT_ALWAYS_CHAR(argv[++i]);
      try {
        this_node_id = std::stoull(argument);
      } catch (const std::exception&) {
        std::cerr << "Invalid Id: " << argument << std::endl;
        return 1;
      }
    } else {
      std::cerr << "Invalid Option: " << argument << std::endl;
      return 1;
    }
  }

  // DDS Entities
  DDS::DomainParticipant_var participant = dpf->create_participant(
    DOMAIN, PARTICIPANT_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!participant) {
    std::cerr << "create_participant failed" << std::endl;
    return 1;
  }
  ConfigTypeSupport_var config_ts = new ConfigTypeSupportImpl;
  if (config_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Config" << std::endl;
    return 1;
  }
  DDS::Topic_var config_topic = participant->create_topic(
    CONFIG_TOPIC_NAME.c_str(), config_ts->get_type_name(), TOPIC_QOS_DEFAULT, 0,
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
    REPORT_TOPIC_NAME.c_str(), report_ts->get_type_name(), TOPIC_QOS_DEFAULT, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_topic) {
    std::cerr << "create_topic report failed" << std::endl;
    return 1;
  }
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
  DDS::DataReader_var config_reader = subscriber->create_datareader(
    config_topic, dr_qos, 0,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!config_reader) {
    std::cerr << "create_datareader config failed" << std::endl;
    return 1;
  }
  ConfigDataReader_var config_reader_impl = ConfigDataReader::_narrow(config_reader);
  if (!config_reader_impl) {
    std::cerr << "narrow config reader failed" << std::endl;
    return 1;
  }
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
  DDS::DataWriter_var report_writer = publisher->create_datawriter(
    report_topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_writer) {
    std::cerr << "create_datawriter report failed" << std::endl;
    return 1;
  }
  report_writer_impl = ReportDataWriter::_narrow(report_writer);
  if (!report_writer_impl) {
    std::cerr << "narrow writer report failed" << std::endl;
    return 1;
  }

  bool waiting = true;
  while (waiting) {
    DDS::ReturnCode_t rc;

    DDS::ReadCondition_var read_condition = config_reader_impl->create_readcondition(
        DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(read_condition);
    DDS::ConditionSeq active;
    rc = ws->wait(active, {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC});
    ws->detach_condition(read_condition);
    config_reader_impl->delete_readcondition(read_condition);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Wait failed" << std::endl;
      return 1;
    }

    ConfigSeq configs;
    DDS::SampleInfoSeq info;
    rc = config_reader_impl->take(
      configs, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Take failed" << std::endl;
      return 1;
    }

    for (size_t node = 0; node < configs.length(); node++) {
      if (configs[node].node_id == this_node_id) {
        for (size_t worker = 0; worker < configs[node].workers.length(); worker++) {
          workers.emplace_back(this_node_id, configs[node].workers[worker]);
        }
        waiting = false;
      }
    }
  }

  for (Worker& worker : workers) {
    worker.run();
  }

  for (Worker& worker : workers) {
    worker.check();
  }

  // Clean up OpenDDS
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return 0;
}
