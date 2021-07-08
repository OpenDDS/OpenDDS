#pragma once

#include "util.h"

namespace Bench {

struct StatsAndTagsVisitor : public ReportVisitor
{
  std::unordered_set<std::string>& stat_names_;
  std::unordered_set<std::string>& tag_names_;

  StatsAndTagsVisitor(std::unordered_set<std::string>& stat_names, std::unordered_set<std::string>& tag_names);

  void on_node_controller_report(const ReportVisitorContext& context) override;
  void on_datareader_report(const ReportVisitorContext& context) override;
  void on_datawriter_report(const ReportVisitorContext& context) override;
};

} // namespace Bench
