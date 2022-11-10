#include "util.h"

#include <memory>
#include <string>
#include <vector>
#include <iostream>

using namespace rapidjson;

namespace {

Value& find_or_create(Value& parent, const std::string& name, Type type, Document::AllocatorType& alloc)
{
  const SizeType length = static_cast<SizeType>(name.length());
  Value::MemberIterator pos = parent.FindMember(Value(name.c_str(), length));
  if (pos == parent.MemberEnd()) {
    parent.AddMember(Value(name.c_str(), length, alloc).Move(), Value(type).Move(), alloc);
    pos = parent.FindMember(Value(name.c_str(), length));
    assert(pos != parent.MemberEnd());
  }
  if (type != kNullType) {
    assert(pos->value.GetType() == type);
  }
  return pos->value;
}

void transfer_double(Value& out, Document::AllocatorType& alloc, const Value& in, const std::string& in_name, const std::string out_name)
{
  double to_write = 0.0;
  const SizeType length = static_cast<SizeType>(in_name.length());
  auto pos = in.FindMember(Value(in_name.c_str(), length));
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
  const SizeType length = static_cast<SizeType>(in_name.length());
  auto pos = in.FindMember(Value(in_name.c_str(), length));
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
  known_scenario_names.insert("disco-relay");
  known_scenario_names.insert("disco-repo");
  known_scenario_names.insert("echo-tcp");
  known_scenario_names.insert("echo-rtps");
  known_scenario_names.insert("fan-tcp");
  known_scenario_names.insert("fan-rtps");
  known_scenario_names.insert("showtime-mixed");
  known_scenario_names.insert("b1-latency-raw-tcp");
  known_scenario_names.insert("b1-latency-raw-udp");
  known_scenario_names.insert("b1-latency-tcp");
  known_scenario_names.insert("b1-latency-udp");
  known_scenario_names.insert("b1-latency-multicast-be");
  known_scenario_names.insert("b1-latency-multicast-rel");
  known_scenario_names.insert("b1-latency-rtps");

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

    // Parse Run Parameters
    const auto rp_it = doc_in.FindMember("run_parameters");
    if (rp_it != doc_in.MemberEnd()) {
      const Value& rp_value_in = rp_it->value;
      Value& rp_value_out = find_or_create(doc_out, "run_parameters", kObjectType, doc_out.GetAllocator());
      for (Value::ConstMemberIterator rpi = rp_value_in.MemberBegin(); rpi != rp_value_in.MemberEnd(); ++rpi) {
        Value& rpi_value_out = find_or_create(rp_value_out, rpi->name.GetString(), kNullType, doc_out.GetAllocator());
        if (rpi_value_out.IsNull()) {
          rpi_value_out = rapidjson::Value(rpi->value, doc_out.GetAllocator());
        } else if (rpi_value_out != rpi->value) {
          std::cerr << "Run Parameter '" << rpi->name.GetString() << "' is not consistent across input files ('" << rpi_value_out.GetString() << "' vs '" << rpi->value.GetString() <<"'). Exiting" << std::endl;
          return 1;
        }
      }
    }

    // Determine Full Scenario Name
    const std::string full_scenario_name = file_names[i].substr(0, file_names[i].find_first_of('.'));

    // Find or Create Values For Scenario
    Value& full_scenario_value = find_or_create(doc_out, full_scenario_name, kObjectType, doc_out.GetAllocator());

    // Parse Scenario Parameters
    const auto sp_it = doc_in.FindMember("scenario_parameters");
    if (sp_it != doc_in.MemberEnd()) {
      const Value& sp_value_in = sp_it->value;
      Value& sp_value_out = find_or_create(full_scenario_value, "scenario_parameters", kObjectType, doc_out.GetAllocator());
      for (Value::ConstMemberIterator spi = sp_value_in.MemberBegin(); spi != sp_value_in.MemberEnd(); ++spi) {
        Value& spi_value_out = find_or_create(sp_value_out, spi->name.GetString(), kObjectType, doc_out.GetAllocator());
        spi_value_out = rapidjson::Value(spi->value, doc_out.GetAllocator());
      }
    }

    // Write Errors Value
    uint64_t errors = 0;
    const auto errors_it = doc_in.FindMember("errors");
    if (errors_it != doc_in.MemberEnd()) {
      const auto total_it = errors_it->value.FindMember("total");
      if (total_it != errors_it->value.MemberEnd()) {
        errors = total_it->value.GetUint64();
      }
    }

    Value& errors_value = find_or_create(full_scenario_value, "Errors", kNumberType, doc_out.GetAllocator());
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

    Value& mdtd_value = find_or_create(full_scenario_value, "Max Discovery Time Delta", kNumberType, doc_out.GetAllocator());
    mdtd_value.SetDouble(discovery_delta_max);

    if (stats_it != doc_in.MemberEnd()) {
      const Value& stats_value = stats_it->value;
      for (Value::ConstMemberIterator ssi = stats_value.MemberBegin(); ssi != stats_value.MemberEnd(); ++ssi) {
        const auto pos = known_stat_names.find(ssi->name.GetString());
        const std::string display_name = pos != known_stat_names.end() ? pos->second : ssi->name.GetString();
        write_statistic(full_scenario_value, doc_out.GetAllocator(), display_name, ssi->value);
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
