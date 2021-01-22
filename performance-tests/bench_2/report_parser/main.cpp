#include <util.h>
#include <json_conversion.h>
#include <PropertyStatBlock.h>

#include "BenchTypeSupportImpl.h"

#include <iostream>
#include <fstream>
#include <unordered_map>

#include <dds/DCPS/RapidJsonWrapper.h>

#include <ace/ace_wchar.h> // For ACE_TCHAR
#include <ace/OS.h> // for ACE_OS

using namespace Bench;
using namespace Bench::TestController;

Report report{};
std::ofstream outputFileStream;

enum class OutputType { None, TimeSeries } outputType;
enum class OutputFormat { None, Gnuplot } outputFormat;

const char* usage = "usage: report_parser [-h|--help] | [OPTIONS...]";

void show_usage_prompt() {
  std::cerr << "Use -h or --help to see the full help message" << std::endl;
}

void show_option_error(std::string option) {
  std::cerr << "Invalid option: " << option << std::endl;
  show_usage_prompt();
}

void show_option_argument_error(std::string optionArgument) {
  std::cerr << "Invalid option argument: " << optionArgument << std::endl;
  show_usage_prompt();
}

void show_usage() {
  std::cout
    << "OPTIONS:" << std::endl
    << "--input-file <filename>      The report file to read" << std::endl
    << "--output-file <filename>     The parsed file to generate" << std::endl
    << "--output-type <type>         Specifies type of data to parse." << std::endl
    << "     Options:" << std::endl
    << "            time-series:     Parses out time-series data" << std::endl
    << "--output-format <format>     Specifies format of output." << std::endl
    << "     Options:" << std::endl
    << "            gnuplot:         Formats output for plotting with gnuplot" << std::endl;
}

bool parse_arguments(int argc, ACE_TCHAR* argv[]) {
  std::string input_file_path;
  std::string output_file_path;

  if (argc == 1) {
    show_usage_prompt();
  }
  else {
    try {
      for (int i = 1; i < argc; i++) {
        const ACE_TCHAR* argument = argv[i];
        std::string option_argument = get_option_argument(i, argc, argv);

        if (!ACE_OS::strcmp(argument, ACE_TEXT("--help")) || !ACE_OS::strcmp(argument, ACE_TEXT("-h"))) {
          std::cout << usage << std::endl
            << std::endl
            << "OPTIONS:" << std::endl
            << "--input-file <filename>      The report file to read" << std::endl
            << "--output-file <filename>     The parsed file to generate" << std::endl
            << "--output-type <type>         Specifies type of data to parse." << std::endl
            << "     Options:" << std::endl
            << "            time-series:     Parses out time-series data" << std::endl
            << "--output-format <format>     Specifies format of output." << std::endl
            << "     Options:" << std::endl
            << "            gnuplot:         Formats output for plotting with gnuplot" << std::endl;
          return false;
        }
        else if (!ACE_OS::strcmp(argument, ACE_TEXT("--input-file"))) {
          input_file_path = option_argument;
        }
        else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-file"))) {
          output_file_path = option_argument;
        }
        else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-type"))) {
          static std::unordered_map<std::string, OutputType> const table =
          {
            {"time-series", OutputType::TimeSeries}
          };
          auto it = table.find(option_argument);
          if (it != table.end()) {
            outputType = it->second;
          }
          else {
            show_option_argument_error(option_argument);
            return false;
          }
        }
        else if (!ACE_OS::strcmp(argument, ACE_TEXT("--output-format"))) {
          static std::unordered_map<std::string, OutputFormat> const table =
          {
            {"gnuplot", OutputFormat::Gnuplot}
          };
          auto it = table.find(option_argument);
          if (it != table.end()) {
            outputFormat = it->second;
          }
          else {
            show_option_argument_error(option_argument);
            return false;
          }
        }
        else {
          show_option_error(argument);
          return false;
        }
      }
    }
    catch (int value) {
      show_usage_prompt();
      return false;
    }
  }

  if (input_file_path.empty()) {
    std::cerr << "Input file not specified" << std::endl;
    return false;
  }
  else if (output_file_path.empty()) {
    std::cerr << "Output file not specified" << std::endl;
    return false;
  }
  else {
    std::ifstream ifs(input_file_path);
    if (ifs.is_open()) {
      if (!json_2_idl(ifs, report)) {
        std::cerr << "Could not parse " << input_file_path << std::endl;
        return false;
      }
      else {
        outputFileStream.open(output_file_path);
        if (!outputFileStream.is_open()) {
          std::cerr << "Could not open output file " << output_file_path << std::endl;
          return false;
        }
      }
    }
    else {
      std::cerr << "Could not open input file " << input_file_path << std::endl;
      return false;
    }
  }

  return true;
}

