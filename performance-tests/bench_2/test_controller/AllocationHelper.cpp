#include "AllocationHelper.h"

void AllocationHelper::alloc_exclusive_node_configs()
{
  // Node controllers allocated to exclusive nodes
  std::set<NodeController::NodeId, OpenDDS::DCPS::GUID_tKeyLessThan> exclusive_ncs;

  // Allocate exclusive node configs first
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    if (nodeproto.exclusive && nodeproto.count > 0) {
      MinNcQueue my_queue;
      for (auto nc_id : matched_ncs[i]) {
        my_queue.push(nc_weights[nc_id]);
      }

      for (unsigned j = 0; j < nodeproto.count; ++j) {
        bool found = false;
        while (!my_queue.empty()) {
          const NcMetaData& top = my_queue.top();
          if (!top.exclusive_occupied) {
            nc_weights[top.id].exclusive_occupied = true;
            found = true;
            exclusive_ncs.insert(top.id);

            // Update the allocated scenario to add a config for this node controller
            CORBA::ULong old_len = allocated_scenario.configs.length();
            allocated_scenario.configs.length(old_len + 1);
            allocated_scenario.configs[old_len].node_id = top.id;
            allocated_scenario.configs[old_len].timeout = scenario_prototype.timeout;
            allocated_scenario.configs[old_len].workers.length(0);
            allocated_scenario.expected_reports += add_protoworkers_to_node(
              nodeproto.workers, worker_configs, allocated_scenario.configs[old_len]);

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

void AllocationHelper::alloc_nonexclusive_node_configs()
{
  // Map from node controller to its index in the allocated scenario
  typedef std::map<NodeController::NodeId, unsigned, OpenDDS::DCPS::GUID_tKeyLessThan> ConfigIndexMap;
  // For node controllers that will be used for non-exclusive nodes
  ConfigIndexMap nonexclusive_ncs;

  // Then allocate non-exclusive node configs
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    if (!nodeproto.exclusive && nodeproto.count > 0) {
      ConfigIndexMap my_ncs;
      for (unsigned j = 0; j < matched_ncs[i].size(); ++j) {
        const NodeController::NodeId& id = matched_ncs[i][j];
        if (!nc_weights[id].exclusive_occupied) {
          if (nonexclusive_ncs.find(id) != nonexclusive_ncs.end()) {
            my_ncs.insert(std::make_pair(id, nonexclusive_ncs[id]));
          } else {
            unsigned next_idx = allocated_scenario.configs.length();
            allocated_scenario.configs.length(next_idx + 1);
            allocated_scenario.configs[next_idx].node_id = id;
            allocated_scenario.configs[next_idx].timeout = scenario_prototype.timeout;
            allocated_scenario.configs[next_idx].workers.length(0);
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
      for (unsigned j = 0; j < nodeproto.count; ++j) {
        allocated_scenario.expected_reports += add_protoworkers_to_node(
          nodeproto.workers, worker_configs, allocated_scenario.configs[it->second]);
        if (++it == my_ncs.end()) {
          it = my_ncs.begin();
        }
      }
    }
  }
}

void AllocationHelper::alloc_anynode_workers()
{
  // Finally, allocate any_node workers
  unsigned worker_count = 0;
  for (unsigned i = 0; i < scenario_prototype.any_node.length(); ++i) {
    worker_count += scenario_prototype.any_node[i].count;
  }

  if (!any_ncs.empty()) { // Assign to the empty node controllers
    unsigned node_count = std::min(static_cast<unsigned>(any_ncs.size()), worker_count);
    unsigned start_id = allocated_scenario.configs.length();
    allocated_scenario.configs.length(start_id + node_count);
    for (unsigned i = 0; i < node_count; ++i) {
      allocated_scenario.configs[start_id + i].node_id = any_ncs[i];
      allocated_scenario.configs[start_id + i].timeout = scenario_prototype.timeout;
      allocated_scenario.configs[start_id + i].workers.length(0);
    }

    unsigned running_id = start_id;
    for (unsigned i = 0; i < scenario_prototype.any_node.length(); ++i) {
      const WorkerPrototype& workerproto = scenario_prototype.any_node[i];
      allocated_scenario.expected_reports += workerproto.count;
      for (unsigned j = 0; j < workerproto.count; ++j) {
        add_single_worker_to_node(workerproto, worker_configs,
          allocated_scenario.configs[running_id++]);
        if (running_id >= start_id + node_count) {
          running_id = start_id;
        }
      }
    }
  } else if (!nonexclusive_ncs.empty()) { // Assign to the nonexlusive node controllers
    ConfigIndexMap::const_iterator it = nonexclusive_ncs.begin();
    for (unsigned i = 0; i < scenario_prototype.any_node.length(); ++i) {
      const WorkerPrototype& workerproto = scenario_prototype.any_node[i];
      allocated_scenario.expected_reports += workerproto.count;
      for (unsigned j = 0; j < workerproto.count; ++j) {
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
    if (!worker_configs.count(filename)) {
      worker_configs[filename] = debug_alloc ?
        filename : read_file(join_path(test_context, "config", "worker", filename));
    }
  }
}

void AllocationHelper::add_single_worker_to_node(const WorkerPrototype& protoworker,
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

unsigned add_protoworkers_to_node(const WorkerPrototypes& protoworkers,
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
