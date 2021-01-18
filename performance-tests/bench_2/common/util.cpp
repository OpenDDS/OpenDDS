#include "util.h"
#include "PropertyStatBlock.h"

#include <iostream>
#include <exception>
#include <sstream>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

#include <ace/Lib_Find.h> // For ACE::get_temp_dir
#include <ace/OS_NS_string.h> // For ACE_OS::strcpy
#include <ace/OS_NS_sys_stat.h> // For ACE_OS::mkdir and ACE_OS::stat
#include <ace/OS_NS_stdlib.h> // For ACE_OS::mktemp
#include <ace/OS_NS_dirent.h> // For ACE_OS::opendir and friends

namespace Bench {

std::string get_option_argument(int& i, int argc, ACE_TCHAR* argv[])
{
  if (i == argc - 1) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i])<< " requires an argument" << std::endl;
    throw int{1};
  }
  return ACE_TEXT_ALWAYS_CHAR(argv[++i]);
}

int get_option_argument_int(int& i, int argc, ACE_TCHAR* argv[])
{
  int value;
  try {
    value = static_cast<int>(std::stoll(get_option_argument(i, argc, argv)));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i]) << " requires an argument that's a valid number" << std::endl;
    throw 1;
  }
  return value;
}

unsigned get_option_argument_uint(int& i, int argc, ACE_TCHAR* argv[])
{
  unsigned value;
  try {
    value = static_cast<unsigned>(std::stoull(get_option_argument(i, argc, argv)));
  } catch (const std::exception&) {
    std::cerr << "Option " << ACE_TEXT_ALWAYS_CHAR(argv[i])
      << " requires an argument that's a valid positive number" << std::endl;
    throw 1;
  }
  return value;
}

std::string join_path(const std::string& arg) {
  return arg;
}

std::string create_temp_dir(const std::string& prefix)
{
  // Create Template for mktemp
  ACE_TCHAR buffer[PATH_MAX];
  if (ACE::get_temp_dir(&buffer[0], PATH_MAX) == -1) {
    return "";
  }
  ACE_OS::strcpy(
    &buffer[0],
    ACE_TEXT_CHAR_TO_TCHAR(join_path(
      ACE_TEXT_ALWAYS_CHAR(&buffer[0]),
      (prefix + "_XXXXXX")).c_str()));

  // Fill the template and create the directory
  if (!ACE_OS::mktemp(buffer)) {
    return "";
  }
  if (ACE_OS::mkdir(buffer, S_IRWXU) == -1) {
    return "";
  }

  return ACE_TEXT_ALWAYS_CHAR(buffer);
}

std::string iso8601(const std::chrono::system_clock::time_point& tp)
{
  using namespace std::chrono;
  std::stringstream ss;
  const std::time_t now = system_clock::to_time_t(tp);
  char buf[sizeof "2011-10-08T07:07:09Z"]; // longest possible for UTC times (other zones add offset suffix)
  std::strftime(buf, sizeof buf, "%FT%TZ", std::gmtime(&now));
  ss << buf;
  return ss.str();
}

std::vector<std::string> get_dir_contents(const std::string& path)
{
  std::vector<std::string> rv;
  ACE_DIR* dir = ACE_OS::opendir(ACE_TEXT_CHAR_TO_TCHAR(path.c_str()));
  if (dir) {
    ACE_DIRENT* entry;
    while ((entry = ACE_OS::readdir(dir))) {
#ifdef ACE_HAS_TCHAR_DIRENT
      rv.push_back(ACE_TEXT_ALWAYS_CHAR(entry->d_name));
#else
      rv.push_back(entry->d_name);
#endif
    }
    ACE_OS::closedir(dir);
  } else {
    std::stringstream ss;
    ss << "getting contents of " << path << ": " << ACE_OS::strerror(errno);
    throw std::runtime_error(ss.str());
  }
  return rv;
}

bool file_exists(const std::string& path)
{
  ACE_stat stat_result;
  if (ACE_OS::stat(path.c_str(), &stat_result) == -1) {
    if (errno != ENOENT) {
      std::stringstream ss;
      ss << "checking if " << path << " exists: " << ACE_OS::strerror(errno);
      throw std::runtime_error(ss.str());
    }
    return false;
  }
  return true;
}

