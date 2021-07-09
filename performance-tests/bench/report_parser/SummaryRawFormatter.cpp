#include "SummaryRawFormatter.h"

#include "SharedSummaryReportVisitor.h"

namespace Bench {

int SummaryRawFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
{
  std::unordered_set<std::string> stats;
  std::unordered_set<std::string> tags;

  Bench::gather_stats_and_tags(report, stats, tags);

  if (!parse_parameters.tags.empty()) {
    tags = parse_parameters.tags;
  }

  if (!parse_parameters.stats.empty()) {
    stats = parse_parameters.stats;
  }

  typedef std::map<std::string, std::vector<Bench::SimpleStatBlock> > stat_vec_map;

  stat_vec_map untagged_stat_vecs;
  std::map<std::string, stat_vec_map> tagged_stat_vecs;

  SharedSummaryReportVisitor visitor(stats, tags, untagged_stat_vecs, tagged_stat_vecs);

  visit_report(report, visitor);

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
