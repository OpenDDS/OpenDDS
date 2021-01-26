#include "time_series_gnuplot_formatter.h"
#include <PropertyStatBlock.h>

using namespace Bench;

int TimeSeriesGnuPlotFormatter::format(const Report& report, std::ofstream& output_file_stream) {
  if (report.node_reports.length() > 0) {
    output_header(report, output_file_stream);
    output_data(report, output_file_stream);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

void TimeSeriesGnuPlotFormatter::output_header(const Report& report, std::ofstream& output_file_stream) {
  const bool show_postfix = report.node_reports.length() > 1;

  // A gnuplot data file header comment line begins with # character.
  output_file_stream << "#";

  // Write header line.
  for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
    if (node_index > 0) {
      output_file_stream << " ";
    }
    output_file_stream << "cpu_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    output_file_stream << " cpu_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");
    output_file_stream << " mem_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    output_file_stream << " mem_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");
    output_file_stream << " virtual_mem_percent_median" << (show_postfix ? std::to_string(node_index + 1) : "");
    output_file_stream << " virtual_mem_percent_timestamp" << (show_postfix ? std::to_string(node_index + 1) : "");

  }

  output_file_stream << std::endl;
}

void TimeSeriesGnuPlotFormatter::output_data(const Report& report, std::ofstream& output_file_stream) {
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
        output_file_stream << consolidated_cpu_percent_stats.median_buffer_[index] << " ";

      if (index < cpu_percent_timestamp_count)
        output_file_stream << consolidated_cpu_percent_stats.timestamp_buffer_[index] << " ";

      if (index < mem_percent_median_count)
        output_file_stream << consolidated_mem_percent_stats.median_buffer_[index] << " ";

      if (index < mem_percent_timestamp_count)
        output_file_stream << consolidated_mem_percent_stats.timestamp_buffer_[index] << " ";

      if (index < virtual_mem_percent_median_count)
        output_file_stream << consolidated_virtual_mem_percent_stats.median_buffer_[index] << " ";

      if (index < virtual_mem_percent_timestamp_count)
        output_file_stream << consolidated_virtual_mem_percent_stats.timestamp_buffer_[index];

      if (node_index + 1 < report.node_reports.length())
        output_file_stream << " ";
    }

    output_file_stream << std::endl;
  }
}
