#include "TimeSeriesGnuPlotFormatter.h"

#include <PropertyStatBlock.h>
#include <util.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Bench;

int TimeSeriesGnuPlotFormatter::format(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  // TODO These functions need to be made generic for requested stats / tags (see other formatters)

  if (report.node_reports.length() > 0) {
    output_header(report, output_stream, parse_parameters);
    output_data(report, output_stream, parse_parameters);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

void TimeSeriesGnuPlotFormatter::output_header(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  const bool show_postfix = report.node_reports.length() > 1;

  // A gnuplot data file header comment line begins with # character.
  output_stream << "#";

  // Write header line.
  std::cerr << "Writing header..." << std::endl;
  for (unsigned int ni = 0; ni < report.node_reports.length(); ni++) {
    if (ni > 0) {
      output_stream << " ";
    }
    output_stream << "cpu_percent_median" << (show_postfix ? std::to_string(ni + 1) : "");
    output_stream << " cpu_percent_timestamp" << (show_postfix ? std::to_string(ni + 1) : "");
    output_stream << " mem_percent_median" << (show_postfix ? std::to_string(ni + 1) : "");
    output_stream << " mem_percent_timestamp" << (show_postfix ? std::to_string(ni + 1) : "");
    output_stream << " virtual_mem_percent_median" << (show_postfix ? std::to_string(ni + 1) : "");
    output_stream << " virtual_mem_percent_timestamp" << (show_postfix ? std::to_string(ni + 1) : "");

    std::unordered_set<std::string> tags;

    for (unsigned int wi = 0; wi < report.node_reports[ni].worker_reports.length(); wi++) {
      for (unsigned int pi = 0; pi < report.node_reports[ni].worker_reports[wi].process_report.participants.length(); pi++) {
        for (unsigned int si = 0; si < report.node_reports[ni].worker_reports[wi].process_report.participants[pi].subscribers.length(); si++) {
          for (unsigned int di = 0; di < report.node_reports[ni].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders.length(); di++) {
            for (unsigned int ti = 0; ti < report.node_reports[ni].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders[di].tags.length(); ti++) {
              const Builder::DataReaderReport& dr_report = report.node_reports[ni].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders[di];
              std::string tag = dr_report.tags[ti].in();
              if (std::find(parse_parameters.tags.begin(), parse_parameters.tags.end(), tag) != parse_parameters.tags.end()) {
                for (unsigned int property_index = 0; property_index < dr_report.properties.length(); property_index++) {
                  Builder::Property prop = dr_report.properties[property_index];
                  const Builder::PropertyValueKind prop_kind = prop.value._d();
                  if (prop_kind == Builder::PropertyValueKind::PVK_DOUBLE || prop_kind == Builder::PropertyValueKind::PVK_ULL) {
                    std::string prop_name = prop.name.in();
                    if (std::find(parse_parameters.values.begin(), parse_parameters.values.end(), prop_name) != parse_parameters.values.end()) {
                      std::string tagged_property = tag + "_" + prop_name;
                      tags.insert(tagged_property);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    for (std::unordered_set<std::string>::iterator it = tags.begin(); it != tags.end(); ++it) {
      output_stream << " " << *it << (show_postfix ? std::to_string(ni + 1) : "");
    }
  }

  output_stream << std::endl;
}

void TimeSeriesGnuPlotFormatter::output_data(const Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
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
        output_stream << cpu_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_stream << "- ";
      }

      if (index < cpu_percent_timestamp_count) {
        output_stream << cpu_percent_stats[node_index].timestamp_buffer_[index] << " ";
      } else {
        output_stream << "- ";
      }

      if (index < mem_percent_median_count) {
        output_stream << mem_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_stream << "- ";
      }

      if (index < mem_percent_timestamp_count) {
        output_stream << mem_percent_stats[node_index].timestamp_buffer_[index] << " ";
      } else {
        output_stream << "- ";
      }

      if (index < virtual_mem_percent_median_count) {
        output_stream << virtual_mem_percent_stats[node_index].median_buffer_[index] << " ";
      } else {
        output_stream << "- ";
      }

      if (index < virtual_mem_percent_timestamp_count) {
        output_stream << virtual_mem_percent_stats[node_index].timestamp_buffer_[index];
      } else {
        output_stream << "-";
      }

      if (index == 0) {
        std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> prop_maps;

        for (unsigned int wi = 0; wi < report.node_reports[node_index].worker_reports.length(); wi++) {
          for (unsigned int pi = 0; pi < report.node_reports[node_index].worker_reports[wi].process_report.participants.length(); pi++) {
            for (unsigned int si = 0; si < report.node_reports[node_index].worker_reports[wi].process_report.participants[pi].subscribers.length(); si++) {
              for (unsigned int di = 0; di < report.node_reports[node_index].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders.length(); di++) {
                for (unsigned int ti = 0; ti < report.node_reports[node_index].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders[di].tags.length(); ti++) {
                  const Builder::DataReaderReport& dr_report = report.node_reports[node_index].worker_reports[wi].process_report.participants[pi].subscribers[si].datareaders[di];
                  std::string tag = dr_report.tags[ti].in();
                  if (std::find(parse_parameters.tags.begin(), parse_parameters.tags.end(), tag) != parse_parameters.tags.end()) {
                    for (unsigned int property_index = 0; property_index < dr_report.properties.length(); property_index++) {
                      Builder::Property prop = dr_report.properties[property_index];
                      const Builder::PropertyValueKind prop_kind = prop.value._d();
                      if (prop_kind == Builder::PropertyValueKind::PVK_DOUBLE || prop_kind == Builder::PropertyValueKind::PVK_ULL) {
                        std::string prop_name = prop.name.in();
                        if (std::find(parse_parameters.values.begin(), parse_parameters.values.end(), prop_name) != parse_parameters.values.end()) {
                          std::unordered_map<std::string, uint64_t> prop_values;
                          const Builder::ConstPropertyIndex prop_index = get_property(dr_report.properties, prop_name, prop_kind);
                          if (prop_index) {
                            std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>>::const_iterator it = prop_maps.find(prop_name);
                            if (it != prop_maps.end()) {
                              std::unordered_map<std::string, uint64_t> map = it->second;
                              update_stats_for_tags(map, dr_report.tags, parse_parameters.tags, prop_index);
                            }
                            else {
                              std::unordered_map<std::string, uint64_t> map;
                              update_stats_for_tags(map, dr_report.tags, parse_parameters.tags, prop_index);
                              prop_maps.insert(std::pair<std::string, std::unordered_map<std::string, uint64_t>>(prop_name, map));
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }

        for (std::unordered_set<std::string>::const_iterator tag_it = parse_parameters.tags.begin(); tag_it != parse_parameters.tags.end(); tag_it++) {
          std::string tag = *tag_it;
          for (std::unordered_set<std::string>::const_iterator value_it = parse_parameters.values.begin(); value_it != parse_parameters.values.end(); value_it++) {
            std::string value = *value_it;
            for (std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>>::const_iterator it = prop_maps.begin(); it != prop_maps.end(); it++) {
              if (it->first == value) {
                std::unordered_map<std::string, uint64_t> prop_map = it->second;
                output_stream << prop_map[tag] << " ";
                break;
              }
            }
          }
        }
      }

      if (node_index + 1 < report.node_reports.length())
        output_stream << " ";
    }

    output_stream << std::endl;
  }
}
