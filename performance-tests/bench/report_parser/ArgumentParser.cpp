#include "ArgumentParser.h"

#include <util.h>
#include <json_conversion.h>
#include <BenchTypeSupportImpl.h>

#include <unordered_map>

using namespace Bench;

bool ArgumentParser::parse(int argc, ACE_TCHAR* argv[], OutputType& output_type,
    OutputFormat& output_format, Bench::TestController::Report& report, std::shared_ptr<std::ostream>& output_stream,
    ParseParameters& parse_parameters)
{
  std::string input_file_path;
  std::string output_file_path;

  if (argc == 1) {
    show_usage_prompt();
  } else {
    try {
      for (int i = 1; i < argc; i++) {
        const ACE_TCHAR* argument = argv[i];

        if (!ACE_OS::strcmp(argument, ACE_TEXT("--help"))
            || !ACE_OS::strcmp(argument, ACE_TEXT("-h"))) {
          show_usage();
          return false;
        }

        if (!ACE_OS::strcmp(argument, ACE_TEXT("--input-file"))) {
          std::string option_argument = get_option_argument(i, argc, argv);
          input_file_path = option_argument;
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-file"))) {
          std::string option_argument = get_option_argument(i, argc, argv);
          output_file_path = option_argument;
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-type"))) {
          std::string option_argument = get_option_argument(i, argc, argv);
          static std::unordered_map<std::string, OutputType> const table = {
            { "single-statistic", OutputType::SingleStatistic },
            { "time-series", OutputType::TimeSeries },
            { "summary", OutputType::Summary }
          };

          auto it = table.find(option_argument);

          if (it != table.end()) {
            output_type = it->second;
          } else {
            show_option_argument_error(option_argument);
            return false;
          }
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-format"))) {
          std::string option_argument = get_option_argument(i, argc, argv);
          static std::unordered_map<std::string, OutputFormat> const table = {
            { "stat-block", OutputFormat::StatBlock },
            { "gnuplot", OutputFormat::Gnuplot },
            { "json", OutputFormat::Json }
          };

          auto it = table.find(option_argument);

          if (it != table.end()) {
            output_format = it->second;
          } else {
            show_option_argument_error(option_argument);
            return false;
          }
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--tags"))) {
          if (!parse_parameters.tags.empty()) {
            continue;
          }

          if (i + 1 < argc) {
            for (int index = i + 1; index < argc; index++) {
              std::string option = ACE_TEXT_ALWAYS_CHAR(argv[index]);
              if (option[0] == '-') {
                break;
              } else {
                parse_parameters.tags.insert(option);
                i++;
              }
            }
          }

          if (parse_parameters.tags.empty()) {
            std::cout << "Missing tag types";
            return false;
          }
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--stats"))) {
          if (!parse_parameters.stats.empty()) {
            continue;
          }

          if (i + 1 < argc) {
            for (int index = i + 1; index < argc; index++) {
              std::string option = ACE_TEXT_ALWAYS_CHAR(argv[index]);
              if (option[0] == '-') {
                break;
              } else {
                parse_parameters.stats.insert(option);
                i++;
              }
            }
          }

          if (parse_parameters.stats.empty()) {
            std::cout << "Missing stat types";
            return false;
          }
        } else if (!ACE_OS::strcmp(argument, ACE_TEXT("--values"))) {
          if (!parse_parameters.values.empty()) {
            continue;
          }

          if (i + 1 < argc) {
            for (int index = i + 1; index < argc; index++) {
              std::string option = ACE_TEXT_ALWAYS_CHAR(argv[index]);
              if (option[0] == '-') {
                break;
              } else {
                parse_parameters.values.insert(option);
                i++;
              }
            }
          }

          if (parse_parameters.values.empty()) {
            std::cout << "Missing value types";
            return false;
          }
        } else {
          show_option_error(ACE_TEXT_ALWAYS_CHAR(argument));
          return false;
        }
      }
    } catch (int) {
      show_usage_prompt();
      return false;
    }
  }

  std::shared_ptr<std::ifstream> ifs;

  if (!input_file_path.empty()) {
    std::cout << "Opening input file: " << input_file_path << std::endl;
    ifs.reset(new std::ifstream(input_file_path));
    if (!ifs->is_open()) {
      std::cerr << "Could not open input file " << input_file_path << std::endl;
      return false;
    }
  }

  if (!output_file_path.empty()) {
    std::cout << "Opening output file: " << output_file_path << std::endl;
    output_stream.reset(new std::ofstream(output_file_path));
    if (!std::dynamic_pointer_cast<std::ofstream>(output_stream)->is_open()) {
      std::cerr << "Could not open output file " << output_file_path << std::endl;
      return false;
    }
  }

  std::istream& is = input_file_path.empty() ? std::cin : *ifs;

  std::cout << "Parsing input JSON" << std::endl;

  if (!json_2_idl(is, report)) {
    std::cerr << "Could not parse input JSON" << std::endl;
    return false;
  }

  check_for_iperf(report);

  return true;
}

void ArgumentParser::show_usage_prompt()
{
  std::cerr << "Use -h or --help to see the full help message" << std::endl;
}

