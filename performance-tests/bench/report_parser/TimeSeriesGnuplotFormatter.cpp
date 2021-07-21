#include "TimeSeriesGnuplotFormatter.h"

#include "SharedSummaryReportVisitor.h"

#include <PropertyStatBlock.h>
#include <util.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace Bench;

namespace {

void write_series(const SimpleStatBlockMap& stats, std::ostream& output_stream, size_t rows) {
  // Write Header
  for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
    if (stat_it != stats.begin()) {
      output_stream << ",";
    }
    output_stream << stat_it->first;
  }
  output_stream << std::endl;

  // Write Data
  for (size_t i = 0; i < rows; ++i) {
    for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
      if (stat_it != stats.begin()) {
        output_stream << ",";
      }
      const size_t delta = rows - stat_it->second.median_buffer_.size();
      output_stream << (i < delta ? 0 : stat_it->second.median_buffer_[i - delta]);
    }
    output_stream << std::endl;
  }

}

}

int TimeSeriesGnuplotFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  SharedSummaryReportVisitor visitor;

  if (!parse_parameters.tags.empty()) {
    visitor.tags_ = parse_parameters.tags;
  }

  if (!parse_parameters.stats.empty()) {
    visitor.stats_ = parse_parameters.stats;
  }

  visit_report(report, visitor);

  const auto& tags = visitor.tags_;
  const auto& stats = visitor.stats_;
  const auto& untagged_stat_vecs = visitor.untagged_stat_vecs_;
  const auto& tagged_stat_vecs = visitor.tagged_stat_vecs_;

  SimpleStatBlockMap untagged_stat_map;
  std::map<std::string, SimpleStatBlockMap> tagged_stat_map;

  size_t untagged_rows = 0;
  for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
    auto stat_pos = untagged_stat_vecs.find(*stat_it);
    if (stat_pos != untagged_stat_vecs.end()) {
      untagged_stat_map[*stat_it] = consolidate(stat_pos->second);
      if (untagged_rows < untagged_stat_map[*stat_it].median_buffer_.size()) {
        untagged_rows = untagged_stat_map[*stat_it].median_buffer_.size();
      }
    }
  }

  write_series(untagged_stat_map, output_stream, untagged_rows);

  for (auto tags_it = tags.begin(); tags_it != tags.end(); ++tags_it) {
    auto tag_pos = tagged_stat_vecs.find(*tags_it);
    if (tag_pos != tagged_stat_vecs.end()) {
      size_t tagged_rows = 0;
      for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
        auto stat_pos = tag_pos->second.find(*stat_it);
        if (stat_pos != tag_pos->second.end()) {
          tagged_stat_map[*tags_it][*stat_it] = consolidate(stat_pos->second);
          if (tagged_rows < tagged_stat_map[*tags_it][*stat_it].median_buffer_.size()) {
            tagged_rows = tagged_stat_map[*tags_it][*stat_it].median_buffer_.size();
          }
        }
      }
      // Write gnuplot index seperator (two new lines)
      output_stream << std::endl << std::endl;

      write_series(tagged_stat_map[*tags_it], output_stream, tagged_rows);
    }
  }

  return EXIT_SUCCESS;
}
