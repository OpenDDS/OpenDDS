#include "ArgumentParser.h"

#include <util.h>
#include <json_conversion.h>
#include <BenchTypeSupportImpl.h>

#include <dds/DCPS/ArgParsing.h>

#include <unordered_map>

using namespace Bench;
using namespace OpenDDS::DCPS::ArgParsing;

namespace {
  class RemainingArgs : public Handler {
  public:
    using ValueType = std::unordered_set<std::string>;

    ValueType& values;

    RemainingArgs(ValueType& values)
      : values(values)
    {
    }

    size_t min_args_required() const
    {
      return 1;
    }

    size_t max_args_required() const
    {
      return size_t_max;
    }

    StrVecIt handle(Argument& /*arg*/, ArgParseState& state, StrVecIt values)
    {
      StrVecIt it = values;
      for (; it != state.args.end() || !state.parser.looks_like_option(*it);) {
        this->values.insert(*values);
        it = state.args.erase(values);
      }
      return it;
    }
  };
}

bool ArgumentParser::parse(int argc, ACE_TCHAR* argv[], OutputType& output_type,
    OutputFormat& output_format, Bench::TestController::Report& report, std::shared_ptr<std::ostream>& output_stream,
    ParseParameters& parse_parameters)
{
  std::string input_file_path;
  std::string output_file_path;

  {
    ArgParser arg_parser("");

    OptionAs<StringValue> input_file_opt(arg_parser, "input-file",
      "Input report file. Default is standard input.", input_file_path, "PATH");
    OptionAs<StringValue> output_file_opt(arg_parser, "output-file",
      "Output file. Default is standard output.", input_file_path, "PATH");

    OptionAs<ChoiceValue<OutputType> > output_type_opt(arg_parser, "output-type",
      "Specifies type of data to parse.", output_type, "");
    output_type_opt.handler.add_choice("single-statistic", OutputType::SingleStatistic,
      "Parses out single statistic.");
    output_type_opt.handler.add_choice("time-series", OutputType::TimeSeries,
      "Parses out time-series data.");
    output_type_opt.handler.add_choice("summary", OutputType::Summary,
      "Parses out summary data.");

    OptionAs<ChoiceValue<OutputFormat> > output_format_opt(arg_parser, "output-format",
      "Specifies format of output.", output_format, "");
    output_format_opt.handler.add_choice("stat-block", OutputFormat::StatBlock,
      "Formats output for printing a single consolidated stat block.");
    output_format_opt.handler.add_choice("gnuplot", OutputFormat::Gnuplot,
      "Formats output for plotting with gnuplot.");
    output_format_opt.handler.add_choice("json", OutputFormat::Json,
      "Formats output for json.");

    OptionAs<RemainingArgs> tags_opt(arg_parser, "tags",
      "Specifies the status block types to output.",
      parse_parameters.tags, "TAG...");

    OptionAs<RemainingArgs> stats_opt(arg_parser, "stats",
      "Specifies the status block value types to output.",
      parse_parameters.stats, "STATUS_TYPE...");

    OptionAs<RemainingArgs> values_opt(arg_parser, "values",
      "Specifies the status block values to output.",
      parse_parameters.values, "STATUS_VALUE...");

    arg_parser.parse(argc, argv);
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

            if (std::sscanf(line.c_str(), "[ %lu] %lf-%lf %s %lf %s %lf %s %lf %s %lf/ %lf (%lf%%) %lf/ %lf/ %lf/ %lf %s %lf %s %lf", &id, &interval_start, &interval_end, interval_units, &transfer, transfer_units, &bandwidth, bandwidth_units, &jitter, jitter_units, &lost, &total, &lost_pct, &latency_avg, &latency_min, &latency_max, &latency_stdev, latency_units, &pps, pps_units, &netpwr) == 0) {
              continue;
            }

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