void ArgumentParser::show_option_error(std::string option)
{
  std::cerr << "Invalid option: " << option << std::endl;
  show_usage_prompt();
}

void ArgumentParser::show_option_argument_error(std::string option_argument)
{
  std::cerr << "Invalid option argument: " << option_argument << std::endl;
  show_usage_prompt();
}

void ArgumentParser::show_usage()
{
  std::cout
    << "usage: report_parser [-h|--help] | [OPTIONS...]" << std::endl
    << "OPTIONS:" << std::endl
    << "--input-file  <filename>      The report file to read" << std::endl
    << "--output-file <filename>      The parsed file to generate" << std::endl
    << "--output-type <type>          Specifies type of data to parse." << std::endl
    << "     Options:" << std::endl
    << "            single-statistic: Parses out single statistic" << std::endl
    << "            time-series:      Parses out time-series data" << std::endl
    << "--output-format <format>      Specifies format of output." << std::endl
    << "     Options:" << std::endl
    << "            stat-block:       Formats output for printing a single consolidated stat block" << std::endl
    << "            gnuplot:          Formats output for plotting with gnuplot" << std::endl
    << "--tags   <list of tags>       Specifies the status block types to output." << std::endl
    << "--stats  <list of stats>      Specifies the status block value types to output." << std::endl
    << "--values <list of values>     Specifies the status block values to output." << std::endl;
}

void ArgumentParser::check_for_iperf(Bench::TestController::Report& report)
{

  // This code is a bit of a hack, and isn't really expected to function well across different versions / flags of iperf
  // This is specifically designed to parse out course-grained latency and jitter values from iperf2's (2.0.13) extended output flag (-e)
  // And then add those stats back into the individual node_controller report's properties with the 'iperf' tag so they will show up in report summaries

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); ++node_index) {
    Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    for (unsigned int spawned_process_index = 0; spawned_process_index < nc_report.spawned_process_logs.length(); ++spawned_process_index) {
      const auto& spawned_process_log = nc_report.spawned_process_logs[spawned_process_index];

      PropertyStatBlock latency_psb(nc_report.properties, "latency", nc_report.spawned_process_logs.length() * 1000);
      PropertyStatBlock jitter_psb(nc_report.properties, "jitter", nc_report.spawned_process_logs.length() * 1000);

      std::stringstream ss;
      ss << spawned_process_log.in();

      bool is_iperf_server = false;
      bool is_first_interval = true;

      std::string line;
      std::getline(ss, line);
      while (ss.good()) {
        if (!line.empty()) {

          // Example of header & interval line (summary line comes at the end)

          //[ ID] Interval        Transfer     Bandwidth        Jitter   Lost/Total  Latency avg/min/max/stdev PPS  NetPwr
          //[  3] 0.00-1.00 sec  1.44 KBytes  11.8 Kbits/sec   0.000 ms    0/    1 (0%)  0.367/ 0.367/ 0.367/ 0.000 ms    1 pps  4.01

          if (is_iperf_server && line[0] == '[') {

            long unsigned int id;
            double interval_start, interval_end, transfer, bandwidth, jitter, lost, total, lost_pct, latency_avg, latency_min, latency_max, latency_stdev, pps, netpwr;
            char interval_units[16];
            char transfer_units[16];
            char bandwidth_units[16];
            char jitter_units[16];
            char latency_units[16];
            char pps_units[16];

            std::sscanf(line.c_str(), "[ %lu] %lf-%lf %s %lf %s %lf %s %lf %s %lf/ %lf (%lf%%) %lf/ %lf/ %lf/ %lf %s %lf %s %lf", &id, &interval_start, &interval_end, interval_units, &transfer, transfer_units, &bandwidth, bandwidth_units, &jitter, jitter_units, &lost, &total, &lost_pct, &latency_avg, &latency_min, &latency_max, &latency_stdev, latency_units, &pps, pps_units, &netpwr);

            if (interval_start > 0.001 || is_first_interval) {
              //std::cout << line << std::endl;
              //std::cout << "Found normal interval from " << interval_start << " '" << interval_units << "' to " << interval_end << " '" << interval_units << "'" << std::endl;
              //std::cout << " - id = " << id << " transfer = " << transfer << " '" << transfer_units << "', bandwidth = " << bandwidth << " '" << bandwidth_units << "'" << std::endl;
              //std::cout << " - jitter = " << jitter << " '" << jitter_units << "', lost " << lost << " out of " << total << " which is " << lost_pct << '%' << std::endl;
              //std::cout << " - latency avg/min/max/stdev = " << latency_avg << " " << latency_min << " " << latency_max << " " << latency_stdev << " '" << latency_units << "'" << std::endl;
              //std::cout << " - pps = " << pps << " '" << pps_units << "', netpwr = " << netpwr << std::endl;

              latency_psb.update(latency_avg / 1000.0);
              jitter_psb.update(jitter / 1000.0);
            }
            is_first_interval = false;
          }

          if (line.find("Latency avg/min/max/stdev PPS") != std::string::npos) {
            is_iperf_server = true;
          }

        }
        std::getline(ss, line);
      }
      latency_psb.finalize();
      jitter_psb.finalize();
    }
  }
}
