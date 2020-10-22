#include "ScenarioManager.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/GuardCondition.h>

#include <ace/OS_NS_unistd.h>

#include <util.h>
#include <json_conversion.h>

using namespace Bench;
using namespace Bench::TestController;

ScenarioManager::ScenarioManager(
  const std::string& bench_root,
  const std::string& test_context,
  const ScenarioOverrides& overrides,
  DdsEntities& dds_entities)
: bench_root_(bench_root)
, test_context_(test_context)
, overrides_(overrides)
, dds_entities_(dds_entities)
{
}

ScenarioManager::~ScenarioManager()
{
}

Nodes ScenarioManager::discover_nodes(unsigned wait_for_nodes)
{
  ACE_OS::sleep(wait_for_nodes);
  Nodes available_nodes = dds_entities_.status_listener_.get_available_nodes();
  if (available_nodes.empty()) {
    throw std::runtime_error("no available nodes discovered during wait");
  }
  return available_nodes;
}

namespace {
  std::string read_file(const std::string& name)
  {
    std::stringstream ss;
    std::ifstream file(name);
    if (file.is_open()) {
      ss << file.rdbuf();
    } else {
      throw std::runtime_error("Couldn't open " + name);
    }
    return ss.str();
  }

  void read_protoworker_configs(
    const std::string& test_context,
    const WorkerPrototypes& protoworkers,
    std::map<std::string, std::string>& worker_configs,
    bool debug_alloc)
  {
    for (unsigned protoworker_i = 0; protoworker_i < protoworkers.length(); protoworker_i++) {
      const std::string filename = protoworkers[protoworker_i].config.in();
      if (!worker_configs.count(filename)) {
        worker_configs[filename] = debug_alloc ?
          filename : read_file(join_path(test_context, "config", "worker", filename));
      }
    }
  }

  void add_single_worker_to_node(
    const WorkerPrototype& protoworker,
    const std::map<std::string, std::string>& worker_configs,
    NodeController::Config& node)
  {
    unsigned worker_i = node.workers.length();
    NodeController::WorkerId worker_id = worker_i ? (node.workers[worker_i - 1].worker_id + node.workers[worker_i - 1].count) : 1;
    node.workers.length(node.workers.length() + 1);
    auto& worker = node.workers[worker_i];
    worker.config = worker_configs.find(protoworker.config.in())->second.c_str();
    worker.count = 1;
    worker.worker_id = worker_id;
  }

  unsigned add_protoworkers_to_node(
    const WorkerPrototypes& protoworkers,
    const std::map<std::string, std::string>& worker_configs,
    NodeController::Config& node)
  {
    unsigned new_worker_total = 0;
    for (unsigned i = 0; i < protoworkers.length(); i++) {
      new_worker_total += protoworkers[i].count;
    }

    unsigned worker_i = node.workers.length();
    NodeController::WorkerId worker_id = worker_i ? (node.workers[worker_i - 1].worker_id + node.workers[worker_i - 1].count) : 1;
    node.workers.length(node.workers.length() + protoworkers.length());
    for (unsigned i = 0; i < protoworkers.length(); i++) {
      auto& protoworker = protoworkers[i];
      auto& worker = node.workers[worker_i];
      worker.config = worker_configs.find(protoworker.config.in())->second.c_str();
      worker.count = protoworker.count;
      worker.worker_id = worker_id;
      worker_id += protoworker.count;
      worker_i++;
    }

    return new_worker_total;
  }

}

