#ifndef ALLOCATION_HELPER_HEADER
#define ALLOCATION_HELPER_HEADER

#include "BenchTypeSupportImpl.h"

#include <dds/DCPS/GuidUtils.h>

#include <queue>
#include <set>
#include <vector>
#include <map>

class AllocationHelper {
private:
  // Internal meta data used to compute allocation of nodes
  struct NcMetaData {
    NcMetaData() : id{},
                   max_prob{0.0},
                   overlap_count{0},
                   exclusive_occupied{false} {}

    Bench::NodeController::NodeId id;

    // Maximum probabilty that it is used for an exclusive node
    double max_prob;

    // Number of node prototypes that can assign a node to it
    unsigned overlap_count;

    // Whether it is occupied by an exclusive node
    bool exclusive_occupied;

    // Node controllers that are less likely to be used by some other node prototypes
    // are considered first. This is done by taking the ones with smaller max_prob values.
    // If there are multiple node controllers with equal max_prob, the ones with smaller
    // overlap_count are considered first.
    bool operator()(const NcMetaData& a, const NcMetaData& b) const
    {
      return (a.max_prob > b.max_prob) ||
        (a.max_prob == b.max_prob && a.overlap_count > b.overlap_count);
    }
  };

public:
  typedef std::vector<Bench::NodeController::NodeId> MatchedNodeControllers;

  // Each of the matched node controllers has an entry in this map
  typedef std::map<Bench::NodeController::NodeId, NcMetaData, OpenDDS::DCPS::GUID_tKeyLessThan> NodeControllerWeights;

  typedef std::priority_queue<NcMetaData, std::vector<NcMetaData>, NcMetaData> MinNcQueue;

  // Map from node controller to its index in the allocated scenario
  typedef std::map<Bench::NodeController::NodeId, uint32_t, OpenDDS::DCPS::GUID_tKeyLessThan> ConfigIndexMap;

  static void alloc_exclusive_node_configs(Bench::TestController::AllocatedScenario& allocated_scenario,
    std::set<Bench::NodeController::NodeId, OpenDDS::DCPS::GUID_tKeyLessThan>& exclusive_ncs,
    NodeControllerWeights& nc_weights,
    const std::vector<MatchedNodeControllers>& matched_ncs,
    const Bench::TestController::ScenarioPrototype& scenario_prototype,
    const std::map<std::string, std::string>& worker_configs);

  static void alloc_nonexclusive_node_configs(Bench::TestController::AllocatedScenario& allocated_scenario,
    ConfigIndexMap& nonexclusive_ncs,
    const NodeControllerWeights& nc_weights,
    const std::vector<MatchedNodeControllers>& matched_ncs,
    const Bench::TestController::ScenarioPrototype& scenario_prototype,
    const std::map<std::string, std::string>& worker_configs);

  static void alloc_anynode_workers(Bench::TestController::AllocatedScenario& allocated_scenario,
    const std::vector<Bench::NodeController::NodeId>& any_ncs,
    const ConfigIndexMap& nonexclusive_ncs,
    const Bench::TestController::ScenarioPrototype& scenario_prototype,
    const std::map<std::string, std::string>& worker_configs);

  static void read_protoworker_configs(const std::string& test_context,
    const Bench::TestController::WorkerPrototypes& protoworkers,
    std::map<std::string, std::string>& worker_configs,
    bool debug_alloc);

private:
  static std::string read_file(const std::string& name);

  static void add_single_worker_to_node(const Bench::TestController::WorkerPrototype& protoworker,
    const std::map<std::string, std::string>& worker_configs,
    Bench::NodeController::Config& node);

  static void add_protoworkers_to_node(const Bench::TestController::WorkerPrototypes& protoworkers,
    const std::map<std::string, std::string>& worker_configs, Bench::NodeController::Config& node,
    unsigned& expected_process_reports, unsigned& expected_worker_reports);
};

#endif
