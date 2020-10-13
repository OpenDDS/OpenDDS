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
#include <condition_variable>

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
#include <tests/Utils/StatusMatching.h>
#include <Common.h>
#include "PropertyStatBlock.h"

using namespace Bench::NodeController;
using Bench::get_option_argument_int;
using Bench::get_option_argument;
using Bench::join_path;
using Bench::create_temp_dir;
using Bench::TestController::AllocatedScenarioDataReader;
using Bench::TestController::AllocatedScenarioDataReader_var;

std::string bench_root;
std::string temp_dir;
std::string output_dir;

int run_cycle(
  const std::string& name,
  ACE_Process_Manager& process_manager,
  DDS::DomainParticipant_var participant,
  StatusDataWriter_var status_writer_impl,
  AllocatedScenarioDataReader_var allocated_scenario_reader_impl,
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
    allocated_scenario_filename_ = create_config(file_base_name_, config.config.in());
    report_filename_ = join_path(output_dir, file_base_name_ + "_report.json");
    log_filename_ = join_path(output_dir, file_base_name_ + "_log.txt");
  }

  ~Worker()
  {
    if (!allocated_scenario_filename_.empty()) {
      ACE_OS::unlink(allocated_scenario_filename_.c_str());
      ACE_OS::unlink(report_filename_.c_str());
      ACE_OS::unlink(log_filename_.c_str());
    }
  }

  WorkerId id()
  {
    return worker_id_;
  }

  void create_worker_report(WorkerReport& report)
  {
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
  }

  std::shared_ptr<ACE_Process_Options> get_proc_opts() const
  {
    std::shared_ptr<ACE_Process_Options> proc_opts = std::make_shared<ACE_Process_Options>();
    std::stringstream ss;
    ss << join_path(bench_root, "worker", "worker")
      << " " << allocated_scenario_filename_
      << " --report " << report_filename_
      << " --log " << log_filename_ << std::flush;
    const std::string command = ss.str();
    std::cerr << command + "\n" << std::flush;
    proc_opts->command_line("%s", command.c_str());
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
  std::string allocated_scenario_filename_;
  std::string report_filename_;
  std::string log_filename_;
};

using WorkerPtr = std::shared_ptr<Worker>;

class WorkerManager : public ACE_Event_Handler {
public:

  explicit WorkerManager(const NodeId& node_id, ACE_Process_Manager& process_manager)
  : node_id_(node_id)
  , process_manager_(process_manager)
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

  bool add_worker(const WorkerConfig& config)
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if (all_workers_.count(config.worker_id)) {
      std::cerr << "Received the same worker id twice: " << config.worker_id << std::endl;
      return true;
    }
    all_workers_[config.worker_id] = std::make_shared<Worker>(node_id_, config);
    remaining_worker_count_++;
    return false;
  }

  // Must hold lock_
  void worker_is_finished(WorkerPtr& worker)
  {
    remaining_worker_count_--;
    finished_workers_.push_back(worker);
    cv_.notify_all();
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
      std::lock_guard<std::mutex> guard(mutex_);
      for (auto worker_i : all_workers_) {
        auto& worker = worker_i.second;
        std::shared_ptr<ACE_Process_Options> proc_opts = worker->get_proc_opts();
        pid_t pid = process_manager_.spawn(*proc_opts);
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
    Report report{};
    report.worker_reports.length(all_workers_.size());
    const size_t max_stat_buffer_size = 3600; // one hour in seconds
    auto cpu_block = std::make_shared<Bench::PropertyStatBlock>(report.properties, "cpu_percent", max_stat_buffer_size, true);
    auto mem_block = std::make_shared<Bench::PropertyStatBlock>(report.properties, "mem_percent", max_stat_buffer_size, true);
    CORBA::ULong pos = 0;
    report.node_id = node_id_;

    bool running = true;

    std::thread stat_collector([&](){

      // TODO replace junk with real stats collection
      size_t junk = 0;

      while (running) {
        // TODO replace junk with real stats collection
        junk = junk + 3;
        cpu_block->update(90.0 - ((junk / 3) % 80));
        mem_block->update(20.0 + (junk * 2) % 60);

        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    });

    while (running) {
      // Check to see if any workers are done and write their reports
      std::list<WorkerPtr> finished_workers;
      {
        std::lock_guard<std::mutex> guard(mutex_);
        finished_workers.swap(finished_workers_);
        running = remaining_worker_count_ != 0;
      }
      for (auto& i : finished_workers) {
        WorkerReport& worker_report = report.worker_reports[pos++];
        i->create_worker_report(worker_report);
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
        std::lock_guard<std::mutex> guard(mutex_);
        kill_all_the_workers(); // Bwahahaha
        running = false;
      }

      if (running) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1));
      }
    }
    ACE_Reactor::instance()->cancel_timer(this);

    stat_collector.join();
    cpu_block->finalize();
    mem_block->finalize();

    std::cout << "Writing report for node " << node_id_ << std::endl;
    if (report_writer_impl->write(report, DDS::HANDLE_NIL)) {
      std::cerr << "Write report failed" << std::endl;
    }

    DDS::Duration_t timeout = { 30, 0 };
    if (report_writer_impl->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
      std::cerr << "Waiting for report acknowledgment failed" << std::endl;
    } else {
      std::cout << "All reports written and acknowledged." << std::endl;
    }
  }

  /// Used to the Handle Exit of a Worker
  virtual int handle_exit(ACE_Process* process)
  {
    pid_t pid = process->getpid();

    std::lock_guard<std::mutex> guard(mutex_);

    const auto i = pid_to_worker_id_.find(pid);
    if (i != pid_to_worker_id_.end()) {
      auto& worker = all_workers_[i->second];
      worker->set_exit_status(process->return_value(), process->exit_code());
      remaining_worker_count_--;
      finished_workers_.push_back(worker);
      cv_.notify_all();
    } else {
      std::cerr << "WorkerManager::handle_exit() received an unknown PID: " << pid << std::endl;
    }

    return 0;
  }

  /// Used to the Handle Scenario Timeout
  virtual int handle_timeout(const ACE_Time_Value&, const void* = nullptr)
  {
    scenario_timedout_.store(true);
    cv_.notify_all();
    return -1;
  }

  /// Used to the Interrupt
  virtual int handle_signal(int signum, siginfo_t* = nullptr, ucontext_t* = nullptr)
  {
    if (signum == SIGINT) {
      sigint_.store(true);
      cv_.notify_all();
    }
    return 0;
  }

