#include "SharedSummaryReportVisitor.h"

namespace Bench {

SharedSummaryReportVisitor::SharedSummaryReportVisitor(std::unordered_set<std::string>& stats,
                                                       std::unordered_set<std::string>& tags,
                                                       stat_vec_map& untagged_stat_vecs,
                                                       std::map<std::string, stat_vec_map>& tagged_stat_vecs)
  : stats_(stats)
  , tags_(tags)
  , untagged_stat_vecs_(untagged_stat_vecs)
  , tagged_stat_vecs_(tagged_stat_vecs)
{}

void SharedSummaryReportVisitor::on_node_controller_report(const ReportVisitorContext& context)
{
  for (auto it = stats_.begin(); it != stats_.end(); ++it) {
    ConstPropertyStatBlock cpsb(context.nc_report_->properties, *it);
    if (cpsb) {
      untagged_stat_vecs_[*it].push_back(cpsb.to_simple_stat_block());
    }
  }
}

void SharedSummaryReportVisitor::on_datareader_report(const ReportVisitorContext& context)
{
  for (auto it = stats_.begin(); it != stats_.end(); ++it) {
    ConstPropertyStatBlock cpsb(context.datareader_report_->properties, *it);
    if (cpsb) {
      const auto& ssb = cpsb.to_simple_stat_block();
      untagged_stat_vecs_[*it].push_back(ssb);
      for (unsigned int tags_index = 0; tags_index < context.datareader_report_->tags.length(); ++tags_index) {
        const std::string tag(context.datareader_report_->tags[tags_index]);
        if (tags_.find(tag) != tags_.end()) {
          tagged_stat_vecs_[tag][*it].push_back(ssb);
        }
      }
    }
  }
}

void SharedSummaryReportVisitor::on_datawriter_report(const ReportVisitorContext& context)
{
  for (auto it = stats_.begin(); it != stats_.end(); ++it) {
    ConstPropertyStatBlock cpsb(context.datawriter_report_->properties, *it);
    if (cpsb) {
      const auto& ssb = cpsb.to_simple_stat_block();
      untagged_stat_vecs_[*it].push_back(ssb);
      for (unsigned int tags_index = 0; tags_index < context.datawriter_report_->tags.length(); ++tags_index) {
        const std::string tag(context.datawriter_report_->tags[tags_index]);
        if (tags_.find(tag) != tags_.end()) {
          tagged_stat_vecs_[tag][*it].push_back(ssb);
        }
      }
    }
  }
}

} // namespace Bench
