#include "AllocationHelper.h"

#include <util.h>

#include <fstream>
#include <sstream>

using namespace Bench;
using namespace Bench::TestController;

void AllocationHelper::alloc_exclusive_node_configs(AllocatedScenario& allocated_scenario,
  std::set<NodeController::NodeId, OpenDDS::DCPS::GUID_tKeyLessThan>& exclusive_ncs,
  NodeControllerWeights& nc_weights,
  const std::vector<MatchedNodeControllers>& matched_ncs,
  const ScenarioPrototype& scenario_prototype,
  const std::map<std::string, std::string>& worker_configs)
{
  for (uint32_t i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    if (nodeproto.exclusive && nodeproto.count > 0) {
      MinNcQueue my_queue;
      for (auto nc_id : matched_ncs[i]) {
        my_queue.push(nc_weights[nc_id]);
      }

      for (uint32_t j = 0; j < nodeproto.count; ++j) {
        bool found = false;
        while (!my_queue.empty()) {
          const NcMetaData& top = my_queue.top();
          if (!top.exclusive_occupied) {
            nc_weights[top.id].exclusive_occupied = true;
            found = true;
            exclusive_ncs.insert(top.id);

            // Update the allocated scenario to add a config for this node controller
            CORBA::ULong old_len = allocated_scenario.configs.length();
            OpenDDS::DCPS::grow(allocated_scenario.configs);
            allocated_scenario.configs[old_len].node_id = top.id;
            allocated_scenario.configs[old_len].timeout = scenario_prototype.timeout;
            allocated_scenario.configs[old_len].spawned_processes.length(0);
            add_protoworkers_to_node(nodeproto.workers, worker_configs, allocated_scenario.configs[old_len],
              allocated_scenario.expected_process_reports, allocated_scenario.expected_worker_reports);

            my_queue.pop();
            break;
          }
          my_queue.pop();
        }
        if (!found) {
          std::string msg = "Could not find a node for exclusive node prototype with wildcard '" +
            std::string(nodeproto.name_wildcard.in()) + "'!";
          throw std::runtime_error(msg);
        }
      }
    }
  }
}

void AllocationHelper::alloc_nonexclusive_node_configs(AllocatedScenario& allocated_scenario,
  ConfigIndexMap& nonexclusive_ncs,
  const NodeControllerWeights& nc_weights,
  const std::vector<MatchedNodeControllers>& matched_ncs,
  const ScenarioPrototype& scenario_prototype,
  const std::map<std::string, std::string>& worker_configs)
{
  for (uint32_t i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    if (!nodeproto.exclusive && nodeproto.count > 0) {
      ConfigIndexMap my_ncs;
      for (uint32_t j = 0; j < matched_ncs[i].size(); ++j) {
        const NodeController::NodeId& id = matched_ncs[i][j];
        if (!nc_weights.at(id).exclusive_occupied) {
          if (nonexclusive_ncs.find(id) != nonexclusive_ncs.end()) {
            my_ncs.insert(std::make_pair(id, nonexclusive_ncs[id]));
          } else {
            unsigned next_idx = allocated_scenario.configs.length();
            OpenDDS::DCPS::grow(allocated_scenario.configs);
            allocated_scenario.configs[next_idx].node_id = id;
            allocated_scenario.configs[next_idx].timeout = scenario_prototype.timeout;
            allocated_scenario.configs[next_idx].spawned_processes.length(0);
            my_ncs.insert(std::make_pair(id, next_idx));
            nonexclusive_ncs.insert(std::make_pair(id, next_idx));
          }
          // Stop when each node has an allocated node controller
          if (my_ncs.size() >= nodeproto.count) break;
        }
      }

      // At least 1 node controller is needed if there are any nodes to be allocated
      if (my_ncs.empty()) {
        std::string msg = "No node left for non-exclusive node prototype with wildcard '" +
          std::string(nodeproto.name_wildcard.in()) + "'!";
        throw std::runtime_error(msg);
      }

      ConfigIndexMap::const_iterator it = my_ncs.begin();
      for (uint32_t j = 0; j < nodeproto.count; ++j) {
        add_protoworkers_to_node(nodeproto.workers, worker_configs, allocated_scenario.configs[it->second],
          allocated_scenario.expected_process_reports, allocated_scenario.expected_worker_reports);
        if (++it == my_ncs.end()) {
          it = my_ncs.begin();
        }
      }
    }
  }
}

