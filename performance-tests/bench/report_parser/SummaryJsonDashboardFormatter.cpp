#include "SummaryJsonDashboardFormatter.h"

#include "SharedSummaryReportVisitor.h"

namespace Bench {

int SummaryJsonDashboardFormatter::format(const Bench::TestController::Report& report, std::ostream& output_stream, const ParseParameters& parse_parameters)
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

  rapidjson::Document document;
  document.SetObject();

  rapidjson::Value& errors_val = document.AddMember("errors", rapidjson::Value(0).Move(), document.GetAllocator())["errors"].SetObject();
  errors_val.AddMember("total", rapidjson::Value(untagged_error_counts.total_).Move(), document.GetAllocator());
  errors_val.AddMember("discovery", rapidjson::Value(untagged_error_counts.discovery_).Move(), document.GetAllocator());

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

      auto tagged_error_counts_pos = tagged_error_counts.find(tag_pos->first);
      auto tagged_error_counts_copy = tagged_error_counts_pos == tagged_error_counts.end() ? SharedSummaryReportVisitor::ErrorCounts() : tagged_error_counts_pos->second;
      rapidjson::Value& tagged_errors_val = tag_val.AddMember("errors", rapidjson::Value(0).Move(), document.GetAllocator())["errors"].SetObject();
      tagged_errors_val.AddMember("total", rapidjson::Value(tagged_error_counts_copy.total_).Move(), document.GetAllocator());
      tagged_errors_val.AddMember("discovery", rapidjson::Value(tagged_error_counts_copy.discovery_).Move(), document.GetAllocator());

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