uint32_t one_at_a_time_hash(const uint8_t* key, size_t length)
{
  size_t i = 0;
  uint32_t hash = 0;
  while (i != length) {
    hash += key[i++];
    hash += hash << 10;
    hash ^= hash >> 6;
  }
  hash += hash << 3;
  hash ^= hash >> 11;
  hash += hash << 15;
  return hash;
}

void update_stats_for_tags(std::unordered_map<std::string, uint64_t>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Builder::ConstPropertyIndex& prop)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      if (stats.find(tag) == stats.end()) {
        stats[tag] = prop->value.ull_prop();
      }
      else {
        stats[tag] += prop->value.ull_prop();
      }
    }
  }
}

void update_details_for_tags(std::unordered_map<std::string, std::string>& details,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const std::string& detail)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      details[tag] += detail;
    }
  }
}

void consolidate_tagged_stats(std::unordered_map<std::string, Bench::SimpleStatBlock>& stats,
  const Builder::StringSeq& reported_tags,
  const std::unordered_set<std::string>& input_tags,
  const Bench::ConstPropertyStatBlock& data)
{
  for (CORBA::ULong i = 0; i < reported_tags.length(); ++i) {
    const std::string tag(reported_tags[i]);
    if (input_tags.find(tag) != input_tags.end()) {
      stats[tag] = consolidate(stats[tag], data.to_simple_stat_block());
    }
  }
}

