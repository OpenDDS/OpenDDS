#pragma once

#include <PropertyStatBlock.h>
#include <util.h>

namespace Bench {

typedef std::map<std::string, std::vector<SimpleStatBlock> > stat_vec_map;

struct SharedSummaryReportVisitor : public ReportVisitor
{
  std::unordered_set<std::string>& stats_;
  std::unordered_set<std::string>& tags_;
  stat_vec_map& untagged_stat_vecs_;
  std::map<std::string, stat_vec_map>& tagged_stat_vecs_;

  SharedSummaryReportVisitor(std::unordered_set<std::string>& stats,
                             std::unordered_set<std::string>& tags,
                             stat_vec_map& untagged_stat_vecs,
                             std::map<std::string, stat_vec_map>& tagged_stat_vecs);

  void on_node_controller_report(const ReportVisitorContext& context) override;
  void on_datareader_report(const ReportVisitorContext& context) override;
  void on_datawriter_report(const ReportVisitorContext& context) override;
};

}
