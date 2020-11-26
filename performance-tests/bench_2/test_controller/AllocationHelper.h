#ifndef ALLOCATION_HELPER_HEADER
#define ALLOCATION_HELPER_HEADER

#include <queue>
#include <set>

class AllocationHelper {
public:
  void alloc_exclusive_node_configs();
  void alloc_nonexclusive_node_configs();
  void alloc_anynode_workers();

private:
  std::string read_file(const std::string& name);

  void read_protoworker_configs(const std::string& test_context,
    const WorkerPrototypes& protoworkers,
    std::map<std::string, std::string>& worker_configs,
    bool debug_alloc);

  void add_single_worker_to_node(const WorkerPrototype& protoworker,
    const std::map<std::string, std::string>& worker_configs,
    NodeController::Config& node);

  unsigned add_protoworkers_to_node(const WorkerPrototypes& protoworkers,
    const std::map<std::string, std::string>& worker_configs,
    NodeController::Config& node);

  struct NcMetaData {
    NcMetaData() : max_prob{0.0},
                   overlap_count{0},
                   exclusive_occupied{false} {}

    NodeController::NodeId id;

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
  typedef std::priority_queue<NcMetaData, std::vector<NcMetaData>, NcMetaData> MinNcQueue;
};

#endif
