#include "time_series_raw_formatter.h"
#include <PropertyStatBlock.h>

int TimeSeriesRawFormatter::format(const Report& report, std::ofstream& output_file_stream) {
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

    output_file_stream << "CPU median buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_cpu_percent_stats.median_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_cpu_percent_stats.median_buffer_[i] << std::endl;
    }

    output_file_stream << "CPU timestamp buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_cpu_percent_stats.timestamp_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_cpu_percent_stats.timestamp_buffer_[i] << std::endl;
    }

    output_file_stream << "Mem median buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_mem_percent_stats.median_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_mem_percent_stats.median_buffer_[i] << std::endl;
    }

    output_file_stream << "Mem timestamp buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_mem_percent_stats.timestamp_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_mem_percent_stats.timestamp_buffer_[i] << std::endl;
    }

    output_file_stream << "Virtual mem median buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_virtual_mem_percent_stats.median_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_virtual_mem_percent_stats.median_buffer_[i] << std::endl;
    }

    output_file_stream << "Virtual mem timestamp buffer values:" << std::endl;
    for (size_t i = 0; i != consolidated_virtual_mem_percent_stats.timestamp_buffer_.size(); i++) {
      output_file_stream << "\t" << consolidated_virtual_mem_percent_stats.timestamp_buffer_[i] << std::endl;
    }
  }

  return EXIT_SUCCESS;
}