void AllocationHelper::alloc_anynode_workers(AllocatedScenario& allocated_scenario,
  const std::vector<NodeController::NodeId>& any_ncs,
  const ConfigIndexMap& nonexclusive_ncs,
  const ScenarioPrototype& scenario_prototype,
  const std::map<std::string, std::string>& worker_configs)
{
  // Finally, allocate any_node workers
  uint32_t worker_count = 0;
  for (uint32_t i = 0; i < scenario_prototype.any_node.length(); ++i) {
    worker_count += scenario_prototype.any_node[i].count;
  }

  if (!any_ncs.empty()) { // Assign to the empty node controllers
    uint32_t node_count = std::min(static_cast<uint32_t>(any_ncs.size()), worker_count);
    uint32_t start_id = allocated_scenario.configs.length();
    allocated_scenario.configs.length(start_id + node_count);
    for (uint32_t i = 0; i < node_count; ++i) {
      allocated_scenario.configs[start_id + i].node_id = any_ncs[i];
      allocated_scenario.configs[start_id + i].timeout = scenario_prototype.timeout;
      allocated_scenario.configs[start_id + i].spawned_processes.length(0);
    }

    uint32_t running_id = start_id;
    for (uint32_t i = 0; i < scenario_prototype.any_node.length(); ++i) {
      const WorkerPrototype& workerproto = scenario_prototype.any_node[i];
      if (!workerproto.no_report) {
        allocated_scenario.expected_worker_reports += workerproto.count;
      }
      allocated_scenario.expected_process_reports += workerproto.count;
      for (uint32_t j = 0; j < workerproto.count; ++j) {
        add_single_worker_to_node(workerproto, worker_configs,
          allocated_scenario.configs[running_id++]);
        if (running_id >= start_id + node_count) {
          running_id = start_id;
        }
      }
    }
  } else if (!nonexclusive_ncs.empty()) { // Assign to the nonexlusive node controllers
    ConfigIndexMap::const_iterator it = nonexclusive_ncs.begin();
    for (uint32_t i = 0; i < scenario_prototype.any_node.length(); ++i) {
      const WorkerPrototype& workerproto = scenario_prototype.any_node[i];
      if (!workerproto.no_report) {
        allocated_scenario.expected_worker_reports += workerproto.count;
      }
      allocated_scenario.expected_process_reports += workerproto.count;
      for (uint32_t j = 0; j < workerproto.count; ++j) {
        add_single_worker_to_node(workerproto, worker_configs,
          allocated_scenario.configs[it->second]);
        if (++it == nonexclusive_ncs.end()) {
          it = nonexclusive_ncs.begin();
        }
      }
    }
  } else if (worker_count > 0) {
    throw std::runtime_error("No node left for any_node workers!");
  }
}

std::string AllocationHelper::read_file(const std::string& name)
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

void AllocationHelper::read_protoworker_configs(const std::string& test_context,
  const WorkerPrototypes& protoworkers,
  std::map<std::string, std::string>& worker_configs,
  bool debug_alloc)
{
  for (unsigned i = 0; i < protoworkers.length(); i++) {
    const std::string filename = protoworkers[i].config.in();
    if (!filename.empty() && !worker_configs.count(filename)) {
      worker_configs[filename] = debug_alloc ?
        filename : read_file(join_path(test_context, "config", "worker", filename));
    }
  }
}

void AllocationHelper::add_single_worker_to_node(const WorkerPrototype& protoworker,
  const std::map<std::string, std::string>& worker_configs,
  NodeController::Config& node)
{
  unsigned worker_i = OpenDDS::DCPS::grow(node.spawned_processes) - 1;
  NodeController::SpawnedProcessId spawned_process_id = worker_i ? (node.spawned_processes[worker_i - 1].spawned_process_id + node.spawned_processes[worker_i - 1].count) : 1;
  auto& process = node.spawned_processes[worker_i];
  process.executable = protoworker.executable.in();
  process.command = protoworker.command.in();
  auto config_iter = worker_configs.find(protoworker.config.in());
  if (config_iter != worker_configs.end()) {
    process.config = config_iter->second.c_str();
  }
  process.count = 1;
  process.spawned_process_id = spawned_process_id;
  process.expect_worker_report = !protoworker.no_report;
  process.ignore_errors = protoworker.ignore_errors;
}

void AllocationHelper::add_protoworkers_to_node(const WorkerPrototypes& protoworkers,
  const std::map<std::string, std::string>& worker_configs,
  NodeController::Config& node, unsigned& expected_process_reports, unsigned& expected_worker_reports)
{
  for (unsigned i = 0; i < protoworkers.length(); i++) {
    auto& protoworker = protoworkers[i];

    if (!protoworker.no_report) {
      expected_worker_reports += protoworkers[i].count;
    }
    expected_process_reports += protoworkers[i].count;
  }

  unsigned worker_i = node.spawned_processes.length();
  NodeController::SpawnedProcessId spawned_process_id = worker_i ? (node.spawned_processes[worker_i - 1].spawned_process_id + node.spawned_processes[worker_i - 1].count) : 1;
  node.spawned_processes.length(node.spawned_processes.length() + protoworkers.length());
  for (unsigned i = 0; i < protoworkers.length(); i++) {
    auto& protoworker = protoworkers[i];
    auto& process = node.spawned_processes[worker_i];
    process.executable = protoworker.executable.in();
    process.expect_worker_report = !protoworker.no_report;
    process.ignore_errors = protoworker.ignore_errors;
    process.command = protoworker.command.in();
    auto config_iter = worker_configs.find(protoworker.config.in());
    if (config_iter != worker_configs.end()) {
      process.config = config_iter->second.c_str();
    }
    process.count = protoworker.count;
    process.spawned_process_id = spawned_process_id;
    spawned_process_id += protoworker.count;
    worker_i++;
  }
}
