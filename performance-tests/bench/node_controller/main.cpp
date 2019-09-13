#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cstdint>

#include <ace/Process_Manager.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#include "BenchTypeSupportImpl.h"

using namespace Bench::NodeController;

ACE_Process_Manager* process_manager = ACE_Process_Manager::instance();
std::string dds_root;
ReportDataWriter_var report_writer_impl;

ACE_TCHAR tempdir_template[] = "/tmp/nc_XXXXXX";
ACE_TCHAR* tempdir = ACE_OS::mktemp(tempdir_template);
int tempdir_mkdir_result = ACE_OS::mkdir(tempdir, S_IRWXU);

std::string
create_config(const std::string& file_base_name, const char* contents)
{
  ACE_TCHAR suffix_template[] = "_XXXXXX";
  ACE_TCHAR* suffix = ACE_OS::mktemp(suffix_template);
  const std::string filename = std::string(tempdir_mkdir_result ? "." : tempdir) + "/" + file_base_name + suffix + "_config.json";
  std::ofstream file(filename);
  if (file.is_open()) {
    file << contents << std::flush;
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
  std::string report_filename_;
  std::string log_filename_;

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
    report_filename_ = config_filename_.substr(0, config_filename_.size() - 12) + "_report.json";
    log_filename_ = config_filename_.substr(0, config_filename_.size() - 12) + "_log.txt";
  }

  ~Worker()
  {
    if (!config_filename_.empty()) {
      ACE_OS::unlink(config_filename_.c_str());
      ACE_OS::unlink(report_filename_.c_str());
      ACE_OS::unlink(log_filename_.c_str());
    }
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
      std::ifstream file(report_filename_);
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
    std::stringstream ss, ess;
    ss << dds_root << "/performance-tests/bench/worker/worker " << config_filename_
      << " --report " << report_filename_
      << " --log " << log_filename_ << std::flush;
    ess << ss.str() << std::endl;
    std::cerr << ess.str() << std::flush;
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

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  dds_root = ACE_OS::getenv("DDS_ROOT");
  if (dds_root.empty()) {
    std::cerr << "ERROR: DDS_ROOT isn't defined" << std::endl;
    return 1;
  }

  std::vector<std::shared_ptr<Worker>> workers;
  NodeId this_node_id;

  TheServiceParticipant->default_configuration_file("rtps_disc.ini");

  // Parse Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  int domain = default_control_domain;
  std::string name;

  try {
    for (int i = 1; i < argc; i++) {
      const char* argument = ACE_TEXT_ALWAYS_CHAR(argv[i]);
      if (!ACE_OS::strcmp(argument, "--domain")) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--name")) {
        name = get_option_argument(i, argc, argv);
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
  this_node_id = reinterpret_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in())->get_id();

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
  DDS::DataWriter_var status_writer = publisher->create_datawriter(
    status_topic, dw_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_writer) {
    std::cerr << "create_datawriter status failed" << std::endl;
    return 1;
  }
  StatusDataWriter_var status_writer_impl = StatusDataWriter::_narrow(status_writer);
  if (!status_writer_impl) {
    std::cerr << "narrow writer status failed" << std::endl;
    return 1;
  }
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

  // Wait for Status Publication with Test Controller and Write Status
  {
    DDS::StatusCondition_var condition = status_writer->get_statuscondition();
    condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
    DDS::WaitSet_var ws = new DDS::WaitSet;
    ws->attach_condition(condition);
    DDS::Duration_t timeout = {
      DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC
    };
    DDS::ConditionSeq conditions;
    DDS::PublicationMatchedStatus match{};
    while (match.current_count == 0) {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK) {
        std::cerr << "wait for Test Controller failed" << std::endl;
        return 1;
      }
      if (status_writer->get_publication_matched_status(match) != DDS::RETCODE_OK) {
        std::cerr << "get_publication_matched_status failed" << std::endl;
        return 1;
      }
    }
    ws->detach_condition(condition);

    Status status;
    status.node_id = this_node_id;
    status.state = AVAILABLE;
    status.name = name.c_str();
    if (status_writer_impl->write(status, DDS::HANDLE_NIL)) {
      std::cerr << "Write status failed" << std::endl;
      return 1;
    }
  }

  // Wait for Our Worker Configs
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
          workers.push_back(std::make_shared<Worker>(this_node_id, configs[node].workers[worker]));
        }
        waiting = false;
      }
    }
  }

  // Run The Workers
  for (std::shared_ptr<Worker>& worker : workers) {
    worker->run();
  }

  for (std::shared_ptr<Worker>& worker : workers) {
    worker->check();
  }

  workers.clear();

  // Clean up OpenDDS
  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  if (tempdir_mkdir_result == 0) {
    ACE_OS::rmdir(tempdir);
  }

  return 0;
}
