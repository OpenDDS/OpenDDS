#include "SummaryJsonDashboardFormatter.h"

#include "SharedSummaryReportVisitor.h"

namespace Bench {

int SummaryJsonDashboardFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
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

  stat_vec_map untagged_stat_vecs;
  std::map<std::string, stat_vec_map> tagged_stat_vecs;

  SharedSummaryReportVisitor visitor(stats, tags, untagged_stat_vecs, tagged_stat_vecs);

  visit_report(report, visitor);

  rapidjson::Document document;
  document.SetObject();

  rapidjson::Value& untagged_stat_val = document.AddMember("stats", rapidjson::Value(0).Move(), document.GetAllocator())["stats"].SetObject();

  for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
    auto stat_pos = untagged_stat_vecs.find(*stat_it);
    if (stat_pos != untagged_stat_vecs.end()) {

      consolidate(stat_pos->second).to_json_summary(*stat_it, untagged_stat_val, document.GetAllocator());
    }
  }

  rapidjson::Value& tags_val = document.AddMember("tags", rapidjson::Value(0).Move(), document.GetAllocator())["tags"].SetObject();

  for (auto tags_it = tags.begin(); tags_it != tags.end(); ++tags_it) {
    auto tag_pos = tagged_stat_vecs.find(*tags_it);
    if (tag_pos != tagged_stat_vecs.end()) {

      rapidjson::Value& tag_val = tags_val.AddMember(rapidjson::StringRef(tag_pos->first.c_str()), rapidjson::Value(0).Move(), document.GetAllocator())[tag_pos->first.c_str()].SetObject();
      rapidjson::Value& tagged_stat_val = tag_val.AddMember("stats", rapidjson::Value(0).Move(), document.GetAllocator())["stats"].SetObject();

      for (auto stat_it = stats.begin(); stat_it != stats.end(); ++stat_it) {
        auto stat_pos = tag_pos->second.find(*stat_it);
        if (stat_pos != tag_pos->second.end()) {

          consolidate(stat_pos->second).to_json_summary(*stat_it, tagged_stat_val, document.GetAllocator());
        }
      }

    }
  }

  rapidjson::OStreamWrapper osw(output_stream);
  rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
  writer.SetMaxDecimalPlaces(6);

  document.Accept(writer);

  osw.Flush();

  output_stream << std::endl;

  return EXIT_SUCCESS;
}

} // namespace Bench
