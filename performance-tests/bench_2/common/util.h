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
std::string Bench_Common_Export get_option_argument(int& i, int argc, ACE_TCHAR* argv[]);
int Bench_Common_Export get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[]);
unsigned Bench_Common_Export get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[]);

// Filesytem
std::string Bench_Common_Export join_path(const std::string& arg);
template <typename... Args>
std::string join_path(const std::string& arg, Args... args) {
  return arg + (arg.back() == ACE_DIRECTORY_SEPARATOR_CHAR_A ? "" : ACE_DIRECTORY_SEPARATOR_STR_A) + join_path(args...);
}

std::string Bench_Common_Export create_temp_dir(const std::string& prefix);
std::vector<std::string> Bench_Common_Export get_dir_contents(const std::string& path);
bool Bench_Common_Export file_exists(const std::string& path);

/**
 * Get Current UTC Time in ISO8601
 */
std::string Bench_Common_Export iso8601(const std::chrono::system_clock::time_point& tp = std::chrono::system_clock::now());

uint32_t Bench_Common_Export one_at_a_time_hash(const uint8_t* key, size_t length);

void Bench_Common_Export update_stats_for_tags(std::unordered_map<std::string, uint64_t>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Builder::ConstPropertyIndex& prop);

void Bench_Common_Export update_details_for_tags(std::unordered_map<std::string, std::string>& details,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const std::string& detail);

void Bench_Common_Export consolidate_tagged_stats(std::unordered_map<std::string, Bench::SimpleStatBlock>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Bench::ConstPropertyStatBlock& data);
}

#endif
