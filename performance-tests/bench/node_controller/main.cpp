/*
 * The Node Controller takes directives from a Test Controller, spawns Spawned Processes,
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
#include <ace/OS_NS_fcntl.h>
#ifdef ACE_WIN32
#include <ace/WFMO_Reactor.h>
#endif

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
#include "ProcessStatsCollector.h"
#include "PropertyStatBlock.h"

using namespace Bench::NodeController;
using Bench::get_option_argument_int;
using Bench::get_option_argument;
using Bench::join_path;
using Bench::string_replace;
using Bench::create_temp_dir;
using Bench::TestController::AllocatedScenarioDataReader;
using Bench::TestController::AllocatedScenarioDataReader_var;

using ProcessManagerPtr = std::shared_ptr<ACE_Process_Manager>;

std::string bench_root;
std::string temp_dir;
std::string output_dir;

int run_cycle(
  const std::string& name,
  ProcessManagerPtr process_manager,
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

void read_file(std::ifstream& ifs, ::TAO::String_Manager& str)
{
  ifs.seekg(0, ios::end);
  const std::ifstream::pos_type end_pos = ifs.tellg();
  ifs.seekg(0, ios::beg);
  const std::ifstream::pos_type beg_pos = ifs.tellg();

  const auto file_length = end_pos - beg_pos;

  std::vector<char> temp(static_cast<std::vector<char>::size_type>(file_length + 1));
  ifs.read(temp.data(), file_length);
  temp[static_cast<std::vector<char>::size_type>(file_length)] = '\0';

  const char* ptr = temp.data();
  str = ptr;
}

class SpawnedProcess {
public:
  SpawnedProcess() = delete;
  SpawnedProcess(const SpawnedProcess&) = delete;
  SpawnedProcess(SpawnedProcess&&) = delete;
  SpawnedProcess& operator=(const SpawnedProcess&) = delete;
  SpawnedProcess& operator=(SpawnedProcess&&) = delete;

  SpawnedProcess(const std::string& node_name, const NodeId& node_id, const SpawnedProcessConfig& config)
  : node_name_(node_name), node_id_(node_id)
  , spawned_process_id_(config.spawned_process_id)
  {
    std::stringstream ss;
    ss << 'n' << node_id_ << 'w' << spawned_process_id_;
    file_base_name_ = ss.str();
    allocated_scenario_filename_ = create_config(file_base_name_, config.config.in());
    report_filename_ = join_path(output_dir, file_base_name_ + "_report.json");
    log_filename_ = join_path(output_dir, file_base_name_ + "_log.txt");
    executable_name_ = config.executable.in();
    spawned_process_command_ = config.command.in();
    ignore_errors_ = config.ignore_errors;
  }

  ~SpawnedProcess()
  {
    if (!allocated_scenario_filename_.empty()) {
      try {
        ACE_OS::unlink(allocated_scenario_filename_.c_str());
        ACE_OS::unlink(report_filename_.c_str());
        ACE_OS::unlink(log_filename_.c_str());
      } catch (...) {
      }
    }
  }

  SpawnedProcessId id() noexcept
  {
    return spawned_process_id_;
  }

  NodeId nodeid() noexcept
  {
    return node_id_;
  }

  void create_spawned_process_report(SpawnedProcessReport& report)
  {
    report.spawned_process_id = spawned_process_id_;
    report.failed = (pid_ == ACE_INVALID_PID || exit_status_ != 0);
    report.details = "";
    report.log = "";

    if (!report.failed) {
      std::ifstream report_file(report_filename_);
      if (report_file.good()) {
        read_file(report_file, report.details);
      }
    }
    std::ifstream log_file(log_filename_);
    if (log_file.good()) {
      read_file(log_file, report.log);
    }
  }

  std::shared_ptr<ACE_Process_Options> get_proc_opts(ACE_HANDLE& log_handle) const
  {
    std::shared_ptr<ACE_Process_Options> proc_opts = std::make_shared<ACE_Process_Options>();
    std::stringstream ss;
    if (spawned_process_command_.empty()) {
      if (executable_name_.empty()) {
        ss << join_path(bench_root, "worker", "worker")
          << " " << allocated_scenario_filename_
          << " --report " << report_filename_
          << " --log " << log_filename_ << std::flush;
      } else {
        ss << join_path(bench_root, executable_name_) << std::flush;
      }
    } else {
      std::string command(spawned_process_command_);
      string_replace(command, "%ds%", ACE_DIRECTORY_SEPARATOR_STR_A);
      string_replace(command, "%bench_root%", bench_root);
      string_replace(command, "%executable_full_path%", join_path(bench_root, executable_name_));
      string_replace(command, "%executable%", executable_name_);
      string_replace(command, "%config%", allocated_scenario_filename_);
      string_replace(command, "%report%", report_filename_);
      if (command.find("%log%") != std::string::npos) {
        string_replace(command, "%log%", log_filename_);
      } else {
        log_handle = ACE_OS::open(log_filename_.c_str(), O_WRONLY | O_CREAT);
        if (log_handle != ACE_INVALID_HANDLE) {
          proc_opts->set_handles(ACE_STDIN, log_handle, log_handle);
        }
      }
      ss << command << std::flush;
    }
    const std::string command = ss.str();
    std::cerr << command + "\n" << std::flush;
    proc_opts->command_line("%s", command.c_str());
    return proc_opts;
  }

  void set_pid(pid_t pid) noexcept
  {
    pid_ = pid;
    running_ = true;
  }

  pid_t get_pid() noexcept
  {
    return pid_;
  }

  void set_exit_status(int return_code, ACE_exitcode exit_code) noexcept
  {
// TODO FIXME This is required to correctly detect segfaults on Linux
// It's possible we need to do something similar on other platforms and we will
// probably want to expand this to do the correct thing on windows / android / macos
#ifdef ACE_LINUX
    exit_status_ = ignore_errors_ ? 0 : (exit_code ? exit_code : return_code);
#else
    exit_status_ = ignore_errors_ ? 0 : return_code;
    ACE_UNUSED_ARG(exit_code);
#endif
    running_ = false;
  }

  bool running() const noexcept
  {
    return running_;
  }

private:
  std::string node_name_;
  NodeId node_id_;
  SpawnedProcessId spawned_process_id_;
  bool running_ = false;
  pid_t pid_ = ACE_INVALID_PID;
  int exit_status_ = 0;
  std::string file_base_name_;
  std::string allocated_scenario_filename_;
  std::string report_filename_;
  std::string log_filename_;
  std::string executable_name_;
  std::string spawned_process_command_;
  bool ignore_errors_;
};

using SpawnedProcessPtr = std::shared_ptr<SpawnedProcess>;
using ProcessStatsCollectorPtr = std::shared_ptr<ProcessStatsCollector>;

class SpawnedProcessManager : public ACE_Event_Handler {
public:

  explicit SpawnedProcessManager(const std::string& node_name, const NodeId& node_id, ProcessManagerPtr process_manager)
  : node_name_(node_name), node_id_(node_id)
  , process_manager_(process_manager)
  {
    process_manager->register_handler(this);
    ACE_Reactor::instance()->register_handler(SIGINT, this);
  }

  SpawnedProcessManager() = delete;
  SpawnedProcessManager(const SpawnedProcessManager&) = delete;
  SpawnedProcessManager(SpawnedProcessManager&&) = delete;
  SpawnedProcessManager& operator=(const SpawnedProcessManager&) = delete;
  SpawnedProcessManager& operator=(SpawnedProcessManager&&) = delete;

  ~SpawnedProcessManager()
  {
    try {
      ACE_Reactor::instance()->remove_handler(SIGINT, nullptr);
      process_manager_->register_handler(nullptr);
    }
    catch (...) {
    }
  }

  void timeout(unsigned value) noexcept
  {
    timeout_ = value;
  }

  bool add_spawned_process(const SpawnedProcessConfig& config)
  {
    std::lock_guard<std::mutex> guard(mutex_);
#ifdef ACE_WIN32
    if (all_spawned_processes_.size() >= ACE_WFMO_Reactor::DEFAULT_SIZE) {
      std::cerr << "ACE_WFMO_Reactor can't handle waiting (for signals) from more than " << ACE_WFMO_Reactor::DEFAULT_SIZE << " objects" << std::endl;
      return true;
    }
#endif
    if (all_spawned_processes_.count(config.spawned_process_id)) {
      std::cerr << "Received the same spawned process id twice: " << config.spawned_process_id << std::endl;
      return true;
    }
    all_spawned_processes_[config.spawned_process_id] = std::make_shared<SpawnedProcess>(node_name_, node_id_, config);
    remaining_spawned_process_count_++;
    return false;
  }

  // Must hold lock_
  void spawned_process_is_finished(SpawnedProcessPtr spawned_process)
  {
    remaining_spawned_process_count_--;
    finished_spawned_processes_.push_back(spawned_process);
    cv_.notify_all();
  }

  // Must hold lock_
  void kill_all_the_spawned_processes()
  {
    ProcessManagerPtr process_manager;
    std::map<SpawnedProcessId, SpawnedProcessPtr> all_spawned_processes;

    {
      std::lock_guard<std::mutex> guard(mutex_);
      process_manager = process_manager_;
      all_spawned_processes = all_spawned_processes_;
    }

    for (auto& spawned_process_i : all_spawned_processes) {
      auto& spawned_process = spawned_process_i.second;
      if (spawned_process->running()) {
#ifndef ACE_WIN32
        if (process_manager->terminate(spawned_process->get_pid(), SIGABRT)) {
          if (process_manager->wait(spawned_process->get_pid(), ACE_Time_Value(0, ACE_ONE_SECOND_IN_USECS / 10))) {
            continue;
          }
        }
#endif
        if (process_manager->terminate(spawned_process->get_pid())) {
          process_manager->wait(spawned_process->get_pid(), ACE_Time_Value(0, ACE_ONE_SECOND_IN_USECS / 10));
        }
      }
    }
  }

  bool run_spawned_processes(ReportDataWriter_var report_writer_impl)
  {
    ACE_Reactor::instance()->schedule_timer(this, nullptr, ACE_Time_Value(timeout_));
    // Spawn Processes
    {
      std::lock_guard<std::mutex> guard(mutex_);
      std::vector<std::shared_ptr<ACE_Process_Options>> spawned_proc_opts;
      spawned_proc_opts.reserve(all_spawned_processes_.size());
      std::vector<ACE_HANDLE> log_handles;
      log_handles.reserve(all_spawned_processes_.size());
      for (auto& spawned_process_i : all_spawned_processes_) {
        auto& spawned_process = spawned_process_i.second;
        log_handles.push_back(ACE_INVALID_HANDLE);
        spawned_proc_opts.push_back(spawned_process->get_proc_opts(log_handles.back()));
      }
      int index = 0;
      for (auto spawned_process_i : all_spawned_processes_) {
        auto& spawned_process = spawned_process_i.second;
        pid_t pid = process_manager_->spawn(*spawned_proc_opts[index]);
        if (log_handles[index] != ACE_INVALID_HANDLE) {
          ACE_OS::close(log_handles[index]);
        }
        if (pid != ACE_INVALID_PID) {
          spawned_process->set_pid(pid);
          pid_to_spawned_process_id_[pid] = spawned_process->id();
          spawned_process_process_stat_collectors_[pid] = std::make_shared<ProcessStatsCollector>(pid);
        } else {
          std::cerr << "Failed to run spawned process " << spawned_process->id() << std::endl;
          spawned_process_is_finished(spawned_process);
        }
        ++index;
      }
    }

    // Wait For Them to Finish, Writing Reports As They Do
    Report report{};
    report.spawned_process_reports.length(static_cast<CORBA::ULong>(all_spawned_processes_.size()));
    constexpr size_t max_stat_buffer_size = 3600; // one hour in seconds
    auto cpu_block = std::make_shared<Bench::PropertyStatBlock>(report.properties, "cpu_percent", max_stat_buffer_size, true);
    auto mem_block = std::make_shared<Bench::PropertyStatBlock>(report.properties, "mem_percent", max_stat_buffer_size, true);
    auto virtual_mem_block = std::make_shared<Bench::PropertyStatBlock>(report.properties, "virtual_mem_percent", max_stat_buffer_size, true);
    CORBA::ULong pos = 0;
    report.node_name = node_name_.c_str();
    report.node_id = node_id_;

    std::atomic<bool> running(true);

    std::thread stat_collector([&](){

      while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        double cpu_sum = 0.0;
        double mem_sum = 0.0;
        double virtual_mem_sum = 0.0;

        for (auto it = spawned_process_process_stat_collectors_.cbegin(); it != spawned_process_process_stat_collectors_.cend(); it++) {
          cpu_sum += it->second->get_cpu_usage();
          mem_sum += it->second->get_mem_usage();
          virtual_mem_sum += it->second->get_virtual_mem_usage();
        }
        const auto time = Builder::get_sys_time();
        cpu_block->update(cpu_sum, time);
        mem_block->update(mem_sum, time);
        virtual_mem_block->update(virtual_mem_sum, time);
      }
    });

    bool spawned_processes_killed = false;
    while (running) {
      // Check to see if any spawned_processs are done and write their reports
      std::list<exited_process> exited_processes;
      {
        std::lock_guard<std::mutex> guard(exited_processes_mutex_);
        exited_processes.swap(exited_processes_);
      }

      for (auto& ep : exited_processes) {
        std::stringstream ss_out, ss_err;
        {
          std::lock_guard<std::mutex> guard(mutex_);
          const auto i = pid_to_spawned_process_id_.find(ep.pid);
          if (i != pid_to_spawned_process_id_.end()) {
            auto& spawned_process = all_spawned_processes_[i->second];
            ss_out << "SpawnedProcessManager::handle_exit() - Handling exit of process " << ep.pid << " at " << Bench::iso8601() << " with exit code " << ep.exit_code << std::endl;
            spawned_process->set_exit_status(ep.return_value, ep.exit_code);
            remaining_spawned_process_count_--;
            finished_spawned_processes_.push_back(spawned_process);
            cv_.notify_all();
          } else {
            ss_err << "SpawnedProcessManager::handle_exit() received an unknown PID: " << ep.pid << std::endl;
          }
        }

        if (!ss_out.str().empty()) {
          std::cout << ss_out.str() << std::flush;
        }
        if (!ss_err.str().empty()) {
          std::cerr << ss_err.str() << std::flush;
        }
      }

      std::list<SpawnedProcessPtr> finished_spawned_processes;
      {
        std::lock_guard<std::mutex> guard(mutex_);
        finished_spawned_processes.swap(finished_spawned_processes_);
        running = remaining_spawned_process_count_ != 0;
      }
      for (auto& i : finished_spawned_processes) {
        SpawnedProcessReport& spawned_process_report = report.spawned_process_reports[pos++];
        i->create_spawned_process_report(spawned_process_report);
      }

      // Check to see if we have to stop prematurely
      bool kill_spawned_processes = false;
      if (!spawned_processes_killed && scenario_timedout_.load()) {
        std::stringstream ss;
        ss << "Scenario timed out at " << Bench::iso8601() << ", Killing Spawned Processes..." << std::endl;
        std::cerr << ss.str() << std::flush;
        kill_spawned_processes = true;
      }
      if (!spawned_processes_killed && sigint_.load()) {
        std::stringstream ss;
        ss << "Interrupted, Killing Spawned Processes..." << std::endl;
        std::cerr << ss.str() << std::flush;
        kill_spawned_processes = true;
      }
      if (!spawned_processes_killed && kill_spawned_processes) {
        kill_all_the_spawned_processes(); // Bwahahaha
        spawned_processes_killed = true;
      }

      if (running) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1));
      }
    }
    ACE_Reactor::instance()->cancel_timer(this);

    stat_collector.join();

    try {
      cpu_block->finalize();
      mem_block->finalize();
      virtual_mem_block->finalize();
    } catch (const std::exception& e) {
      std::cerr << "Exception caught trying to finalize statistic blocks: " << e.what() << std::endl;
      return false;
    }

    if (sigint_.load()) {
      return false;
    }

    if (node_name_.length() > 0) {
      std::cout << "Writing report for node name '" << node_name_ << "' (id '" << node_id_ << "') with " << report.spawned_process_reports.length() << " spawned process reports" << std::endl;
    } else {
      std::cout << "Writing report for node id '" << node_id_ << "' with " << report.spawned_process_reports.length() << " spawned process reports" << std::endl;
    }
    if (report_writer_impl->write(report, DDS::HANDLE_NIL)) {
      std::cerr << "Write report failed" << std::endl;
    }

    DDS::Duration_t timeout = { 30, 0 };
    if (report_writer_impl->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
      std::cerr << "Waiting for report acknowledgment failed" << std::endl;
    } else {
      std::cout << "All reports written and acknowledged." << std::endl;
    }
    return true;
  }

  /// Used to the Handle Exit of a SpawnedProcess
  int handle_exit(ACE_Process* process) override
  {
    assert(process != nullptr);

    const exited_process ep = { process->getpid(), process->return_value(), process->exit_code() };
    {
      std::lock_guard<std::mutex> guard(exited_processes_mutex_);
      exited_processes_.push_back(ep);
    }

    cv_.notify_all();
    return 0;
  }

  /// Used to the Handle Scenario Timeout
  int handle_timeout(const ACE_Time_Value&, const void* = nullptr) noexcept override
  {
    scenario_timedout_.store(true);
    cv_.notify_all();
    return -1;
  }

  /// Used to the Interrupt
  int handle_signal(int signum, siginfo_t* = nullptr, ucontext_t* = nullptr) noexcept override
  {
    if (signum == SIGINT) {
      sigint_.store(true);
      cv_.notify_all();
    }
    return 0;
  }

private:

  struct exited_process {
    pid_t pid;
    int return_value;
    ACE_exitcode exit_code;
  };
  std::mutex exited_processes_mutex_;
  std::list<exited_process> exited_processes_;

  unsigned timeout_ = 0;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::map<SpawnedProcessId, SpawnedProcessPtr> all_spawned_processes_;
  size_t remaining_spawned_process_count_ = 0;
  std::map<pid_t, SpawnedProcessId> pid_to_spawned_process_id_;
  std::map<pid_t, ProcessStatsCollectorPtr> spawned_process_process_stat_collectors_;
  std::list<SpawnedProcessPtr> finished_spawned_processes_;
  std::string node_name_;
  NodeId node_id_;
  ProcessManagerPtr process_manager_;
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
    bench_root = join_path(dds_root, "performance-tests", "bench");
  }

  TheServiceParticipant->default_configuration_file(
    ACE_TEXT_CHAR_TO_TCHAR(join_path(bench_root, "control_opendds_config.ini").c_str()));

  // Parse Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  RunMode run_mode = RunMode::one_shot;
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
  } catch(const int value) {
    std::cerr << "See DDS_ROOT/performance-tests/bench/README.md for usage" << std::endl;
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

  ACE_Reactor* reactor = ACE_Reactor::instance();
  ACE_Thread_Manager* thread_manager = ACE_Thread_Manager::instance();

  // Wait For and Run Node Configurations
  std::thread reactor_thread([&]() {
    if (reactor->owner(thread_manager->thr_self())) {
      std::cerr << "Error setting Reactor Thread" << std::endl;
    }
    reactor->run_reactor_event_loop();
  });

  int exit_status = 0;
  {
    ProcessManagerPtr process_manager = std::make_shared<ACE_Process_Manager>(ACE_Process_Manager::DEFAULT_SIZE, reactor);
    while (true) {
      exit_status = run_cycle(name, process_manager, participant,
        status_writer_impl, allocated_scenario_reader_impl, report_writer_impl);

      if (run_mode == RunMode::one_shot || (run_mode == RunMode::daemon_exit_on_error && exit_status != 0)) {
        break;
      }
    }
  }

  // Clean up
  reactor->end_reactor_event_loop();
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
  StatusDataWriter& status_writer_impl)
{
  Status status;
  status.node_id = this_node_id;
  status.state = state;
  status.name = name.c_str();
  if (status_writer_impl.write(status, DDS::HANDLE_NIL)) {
    return false;
  }
  return true;
}

bool wait_for_scenario_data(AllocatedScenarioDataReader& allocated_scenario_reader_impl)
{

  DDS::ReadCondition_var read_condition = allocated_scenario_reader_impl.create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  DDS::WaitSet_var ws(new DDS::WaitSet());
  ws->attach_condition(read_condition);

  while (!read_condition->get_trigger_value()) {
    DDS::ConditionSeq active;
    const DDS::Duration_t wake_interval = { 0, 500000000 };
    const DDS::ReturnCode_t rc = ws->wait(active, wake_interval);
    if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_TIMEOUT) {
      std::cerr << "Wait for node config failed" << std::endl;
      return false;
    }
  }

  ws->detach_condition(read_condition);
  allocated_scenario_reader_impl.delete_readcondition(read_condition);

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
    while (!wait_for_scenario_data(*allocated_scenario_reader_impl)) {
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
        initial_attempt + std::chrono::seconds(30) < std::chrono::system_clock::now()) {
      if (write_status(name, this_node_id, AVAILABLE, *status_writer_impl)) {
        allocated_scenario.scenario_id = TAO::String_Manager();
        allocated_scenario.launch_time = ZERO;
      }
    }

    for (CORBA::ULong scenario = 0; scenario < scenarios.length(); ++scenario) {
      Bench::NodeController::Configs& configs = scenarios[scenario].configs;
      for (CORBA::ULong node = 0; node < configs.length(); ++node) {
        if (configs[node].node_id == this_node_id) {
          if (allocated_scenario.scenario_id == TAO::String_Manager()) {
            if (write_status(name, this_node_id, BUSY, *status_writer_impl)) {
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
  ProcessManagerPtr process_manager,
  DDS::DomainParticipant_var participant,
  StatusDataWriter_var status_writer_impl,
  AllocatedScenarioDataReader_var allocated_scenario_reader_impl,
  ReportDataWriter_var report_writer_impl)
{
  OpenDDS::DCPS::DomainParticipantImpl* part_impl = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());
  if (!part_impl) {
    std::cerr << "Invalid Participant\n" << std::flush;
    return 1;
  }

  const NodeId this_node_id = part_impl->get_id();

  // Wait for Status Publication with Test Controller and Write Status
  if (!write_status(name, this_node_id, AVAILABLE, *status_writer_impl)) {
    std::cerr << "Write status (available) failed\n" << std::flush;
    return 1;
  }

  Bench::TestController::AllocatedScenario scenario;
  wait_for_full_scenario(name, this_node_id, status_writer_impl, allocated_scenario_reader_impl, scenario);

  // This constructor traps signals, wait until we really need it.
  auto spawned_process_manager = std::make_shared<SpawnedProcessManager>(name, this_node_id, process_manager);

  Bench::NodeController::Configs& configs = scenario.configs;
  for (CORBA::ULong node = 0; node < configs.length(); ++node) {
    if (configs[node].node_id == this_node_id) {
      spawned_process_manager->timeout(configs[node].timeout);
      const CORBA::ULong allocated_scenario_count = configs[node].spawned_processes.length();
      for (CORBA::ULong config = 0; config < allocated_scenario_count; config++) {
        SpawnedProcessId& id = configs[node].spawned_processes[config].spawned_process_id;
        const SpawnedProcessId end = id + configs[node].spawned_processes[config].count;
        for (; id < end; id++) {
          if (spawned_process_manager->add_spawned_process(configs[node].spawned_processes[config])) {
            return 1;
          }
        }
      }
      break;
    }
  }

  using Builder::ZERO;

  if (scenario.launch_time < ZERO) {
    const auto duration = -1 * get_duration(scenario.launch_time);
    if (duration > std::chrono::milliseconds(100)) {
      std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
    }
  } else {
    const auto now = std::chrono::system_clock::now();
    const auto duration = std::chrono::system_clock::time_point(get_duration(scenario.launch_time)) - now;
    if (duration > std::chrono::milliseconds(100)) {
      std::this_thread::sleep_until(std::chrono::steady_clock::now() + duration);
    }
  }

  // Run Spawned Processess and Wait for Them to Finish
  if (!spawned_process_manager->run_spawned_processes(report_writer_impl)) {
    std::cerr << "Running spawned processes failed (likely because we received a SIGINT)" << std::endl;
    return 1;
  }
  spawned_process_manager.reset();

  const DDS::Duration_t timeout = { 10, 0 };
  if (report_writer_impl->wait_for_acknowledgments(timeout) != DDS::RETCODE_OK) {
    std::cerr << "Waiting for report acknowledgment failed" << std::endl;
    return 1;
  }

  return 0;
}
