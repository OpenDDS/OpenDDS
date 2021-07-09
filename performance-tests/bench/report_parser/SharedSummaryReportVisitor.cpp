#include "SharedSummaryReportVisitor.h"

namespace Bench {

SharedSummaryReportVisitor::SharedSummaryReportVisitor()
{
}

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
  Builder::ConstPropertyIndex et_cpi = get_property(context.datareader_report_->properties, "enable_time", Builder::PVK_TIME);
  const Builder::TimeStamp et = et_cpi ? et_cpi->value.time_prop() : Builder::ZERO;

  Builder::ConstPropertyIndex ldt_cpi = get_property(context.datareader_report_->properties, "last_discovery_time", Builder::PVK_TIME);
  const Builder::TimeStamp ldt = ldt_cpi ? ldt_cpi->value.time_prop() : Builder::ZERO;

  bool correctly_matched = Builder::ZERO < et && Builder::ZERO < ldt;
  if (!correctly_matched) {
    ++untagged_error_counts_.total_;
    ++untagged_error_counts_.discovery_;
    for (unsigned int tags_index = 0; tags_index < context.datareader_report_->tags.length(); ++tags_index) {
      const std::string tag(context.datareader_report_->tags[tags_index]);
      if (tags_.find(tag) != tags_.end()) {
        ErrorCounts& ec = tagged_error_counts_[tag];
        ++ec.total_;
        ++ec.discovery_;
      }
    }
  }

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
  Builder::ConstPropertyIndex et_cpi = get_property(context.datawriter_report_->properties, "enable_time", Builder::PVK_TIME);
  const Builder::TimeStamp et = et_cpi ? et_cpi->value.time_prop() : Builder::ZERO;

  Builder::ConstPropertyIndex ldt_cpi = get_property(context.datawriter_report_->properties, "last_discovery_time", Builder::PVK_TIME);
  const Builder::TimeStamp ldt = ldt_cpi ? ldt_cpi->value.time_prop() : Builder::ZERO;

  bool correctly_matched = Builder::ZERO < et && Builder::ZERO < ldt;
  if (!correctly_matched) {
    ++untagged_error_counts_.total_;
    ++untagged_error_counts_.discovery_;
    for (unsigned int tags_index = 0; tags_index < context.datawriter_report_->tags.length(); ++tags_index) {
      const std::string tag(context.datawriter_report_->tags[tags_index]);
      if (tags_.find(tag) != tags_.end()) {
        ErrorCounts& ec = tagged_error_counts_[tag];
        ++ec.total_;
        ++ec.discovery_;
      }
    }
  }

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