void ScenarioManager::customize_configs(std::map<std::string, std::string>& worker_configs) {
  using namespace std::chrono;
  system_clock::time_point cnow = system_clock::now();
  Builder::TimeStamp now = Builder::get_sys_time();

  for (auto it = worker_configs.begin(); it != worker_configs.end(); ++it) {

    // Convert to C++ IDL-generated structures
    std::stringstream iss(it->second);
    Bench::WorkerConfig wc{};
    if (!Bench::json_2_idl(iss, wc)) {
      throw std::runtime_error("Can't parse json configs for customization");
    }

    // Apply Individual Overrides
    if (!overrides_.bench_partition_suffix.empty()) {
      for (CORBA::ULong i = 0; i < wc.process.participants.length(); ++i) {
        for (CORBA::ULong j = 0; j < wc.process.participants[i].subscribers.length(); ++j) {
          for (CORBA::ULong k = 0; k < wc.process.participants[i].subscribers[j].qos.partition.name.length(); ++k) {
            std::string temp(wc.process.participants[i].subscribers[j].qos.partition.name[k].in());
            if (temp.substr(0, 6) == "bench_") {
              wc.process.participants[i].subscribers[j].qos.partition.name[k] = (temp + overrides_.bench_partition_suffix).c_str();
              wc.process.participants[i].subscribers[j].qos_mask.partition.has_name = true;
            }
          }
        }
        for (CORBA::ULong j = 0; j < wc.process.participants[i].publishers.length(); ++j) {
          for (CORBA::ULong k = 0; k < wc.process.participants[i].publishers[j].qos.partition.name.length(); ++k) {
            std::string temp(wc.process.participants[i].publishers[j].qos.partition.name[k].in());
            if (temp.substr(0, 6) == "bench_") {
              wc.process.participants[i].publishers[j].qos.partition.name[k] = (temp + overrides_.bench_partition_suffix).c_str();
              wc.process.participants[i].publishers[j].qos_mask.partition.has_name = true;
            }
          }
        }
      }
    }

    std::cout << "Processing Overrides for config '" << it->first << "'" << std::endl;

    if (overrides_.create_time_delta) {
      wc.create_time = now + Builder::from_seconds(overrides_.create_time_delta);
      std::cout << "- Overriding create_time to be "
                << overrides_.create_time_delta << " seconds from now: "
                << iso8601(cnow + seconds(overrides_.create_time_delta)) << std::endl;
    } else {
      // TODO FIXME This won't be right for all test scenarios, but not sure how else to avoid uninitialized values for now
      wc.create_time = Builder::ZERO;
    }

    if (overrides_.enable_time_delta) {
      wc.enable_time = now + Builder::from_seconds(overrides_.enable_time_delta);
      std::cout << "- Overriding enable_time to be "
                << overrides_.enable_time_delta << " seconds from now: "
                << iso8601(cnow + seconds(overrides_.enable_time_delta)) << std::endl;
    }

    if (overrides_.start_time_delta) {
      wc.start_time = now + Builder::from_seconds(overrides_.start_time_delta);
      std::cout << "- Overriding start_time to be "
                << overrides_.start_time_delta << " seconds from now: "
                << iso8601(cnow + seconds(overrides_.start_time_delta)) << std::endl;
    }

    if (overrides_.stop_time_delta) {
      wc.stop_time = now + Builder::from_seconds(overrides_.stop_time_delta);
      std::cout << "- Overriding stop_time to be "
                << overrides_.stop_time_delta << " seconds from now: "
                << iso8601(cnow + seconds(overrides_.stop_time_delta)) << std::endl;
    }

    if (overrides_.destruction_time_delta) {
      wc.destruction_time = now + Builder::from_seconds(overrides_.destruction_time_delta);
      std::cout << "- Overriding destruction_time to be "
                << overrides_.destruction_time_delta << " seconds from now: "
                << iso8601(cnow + seconds(overrides_.destruction_time_delta)) << std::endl;
    }

    // Convert back to JSON
    std::stringstream oss;
    if (!Bench::idl_2_json(wc, oss, false)) {
      throw std::runtime_error("Can't reserialize customized json configs");
    }
    it->second = oss.str();
  }
}

