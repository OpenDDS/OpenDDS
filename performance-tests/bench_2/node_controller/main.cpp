/*
 * The Node Controller takes directives from a Test Controller, spawns Workers,
 * and report the results back.
 */
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <memory>
#include <sstream>
#include <cstdint>
#include <mutex>
#include <thread>
#include <atomic>

#include <ace/Process_Manager.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <util.h>
#include <BenchTypeSupportImpl.h>

using namespace Bench::NodeController;
using Bench::get_option_argument_int;
using Bench::get_option_argument;
using Bench::join_path;
using Bench::create_temp_dir;

std::string bench_root;
std::string temp_dir;
std::string output_dir;

int run_cycle(
  const std::string& name,
  ACE_Process_Manager& process_manager,
  DDS::DomainParticipant_var participant,
  StatusDataWriter_var status_writer_impl,
  ConfigDataReader_var config_reader_impl,
  ReportDataWriter_var report_writer_impl);

std::string create_config(const std::string& file_base_name, const char* contents)
{
  const std::string filename = join_path(output_dir, file_base_name + "_config.json");
  std::ofstream file(filename);
  if (file.is_open()) {
    file << contents << std::flush;
  } else {
    std::cerr << "Could not write " << filename << std::endl;
  }
  return filename;
}

class Worker {
public:
  Worker(const NodeId& node_id, const WorkerConfig& config)
  : node_id_(node_id)
  , worker_id_(config.worker_id)
  {
    std::stringstream ss;
    ss << 'n' << node_id_ << 'w' << worker_id_;
    file_base_name_ = ss.str();
    config_filename_ = create_config(file_base_name_, config.config.in());
    report_filename_ = join_path(output_dir, file_base_name_ + "_report.json");
    log_filename_ = join_path(output_dir, file_base_name_ + "_log.txt");
  }

  ~Worker()
  {
    if (!config_filename_.empty()) {
      ACE_OS::unlink(config_filename_.c_str());
      ACE_OS::unlink(report_filename_.c_str());
      ACE_OS::unlink(log_filename_.c_str());
    }
  }

  WorkerId id()
  {
    return worker_id_;
  }

  void write_report(ReportDataWriter_var report_writer_impl)
  {
    Report report;
    report.node_id = node_id_;
    report.worker_id = worker_id_;
    report.failed = (pid_ == ACE_INVALID_PID || exit_status_ != 0);
    report.details = "";
    report.log = "";

    if (!report.failed) {
      std::ifstream report_file(report_filename_);
      if (report_file.good()) {
        std::string str((std::istreambuf_iterator<char>(report_file)), std::istreambuf_iterator<char>());
        report.details = str.c_str();
      }
    } else {
      std::ifstream log_file(log_filename_);
      if (log_file.good()) {
        std::string str((std::istreambuf_iterator<char>(log_file)), std::istreambuf_iterator<char>());
        report.log = str.c_str();
      }
    }
    if (report_writer_impl->write(report, DDS::HANDLE_NIL)) {
      std::cerr << "Write report failed" << std::endl;
    }
  }

  ACE_Process_Options get_proc_opts() const
  {
    ACE_Process_Options proc_opts;
    std::stringstream ss;
    ss << join_path(bench_root, "worker", "worker")
      << " " << config_filename_
      << " --report " << report_filename_
      << " --log " << log_filename_ << std::flush;
    const std::string command = ss.str();
    std::cerr << command + "\n" << std::flush;
    proc_opts.command_line("%s", command.c_str());
    return proc_opts;
  }

  void set_pid(pid_t pid)
  {
    pid_ = pid;
    running_ = true;
  }

  pid_t get_pid()
  {
    return pid_;
  }

  void set_exit_status(int return_code, ACE_exitcode exit_code)
  {
// TODO FIXME This is required to correctly detect segfaults on Linux
// It's possible we need to do something similar on other platforms and we will
// probably want to expand this to do the correct thing on windows / android / macos
#ifdef ACE_LINUX
    exit_status_ = exit_code ? exit_code : return_code;
#else
    exit_status_ = return_code;
#endif
    running_ = false;
  }

  bool running()
  {
    return running_;
  }

private:
  NodeId node_id_;
  WorkerId worker_id_;
  bool running_ = false;
  pid_t pid_ = ACE_INVALID_PID;
  int exit_status_ = 0;
  std::string file_base_name_;
  std::string config_filename_;
  std::string report_filename_;
  std::string log_filename_;
};

using WorkerPtr = std::shared_ptr<Worker>;

class WorkerManager : public ACE_Event_Handler {
public:

  explicit WorkerManager(ACE_Process_Manager& process_manager)
  : process_manager_(process_manager)
  {
    process_manager.register_handler(this);
    ACE_Reactor::instance()->register_handler(SIGINT, this);
  }

  ~WorkerManager()
  {
    ACE_Reactor::instance()->remove_handler(SIGINT, nullptr);
    process_manager_.register_handler(nullptr);
  }

  void timeout(unsigned value)
  {
    timeout_ = value;
  }

