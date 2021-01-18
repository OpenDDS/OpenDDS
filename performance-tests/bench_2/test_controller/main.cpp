#include "ScenarioOverrides.h"
#include "ScenarioManager.h"

#include <PropertyStatBlock.h>
#include <util.h>
#include <json_conversion.h>

#ifdef __GNUC__
#  pragma GCC diagnostic push
#  if defined(__has_warning)
#    if __has_warning("-Wclass-memaccess")
#      pragma GCC diagnostic ignored "-Wclass-memaccess"
#    endif
#  elif __GNUC__ > 7
#    pragma GCC diagnostic ignored "-Wclass-memaccess"
#  endif
#endif
#include "BenchTypeSupportImpl.h"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <unordered_map>
#include <unordered_set>

using namespace Bench;
using namespace Bench::NodeController;
using namespace Bench::TestController;

std::string bench_root;

namespace {
  const size_t DEFAULT_MAX_DECIMAL_PLACES = 9u;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  int result = EXIT_SUCCESS;
  const char* cstr = ACE_OS::getenv("BENCH_ROOT");
  bench_root = cstr ? cstr : "";
  if (bench_root.empty()) {
    cstr = ACE_OS::getenv("DDS_ROOT");
    const std::string dds_root{cstr ? cstr : ""};
    if (dds_root.empty()) {
      std::cerr << "ERROR: BENCH_ROOT or DDS_ROOT must be defined" << std::endl;
      return 1;
    }
    bench_root = join_path(dds_root, "performance-tests", "bench_2");
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
  /// Max time to wait for the scenario to complete
  bool timeout_passed = false;
  unsigned timeout = 0; // Default defined in scenario config or alloc

  /// If not empty, Stop After Allocation and Output the Allocation to File
  std::string preallocated_scenario_output_path;
  /// Pretty Print the Preallocated Scenario JSON
  bool pretty = false;
  /// Print out debug allocation to the screen and exit.
  bool debug_alloc = false;
  /// If not empty, Don't Discover Nodes, Send Out an Existing Set of Node Configs
  std::string preallocated_scenario_input_path;

  /// Location of Test Artifacts and Configurations
  std::string test_context_path;

  /// Name of scenario config file without the json ext
  std::string scenario_id;
  /**
   * Name to store the results under. By default it is the highest number found
   * in the result directory of the scenario + 1 and "1" if no result with id that's
   * a number is found. Can be manually assigned to be any valid file name,
   * not just a number.
   */
  std::string result_id;
  bool overwrite_result = false, json_result = false, show_worker_logs = false;

  // List of tags
  std::unordered_set<std::string> tags;

  ScenarioOverrides overrides;
  {
    char host[256];
    ACE_OS::hostname(host, sizeof(host));
    pid_t pid = ACE_OS::getpid();
    std::stringstream ss;
    ss << "_" << host << "_" << pid << std::flush;
    overrides.bench_partition_suffix = ss.str();
  }

  const char* usage = "usage: test_controller [-h|--help] | TEST_CONTEXT SCENARIO_ID [OPTIONS...]";
  try {
    for (int i = 1; i < argc; i++) {
      const ACE_TCHAR* argument = argv[i];
      if (!ACE_OS::strcmp(argument, ACE_TEXT("--domain"))) {
        domain = get_option_argument_int(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--wait-for-nodes"))) {
        wait_for_nodes = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--timeout"))) {
        timeout = get_option_argument_uint(i, argc, argv);
        timeout_passed = true;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--help")) || !ACE_OS::strcmp(argument, ACE_TEXT("-h"))) {
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
          << "--timeout N                  The number of seconds to wait for a scenario to" << std::endl
          << "                             complete. Overrides the value defined in the" << std::endl
          << "                             scenario. If N is 0, there is no timeout." << std::endl
          << "--prealloc-scenario-out PATH Instead of running the scenario, write the" << std::endl
          << "                             directives (in JSON) that would have been sent to" << std::endl
          << "                             the node controllers to the file at this path." << std::endl
          << "--pretty                     Write the JSON output of `--prealloc-scenario-out`" << std::endl
          << "                             with indentation." << std::endl
          << "--debug-alloc                Print out a debug version of what is saved with" << std::endl
          << "                             --prealloc-scenario-out and exit." << std::endl
          << "--prealloc-scenario-in PATH  Take result of --prealloc-scneario-out and use that" << std::endl
          << "                             to run the scenario instead of discovering nodes." << std::endl
          << "                             This might fail if the nodes go offline after the" << std::endl
          << "                             preallocated scenario is saved." << std::endl
          << "--result-id ID               Name to store the results under. By default" << std::endl
          << "                             incrementing numbers are assigned, but ID doesn't" << std::endl
          << "                             have to be a valid number, just a valid file name." << std::endl
          << "--overwrite-result           Write the result when using --result-id, even if" << std::endl
          << "                             it would overwrite another result." << std::endl
          << "--override-bench-partition-suffix SUFFIX     Override the system-wide partition suffix." << std::endl
          << "--override-create-time N                     Override the system-wide creation time." << std::endl
          << "--override-enable-time N                     Override the system-wide enabling time." << std::endl
          << "--override-start-time N                      Override the system-wide starting time." << std::endl
          << "--override-stop-time N                       Override the system-wide stopping time." << std::endl
          << "--override-destruction-time N                Override the system-wide destruction time." << std::endl
          << "--tag TAG                    Specify a tag for which the user wants to collect" << std::endl
          << "                             the statistics information. User can specify multiple" << std::endl
          << "                             --tag options, each with a single tag." << std::endl
          << "--json                       Output full JSON report as '<resuld-id>.json' in the test conext." << std::endl
          << "                             By default, this not enabled. This report will contain" << std::endl
          << "                             the full raw Bench::TestController report, including all" << std::endl
          << "                             node controller and worker reports (and DDS entity reports)" << std::endl;
//            ################################################################################
        return 0;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--prealloc-scenario-out"))) {
        preallocated_scenario_output_path = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--prealloc-scenario-in"))) {
        preallocated_scenario_input_path = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--pretty"))) {
        pretty = true;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--debug-alloc"))) {
        debug_alloc = true;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--result-id"))) {
        result_id = get_option_argument(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--overwrite-result"))) {
        overwrite_result = true;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-bench-partition-suffix"))) {
        std::string suffix = get_option_argument(i, argc, argv);
        overrides.bench_partition_suffix = suffix == "none" ? "" : suffix;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-create-time"))) {
        overrides.create_time_delta = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-enable-time"))) {
        overrides.enable_time_delta = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-start-time"))) {
        overrides.start_time_delta = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-stop-time"))) {
        overrides.stop_time_delta = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--override-destruction-time"))) {
        overrides.destruction_time_delta = get_option_argument_uint(i, argc, argv);
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--tag"))) {
        tags.insert(get_option_argument(i, argc, argv));
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--json"))) {
        json_result = true;
      } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--show-worker-logs"))) {
        show_worker_logs = true;
      } else if (test_context_path.empty() && argument[0] != '-') {
        test_context_path = ACE_TEXT_ALWAYS_CHAR(argument);
      } else if (scenario_id.empty() && argument[0] != '-') {
        scenario_id = ACE_TEXT_ALWAYS_CHAR(argument);
      } else {
        std::cerr << "Invalid option: " << argument << std::endl;
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

  // Try To Load Scenario
  std::string scenario_path = join_path(test_context_path, "config", "scenario", scenario_id + ".json");
  ScenarioPrototype scenario_prototype{};
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

  // Determine Where The Results Will Go
  const std::string results_path = join_path(test_context_path, "result");
  if (result_id.empty()) { // Assign a Number starting at 1 if not specified
    try {
      unsigned long max_value = 0;
      for (const std::string& i : get_dir_contents(results_path)) {
        try {
          max_value = std::max(std::stoul(i), max_value);
        } catch (const std::exception&) {
          continue;
        }
      }
      result_id = std::to_string(max_value + 1);
    } catch (const std::runtime_error& e) {
      std::cerr << "Error: " << e.what() << std::endl;
      return 1;
    }
  }
  const std::string result_path = join_path(results_path, result_id);
  const std::string json_result_path = json_result ? result_path + ".json" : "";

  // Check to see if the Result Already Exists
  try {
    if (file_exists(result_path)) {
      if (overwrite_result) {
        std::cerr << "Warning: overwriting " << result_path << std::endl;
      } else {
        std::stringstream ss;
        ss << "would overwrite " << result_path << ". Pass --overwrite-result to force it to.";
        throw std::runtime_error(ss.str());
      }
    }
    if (!json_result_path.empty() && file_exists(json_result_path)) {
      if (overwrite_result) {
        std::cerr << "Warning: overwriting " << json_result_path << std::endl;
      } else {
        std::stringstream ss;
        ss << "would overwrite " << json_result_path << ". Pass --overwrite-result to force it to.";
        throw std::runtime_error(ss.str());
      }
    }
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  // Print Scenario File, Name, and Description
  std::string scenario_info;
  {
    std::ostringstream ss;
    ss << "Using " << scenario_path;
    const std::string name = scenario_prototype.name.in();
    const std::string desc = scenario_prototype.desc.in();
    if (!name.empty() || !desc.empty()) {
      ss << ':';
      if (!name.empty()) {
        ss << std::endl << "    Name: " << name;
      }
      if (!desc.empty()) {
        ss << std::endl << "    Description: " << desc;
      }
    }
    ss << std::endl;
    scenario_info = ss.str();
  }
  std::cout << scenario_info;

  try {
    DdsEntities dds_entities(dpf, domain);
    ScenarioManager scenario_manager(bench_root, test_context_path, overrides, dds_entities);

    // Part 1: Get What Needs to be Broadcast
    AllocatedScenario allocated_scenario{};
    if (preallocated_scenario_input_path.size()) {
      std::cout << "Loading Scenario Allocation from File..." << std::endl;
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
      allocated_scenario = scenario_manager.allocate_scenario(scenario_prototype, available_nodes, debug_alloc);
    }

    if (timeout_passed) {
      allocated_scenario.timeout = timeout;
    }

    // Part 2: Broadcast the Directives and Wait for Reports
    if (preallocated_scenario_output_path.size()) {
      std::cout << "Saving Scenario Allocation to File..." << std::endl;
      std::ofstream file(preallocated_scenario_output_path);
      if (file.is_open()) {
        if ((pretty && !idl_2_json<AllocatedScenario, rapidjson::PrettyWriter<rapidjson::OStreamWrapper> >(allocated_scenario, file)) ||
            (!pretty && !idl_2_json(allocated_scenario, file))) {
          throw std::runtime_error("Could not encode allocated scenario");
        }
      } else {
        throw std::runtime_error("Could not open file for writing allocated scenario");
      }
    } else if (debug_alloc) {
      if (!idl_2_json<AllocatedScenario, rapidjson::PrettyWriter<rapidjson::OStreamWrapper> >(allocated_scenario, std::cout)) {
        throw std::runtime_error("Could not encode allocated scenario");
      }
    } else {
      std::cout << "Running the Scenario..." << std::endl;

      std::string scenario_start;
      {
        std::ostringstream ss;
        ss << "Results will be stored in " << result_path << std::endl;
        if (!json_result_path.empty()) {
          ss << "JSON Results will be stored in " << json_result_path << std::endl;
        }
        ss << std::endl << "Started at " << iso8601() << std::endl;
        scenario_start = ss.str();
      }
      std::cout << scenario_start;

      Bench::TestController::Report report{};
      report.scenario_name = scenario_id.c_str();
      report.time = Builder::get_sys_time();
      scenario_manager.execute(allocated_scenario, report);

      std::ofstream result_file(result_path);
      if (!result_file.is_open()) {
        std::stringstream error;
        error << "Could not open " << result_path;
        throw std::runtime_error(error.str());
      }

      result_file
        << scenario_info
        << scenario_start
        << std::endl
        << "Ran with " << allocated_scenario.configs.length() << " nodes" << std::endl;

      std::string scenario_end;
      {
        std::ostringstream ss;
        ss
          << std::endl
          << "Ended at " << iso8601() << std::endl
          << std::endl;
        result = handle_report(report, tags, ss);
        scenario_end = ss.str();
      }
      result_file << scenario_end;
      std::cout << scenario_end;

      std::cout << "Wrote results to " << result_path << std::endl;

      size_t total_worker_reports = 0;
      for (CORBA::ULong i = 0; i < report.node_reports.length(); ++i) {
        total_worker_reports += report.node_reports[i].worker_reports.length();
        if (show_worker_logs) {
          for (CORBA::ULong j = 0; j < report.node_reports[i].worker_logs.length(); ++j) {
            std::stringstream header;
            header << "=== Showing Log for Node " << report.node_reports[i].node_id << " Worker #" << report.node_reports[i].worker_ids[j] << " ===" << std::endl;
            result_file << header.str();
            result_file << report.node_reports[i].worker_logs[j] << std::endl << std::endl;
            std::cout << header.str();
            std::cout << report.node_reports[i].worker_logs[j] << std::endl << std::endl;
          }
        }
      }

      if (total_worker_reports != allocated_scenario.expected_reports) {
        std::string log_msg = "ERROR: Only received " + std::to_string(total_worker_reports) +
          " out of " + std::to_string(allocated_scenario.expected_reports) + " valid reports!\n";
        result_file << log_msg;
        std::cerr << log_msg;
        result = EXIT_FAILURE;
      }

      if (!json_result_path.empty()) {
        std::ofstream json_result_file(json_result_path);
        if (!json_result_file.is_open()) {
          std::stringstream error;
          error << "Could not open " << json_result_path;
          throw std::runtime_error(error.str());
        }
        idl_2_json(report, json_result_file, DEFAULT_MAX_DECIMAL_PLACES);
        std::cout << "Wrote JSON results to " << json_result_path << std::endl;
      }
    }
  } catch (const std::runtime_error& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "Finished" << std::endl;

  return result;
};