AllocatedScenario ScenarioManager::allocate_scenario(
  const ScenarioPrototype& scenario_prototype, Nodes& available_nodes, bool debug_alloc)
{
  if (scenario_prototype.nodes.length() == 0 && scenario_prototype.any_node.length() == 0) {
    throw std::runtime_error("Scenario Prototype is empty!");
  }

  // Get Maximum and Exclusive Node Counts
  unsigned maximum_nodes = 0;
  unsigned exclusive_nodes = 0;
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); i++) {
    if (scenario_prototype.nodes[i].exclusive) {
      exclusive_nodes += scenario_prototype.nodes[i].count;
    }
    maximum_nodes += scenario_prototype.nodes[i].count;
  }
  // If there are enough nodes, each any node worker can get one to itself.
  for (unsigned protoworker_i = 0; protoworker_i < scenario_prototype.any_node.length(); protoworker_i++) {
    maximum_nodes += scenario_prototype.any_node[protoworker_i].count;
  }

  // Get Minimum Node Count
  unsigned minimum_nodes = 0;
  minimum_nodes = exclusive_nodes;
  if (maximum_nodes - exclusive_nodes) { // If there are any non-exclusive node configs
    minimum_nodes++; // Then there just has to be at least one node for them
  }

  // Make Sure We Have Enough Nodes
  if (available_nodes.size() < minimum_nodes) {
    std::stringstream ss;
    ss
      << "At least " << minimum_nodes << " available node" << (minimum_nodes == 1 ? " is " : "s are ")
      << "required for this scenario.";
    throw std::runtime_error(ss.str());
  }

  // Start Building the Scenario
  const unsigned node_count = std::min(static_cast<unsigned>(available_nodes.size()), maximum_nodes);
  AllocatedScenario allocated_scenario;
  allocated_scenario.expected_reports = 0;
  allocated_scenario.timeout = scenario_prototype.timeout;
  allocated_scenario.configs.length(node_count);

  // Set the Node IDs and Initialize Worker Length
  auto ani = available_nodes.begin();
  for (unsigned i = 0; i < node_count; i++) {
    allocated_scenario.configs[i].node_id = ani->first;
    allocated_scenario.configs[i].timeout = scenario_prototype.timeout;
    allocated_scenario.configs[i].workers.length(0);
    ani++;
  }

  // Read in Worker Config Files
  std::map<std::string, std::string> worker_configs;
  for (unsigned protonode_i = 0; protonode_i < scenario_prototype.nodes.length(); protonode_i++) {
    read_protoworker_configs(
      test_context_, scenario_prototype.nodes[protonode_i].workers, worker_configs, debug_alloc);
  }
  read_protoworker_configs(
    test_context_, scenario_prototype.any_node, worker_configs, debug_alloc);

  customize_configs(worker_configs);

  // Allocate Exclusive Node Configs First
  unsigned node_i = 0; // iter for allocated_scenario.configs
  for (unsigned protonode_i = 0; protonode_i < scenario_prototype.nodes.length(); protonode_i++) {
    auto& protonode = scenario_prototype.nodes[protonode_i];
    if (protonode.exclusive) {
      for (unsigned count = 0; count < protonode.count; count++) {
        allocated_scenario.expected_reports += add_protoworkers_to_node(
          protonode.workers, worker_configs, allocated_scenario.configs[node_i++]);
      }
    }
  }

  // Then Allocate Nonexclusive Node Configs
  const unsigned nonexclusive_start = node_i;
  for (unsigned protonode_i = 0; protonode_i < scenario_prototype.nodes.length(); protonode_i++) {
    auto& protonode = scenario_prototype.nodes[protonode_i];
    if (!protonode.exclusive) {
      for (unsigned count = 0; count < protonode.count; count++) {
        allocated_scenario.expected_reports += add_protoworkers_to_node(
          protonode.workers, worker_configs, allocated_scenario.configs[node_i++]);
        if (node_i >= node_count) {
          node_i = nonexclusive_start;
        }
      }
    }
  }

  // Finally Allocate Any-Node Worker Configs
  for (unsigned protoworker_i = 0; protoworker_i < scenario_prototype.any_node.length(); protoworker_i++) {
    const WorkerPrototype& protoworker = scenario_prototype.any_node[protoworker_i];
    allocated_scenario.expected_reports += protoworker.count;
    for (unsigned count_i = 0; count_i < protoworker.count; count_i++) {
      add_single_worker_to_node(protoworker, worker_configs, allocated_scenario.configs[node_i++]);
      if (node_i >= node_count) {
        node_i = nonexclusive_start;
      }
    }
  }

  char host[256];
  ACE_OS::hostname(host, sizeof(host));
  pid_t pid = ACE_OS::getpid();
  std::stringstream ss;
  ss << host << "_" << pid << std::flush;
  allocated_scenario.scenario_id = ss.str().c_str();

  allocated_scenario.launch_time = Builder::ZERO;

  return allocated_scenario;
}

