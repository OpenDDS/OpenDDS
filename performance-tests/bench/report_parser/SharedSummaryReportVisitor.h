#pragma once

#include <PropertyStatBlock.h>
#include <util.h>

namespace Bench {

typedef std::map<std::string, std::vector<SimpleStatBlock> > stat_vec_map;

struct SharedSummaryReportVisitor : public ReportVisitor
{
  std::unordered_set<std::string> stats_;
  std::unordered_set<std::string> tags_;
  stat_vec_map untagged_stat_vecs_;
  std::map<std::string, stat_vec_map> tagged_stat_vecs_;
  uint64_t untagged_error_count_;
  std::map<std::string, uint64_t> tagged_error_counts_;

  SharedSummaryReportVisitor();

  void on_node_controller_report(const ReportVisitorContext& context) override;
  void on_datareader_report(const ReportVisitorContext& context) override;
  void on_datawriter_report(const ReportVisitorContext& context) override;
};

}
