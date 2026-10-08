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

  // Warnings indicate conditions worth investigating which aren't
  // necessarily caused by the test (and so aren't counted as errors)
  struct WarningCounts {
    WarningCounts() : host_network_(0), host_network_unavailable_(0) {}
    uint64_t host_network_; // nodes with non-zero host network error or drop counts
    uint64_t host_network_unavailable_; // nodes which couldn't collect host network counters
  };

  std::unordered_set<std::string> stats_;
  std::unordered_set<std::string> tags_;
  stat_vec_map untagged_stat_vecs_;
  std::map<std::string, stat_vec_map> tagged_stat_vecs_;
  ErrorCounts untagged_error_counts_;
  std::map<std::string, ErrorCounts> tagged_error_counts_;
  WarningCounts untagged_warning_counts_;

  SharedSummaryReportVisitor();

  void on_node_controller_report(const ReportVisitorContext& context) override;
  void on_datareader_report(const ReportVisitorContext& context) override;
  void on_datawriter_report(const ReportVisitorContext& context) override;
};

}