  bool add_worker(const NodeId& node_id, const WorkerConfig& config)
  {
    std::lock_guard<std::mutex> ul(lock_);
    if (all_workers_.count(config.worker_id)) {
      std::cerr << "Received the same worker id twice: " << config.worker_id << std::endl;
      return true;
    }
    all_workers_[config.worker_id] = std::make_shared<Worker>(node_id, config);
    remaining_workers_++;
    return false;
  }

  // Must hold lock_
  void worker_is_finished(WorkerPtr& worker)
  {
    remaining_workers_--;
    finished_workers_.push_back(worker);
  }

  // Must hold lock_
  void kill_all_the_workers()
  {
    for (auto worker_i : all_workers_) {
      auto& worker = worker_i.second;
      if (worker->running()) {
        if (process_manager_.terminate(worker->get_pid())) {
          process_manager_.wait(worker->get_pid(), ACE_Time_Value(0, ACE_ONE_SECOND_IN_USECS / 10));
        }
      }
    }
  }

  void run_workers(ReportDataWriter_var report_writer_impl)
  {
    ACE_Reactor::instance()->schedule_timer(this, nullptr, ACE_Time_Value(timeout_));
    // Spawn Workers
    {
      std::lock_guard<std::mutex> guard(lock_);
      for (auto worker_i : all_workers_) {
        auto& worker = worker_i.second;
        ACE_Process_Options proc_opts = worker->get_proc_opts();
        pid_t pid = process_manager_.spawn(proc_opts);
        if (pid != ACE_INVALID_PID) {
          worker->set_pid(pid);
          pid_to_worker_id_[pid] = worker->id();
        } else {
          std::cerr << "Failed to run worker " << worker->id() << std::endl;
          worker_is_finished(worker);
        }
      }
    }

    // Wait For Them to Finish, Writing Reports As They Do
    bool running = true;
    while (running) {
      // Check to see if any workers are done and write their reports
      std::list<WorkerPtr> finished_workers;
      {
        std::lock_guard<std::mutex> guard(lock_);
        finished_workers = finished_workers_;
        finished_workers_.clear();
        running = remaining_workers_;
      }
      for (auto& i : finished_workers) {
        i->write_report(report_writer_impl);
      }

      // Check to see if we have to stop prematurely
      bool kill_workers = false;
      if (scenario_timedout_.load()) {
        std::stringstream ss;
        ss << "Scenario Timedout, Killing Workers..." << std::endl;
        std::cerr << ss.str() << std::flush;
        kill_workers = true;
      }
      if (sigint_.load()) {
        std::stringstream ss;
        ss << "Interrupted, Killing Workers..." << std::endl;
        std::cerr << ss.str() << std::flush;
        kill_workers = true;
      }
      if (kill_workers) {
        std::lock_guard<std::mutex> guard(lock_);
        kill_all_the_workers(); // Bwahahaha
        running = false;
      }

      if (running) {
        ACE_OS::sleep(1);
      }
    }
    ACE_Reactor::instance()->cancel_timer(this);
  }

  /// Used to the Handle Exit of a Worker
  virtual int handle_exit(ACE_Process* process)
  {
    pid_t pid = process->getpid();

    std::lock_guard<std::mutex> guard(lock_);

    const auto i = pid_to_worker_id_.find(pid);
    if (i != pid_to_worker_id_.end()) {
      auto& worker = all_workers_[i->second];
      worker->set_exit_status(process->return_value(), process->exit_code());
      remaining_workers_--;
      finished_workers_.push_back(worker);
    } else {
      std::cerr << "WorkerManager::handle_exit() received an unknown PID: " << pid << std::endl;
    }

    return 0;
  }

  /// Used to the Handle Scenario Timeout
  virtual int handle_timeout(const ACE_Time_Value&, const void* = nullptr)
  {
    scenario_timedout_.store(true);
    return -1;
  }

  /// Used to the Interrupt
  virtual int handle_signal(int signum, siginfo_t* = nullptr, ucontext_t* = nullptr)
  {
    if (signum == SIGINT) {
      sigint_.store(true);
    }
    return 0;
  }

private:
  unsigned timeout_ = 0;
  std::mutex lock_;
  std::map<WorkerId, WorkerPtr> all_workers_;
  size_t remaining_workers_ = 0;
  std::map<pid_t, WorkerId> pid_to_worker_id_;
  std::list<WorkerPtr> finished_workers_;
  ACE_Process_Manager& process_manager_;
  std::atomic_bool scenario_timedout_{false};
  std::atomic_bool sigint_{false};
};

