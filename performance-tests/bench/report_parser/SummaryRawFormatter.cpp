#include "SummaryRawFormatter.h"

#include "SharedSummaryReportVisitor.h"

namespace Bench {

int SummaryRawFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{

  SharedSummaryReportVisitor visitor;

  Bench::gather_stats_and_tags(report, visitor.stats_, visitor.tags_);

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
  const auto& untagged_error_counts = visitor.untagged_error_counts_;
  const auto& tagged_error_counts = visitor.tagged_error_counts_;

  output_stream << std::endl;
  output_stream << "Total Errors: " << untagged_error_counts.total_ << std::endl;
  output_stream << "Discovery Errors: " << untagged_error_counts.discovery_ << std::endl;

  for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
    auto stat_pos = untagged_stat_vecs.find(*stat_it);
    if (stat_pos != untagged_stat_vecs.end()) {
      output_stream << std::endl;
      consolidate(stat_pos->second).pretty_print(output_stream, *stat_it);
    }
  }
  for (auto tags_it = tags.begin(); tags_it != tags.end(); ++tags_it) {
    auto tag_pos = tagged_stat_vecs.find(*tags_it);
    if (tag_pos != tagged_stat_vecs.end()) {
      output_stream << std::endl;
      output_stream << "---=== Stats For Tag '" << *tags_it << "' ===---" << std::endl;

      auto tagged_error_counts_pos = tagged_error_counts.find(tag_pos->first);
      auto tagged_error_counts_copy = tagged_error_counts_pos == tagged_error_counts.end() ? SharedSummaryReportVisitor::ErrorCounts() : tagged_error_counts_pos->second;
      output_stream << std::endl;
      output_stream << "Total Errors: " << tagged_error_counts_copy.total_ << std::endl;
      output_stream << "Discovery Errors: " << tagged_error_counts_copy.discovery_ << std::endl;

      for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
        auto stat_pos = tag_pos->second.find(*stat_it);
        if (stat_pos != tag_pos->second.end()) {
          output_stream << std::endl;
          consolidate(stat_pos->second).pretty_print(output_stream, *stat_it);
        }
      }

    }
  }
  return EXIT_SUCCESS;
}

} // namespace Bench
