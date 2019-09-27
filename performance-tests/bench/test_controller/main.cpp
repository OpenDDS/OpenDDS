#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include "BenchTypeSupportImpl.h"
#include "PropertyStatBlock.h"
#pragma GCC diagnostic pop

#include <util.h>
#include <json_conversion.h>

#include "ScenarioManager.h"

using namespace Bench;
using namespace Bench::NodeController;
using namespace Bench::TestController;

std::string bench_root;

void handle_reports(std::vector<Bench::WorkerReport> parsed_reports);

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

  /*
   * Parse Arguments
   */

  /// Process Any OpenDDS Arguments
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

  /// DDS Domain
  int domain = default_control_domain;
  /// How much time for the node discovery period.
  unsigned wait_for_nodes = 10;
  /// Max time to wait inbetween reports comming in.
  unsigned wait_for_reports = 120;

  /// If not empty, Stop After Allocation and Output the Allocation to File
  std::string preallocated_scenario_output_path;
  /// Pretty Print the Preallocated Scenario JSON
  bool pretty = false;
  /// If not empty, Don't Discover Nodes, Send Out an Existing Set of Node Configs
  std::string preallocated_scenario_input_path;

  /// Location of Test Artifacts and Configurations
  std::string test_context_path;

  /// Name of scenario config file without the json ext
  std::string scenario_id;

  const char* usage = "usage: test_controller [-h|--help] | TEST_CONTEXT SCENARIO_ID [OPTIONS...]";
  try {
    for (int i = 1; i < argc; i++) {
      const char* argument = ACE_TEXT_ALWAYS_CHAR(argv[i]);
      if (!ACE_OS::strcmp(argument, "--domain")) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--wait-for-nodes")) {
        wait_for_nodes = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--wait-for-reports")) {
        wait_for_reports = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--help") || !ACE_OS::strcmp(argument, "-h")) {
        std::cout << usage << std::endl
          << std::endl
//            ################################################################################
          << "TEST_CONTEXT is the path to the directory of the test configurations and" << std::endl
          << "artifacts." << std::endl
          << "SCENARIO_ID is the name of the scenario file in the test context to use without" << std::endl
          << "the `.json` extension." << std::endl
          << std::endl
          << "OPTIONS:" << std::endl
          << "--domain N                   The DDS Domain to use. The default is 89." << std::endl
          << "--wait-for-nodes N           The number of seconds to wait for nodes before" << std::endl
          << "                             broadcasting the scenario to them. The default is" << std::endl
          << "                             10 seconds." << std::endl
          << "--wait-for-reports N         The number of seconds to wait for a report to come" << std::endl
          << "                             in before timing out. The default is 120 seconds." << std::endl
          << "--prealloc-scenario-out PATH Instead of running the scenario, write the" << std::endl
          << "                             directives (in JSON) that would have been sent to" << std::endl
          << "                             the node controllers to the file at this path." << std::endl
          << "--pretty                     Write the JSON output of `--prealloc-scenario-out`" << std::endl
          << "                             with indentation." << std::endl
          << "--prealloc-scenario-in PATH  Take result of --prealloc-scneario-out and use that" << std::endl
          << "                             to run the scenario instead of discovering nodes." << std::endl
          << "                             This might fail if the nodes go offline after the" << std::endl
          << "                             preallocated scenario is saved." << std::endl;
//            ################################################################################
        return 0;
      } else if (!ACE_OS::strcmp(argument, "--prealloc-scenario-out")) {
        preallocated_scenario_output_path = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--prealloc-scenario-in")) {
        preallocated_scenario_input_path = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, "--pretty")) {
        pretty = true;
      } else if (test_context_path.empty() && argument[0] != '-') {
        test_context_path = argument;
      } else if (scenario_id.empty() && argument[0] != '-') {
        scenario_id = argument;
      } else {
        std::cerr << "Invalid Option: " << argument << std::endl;
        throw 1;
      }
    }

    if (test_context_path.empty()) {
      std::cerr << "The path to the text context is required!" << std::endl;
      throw 1;
    }
    if (scenario_id.empty()) {
      std::cerr << "The id of the scenario to run is required!" << std::endl;
      throw 1;
    }
  } catch(int value) {
    std::cerr << usage << std::endl
      << "Use -h or --help to see the full help message" << std::endl;
    return value;
  }

  // Try To Load Scenerio
  std::string scenario_path = join_path(test_context_path, "config", "scenario", scenario_id + ".json");
  ScenarioPrototype scenario_prototype;
  std::ifstream scenario_file(scenario_path);
  if (scenario_file.is_open()) {
    if (!json_2_idl(scenario_file, scenario_prototype)) {
      std::cerr << "Could not parse " << scenario_path << std::endl;
      return 1;
    }
  } else {
    std::cerr << "Could not open " << scenario_path << std::endl;
    return 1;
  }

  // Print Scenerio File, Name, and Description
  {
    std::cout << "Using " << scenario_path;
    const std::string name = scenario_prototype.name.in();
    const std::string desc = scenario_prototype.desc.in();
    if (!name.empty() || !desc.empty()) {
      std::cout << ':';
      if (!name.empty()) {
        std::cout << std::endl << "    Name: " << name;
      }
      if (!desc.empty()) {
        std::cout << std::endl << "    Description: " << desc;
      }
    }
    std::cout << std::endl;
  }

  // Run Scenerio
  try {
    DdsEntities dds_entities(dpf, domain);
    ScenarioManager scenario_manager(bench_root, test_context_path, dds_entities);

    AllocatedScenario allocated_scenario;
    if (preallocated_scenario_input_path.size()) {
      std::cout << "Loading Scenario Allcoation from File..." << std::endl;
      std::ifstream file(preallocated_scenario_input_path);
      if (file.is_open()) {
        if (!json_2_idl(file, allocated_scenario)) {
          throw std::runtime_error("Could not decode allocated scenario");
        }
      } else {
        throw std::runtime_error("Could not open file for reading allocated scenario");
      }
    } else {
      std::cout << "Waiting for nodes for " << wait_for_nodes << " seconds..." << std::endl;
      Nodes available_nodes = scenario_manager.discover_nodes(wait_for_nodes);
      std::cout << "Discovered " << available_nodes.size() << " nodes" << std::endl;
      allocated_scenario = scenario_manager.allocate_scenario(scenario_prototype, available_nodes);
    }

    if (preallocated_scenario_output_path.size()) {
      std::cout << "Saving Scenario Allcoation to File..." << std::endl;
      std::ofstream file(preallocated_scenario_output_path);
      if (file.is_open()) {
        if (!idl_2_json(allocated_scenario, file, pretty)) {
          throw std::runtime_error("Could not encode allocated scenario");
        }
      } else {
        throw std::runtime_error("Could not open file for writing allocated scenario");
      }
    } else {
      std::cout << "Running the Scenario..." << std::endl;
      std::vector<Bench::WorkerReport> reports = scenario_manager.execute(allocated_scenario, wait_for_reports);
      handle_reports(reports);
    }
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  std::cout << "Finished" << std::endl;

  return 0;
};

