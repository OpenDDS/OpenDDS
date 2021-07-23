#pragma once

#include <PropertyStatBlock.h>
#include <util.h>

namespace Bench {

typedef std::map<std::string, std::vector<SimpleStatBlock> > stat_vec_map;

typedef std::map<std::string, SimpleStatBlock> SimpleStatBlockMap;

struct SharedSummaryReportVisitor : public ReportVisitor
{
  struct ErrorCounts {
    ErrorCounts() : total_(0), discovery_(0) {}
    uint64_t total_;
    uint64_t discovery_;
  };

  std::unordered_set<std::string> stats_;
  std::unordered_set<std::string> tags_;
  stat_vec_map untagged_stat_vecs_;
  std::map<std::string, stat_vec_map> tagged_stat_vecs_;
  ErrorCounts untagged_error_counts_;
  std::map<std::string, ErrorCounts> tagged_error_counts_;

  SharedSummaryReportVisitor();

  void on_node_controller_report(const ReportVisitorContext& context) override;
  void on_datareader_report(const ReportVisitorContext& context) override;
  void on_datawriter_report(const ReportVisitorContext& context) override;
};

}
