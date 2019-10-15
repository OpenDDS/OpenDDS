#include "ScenarioManager.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/GuardCondition.h>

#include <util.h>
#include <json_conversion.h>

using namespace Bench;
using namespace Bench::TestController;
using namespace Bench::NodeController;

ScenarioManager::ScenarioManager(
  const std::string& bench_root,
  const std::string& test_context,
  DdsEntities& dds_entities)
: bench_root_(bench_root)
, test_context_(test_context)
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
    std::map<std::string, std::string>& worker_configs)
  {
    for (unsigned protoworker_i = 0; protoworker_i < protoworkers.length(); protoworker_i++) {
      const std::string filename = protoworkers[protoworker_i].config.in();
      if (!worker_configs.count(filename)) {
        worker_configs[filename] = read_file(join_path(test_context, "config", "worker", filename));
      }
    }
  }

  void add_single_protoworker_to_node(
    const WorkerPrototype& protoworker,
    const std::map<std::string, std::string>& worker_configs,
    Config& node)
  {
    unsigned worker_i = node.workers.length();
    WorkerId worker_id = worker_i ? (node.workers[worker_i - 1].worker_id + node.workers[worker_i - 1].count) : 1;
    node.workers.length(node.workers.length() + 1);
    auto& worker = node.workers[worker_i];
    worker.config = worker_configs.find(protoworker.config.in())->second.c_str();
    worker.count = 1;
    worker.worker_id = worker_id;
  }

  unsigned add_protoworkers_to_node(
    const WorkerPrototypes& protoworkers,
    const std::map<std::string, std::string>& worker_configs,
    Config& node)
  {
    unsigned new_worker_total = 0;
    for (unsigned i = 0; i < protoworkers.length(); i++) {
      new_worker_total += protoworkers[i].count;
    }

    unsigned worker_i = node.workers.length();
    WorkerId worker_id = worker_i ? (node.workers[worker_i - 1].worker_id + node.workers[worker_i - 1].count) : 1;
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
    allocated_scenario.configs[i].workers.length(0);
    ani++;
  }

  // Read in Worker Config Files
  std::map<std::string, std::string> worker_configs; // This is filename (no dirs) to the contents
  if (debug_alloc) {
    // Just Fill in the Config Field with the Filename
    for (unsigned protonode_i = 0; protonode_i < scenario_prototype.nodes.length(); protonode_i++) {
      const WorkerPrototypes& protoworkers = scenario_prototype.nodes[protonode_i].workers;
      for (unsigned protoworker_i = 0; protoworker_i < protoworkers.length(); protoworker_i++) {
        const std::string filename = protoworkers[protoworker_i].config.in();
        worker_configs[filename] = filename;
      }
    }
    for (unsigned protoworker_i = 0; protoworker_i < scenario_prototype.any_node.length(); protoworker_i++) {
      const std::string filename = scenario_prototype.any_node[protoworker_i].config.in();
      worker_configs[filename] = filename;
    }
  } else {
    // Do the Real Thing
    for (unsigned protonode_i = 0; protonode_i < scenario_prototype.nodes.length(); protonode_i++) {
      read_protoworker_configs(test_context_, scenario_prototype.nodes[protonode_i].workers, worker_configs);
    }
    read_protoworker_configs(test_context_, scenario_prototype.any_node, worker_configs);
  }

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
      add_single_protoworker_to_node(protoworker, worker_configs, allocated_scenario.configs[node_i++]);
      if (node_i >= node_count) {
        node_i = nonexclusive_start;
      }
    }
  }

  return allocated_scenario;
}

std::vector<WorkerReport> ScenarioManager::execute(const AllocatedScenario& allocated_scenario)
{
  // Write Configs
  for (unsigned i = 0; i < allocated_scenario.configs.length(); i++) {
    if (dds_entities_.config_writer_impl_->write(allocated_scenario.configs[i], DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
      throw std::runtime_error("Config write failed!");
    }
  }

  // Set up Waiting for Reading Reports or the Scenario Timeout
  DDS::WaitSet_var wait_set = new DDS::WaitSet;
  DDS::ReadCondition_var read_condition = dds_entities_.report_reader_impl_->create_readcondition(
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
  wait_set->attach_condition(read_condition);
  DDS::GuardCondition_var guard_condition = new DDS::GuardCondition;
  wait_set->attach_condition(guard_condition);

  // Timeout Thread
  std::condition_variable timeout_cv;
  const std::chrono::seconds timeout(allocated_scenario.timeout);
  std::thread timeout_thread;
  if (timeout.count() > 0) {
    std::thread temp([&] {
      std::mutex mutex;
      std::unique_lock<std::mutex> lock(mutex);
      if (timeout_cv.wait_for(lock, timeout) == std::cv_status::timeout) {
        guard_condition->set_trigger_value(true);
      }
    });
    timeout_thread.swap(temp);
  }

  // Wait for reports
  std::vector<WorkerReport> parsed_reports;
  while (parsed_reports.size() < allocated_scenario.expected_reports) {
    DDS::ReturnCode_t rc;

    DDS::ConditionSeq active;
    const DDS::Duration_t infinity = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
    wait_set->wait(active, infinity);
    for (unsigned i = 0; i < active.length(); i++) {
      if (active[i] == guard_condition) {
        timeout_cv.notify_all();
        timeout_thread.join();
        std::stringstream ss;
        ss << "Timedout waiting for the scenario to complete";
        throw std::runtime_error(ss.str());
      }
    }

    ReportSeq reports;
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

    for (size_t r = 0; r < reports.length(); r++) {
      if (info[r].valid_data) {
        if (reports[r].failed) {
          std::stringstream ss;
          ss << "Worker " << reports[r].worker_id << " of node " << reports[r].node_id << " failed";
          throw std::runtime_error(ss.str());
        } else {
          WorkerReport report;
          std::stringstream ss;
          ss << reports[r].details << std::flush;
          if (json_2_idl(ss, report)) {
            parsed_reports.push_back(report);
          } else {
            std::stringstream ess;
            ess << "Error parsing report details from Worker " << reports[r].worker_id
              << " of node " << reports[r].node_id;
            throw std::runtime_error(ess.str());
          }
        }
        std::cout << "Got " << parsed_reports.size() << " out of "
          << allocated_scenario.expected_reports << " expected reports" << std::endl;
      }
    }
  }

  if (timeout.count() > 0) {
    timeout_cv.notify_all();
    timeout_thread.join();
  }

  wait_set->detach_condition(read_condition);
  dds_entities_.report_reader_impl_->delete_readcondition(read_condition);
  wait_set->detach_condition(guard_condition);

  return parsed_reports;
}