void output_gnuplot_header() {
  const bool show_postfix = report.node_reports.length() > 1;

  // A gnuplot data file header comment line begins with # character.
  outputFileStream << "#";

  // Write header line.
  for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
    if (node_index > 0) outputFileStream << " ";
    outputFileStream << "cpu_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    outputFileStream << " cpu_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");
    outputFileStream << " mem_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    outputFileStream << " mem_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");
    outputFileStream << " virtual_mem_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    outputFileStream << " virtual_mem_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");
  }

  outputFileStream << std::endl;
}

void output_gnuplot_data() {
  Bench::SimpleStatBlock consolidated_cpu_percent_stats;
  Bench::SimpleStatBlock consolidated_mem_percent_stats;
  Bench::SimpleStatBlock consolidated_virtual_mem_percent_stats;

  // Determine the biggest buffer size.
  size_t biggest_buffer_size = 0;

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
    const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    Bench::ConstPropertyStatBlock cpu_percent(nc_report.properties, "cpu_percent");
    Bench::ConstPropertyStatBlock mem_percent(nc_report.properties, "mem_percent");
    Bench::ConstPropertyStatBlock virtual_mem_percent(nc_report.properties, "virtual_mem_percent");

    consolidated_cpu_percent_stats = consolidate(consolidated_cpu_percent_stats, cpu_percent.to_simple_stat_block());
    consolidated_mem_percent_stats = consolidate(consolidated_mem_percent_stats, mem_percent.to_simple_stat_block());
    consolidated_virtual_mem_percent_stats = consolidate(consolidated_virtual_mem_percent_stats, virtual_mem_percent.to_simple_stat_block());

    if (consolidated_cpu_percent_stats.median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_cpu_percent_stats.median_buffer_.size();

    if (consolidated_cpu_percent_stats.timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_cpu_percent_stats.timestamp_buffer_.size();

    if (consolidated_mem_percent_stats.median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_mem_percent_stats.median_buffer_.size();

    if (consolidated_mem_percent_stats.timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_mem_percent_stats.timestamp_buffer_.size();

    if (consolidated_virtual_mem_percent_stats.median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_virtual_mem_percent_stats.median_buffer_.size();

    if (consolidated_virtual_mem_percent_stats.timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = consolidated_virtual_mem_percent_stats.timestamp_buffer_.size();
  }

  for (unsigned int index = 0; index < biggest_buffer_size; index++) {
    for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
      const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

      Bench::ConstPropertyStatBlock cpu_percent(nc_report.properties, "cpu_percent");
      Bench::ConstPropertyStatBlock mem_percent(nc_report.properties, "mem_percent");
      Bench::ConstPropertyStatBlock virtual_mem_percent(nc_report.properties, "virtual_mem_percent");

      consolidated_cpu_percent_stats = consolidate(consolidated_cpu_percent_stats, cpu_percent.to_simple_stat_block());
      consolidated_mem_percent_stats = consolidate(consolidated_mem_percent_stats, mem_percent.to_simple_stat_block());
      consolidated_virtual_mem_percent_stats = consolidate(consolidated_virtual_mem_percent_stats, virtual_mem_percent.to_simple_stat_block());

      const size_t cpu_percent_median_count = consolidated_cpu_percent_stats.median_buffer_.size();
      const size_t cpu_percent_timestamp_count = consolidated_cpu_percent_stats.timestamp_buffer_.size();

      const size_t mem_percent_median_count = consolidated_mem_percent_stats.median_buffer_.size();
      const size_t mem_percent_timestamp_count = consolidated_mem_percent_stats.timestamp_buffer_.size();

      const size_t virtual_mem_percent_median_count = consolidated_virtual_mem_percent_stats.median_buffer_.size();
      const size_t virtual_mem_percent_timestamp_count = consolidated_virtual_mem_percent_stats.timestamp_buffer_.size();

      if (index < cpu_percent_median_count)
        outputFileStream << consolidated_cpu_percent_stats.median_buffer_[index] << " ";

      if (index < cpu_percent_timestamp_count)
        outputFileStream << consolidated_cpu_percent_stats.timestamp_buffer_[index] << " ";

      if (index < mem_percent_median_count)
        outputFileStream << consolidated_mem_percent_stats.median_buffer_[index] << " ";

      if (index < mem_percent_timestamp_count)
        outputFileStream << consolidated_mem_percent_stats.timestamp_buffer_[index] << " ";

      if (index < virtual_mem_percent_median_count)
        outputFileStream << consolidated_virtual_mem_percent_stats.median_buffer_[index] << " ";

      if (index < virtual_mem_percent_timestamp_count)
        outputFileStream << consolidated_virtual_mem_percent_stats.timestamp_buffer_[index];

      if (node_index + 1 < report.node_reports.length())
        outputFileStream << " ";
    }

    outputFileStream << std::endl;
  }
}

int parse_time_series_to_gnuplot_format() {
  if (report.node_reports.length() > 0) {
    output_gnuplot_header();
    output_gnuplot_data();

    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int parse_time_series_to_raw_format() {
  Bench::SimpleStatBlock consolidated_cpu_percent_stats;
  Bench::SimpleStatBlock consolidated_mem_percent_stats;
  Bench::SimpleStatBlock consolidated_virtual_mem_percent_stats;

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
    const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    Bench::ConstPropertyStatBlock cpu_percent(nc_report.properties, "cpu_percent");
    Bench::ConstPropertyStatBlock mem_percent(nc_report.properties, "mem_percent");
    Bench::ConstPropertyStatBlock virtual_mem_percent(nc_report.properties, "virtual_mem_percent");

    consolidated_cpu_percent_stats = consolidate(consolidated_cpu_percent_stats, cpu_percent.to_simple_stat_block());
    consolidated_mem_percent_stats = consolidate(consolidated_mem_percent_stats, mem_percent.to_simple_stat_block());
    consolidated_virtual_mem_percent_stats = consolidate(consolidated_virtual_mem_percent_stats, virtual_mem_percent.to_simple_stat_block());

    outputFileStream << "CPU median buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_cpu_percent_stats.median_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_cpu_percent_stats.median_buffer_[i] << std::endl;
    }

    outputFileStream << "CPU timestamp buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_cpu_percent_stats.timestamp_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_cpu_percent_stats.timestamp_buffer_[i] << std::endl;
    }

    outputFileStream << "Mem median buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_mem_percent_stats.median_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_mem_percent_stats.median_buffer_[i] << std::endl;
    }

    outputFileStream << "Mem timestamp buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_mem_percent_stats.timestamp_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_mem_percent_stats.timestamp_buffer_[i] << std::endl;
    }

    outputFileStream << "Virtual mem median buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_virtual_mem_percent_stats.median_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_virtual_mem_percent_stats.median_buffer_[i] << std::endl;
    }

    outputFileStream << "Virtual mem timestamp buffer values:" << std::endl;
    for (std::vector<double>::size_type i = 0; i != consolidated_virtual_mem_percent_stats.timestamp_buffer_.size(); i++) {
      outputFileStream << "\t" << consolidated_virtual_mem_percent_stats.timestamp_buffer_[i] << std::endl;
    }
  }

  return EXIT_SUCCESS;
}

int parse_time_series() {
  switch (outputFormat) {
  case OutputFormat::Gnuplot:
    return parse_time_series_to_gnuplot_format();
    break;
  default:
    break;
  }

  return parse_time_series_to_raw_format();
}

int parse_report() {
  switch (outputType) {
  case OutputType::TimeSeries:
    return parse_time_series();
    break;
  default:
    break;
  }

  return EXIT_FAILURE;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::cout << "NOT YET IMPLEMENTED!\n";

  if (!parse_arguments(argc, argv)) {
    return EXIT_FAILURE;
  }

  if (!parse_report()) {
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}
