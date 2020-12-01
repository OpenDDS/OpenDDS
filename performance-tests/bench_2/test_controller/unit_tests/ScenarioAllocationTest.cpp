#include "../ScenarioManager.h"

#include <gtest/gtest.h>

class AllocationTest : public ::testing::Test {
protected:
  void SetUp() override
  {
    DdsEntities dds_entities(OpenDDS::DCPS::TheParticipantFactory, 89);
    scenario_manager_ = ScenarioManager("", "", ScenarioOverrides(), dds_entities);

    // Node prototypes
    scenario_proto_.nodes.length(3);
    scenario_proto_.nodes[0].name_wildcard = ""; // Match all
    scenario_proto_.nodes[0].count = 3;
    scenario_proto_.nodes[0].exclusive = false;
    scenario_proto_.nodes[0].workers.length(2);
    scenario_proto_.nodes[0].workers[0].config = "ci_disco.json";
    scenario_proto_.nodes[0].workers[0].count = 2;
    scenario_proto_.nodes[0].workers[1] = scenario_proto.nodes[0].workers[0];

    scenario_proto_.nodes[1].name_wildcard = "";
    scenario_proto_.nodes[1].count = 2;
    scenario_proto_.nodes[1].exclusive = true;
    scenario_proto_.nodes[1].workers = scenario_proto.nodes[0].workers;

    scenario_proto_.nodes[2].name_wildcard = "";
    scenario_proto_.nodes[2].count = 2;
    scenario_proto_.nodes[2].exclusive = false;
    scenario_proto_.nodes[2].workers = scenario_proto.nodes[0].workers;

    // Any node workers
    scenario_proto_.any_node.config = "ci_mixed_worker.json";
    scenario_proto_.any_node.count = 5;

    // Node controllers
    Bench::NodeController::NodeId id = { { 0 }, { { 0 }, 1 } };
    Bench::NodeController::Status status = { id, Bench::NodeController::AVAILABLE, "test_nc_1" };
    avail_nodes_[id] = status;

    id = { { 0 }, { { 0 }, 2 } };
    status.id = id;
    status.name = "test_nc_2";
    avail_nodes_[id] = status;

    id = { { 0 }, { { 0 }, 3 } };
    status.id = id;
    status.name = "test_nc_3";
    avail_nodes_[id] = status;
  }

  void AddNodeControllers()
  {
    // Add 2 more node controllers (for a total of 5 node controllers)
    Bench::NodeController::NodeId id = { { 0 }, { { 0 }, 4 } };
    Bench::NodeController::Status status = {id, Bench::NodeController::AVAILABLE, "test_nc_4" };
    avail_nodes_[id] = status;

    id = { { 0 }, { { 0 }, 5 } };
    status.id = id;
    status.name = "test_nc_5";
    avail_nodes_[id] = status;
  }

  Bench::TestController::ScenarioPrototype scenario_proto_;
  Nodes avail_nodes_;
  ScenarioManager scenario_manager_;
};

TEST_F(AllocationTest, Success)
{
  // 2 exclusive nodes and 1 non-exclusive node
  EXPECT_NO_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true));

  // Adding more node controllers still works
  AddNodeControllers();
  EXPECT_NO_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true));

  // Exclusive and non-exclusive node configs match distinct sets of node controllers
  scenario_proto_.nodes[0].name_wildcard = "nonexclusive_nc*";
  scenario_proto_.nodes[1].name_wildcard = "config1_exclusive_nc*";
  scenario_proto_.nodes[2].exclusive = true;
  scenario_proto_.nodes[2].name_wildcard = "*exclusive_nc*";
  Nodes::iterator it = avail_nodes_.begin();
  it->second.name = "config1_exclusive_nc1"; ++it; // Match config 1
  it->second.name = "config1_exclusive_nc2"; ++it; // Match config 1
  it->second.name = "common_exclusive_nc3"; ++it;  // Match config 2
  it->second.name = "common_exclusive_nc4"; ++it;  // Match config 2
  it->second.name = "nonexclusive_nc5"; // Match configs 0, 2
  EXPECT_NO_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true));
}

TEST_F(AllocationTest, ExclusiveAllocSimpleFailure)
{
  scenario_proto_.nodes[1].name_wildcard = "exclusive_nc*";
  Nodes::iterator it = avail_nodes_.begin();
  it->second.name = "exclusive_nc1";
  EXPECT_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true),
               std::runtime_error);
}

TEST_F(AllocationTest, ExclusiveAllocOverlapFailure)
{
  AddNodeControllers();
  scenario_proto_.nodes[0].exclusive = true;
  scenario_proto_.nodes[0].name_wildcard = "config0_*";
  scenario_proto_.nodes[1].name_wildcard = "*config1_nc*";
  Nodes::iterator it = avail_nodes_.begin();
  it->second.name = "config0_exclusive_nc1"; ++it; // Match config 0
  it->second.name = "config0_exclusive_nc2"; ++it; // Match config 0
  it->second.name = "config0_config1_nc3"; ++it; // Match configs 0, 1
  it->second.name = "config1_nc4"; ++it;         // Match config 1
  EXPECT_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true),
               std::runtime_error);
}

TEST_F(AllocationTest, NonexclusiveAllocFailure)
{
  scenario_proto_.nodes[0].exclusive = true;
  scenario_proto_.nodes[1].exclusive =  false;
  EXPECT_THROW(scenario_manager_.allocate_scenario(proto_, avail_nodes_, true),
               std::runtime_error);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
