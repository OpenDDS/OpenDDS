#ifndef SCENARIO_MANAGER_HEADER
#define SCENARIO_MANAGER_HEADER

#include <string>
#include <map>
#include <vector>

#include "ScenarioOverrides.h"
#include "DdsEntities.h"

/**
 * ScenarioManager is responsible for organizing nodes and planning and
 * carrying out the scenario.
 */
class ScenarioManager {
public:
  ScenarioManager(
    const std::string& bench_root,
    const std::string& test_context,
    const ScenarioOverrides& overrides,
    DdsEntities& dds_entities);

  ~ScenarioManager();

  /**
   * Sleep for the number of seconds passed and return the nodes that have said
   * they are available.
   */
  Nodes discover_nodes(unsigned wait_for_nodes);

  /**
   * Given a Scenario and nodes, plan what workers to allocate to the nodes
   * according to the scenario and return the allocation plan.
   */
  Bench::TestController::AllocatedScenario allocate_scenario(
    const Bench::TestController::ScenarioPrototype& scenario_prototype,
    Nodes& available_nodes, bool debug_alloc);

  /**
   * Execute a Scenario by sending out the node configurations and wait for the
   * worker reports to come back.
   */
  void execute(const Bench::TestController::AllocatedScenario& allocated_scenario, std::vector<Bench::WorkerReport>& worker_reports, Bench::NodeController::ReportSeq& nc_reports);

private:
  void customize_configs(std::map<std::string, std::string>& worker_configs);

  const std::string bench_root_;
  const std::string test_context_;
  ScenarioOverrides overrides_;
  DdsEntities& dds_entities_;
};

#endif