void ScenarioManager::execute(const Bench::TestController::AllocatedScenario& allocated_scenario, std::vector<Bench::WorkerReport>& worker_reports, NodeController::ReportSeq& nc_reports)
{
  worker_reports.clear();
  nc_reports.length(0);

  using namespace std::chrono;
  // Write Configs
  if (dds_entities_.scenario_writer_impl_->write(allocated_scenario, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    throw std::runtime_error("Config Write Failed!");
  }

  DDS::Duration_t delay = { 3, 0 };
  if (dds_entities_.scenario_writer_impl_->wait_for_acknowledgments(delay) != DDS::RETCODE_OK) {
    throw std::runtime_error("Wait For Ack Failed");
  }

  AllocatedScenario temp = allocated_scenario;
  temp.configs.length(0);
  temp.launch_time = Builder::get_sys_time() + Builder::from_seconds(3);
  std::cout << "Setting scenario launch_time to be 3 seconds from now: "
            << iso8601(system_clock::now() + seconds(3)) << std::endl;

  // Write Configs
  if (dds_entities_.scenario_writer_impl_->write(temp, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    throw std::runtime_error("Config Write Failed!");
  }

  if (dds_entities_.scenario_writer_impl_->wait_for_acknowledgments(delay) != DDS::RETCODE_OK) {
    throw std::runtime_error("Wait For 'Launch Time' Ack Failed");
  }

  // Set up Waiting for Reading Reports or the Scenario Timeout
  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  DDS::ReadCondition_var read_condition = dds_entities_.report_reader_impl_->create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  wait_set->attach_condition(read_condition);
  DDS::GuardCondition_var guard_condition = new DDS::GuardCondition;
  wait_set->attach_condition(guard_condition);

  // Timeout Thread
  size_t reports_left = allocated_scenario.expected_reports;
  std::mutex reports_left_mutex;
  std::condition_variable timeout_cv;
  const std::chrono::seconds timeout(allocated_scenario.timeout);
  std::shared_ptr<std::thread> timeout_thread;
  if (timeout.count() > 0) {
    timeout_thread.reset(new std::thread([&] {
      std::unique_lock<std::mutex> lock(reports_left_mutex);
      if (!timeout_cv.wait_for(lock, timeout, [&] {return reports_left == 0;})) {
        guard_condition->set_trigger_value(true);
      }
    }));
  }

  // Wait for reports
  std::vector<WorkerReport> parsed_reports;
  size_t parse_failures = 0;
  size_t worker_failures = 0;
  while (true) {
    DDS::ReturnCode_t rc;

    while (!read_condition->get_trigger_value()) {
      DDS::ConditionSeq active;
      const DDS::Duration_t wake_interval = { 0, 500000000 };
      rc = wait_set->wait(active, wake_interval);
      if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_TIMEOUT) {
        throw std::runtime_error("Error while waiting for reports");
      }
      if (rc == DDS::RETCODE_OK) {
        for (unsigned i = 0; i < active.length(); i++) {
          if (active[i] == guard_condition) {
            timeout_cv.notify_all();
            if (timeout_thread) {
              timeout_thread->join();
            }
            throw std::runtime_error("Timedout waiting for the scenario to complete");
          }
        }
      }
    }

    NodeController::ReportSeq reports;
    DDS::SampleInfoSeq info;
    rc = dds_entities_.report_reader_impl_->take(
      reports, info,
      DDS::LENGTH_UNLIMITED,
      DDS::ANY_SAMPLE_STATE,
      DDS::ANY_VIEW_STATE,
      DDS::ANY_INSTANCE_STATE);
    if (rc != DDS::RETCODE_OK) {
      throw std::runtime_error("Take from report reader failed");
    }

    for (CORBA::ULong r = 0; r < reports.length(); r++) {
      if (info[r].valid_data) {
        const NodeController::WorkerReports& worker_reports = reports[r].worker_reports;
        for (CORBA::ULong wr = 0; wr < worker_reports.length(); wr++) {
          if (worker_reports[wr].failed) {
            ++worker_failures;
            std::stringstream ss;
            ss << "Worker " << worker_reports[wr].worker_id << " of node "
              << reports[r].node_id << " failed with log:\n===\n"
              << std::string(worker_reports[wr].log.in()) + "\n===\n";
            std::cerr << ss.str() << std::flush;
          } else {
            WorkerReport report{};
            std::stringstream ss;
            ss << worker_reports[wr].details << std::flush;
            if (json_2_idl(ss, report)) {
              parsed_reports.push_back(report);
            } else {
              ++parse_failures;
              std::stringstream ess;
              ess << "Error parsing report details from Worker " << worker_reports[wr].worker_id
                << " of node " << reports[r].node_id;
              std::cerr << ess.str() + "\n" << std::flush;
            }
          }
          std::stringstream ss;
          ss << "Got " << parsed_reports.size() << " out of "
            << allocated_scenario.expected_reports << " expected reports";
          if (worker_failures != 0 || parse_failures != 0) {
            ss << " (with " << worker_failures << " worker failures and "
              << parse_failures << " parse failures)";
          }
          ss << std::endl;
          std::cerr << ss.str() << std::flush;
        }
        reports[r].worker_reports.length(0);
        nc_reports.length(nc_reports.length() + 1);
        nc_reports[nc_reports.length() - 1] = reports[r];
      }
    }
    {
      std::lock_guard<std::mutex> guard(reports_left_mutex);
      reports_left = static_cast<size_t>(allocated_scenario.expected_reports) - parsed_reports.size() - worker_failures - parse_failures;
      if (reports_left == 0) {
        break;
      }
    }
  }

  if (timeout.count() > 0) {
    timeout_cv.notify_all();
    if (timeout_thread) {
      timeout_thread->join();
    }
  }

  wait_set->detach_condition(read_condition);
  dds_entities_.report_reader_impl_->delete_readcondition(read_condition);
  wait_set->detach_condition(guard_condition);

  worker_reports = parsed_reports;
}