int handle_report(const Bench::TestController::Report& report,
  const std::unordered_set<std::string>& tags,
  std::ostringstream& result_out)
{
  int result = EXIT_SUCCESS;
  using Builder::ZERO;

  Bench::SimpleStatBlock consolidated_cpu_percent_stats;
  Bench::SimpleStatBlock consolidated_mem_percent_stats;
  Bench::SimpleStatBlock consolidated_virtual_mem_percent_stats;

  std::vector<const Bench::WorkerReport*> parsed_reports;
  for (CORBA::ULong n = 0; n < report.node_reports.length(); ++n) {
    const Bench::TestController::NodeReport& nc_report = report.node_reports[n];

    Bench::ConstPropertyStatBlock cpu_percent(nc_report.properties, "cpu_percent");
    Bench::ConstPropertyStatBlock mem_percent(nc_report.properties, "mem_percent");
    Bench::ConstPropertyStatBlock virtual_mem_percent(nc_report.properties, "virtual_mem_percent");

    consolidated_cpu_percent_stats = consolidate(consolidated_cpu_percent_stats, cpu_percent.to_simple_stat_block());
    consolidated_mem_percent_stats = consolidate(consolidated_mem_percent_stats, mem_percent.to_simple_stat_block());
    consolidated_virtual_mem_percent_stats = consolidate(consolidated_virtual_mem_percent_stats, virtual_mem_percent.to_simple_stat_block());
    for (CORBA::ULong w = 0; w < nc_report.worker_reports.length(); ++w) {
      parsed_reports.push_back(&nc_report.worker_reports[w]);
    }
  }

  Builder::TimeStamp max_construction_time = ZERO;
  Builder::TimeStamp max_enable_time = ZERO;
  Builder::TimeStamp max_start_time = ZERO;
  Builder::TimeStamp max_stop_time = ZERO;
  Builder::TimeStamp max_destruction_time = ZERO;

  uint64_t total_undermatched_readers = 0;
  uint64_t total_undermatched_writers = 0;
  uint64_t total_lost_sample_count = 0;
  uint64_t total_rejected_sample_count = 0;
  uint64_t total_out_of_order_data_count = 0;
  uint64_t total_duplicate_data_count = 0;
  uint64_t total_missing_data_count = 0;
  Builder::TimeStamp max_discovery_time_delta = ZERO;

  Bench::SimpleStatBlock consolidated_discovery_delta_stats;
  Bench::SimpleStatBlock consolidated_latency_stats;
  Bench::SimpleStatBlock consolidated_jitter_stats;
  Bench::SimpleStatBlock consolidated_round_trip_latency_stats;
  Bench::SimpleStatBlock consolidated_round_trip_jitter_stats;

  bool missing_durable_data = false;

  // Stats and details associated to user-specified tags
  std::unordered_map<std::string, uint64_t> lost_sample_counts, rejected_sample_counts,
    out_of_order_data_counts, duplicate_data_counts, missing_data_counts;
  std::unordered_map<std::string, std::string> out_of_order_data_details, duplicate_data_details, missing_data_details;
  std::unordered_map<std::string, Bench::SimpleStatBlock> tagged_discovery_delta_stats, tagged_latency_stats,
    tagged_jitter_stats, tagged_round_trip_latency_stats, tagged_round_trip_jitter_stats;

  for (size_t r = 0; r < parsed_reports.size(); ++r) {
    const Bench::WorkerReport& worker_report = *(parsed_reports[r]);

    max_construction_time = std::max(max_construction_time, worker_report.construction_time);
    max_enable_time = std::max(max_enable_time, worker_report.enable_time);
    max_start_time = std::max(max_start_time, worker_report.start_time);
    max_stop_time = std::max(max_stop_time, worker_report.stop_time);
    max_destruction_time = std::max(max_destruction_time, worker_report.destruction_time);

    total_undermatched_readers += worker_report.undermatched_readers;
    total_undermatched_writers += worker_report.undermatched_writers;
    max_discovery_time_delta = std::max(max_discovery_time_delta, worker_report.max_discovery_time_delta);

    const Builder::ProcessReport& process_report = worker_report.process_report;

    for (CORBA::ULong i = 0; i < process_report.participants.length(); ++i) {
      for (CORBA::ULong j = 0; j < process_report.participants[i].subscribers.length(); ++j) {
        for (CORBA::ULong k = 0; k < process_report.participants[i].subscribers[j].datareaders.length(); ++k) {
          const Builder::DataReaderReport& dr_report = process_report.participants[i].subscribers[j].datareaders[k];

          Bench::ConstPropertyStatBlock dr_discovery_delta(dr_report.properties, "discovery_delta");
          Bench::ConstPropertyStatBlock dr_latency(dr_report.properties, "latency");
          Bench::ConstPropertyStatBlock dr_jitter(dr_report.properties, "jitter");
          Bench::ConstPropertyStatBlock dr_round_trip_latency(dr_report.properties, "round_trip_latency");
          Bench::ConstPropertyStatBlock dr_round_trip_jitter(dr_report.properties, "round_trip_jitter");

          Builder::ConstPropertyIndex lost_sample_count_prop = get_property(dr_report.properties, "lost_sample_count", Builder::PVK_ULL);
          if (lost_sample_count_prop) {
            total_lost_sample_count += lost_sample_count_prop->value.ull_prop();
            update_stats_for_tags(lost_sample_counts, dr_report.tags, tags, lost_sample_count_prop);
          }
          Builder::ConstPropertyIndex rejected_sample_count_prop = get_property(dr_report.properties, "rejected_sample_count", Builder::PVK_ULL);
          if (rejected_sample_count_prop) {
            total_rejected_sample_count += rejected_sample_count_prop->value.ull_prop();
            update_stats_for_tags(rejected_sample_counts, dr_report.tags, tags, rejected_sample_count_prop);
          }
          Builder::ConstPropertyIndex out_of_order_data_count_prop = get_property(dr_report.properties, "out_of_order_data_count", Builder::PVK_ULL);
          if (out_of_order_data_count_prop) {
            if (out_of_order_data_count_prop->value.ull_prop()) {
              Builder::ConstPropertyIndex out_of_order_data_details_prop = get_property(dr_report.properties, "out_of_order_data_details", Builder::PVK_STRING);
              if (out_of_order_data_details_prop) {
                std::string detail = "Out Of Order Data (" + std::to_string(out_of_order_data_count_prop->value.ull_prop()) +
                  ") Details: " + out_of_order_data_details_prop->value.string_prop() + "\n";
                result_out << detail;
                update_details_for_tags(out_of_order_data_details, dr_report.tags, tags, detail);
              }
            }
            total_out_of_order_data_count += out_of_order_data_count_prop->value.ull_prop();
            update_stats_for_tags(out_of_order_data_counts, dr_report.tags, tags, out_of_order_data_count_prop);
          }
          Builder::ConstPropertyIndex duplicate_data_count_prop = get_property(dr_report.properties, "duplicate_data_count", Builder::PVK_ULL);
          if (duplicate_data_count_prop) {
            if (duplicate_data_count_prop->value.ull_prop()) {
              Builder::ConstPropertyIndex duplicate_data_details_prop = get_property(dr_report.properties, "duplicate_data_details", Builder::PVK_STRING);
              if (duplicate_data_details_prop) {
                std::string detail = "Duplicate Data (" + std::to_string(duplicate_data_count_prop->value.ull_prop()) +
                  ") Details: " + duplicate_data_details_prop->value.string_prop() + "\n";
                result_out << detail;
                update_details_for_tags(duplicate_data_details, dr_report.tags, tags, detail);
              }
            }
            total_duplicate_data_count += duplicate_data_count_prop->value.ull_prop();
            update_stats_for_tags(duplicate_data_counts, dr_report.tags, tags, duplicate_data_count_prop);
          }
          Builder::ConstPropertyIndex missing_data_count_prop = get_property(dr_report.properties, "missing_data_count", Builder::PVK_ULL);
          if (missing_data_count_prop) {
            if (missing_data_count_prop->value.ull_prop()) {
              Builder::ConstPropertyIndex missing_data_details_prop = get_property(dr_report.properties, "missing_data_details", Builder::PVK_STRING);
              if (missing_data_details_prop) {
                std::string mdd(missing_data_details_prop->value.string_prop());
                std::string detail = "Missing Data (" + std::to_string(missing_data_count_prop->value.ull_prop()) + ") Details: " + mdd + "\n";
                result_out << detail;
                update_details_for_tags(missing_data_details, dr_report.tags, tags, detail);
                if (mdd.find("Durable: true") != std::string::npos) {
                  missing_durable_data = true;
                }
              }
            }
            total_missing_data_count += missing_data_count_prop->value.ull_prop();
            update_stats_for_tags(missing_data_counts, dr_report.tags, tags, missing_data_count_prop);
          }

          consolidated_discovery_delta_stats = consolidate(consolidated_discovery_delta_stats, dr_discovery_delta.to_simple_stat_block());
          consolidated_latency_stats = consolidate(consolidated_latency_stats, dr_latency.to_simple_stat_block());
          consolidated_jitter_stats = consolidate(consolidated_jitter_stats, dr_jitter.to_simple_stat_block());
          consolidated_round_trip_latency_stats = consolidate(consolidated_round_trip_latency_stats, dr_round_trip_latency.to_simple_stat_block());
          consolidated_round_trip_jitter_stats = consolidate(consolidated_round_trip_jitter_stats, dr_round_trip_jitter.to_simple_stat_block());

          consolidate_tagged_stats(tagged_discovery_delta_stats, dr_report.tags, tags, dr_discovery_delta);
          consolidate_tagged_stats(tagged_latency_stats, dr_report.tags, tags, dr_latency);
          consolidate_tagged_stats(tagged_jitter_stats, dr_report.tags, tags, dr_jitter);
          consolidate_tagged_stats(tagged_round_trip_latency_stats, dr_report.tags, tags, dr_round_trip_latency);
          consolidate_tagged_stats(tagged_round_trip_jitter_stats, dr_report.tags, tags, dr_round_trip_jitter);
        }
      }

      for (CORBA::ULong j = 0; j < process_report.participants[i].publishers.length(); ++j) {
        for (CORBA::ULong k = 0; k < process_report.participants[i].publishers[j].datawriters.length(); ++k) {
          const Builder::DataWriterReport& dw_report = process_report.participants[i].publishers[j].datawriters[k];

          Bench::ConstPropertyStatBlock dw_discovery_delta(dw_report.properties, "discovery_delta");
          consolidated_discovery_delta_stats = consolidate(consolidated_discovery_delta_stats, dw_discovery_delta.to_simple_stat_block());
        }
      }
    }
  }

  result_out << std::endl;

  consolidated_cpu_percent_stats.pretty_print(result_out, "percent cpu utilization");
  result_out << std::endl;

  consolidated_mem_percent_stats.pretty_print(result_out, "percent memory utilization");
  result_out << std::endl;

  consolidated_virtual_mem_percent_stats.pretty_print(result_out, "percent virtual memory utilization");
  result_out << std::endl;

  result_out << "Test Timing Stats:" << std::endl;
  result_out << "  Max Construction Time: " << max_construction_time << " seconds" << std::endl;
  result_out << "  Max Enable Time: " << max_enable_time << " seconds" << std::endl;
  result_out << "  Max Start Time: " << max_start_time << " seconds" << std::endl;
  result_out << "  Max Stop Time: " << max_stop_time << " seconds" << std::endl;
  result_out << "  Max Destruction Time: " << max_destruction_time << " seconds" << std::endl;
  result_out << std::endl;

  result_out << "Discovery Stats:" << std::endl;
  result_out << (total_undermatched_readers != 0 ? "  ERROR: " : "  ") <<
    "Total Undermatched Readers: " << total_undermatched_readers <<
    (total_undermatched_writers != 0 ? ", ERROR: " : ", ") <<
    "Total Undermatched Writers: " << total_undermatched_writers << std::endl;
  result_out << std::endl;

  consolidated_discovery_delta_stats.pretty_print(result_out, "discovery time delta");
  result_out << std::endl;

  result_out << "DDS Sample Count Stats:" << std::endl;
  result_out << "  Total Lost Samples: " << total_lost_sample_count << std::endl;
  result_out << "  Total Rejected Samples: " << total_rejected_sample_count << std::endl;
  result_out << std::endl;

  result_out << "Data Count Stats:" << std::endl;
  result_out << "  Total Out-Of-Order Data Samples: " << total_out_of_order_data_count << std::endl;
  result_out << "  Total Duplicate Data Samples: " << total_duplicate_data_count << std::endl;
  result_out << "  Total Missing Data Samples: " << total_missing_data_count << std::endl;
  result_out << std::endl;

  result_out << "Data Timing Stats:" << std::endl;
  result_out << std::endl;

  consolidated_latency_stats.pretty_print(result_out, "latency", "  ", 1);
  result_out << std::endl;

  consolidated_jitter_stats.pretty_print(result_out, "jitter", "  ", 1);
  result_out << std::endl;

  consolidated_round_trip_latency_stats.pretty_print(result_out, "round trip latency", "  ", 1);
  result_out << std::endl;

  consolidated_round_trip_jitter_stats.pretty_print(result_out, "round trip jitter", "  ", 1);
  result_out << "\n\n";

  // Print stats information for the input tags
  for (const std::string& tag : tags) {
    result_out << "===== Stats For Tag: " << tag << "\n";
    if (out_of_order_data_details.count(tag)) {
      result_out << out_of_order_data_details[tag] << std::endl;
    }
    if (duplicate_data_details.count(tag)) {
      result_out << duplicate_data_details[tag] << std::endl;
    }
    if (missing_data_details.count(tag)) {
      result_out << missing_data_details[tag] << std::endl;
    }

    if (tagged_discovery_delta_stats.count(tag)) {
      tagged_discovery_delta_stats[tag].pretty_print(result_out, "discovery time delta");
      result_out << std::endl;
    }
    if (tagged_latency_stats.count(tag)) {
      tagged_latency_stats[tag].pretty_print(result_out, "latency");
      result_out << std::endl;
    }
    if (tagged_jitter_stats.count(tag)) {
      tagged_jitter_stats[tag].pretty_print(result_out, "jitter");
      result_out << std::endl;
    }
    if (tagged_round_trip_latency_stats.count(tag)) {
      tagged_round_trip_latency_stats[tag].pretty_print(result_out, "round trip latency");
      result_out << std::endl;
    }
    if (tagged_round_trip_jitter_stats.count(tag)) {
      tagged_round_trip_jitter_stats[tag].pretty_print(result_out, "round trip jitter");
      result_out << std::endl;
    }

    result_out << "DDS Sample Count Stats:" << std::endl;
    result_out << "  Total Lost Samples: " <<
      (lost_sample_counts.count(tag) > 0 ? std::to_string(lost_sample_counts[tag]) : "") << std::endl;
    result_out << "  Total Rejected Samples: " <<
      (rejected_sample_counts.count(tag) > 0 ? std::to_string(rejected_sample_counts[tag]) : "") << std::endl;
    result_out << std::endl;

    result_out << "Data Count Stats:" << std::endl;
    result_out << "  Total Out-Of-Order Data Samples: " <<
      (out_of_order_data_counts.count(tag) > 0 ? std::to_string(out_of_order_data_counts[tag]) : "") << std::endl;
    result_out << "  Total Duplicate Data Samples: " <<
      (duplicate_data_counts.count(tag) > 0 ? std::to_string(duplicate_data_counts[tag]) : "") << std::endl;
    result_out << "  Total Missing Data Samples: " <<
      (missing_data_counts.count(tag) > 0 ? std::to_string(missing_data_counts[tag]) : "") << std::endl;
    result_out << "\n\n";
  }

  if (total_undermatched_readers ||
    total_undermatched_writers ||
    total_out_of_order_data_count ||
    total_duplicate_data_count ||
    missing_durable_data) {
    result = EXIT_FAILURE;
  }
  return result;
}
}
