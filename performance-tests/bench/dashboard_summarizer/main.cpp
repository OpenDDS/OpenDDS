#include "util.h"

#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace rapidjson;

namespace {

Value& find_or_create(Value& parent, const std::string& name, Type type, Document::AllocatorType& alloc)
{
  Value::MemberIterator pos = parent.FindMember(Value(name.c_str(), name.length()));
  if (pos == parent.MemberEnd()) {
    parent.AddMember(Value(name.c_str(), name.length(), alloc).Move(), Value(type).Move(), alloc);
    pos = parent.FindMember(Value(name.c_str(), name.length()));
    assert(pos != parent.MemberEnd());
  }
  assert(pos->value.GetType() == type);
  return pos->value;
}

void transfer_double(Value& out, Document::AllocatorType& alloc, const Value& in, const std::string& in_name, const std::string out_name)
{
  double to_write = 0.0;
  auto pos = in.FindMember(Value(in_name.c_str(), in_name.length()));
  if (pos != in.MemberEnd() && pos->value.IsDouble()) {
    to_write = pos->value.GetDouble();
  }
  assert(out.GetType() == kObjectType);
  Value& double_value = find_or_create(out, out_name, kNumberType, alloc);
  double_value.SetDouble(to_write);
  assert(out.GetType() == kObjectType);
}

void transfer_uint64(Value& out, Document::AllocatorType& alloc, const Value& in, const std::string& in_name, const std::string out_name)
{
  uint64_t to_write = 0;
  auto pos = in.FindMember(Value(in_name.c_str(), in_name.length()));
  if (pos != in.MemberEnd() && pos->value.IsUint64()) {
    to_write = pos->value.GetUint64();
  }
  assert(out.GetType() == kObjectType);
  Value& double_value = find_or_create(out, out_name, kNumberType, alloc);
  double_value.SetUint64(to_write);
  assert(out.GetType() == kObjectType);
}

void write_statistic(Value& parent_out, Document::AllocatorType& alloc, const std::string& name, const Value& in)
{
  Value& out = find_or_create(parent_out, name, kObjectType, alloc);
  assert(out.GetType() == kObjectType);

  transfer_uint64(out, alloc, in, "sample_count", "count");

  transfer_double(out, alloc, in, "min", "min");
  transfer_double(out, alloc, in, "max", "max");
  transfer_double(out, alloc, in, "mean", "mean");
  transfer_double(out, alloc, in, "stdev", "stdev");
  transfer_double(out, alloc, in, "median", "median");
  transfer_double(out, alloc, in, "madev", "madev");

  transfer_uint64(out, alloc, in, "median_sample_overflow", "overflow");
}

}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  std::vector<std::string> file_names;
  std::vector<std::shared_ptr<std::ifstream>> files;

  for (size_t i = 1; i < static_cast<size_t>(argc); ++i) {
    const std::string full_file_name(ACE_TEXT_ALWAYS_CHAR(argv[i]));
    const size_t div = full_file_name.find_last_of("/\\");
    file_names.push_back(full_file_name.substr(div ? div + 1 : div));
    files.push_back(std::make_shared<std::ifstream>(ACE_TEXT_ALWAYS_CHAR(argv[i])));
    if (!files.back()->good()) {
      std::cerr << "Unable to open file: '" << ACE_TEXT_ALWAYS_CHAR(argv[i]) << "'. Exiting" << std::endl;
      return 1;
    }
  }

  Document doc_out;
  doc_out.SetObject();

  std::set<std::string> known_scenario_names;
  known_scenario_names.insert("disco");
  known_scenario_names.insert("echo_tcp");
  known_scenario_names.insert("echo_rtps");
  known_scenario_names.insert("fan_tcp");
  known_scenario_names.insert("fan_rtps");
  known_scenario_names.insert("showtime_mixed");
  known_scenario_names.insert("b1_latency_raw-tcp");
  known_scenario_names.insert("b1_latency_raw-udp");
  known_scenario_names.insert("b1_latency_tcp");
  known_scenario_names.insert("b1_latency_udp");
  known_scenario_names.insert("b1_latency_multicast-be");
  known_scenario_names.insert("b1_latency_multicast-rel");
  known_scenario_names.insert("b1_latency_rtps");

  typedef std::map<std::string, std::string> StatNameMap;
  StatNameMap known_stat_names;
  known_stat_names["cpu_percent"] = "Cpu Utilization";
  known_stat_names["mem_percent"] = "Memory Utilization";
  known_stat_names["virtual_mem_percent"] = "Virtual Memory Utilization";
  known_stat_names["discovery_delta"] = "Discovery Time Delta";
  known_stat_names["latency"] = "Latency";
  known_stat_names["round_trip_latency"] = "Round Trip Latency";
  known_stat_names["jitter"] = "Jitter";
  known_stat_names["round_trip_jitter"] = "Round Trip Jitter";
  known_stat_names["throughput"] = "Throughput";
  known_stat_names["round_trip_throughput"] = "Round Trip Throughput";

  for (size_t i = 0; i < files.size(); ++i) {

    // Parse Input Document
    Document doc_in;
    IStreamWrapper isw(*files[i]);
    doc_in.ParseStream(isw);
    if (!doc_in.IsObject()) {
      std::cerr << "Unable to parse JSON file: '" << file_names[i] << "'. Input isn't valid JSON" << std::endl;
      return 1;
    }

    // Determine Scenario Name
    std::string scn;
    for (auto it = known_scenario_names.begin(); scn.empty() && it != known_scenario_names.end(); ++it) {
      if (file_names[i].find(*it) == 0) {
        scn = *it;
      }
    }
    if (scn.empty()) {
      scn = file_names[i].substr(0, file_names[i].find_last_of('_'));
      std::cerr << "Found Unknown Scenario: " << scn << std::endl;
    }
    const std::string scenario_name = scn;
    const size_t offset = scenario_name.size() + 1;
    const std::string variety_name = file_names[i].substr(offset, file_names[i].find_first_of('.') - offset);

    // Find or Create Values For Scenario and 'Variety'
    Value& scenario_value = find_or_create(doc_out, scenario_name, kObjectType, doc_out.GetAllocator());
    Value& variety_value = find_or_create(scenario_value, variety_name, kObjectType, doc_out.GetAllocator());

    // Write Errors Value
    uint64_t errors = 0;
    const auto errors_it = doc_in.FindMember("errors");
    if (errors_it != doc_in.MemberEnd()) {
      const auto total_it = errors_it->value.FindMember("total");
      if (total_it != errors_it->value.MemberEnd()) {
        errors = total_it->value.GetUint64();
      }
    }

    Value& errors_value = find_or_create(variety_value, "Errors", kNumberType, doc_out.GetAllocator());
    errors_value.SetUint64(errors);

    // Write Max Discovery Time Delta Value
    double discovery_delta_max = 0.0;
    const auto stats_it = doc_in.FindMember("stats");
    if (stats_it != doc_in.MemberEnd()) {
      const auto discovery_delta_it = stats_it->value.FindMember("discovery_delta");
      if (discovery_delta_it != stats_it->value.MemberEnd()) {
        const auto max_it = discovery_delta_it->value.FindMember("max");
        if (max_it != discovery_delta_it->value.MemberEnd()) {
          discovery_delta_max = max_it->value.GetDouble();
        }
      }
    }

    Value& mdtd_value = find_or_create(variety_value, "Max Discovery Time Delta", kNumberType, doc_out.GetAllocator());
    mdtd_value.SetDouble(discovery_delta_max);

    if (stats_it != doc_in.MemberEnd()) {
      const Value& stats_value = stats_it->value;
      for (Value::ConstMemberIterator ssi = stats_value.MemberBegin(); ssi != stats_value.MemberEnd(); ++ssi) {
        const auto pos = known_stat_names.find(ssi->name.GetString());
        const std::string display_name = pos != known_stat_names.end() ? pos->second : ssi->name.GetString();
        write_statistic(variety_value, doc_out.GetAllocator(), display_name, ssi->value);
      }
    }
  }

  OStreamWrapper osw(std::cout);
  PrettyWriter<OStreamWrapper> writer(osw);
  doc_out.Accept(writer);
  osw.Flush();
  std::cout << std::endl;

  return 0;
}