enum class RunMode {
  one_shot,
  daemon,
  daemon_exit_on_error,
};

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
    bench_root = join_path(dds_root, "performance-tests", "bench_2");
  }

  TheServiceParticipant->default_configuration_file(
    ACE_TEXT_CHAR_TO_TCHAR(join_path(bench_root, "control_opendds_config.ini").c_str()));

  // Parse Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  RunMode run_mode;
  int domain = default_control_domain;
  std::string name;

  try {
    if (argc == 1) {
      std::cerr << "A valid run type is required" << std::endl;
      throw 1;
    }

    const ACE_TCHAR* run_mode_arg = argv[1];
    if (!ACE_OS::strcmp(run_mode_arg, ACE_TEXT("one-shot"))) {
      run_mode = RunMode::one_shot;
    } else if (!ACE_OS::strcmp(run_mode_arg, ACE_TEXT("daemon"))) {
      run_mode = RunMode::daemon;
    } else if (!ACE_OS::strcmp(run_mode_arg, ACE_TEXT("daemon-exit-on-error"))) {
      run_mode = RunMode::daemon_exit_on_error;
    } else {
      std::cerr << "Invalid run mode: " << ACE_TEXT_ALWAYS_CHAR(run_mode_arg) << std::endl;
      throw 1;
    }

    for (int i = 2; i < argc; i++) {
      if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--domain"))) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argv[i], ACE_TEXT("--name"))) {
        name = get_option_argument(i, argc, argv);
      } else {
        std::cerr << "Invalid option: " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << std::endl;
        return 1;
      }
    }
  } catch(int value) {
    std::cerr << "See DDS_ROOT/performance-tests/bench_2/README.md for usage" << std::endl;
    return value;
  }

  // Try to get a temp_dir
  temp_dir = create_temp_dir("opendds_bench_nc");
  output_dir = temp_dir.empty() ? "." : temp_dir;

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
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  DDS::DataReader_var config_reader = subscriber->create_datareader(
    config_topic, dr_qos, nullptr,
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
    PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher) {
    std::cerr << "create_publisher failed" << std::endl;
    return 1;
  }
  DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  DDS::DataWriter_var status_writer = publisher->create_datawriter(
    status_topic, dw_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
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
    report_topic, dw_qos, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_writer) {
    std::cerr << "create_datawriter report failed" << std::endl;
    return 1;
  }
  ReportDataWriter_var report_writer_impl = ReportDataWriter::_narrow(report_writer);
  if (!report_writer_impl) {
    std::cerr << "narrow writer report failed" << std::endl;
    return 1;
  }

  // Wait For and Run Node Configurations
  std::thread reactor_thread([]() {
    if (ACE_Reactor::instance()->owner(ACE_Thread_Manager::instance()->thr_self())) {
      std::cerr << "Error setting Reactor Thread" << std::endl;
    }
    ACE_Reactor::instance()->run_reactor_event_loop();
  });
  ACE_Process_Manager process_manager(ACE_Process_Manager::DEFAULT_SIZE, ACE_Reactor::instance());
  int exit_status = 0;
  while (true) {
    exit_status = run_cycle(name, process_manager, participant,
      status_writer_impl, config_reader_impl, report_writer_impl);

    if (run_mode == RunMode::one_shot || (run_mode == RunMode::daemon_exit_on_error && exit_status != 0)) {
      break;
    }
  }

  // Clean up
  ACE_Reactor::instance()->end_reactor_event_loop();
  reactor_thread.join();

  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  if (temp_dir.size()) {
    ACE_OS::rmdir(temp_dir.c_str());
  }

  return exit_status;
}

int run_cycle(
  const std::string& name,
  ACE_Process_Manager& process_manager,
  DDS::DomainParticipant_var participant,
  StatusDataWriter_var status_writer_impl,
  ConfigDataReader_var config_reader_impl,
  ReportDataWriter_var report_writer_impl)
{
  NodeId this_node_id = reinterpret_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in())->get_id();

  // Wait for Status Publication with Test Controller and Write Status
  {
    DDS::StatusCondition_var condition = status_writer_impl->get_statuscondition();
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
        std::cerr << "Wait for test controller failed" << std::endl;
        return 1;
      }
      if (status_writer_impl->get_publication_matched_status(match) != DDS::RETCODE_OK) {
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

  WorkerManager worker_manager(process_manager);

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
      std::cerr << "Wait for node config failed" << std::endl;
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
      std::cerr << "Take node config failed" << std::endl;
      return 1;
    }

    for (size_t node = 0; node < configs.length(); node++) {
      if (configs[node].node_id == this_node_id) {
        worker_manager.timeout(configs[node].timeout);
        size_t config_count = configs[node].workers.length();
        for (size_t config = 0; config < config_count; config++) {
          WorkerId& id = configs[node].workers[config].worker_id;
          const WorkerId end = id + configs[node].workers[config].count;
          for (; id < end; id++) {
            if (worker_manager.add_worker(this_node_id, configs[node].workers[config])) {
              return 1;
            }
          }
        }
        waiting = false;
        break;
      }
    }
  }

  // Report Busy Status
  {
    Status status;
    status.node_id = this_node_id;
    status.state = BUSY;
    status.name = name.c_str();
    if (status_writer_impl->write(status, DDS::HANDLE_NIL)) {
      std::cerr << "Write status failed" << std::endl;
      return 1;
    }
  }

  // Run Workers and Wait for Them to Finish
  worker_manager.run_workers(report_writer_impl);

  DDS::Duration_t timeout = { 10, 0 };
  if (report_writer_impl->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
    std::cerr << "Waiting for report acknowledgment failed" << std::endl;
    return 1;
  }

  return 0;
}
