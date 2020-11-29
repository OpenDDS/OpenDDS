#include "ScenarioManager.h"
#include "AllocationHelper.h"

#include <util.h>
#include <json_conversion.h>

#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/GuardCondition.h>
#include <dds/DCPS/DCPS_Utils.h>

#include <ace/OS_NS_unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

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

void ScenarioManager::customize_configs(std::map<std::string, std::string>& worker_configs)
{
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

bool ScenarioManager::is_matched(const std::string& str, const std::string& pat) const
{
  if (pat.empty() ||
      (!OpenDDS::DCPS::is_wildcard(pat.c_str()) && pat == str) ||
      (OpenDDS::DCPS::is_wildcard(pat.c_str()) && ACE::wild_match(str.c_str(), pat.c_str(), true, true))) {
    return true;
  }
  return false;
}

AllocatedScenario ScenarioManager::allocate_scenario(const ScenarioPrototype& scenario_prototype,
  const Nodes& available_nodes, bool debug_alloc)
{
  if (scenario_prototype.nodes.length() == 0 && scenario_prototype.any_node.length() == 0) {
    throw std::runtime_error("Scenario Prototype is empty!");
  }

  // Get a set of matched node controllers for each node prototype
  std::vector<AllocationHelper::MatchedNodeControllers> matched_ncs(scenario_prototype.nodes.length());
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    for (Nodes::const_iterator it = available_nodes.begin(); it != available_nodes.end(); ++it) {
      if (is_matched(it->second.name.in(), nodeproto.name_wildcard.in())) {
        matched_ncs[i].push_back(it->first);
      }
    }
  }

  AllocationHelper::NodeControllerWeights nc_weights;
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); ++i) {
    const NodePrototype& nodeproto = scenario_prototype.nodes[i];
    // Probability that a matched node controller is allocated to a node in this prototype
    double my_prob = 0.0;
    if (nodeproto.exclusive && nodeproto.count > 0) {
      if (nodeproto.count > matched_ncs[i].size()) {
        std::string msg = "Not enough nodes for node prototype with wildcard '" +
          std::string(nodeproto.name_wildcard.in()) + "'!";
        throw std::runtime_error(msg);
      }
      my_prob = static_cast<double>(nodeproto.count)/matched_ncs[i].size();
    }

    // Update the weight for each of the matched node controller
    for (unsigned j = 0; j < matched_ncs[i].size(); ++j) {
      const NodeController::NodeId& nc_id = matched_ncs[i][j];
      if (nc_weights.find(nc_id) != nc_weights.end()) {
        nc_weights[nc_id].max_prob = std::max(nc_weights[nc_id].max_prob, my_prob);
        ++nc_weights[nc_id].overlap_count;
      } else {
        nc_weights[nc_id].id = nc_id;
        nc_weights[nc_id].max_prob = my_prob;
        nc_weights[nc_id].overlap_count = 1;
      }
    }
  }

  // Read in worker config files
  std::map<std::string, std::string> worker_configs;
  for (unsigned i = 0; i < scenario_prototype.nodes.length(); ++i) {
    AllocationHelper::read_protoworker_configs(test_context_, scenario_prototype.nodes[i].workers,
      worker_configs, debug_alloc);
  }
  AllocationHelper::read_protoworker_configs(test_context_, scenario_prototype.any_node,
    worker_configs, debug_alloc);

  AllocatedScenario allocated_scenario;
  allocated_scenario.expected_reports = 0;
  allocated_scenario.timeout = scenario_prototype.timeout;
  allocated_scenario.configs.length(0);

  // Node controllers allocated to exclusive nodes
  std::set<NodeController::NodeId, OpenDDS::DCPS::GUID_tKeyLessThan> exclusive_ncs;

  // Allocate exclusive node configs first
  AllocationHelper::alloc_exclusive_node_configs(allocated_scenario, exclusive_ncs,
                                                 nc_weights, matched_ncs,
                                                 scenario_prototype, worker_configs);

  /*
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
  */

  // For node controllers that will be used for non-exclusive nodes
  AllocationHelper::ConfigIndexMap nonexclusive_ncs;

  // Then allocate non-exclusive node configs
  AllocationHelper::alloc_nonexclusive_node_configs(allocated_scenario, nonexclusive_ncs,
                                                    nc_weights, matched_ncs,
                                                    scenario_prototype, worker_configs);

  /*
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
  */

  // Node controllers that haven't been used at all
  std::vector<NodeController::NodeId> any_ncs;

  for (Nodes::const_iterator it = available_nodes.begin(); it != available_nodes.end(); ++it) {
    const NodeController::NodeId& nid = it->first;
    if ((nc_weights.find(nid) == nc_weights.end()) ||
        (exclusive_ncs.find(nid) == exclusive_ncs.end() &&
         nonexclusive_ncs.find(nid) == nonexclusive_ncs.end())) {
      any_ncs.push_back(nid);
    }
  }

  // Finally, allocate any_node workers
  AllocationHelper::alloc_anynode_workers(allocated_scenario, any_ncs, nonexclusive_ncs,
                                          scenario_prototype, worker_configs);

  /*
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
  */

  char host[256];
  ACE_OS::hostname(host, sizeof(host));
  pid_t pid = ACE_OS::getpid();
  std::stringstream ss;
  ss << host << "_" << pid << std::flush;
  allocated_scenario.scenario_id = ss.str().c_str();
  allocated_scenario.launch_time = Builder::ZERO;

  return allocated_scenario;
}

void ScenarioManager::execute(const Bench::TestController::AllocatedScenario& allocated_scenario,
  Bench::TestController::Report& report)
{
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
  size_t parsed_report_count = 0;
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
        report.node_reports.length(report.node_reports.length() + 1);
        Bench::TestController::NodeReport& node_report = report.node_reports[report.node_reports.length() - 1];
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
              node_report.worker_reports.length(node_report.worker_reports.length() + 1);
              node_report.worker_reports[node_report.worker_reports.length() - 1] = report;
              ++parsed_report_count;
            } else {
              ++parse_failures;
              std::stringstream ess;
              ess << "Error parsing report details from Worker " << worker_reports[wr].worker_id
                << " of node " << reports[r].node_id;
              std::cerr << ess.str() + "\n" << std::flush;
            }
          }
          std::stringstream ss;
          ss << "Got " << parsed_report_count << " out of "
            << allocated_scenario.expected_reports << " expected reports";
          if (worker_failures != 0 || parse_failures != 0) {
            ss << " (with " << worker_failures << " worker failures and "
              << parse_failures << " parse failures)";
          }
          ss << std::endl;
          std::cerr << ss.str() << std::flush;
        }
        node_report.node_id = reports[r].node_id;
        node_report.properties = reports[r].properties;
      }
    }
    {
      std::lock_guard<std::mutex> guard(reports_left_mutex);
      reports_left = static_cast<size_t>(allocated_scenario.expected_reports) - parsed_report_count - worker_failures - parse_failures;
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
}