private:
  unsigned timeout_ = 0;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::map<WorkerId, WorkerPtr> all_workers_;
  size_t remaining_worker_count_ = 0;
  std::map<pid_t, WorkerId> pid_to_worker_id_;
  std::list<WorkerPtr> finished_workers_;
  NodeId node_id_;
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
  CORBA::String_var type_name = status_ts->get_type_name();
  DDS::Topic_var status_topic = participant->create_topic(
    status_topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_topic) {
    std::cerr << "create_topic status failed" << std::endl;
    return 1;
  }
  Bench::TestController::AllocatedScenarioTypeSupport_var allocated_scenario_ts = new Bench::TestController::AllocatedScenarioTypeSupportImpl;
  if (allocated_scenario_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Config" << std::endl;
    return 1;
  }
  type_name = allocated_scenario_ts->get_type_name();
  DDS::Topic_var allocated_scenario_topic = participant->create_topic(
    allocated_scenario_topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!allocated_scenario_topic) {
    std::cerr << "create_topic config failed" << std::endl;
    return 1;
  }
  ReportTypeSupport_var report_ts = new ReportTypeSupportImpl;
  if (report_ts->register_type(participant, "")) {
    std::cerr << "register_type failed for Report" << std::endl;
    return 1;
  }
  type_name = report_ts->get_type_name();
  DDS::Topic_var report_topic = participant->create_topic(
    report_topic_name, type_name, TOPIC_QOS_DEFAULT, nullptr,
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
  DDS::DataReader_var allocated_scenario_reader = subscriber->create_datareader(
    allocated_scenario_topic, dr_qos, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!allocated_scenario_reader) {
    std::cerr << "create_datareader config failed" << std::endl;
    return 1;
  }
  AllocatedScenarioDataReader_var allocated_scenario_reader_impl = AllocatedScenarioDataReader::_narrow(allocated_scenario_reader);
  if (!allocated_scenario_reader_impl) {
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
  dw_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  dw_qos.history.depth = 1;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
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
  publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
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

  int exit_status = 0;
  {
    ACE_Process_Manager process_manager(ACE_Process_Manager::DEFAULT_SIZE, ACE_Reactor::instance());
    while (true) {
      exit_status = run_cycle(name, process_manager, participant,
        status_writer_impl, allocated_scenario_reader_impl, report_writer_impl);

      if (run_mode == RunMode::one_shot || (run_mode == RunMode::daemon_exit_on_error && exit_status != 0)) {
        break;
      }
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

bool write_status(
  const std::string& name,
  const NodeId& this_node_id,
  Bench::NodeController::StateEnum state,
  StatusDataWriter_var status_writer_impl)
{
  Status status;
  status.node_id = this_node_id;
  status.state = state;
  status.name = name.c_str();
  if (status_writer_impl->write(status, DDS::HANDLE_NIL)) {
    return false;
  }
  return true;
}

bool wait_for_scenario_data(AllocatedScenarioDataReader_var allocated_scenario_reader_impl)
{

  DDS::ReadCondition_var read_condition = allocated_scenario_reader_impl->create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(read_condition);

  while (!read_condition->get_trigger_value()) {
    DDS::ConditionSeq active;
    const DDS::Duration_t wake_interval = { 0, 500000000 };
    DDS::ReturnCode_t rc = ws->wait(active, wake_interval);
    if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_TIMEOUT) {
      std::cerr << "Wait for node config failed" << std::endl;
      return false;
    }
  }

  ws->detach_condition(read_condition);
  allocated_scenario_reader_impl->delete_readcondition(read_condition);

  return true;
}

void wait_for_full_scenario(
  const std::string& name,
  NodeId this_node_id,
  StatusDataWriter_var status_writer_impl,
  AllocatedScenarioDataReader_var allocated_scenario_reader_impl,
  Bench::TestController::AllocatedScenario& result)
{
  using Builder::ZERO;

  Bench::TestController::AllocatedScenario allocated_scenario;
  allocated_scenario.scenario_id = TAO::String_Manager();
  allocated_scenario.launch_time = ZERO;
  std::chrono::system_clock::time_point initial_attempt;

  bool complete = false;
  while (!complete) {
    while (!wait_for_scenario_data(allocated_scenario_reader_impl)) {
      // There was an actual DDS failure of some kind (not just a timeout), give it a few seconds and retry
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    DDS::ReturnCode_t rc = DDS::RETCODE_ERROR;
    Bench::TestController::AllocatedScenarioSeq scenarios;
    DDS::SampleInfoSeq info;
    rc = allocated_scenario_reader_impl->take(
      scenarios, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      std::cerr << "Take node config failed\n" << std::flush;
      continue;
    }

    if (allocated_scenario.scenario_id != TAO::String_Manager() &&
        initial_attempt + std::chrono::seconds(30) < std::chrono::system_clock::now())
    {
      if (write_status(name, this_node_id, AVAILABLE, status_writer_impl)) {
        allocated_scenario.scenario_id = TAO::String_Manager();
        allocated_scenario.launch_time = ZERO;
      }
    }

    for (CORBA::ULong scenario = 0; scenario < scenarios.length(); ++scenario) {
      Bench::NodeController::Configs& configs = scenarios[scenario].configs;
      for (CORBA::ULong node = 0; node < configs.length(); ++node) {
        if (configs[node].node_id == this_node_id) {
          if (allocated_scenario.scenario_id == TAO::String_Manager()) {
            if (write_status(name, this_node_id, BUSY, status_writer_impl)) {
              allocated_scenario = scenarios[scenario];
              initial_attempt = std::chrono::system_clock::now();
            }
          }
        }
      }
      if (std::string(scenarios[scenario].scenario_id.in()) == std::string(allocated_scenario.scenario_id.in()) &&
          !(scenarios[scenario].launch_time == ZERO))
      {
        allocated_scenario.launch_time = scenarios[scenario].launch_time;
      }
    }
    if (allocated_scenario.configs.length() != 0 && !(allocated_scenario.launch_time == ZERO)) {
      complete = true;
    }
  }
  result = allocated_scenario;
}

int run_cycle(
  const std::string& name,
  ACE_Process_Manager& process_manager,
  DDS::DomainParticipant_var participant,
  StatusDataWriter_var status_writer_impl,
  AllocatedScenarioDataReader_var allocated_scenario_reader_impl,
  ReportDataWriter_var report_writer_impl)
{
  NodeId this_node_id = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in())->get_id();

  WorkerManager worker_manager(this_node_id, process_manager);

  // Wait for Status Publication with Test Controller and Write Status
  if (!write_status(name, this_node_id, AVAILABLE, status_writer_impl)) {
    std::cerr << "Write status (available) failed\n" << std::flush;
    return 1;
  }

  Bench::TestController::AllocatedScenario scenario;
  wait_for_full_scenario(name, this_node_id, status_writer_impl, allocated_scenario_reader_impl, scenario);

  Bench::NodeController::Configs& configs = scenario.configs;
  for (CORBA::ULong node = 0; node < configs.length(); ++node) {
    if (configs[node].node_id == this_node_id) {
      worker_manager.timeout(configs[node].timeout);
      CORBA::ULong allocated_scenario_count = configs[node].workers.length();
      for (CORBA::ULong config = 0; config < allocated_scenario_count; config++) {
        WorkerId& id = configs[node].workers[config].worker_id;
        const WorkerId end = id + configs[node].workers[config].count;
        for (; id < end; id++) {
          if (worker_manager.add_worker(configs[node].workers[config])) {
            return 1;
          }
        }
      }
      break;
    }
  }

  using Builder::ZERO;

  if (scenario.launch_time < ZERO) {
    auto duration = -1 * get_duration(scenario.launch_time);
    if (duration > std::chrono::milliseconds(100)) {
      std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
    }
  } else {
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::system_clock::time_point(get_duration(scenario.launch_time)) - now;
    if (duration > std::chrono::milliseconds(100)) {
      std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
    }
  }

  // Run Workers and Wait for Them to Finish
  worker_manager.run_workers(report_writer_impl);

  DDS::Duration_t timeout = { 10, 0 };
  if (report_writer_impl->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
    std::cerr << "Waiting for report acknowledgment failed" << std::endl;
    return 1;
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));

  return 0;
}