void handle_reports(std::vector<Bench::WorkerReport> parsed_reports)
{
  using Builder::ZERO;

  Builder::TimeStamp max_construction_time = ZERO;
  Builder::TimeStamp max_enable_time = ZERO;
  Builder::TimeStamp max_start_time = ZERO;
  Builder::TimeStamp max_stop_time = ZERO;
  Builder::TimeStamp max_destruction_time = ZERO;

  size_t total_undermatched_readers = 0;
  size_t total_undermatched_writers = 0;
  Builder::TimeStamp max_discovery_time_delta = ZERO;

  Bench::SimpleStatBlock consolidated_latency_stats;
  Bench::SimpleStatBlock consolidated_jitter_stats;
  Bench::SimpleStatBlock consolidated_round_trip_latency_stats;
  Bench::SimpleStatBlock consolidated_round_trip_jitter_stats;

  for (size_t r = 0; r < parsed_reports.size(); ++r) {

    const Bench::WorkerReport& worker_report = parsed_reports[r];

    max_construction_time = std::max(max_construction_time, worker_report.construction_time);
    max_enable_time = std::max(max_enable_time, worker_report.enable_time);
    max_start_time = std::max(max_start_time, worker_report.start_time);
    max_stop_time = std::max(max_stop_time, worker_report.stop_time);
    max_destruction_time = std::max(max_destruction_time, worker_report.destruction_time);

    total_undermatched_readers += worker_report.undermatched_readers;
    total_undermatched_writers += worker_report.undermatched_writers;
    max_discovery_time_delta = std::max(max_discovery_time_delta, worker_report.max_discovery_time_delta);

    const Builder::ProcessReport& process_report = worker_report.process_report;

    for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
      for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
        for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {

          const Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];

          Bench::ConstPropertyStatBlock dr_latency(dr_report.properties, "latency");
          Bench::ConstPropertyStatBlock dr_jitter(dr_report.properties, "jitter");
          Bench::ConstPropertyStatBlock dr_round_trip_latency(dr_report.properties, "round_trip_latency");
          Bench::ConstPropertyStatBlock dr_round_trip_jitter(dr_report.properties, "round_trip_jitter");

          consolidated_latency_stats = consolidate(consolidated_latency_stats, dr_latency.to_simple_stat_block());
          consolidated_jitter_stats = consolidate(consolidated_jitter_stats, dr_jitter.to_simple_stat_block());
          consolidated_round_trip_latency_stats = consolidate(consolidated_round_trip_latency_stats, dr_round_trip_latency.to_simple_stat_block());
          consolidated_round_trip_jitter_stats = consolidate(consolidated_round_trip_jitter_stats, dr_round_trip_jitter.to_simple_stat_block());
        }
      }
    }
  }

  std::cout << std::endl;

  std::cout << "Test Timing Stats:" << std::endl;
  std::cout << "  Max Construction Time: " << max_construction_time << " seconds" << std::endl;
  std::cout << "  Max Enable Time: " << max_enable_time << " seconds" << std::endl;
  std::cout << "  Max Start Time: " << max_start_time << " seconds" << std::endl;
  std::cout << "  Max Stop Time: " << max_stop_time << " seconds" << std::endl;
  std::cout << "  Max Destruction Time: " << max_destruction_time << " seconds" << std::endl;

  std::cout << std::endl;

  std::cout << "Discovery Stats:" << std::endl;
  std::cout << "  Total Undermatched Readers: " << total_undermatched_readers << ", Total Undermatched Writers: " << total_undermatched_writers << std::endl;
  std::cout << "  Max Discovery Time Delta: " << max_discovery_time_delta << " seconds" << std::endl;

  std::cout << std::endl;

  consolidated_latency_stats.pretty_print(std::cout, "latency");

  std::cout << std::endl;

  consolidated_jitter_stats.pretty_print(std::cout, "jitter");

  std::cout << std::endl;

  consolidated_round_trip_latency_stats.pretty_print(std::cout, "round trip latency");

  std::cout << std::endl;

  consolidated_round_trip_jitter_stats.pretty_print(std::cout, "round trip jitter");
}
