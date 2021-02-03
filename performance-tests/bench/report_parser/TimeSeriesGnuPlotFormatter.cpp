#include "TimeSeriesGnuPlotFormatter.h"

#include <PropertyStatBlock.h>

using namespace Bench;

int TimeSeriesGnuPlotFormatter::format(const Report& report, std::ofstream& output_file_stream,
    ParseParameters& parseParameters)
{
  if (report.node_reports.length() > 0) {
    output_header(report, output_file_stream);
    output_data(report, output_file_stream);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

void TimeSeriesGnuPlotFormatter::output_header(const Report& report, std::ofstream& output_file_stream)
{
  const bool show_postfix = report.node_reports.length() > 1;

  // A gnuplot data file header comment line begins with # character.
  output_file_stream << "#";

  // Write header line.
  std::cerr << "Writing header..." << std::endl;
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

void TimeSeriesGnuPlotFormatter::output_data(const Report& report, std::ofstream& output_file_stream)
{
  std::vector<Bench::SimpleStatBlock> cpu_percent_stats;
  std::vector<Bench::SimpleStatBlock> mem_percent_stats;
  std::vector<Bench::SimpleStatBlock> virtual_mem_percent_stats;

  // Consolidate data and determine the biggest buffer size.
  size_t biggest_buffer_size = 0;

  for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
    std::cout << "Consolidating report " << node_index + 1 << "/" << report.node_reports.length() << std::endl;

    const Bench::TestController::NodeReport& nc_report = report.node_reports[node_index];

    cpu_percent_stats.push_back(Bench::ConstPropertyStatBlock(nc_report.properties, "cpu_percent").to_simple_stat_block());
    mem_percent_stats.push_back(Bench::ConstPropertyStatBlock(nc_report.properties, "mem_percent").to_simple_stat_block());
    virtual_mem_percent_stats.push_back(Bench::ConstPropertyStatBlock(nc_report.properties, "virtual_mem_percent").to_simple_stat_block());

    if (cpu_percent_stats[node_index].median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = cpu_percent_stats[node_index].median_buffer_.size();

    if (cpu_percent_stats[node_index].timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = cpu_percent_stats[node_index].timestamp_buffer_.size();

    if (mem_percent_stats[node_index].median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = mem_percent_stats[node_index].median_buffer_.size();

    if (mem_percent_stats[node_index].timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = mem_percent_stats[node_index].timestamp_buffer_.size();

    if (virtual_mem_percent_stats[node_index].median_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = virtual_mem_percent_stats[node_index].median_buffer_.size();

    if (virtual_mem_percent_stats[node_index].timestamp_buffer_.size() > biggest_buffer_size)
      biggest_buffer_size = virtual_mem_percent_stats[node_index].timestamp_buffer_.size();
  }

  for (unsigned int index = 0; index < biggest_buffer_size; index++) {
    for (unsigned int node_index = 0; node_index < report.node_reports.length(); node_index++) {
      std::cout << "Processing line " << index + 1 << "/" << biggest_buffer_size
        << ", column " << node_index + 1 << "/" << report.node_reports.length() << std::endl;

      const size_t cpu_percent_median_count = cpu_percent_stats[node_index].median_buffer_.size();
      const size_t cpu_percent_timestamp_count = cpu_percent_stats[node_index].timestamp_buffer_.size();

      const size_t mem_percent_median_count = mem_percent_stats[node_index].median_buffer_.size();
      const size_t mem_percent_timestamp_count = mem_percent_stats[node_index].timestamp_buffer_.size();

      const size_t virtual_mem_percent_median_count = virtual_mem_percent_stats[node_index].median_buffer_.size();
      const size_t virtual_mem_percent_timestamp_count = virtual_mem_percent_stats[node_index].timestamp_buffer_.size();

      if (index < cpu_percent_median_count) {
        output_file_stream << cpu_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_file_stream << "- ";
      }

      if (index < cpu_percent_timestamp_count) {
        output_file_stream << cpu_percent_stats[node_index].timestamp_buffer_[index] << " ";
      } else {
        output_file_stream << "- ";
      }

      if (index < mem_percent_median_count) {
        output_file_stream << mem_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_file_stream << "- ";
      }

      if (index < mem_percent_timestamp_count) {
        output_file_stream << mem_percent_stats[node_index].timestamp_buffer_[index] << " ";
      } else {
        output_file_stream << "- ";
      }

      if (index < virtual_mem_percent_median_count) {
        output_file_stream << virtual_mem_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_file_stream << "- ";
      }

      if (index < virtual_mem_percent_timestamp_count) {
        output_file_stream << virtual_mem_percent_stats[node_index].timestamp_buffer_[index];
      } else {
        output_file_stream << "-";
      }

      if (node_index + 1 < report.node_reports.length())
        output_file_stream << " ";
    }

    output_file_stream << std::endl;
  }
}
