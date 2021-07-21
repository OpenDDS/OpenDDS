#ifndef BENCH_UTIL_HEADER
#define BENCH_UTIL_HEADER

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <ace/ace_wchar.h> // For ACE_TCHAR
#include <ace/Default_Constants.h> // For ACE_DIRECTORY_SEPARATOR_*
#include <ace/SString.h>
#include "ace/Time_Value.h"

#include "PropertyStatBlock.h"
#include "Bench_Common_Export.h"

const ACE_Time_Value ZERO_TIME(0, 0);

namespace Bench {

// Argument Parsing
Bench_Common_Export std::string get_option_argument(int& i, int argc, ACE_TCHAR* argv[]);
Bench_Common_Export int get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[]);
Bench_Common_Export unsigned get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[]);

Bench_Common_Export std::string& string_replace(std::string& input, const std::string& oldstr, const std::string& newstr);

// Filesytem
inline std::string join_path(const std::string& arg)
{
  return arg;
}

template <typename... Args>
std::string join_path(const std::string& arg, Args... args)
{
  return arg + (arg.back() == ACE_DIRECTORY_SEPARATOR_CHAR_A ? "" : ACE_DIRECTORY_SEPARATOR_STR_A) + join_path(args...);
}

Bench_Common_Export std::string create_temp_dir(const std::string& prefix);
Bench_Common_Export std::vector<std::string> get_dir_contents(const std::string& path);
Bench_Common_Export bool file_exists(const std::string& path);

/**
 * Get Current UTC Time in ISO8601
 */
Bench_Common_Export std::string iso8601(const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now());

Bench_Common_Export uint32_t one_at_a_time_hash(const uint8_t* key, size_t length);

Bench_Common_Export void update_stats_for_tags(std::unordered_map<std::string, uint64_t>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Builder::ConstPropertyIndex& prop);

Bench_Common_Export void update_details_for_tags(std::unordered_map<std::string, std::string>& details,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const std::string& detail);

Bench_Common_Export void consolidate_tagged_stats(std::unordered_map<std::string, Bench::SimpleStatBlock>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Bench::ConstPropertyStatBlock& data);

struct Bench_Common_Export ReportVisitorContext {
  explicit ReportVisitorContext(const Bench::TestController::NodeReport& nc_report)
    : nc_report_(&nc_report)
    , datareader_report_(nullptr)
    , datawriter_report_(nullptr)
  {}
  ReportVisitorContext(const Bench::TestController::NodeReport& nc_report,
                       const Builder::DataReaderReport& datareader_report)
    : nc_report_(&nc_report)
    , datareader_report_(&datareader_report)
    , datawriter_report_(nullptr)
  {}
  ReportVisitorContext(const Bench::TestController::NodeReport& nc_report,
                       const Builder::DataWriterReport& datawriter_report)
    : nc_report_(&nc_report)
    , datareader_report_(nullptr)
    , datawriter_report_(&datawriter_report)
  {}

  const Bench::TestController::NodeReport* nc_report_;
  const Builder::DataReaderReport* datareader_report_;
  const Builder::DataWriterReport* datawriter_report_;
};

struct Bench_Common_Export ReportVisitor {
  virtual ~ReportVisitor() {}
  virtual void on_node_controller_report(const ReportVisitorContext& context) = 0;
  virtual void on_datareader_report(const ReportVisitorContext& context) = 0;
  virtual void on_datawriter_report(const ReportVisitorContext& context) = 0;
};

Bench_Common_Export void visit_report(const TestController::Report& report, ReportVisitor& visitor);

Bench_Common_Export void gather_stats_and_tags(const TestController::Report& report,
  std::unordered_set<std::string>& stat_names,
  std::unordered_set<std::string>& tag_names);
}

#endif
