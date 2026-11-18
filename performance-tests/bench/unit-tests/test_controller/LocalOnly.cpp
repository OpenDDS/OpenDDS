#include "ScenarioManager.h"

#include <json_conversion.h>

#include <gtest/gtest.h>

#include <sstream>

using namespace Bench::TestController;

namespace {

const char* const worker_json =
  "{"
  "  \"process\": {"
  "    \"config_sections\": ["
  "      { \"name\": \"common\", \"properties\": [ { \"name\": \"DCPSDefaultDiscovery\", \"value\": \"disc\" } ] },"
  "      { \"name\": \"rtps_discovery/disc\", \"properties\": [ { \"name\": \"SedpMulticast\", \"value\": \"1\" } ] },"
  "      { \"name\": \"transport/rtps\", \"properties\": [ { \"name\": \"transport_type\", \"value\": \"rtps_udp\" } ] },"
  "      { \"name\": \"transport/tcp\", \"properties\": [ { \"name\": \"transport_type\", \"value\": \"tcp\" } ] }"
  "    ],"
  "    \"participants\": [ { \"name\": \"p\", \"domain\": 7 } ]"
  "  }"
  "}";

const char* const no_participants_json = "{ \"wait_for_discovery\": false }";

std::string property(const Bench::WorkerConfig& wc, const std::string& section, const std::string& name)
{
  for (CORBA::ULong i = 0; i < wc.process.config_sections.length(); ++i) {
    const Builder::ConfigSection& s = wc.process.config_sections[i];
    if (section == s.name.in()) {
      for (CORBA::ULong j = 0; j < s.properties.length(); ++j) {
        if (name == s.properties[j].name.in()) {
          return s.properties[j].value.in();
        }
      }
    }
  }
  return "<missing>";
}

Bench::WorkerConfig parse(const std::string& json)
{
  std::stringstream ss(json);
  Bench::WorkerConfig wc;
  EXPECT_TRUE(Bench::json_2_idl(ss, wc));
  return wc;
}

}

TEST(LocalOnlyTest, RewritesRtpsSections)
{
  ScenarioPrototype sp;
  sp.nodes.length(1);
  sp.nodes[0].count = 2;
  sp.nodes[0].workers.length(1);
  sp.nodes[0].workers[0].config = "w.json";
  sp.nodes[0].workers[0].count = 3;
  sp.any_node.length(1);
  sp.any_node[0].config = "noop.json";
  sp.any_node[0].count = 1;

  std::map<std::string, std::string> configs;
  configs["w.json"] = worker_json;
  configs["noop.json"] = no_participants_json;
  ScenarioManager::apply_local_only(sp, configs);

  EXPECT_EQ(configs["noop.json"], no_participants_json);

  const Bench::WorkerConfig wc = parse(configs["w.json"]);
  EXPECT_EQ(property(wc, "common", "DCPSDefaultAddress"), "127.0.0.1");
  EXPECT_EQ(property(wc, "common", "DCPSDefaultDiscovery"), "disc");
  EXPECT_EQ(property(wc, "rtps_discovery/disc", "SedpMulticast"), "0");
  EXPECT_EQ(property(wc, "rtps_discovery/disc", "Ipv6SpdpLocalAddress"), "[::1]:0");
  EXPECT_EQ(property(wc, "transport/rtps", "use_multicast"), "0");
  EXPECT_EQ(property(wc, "transport/rtps", "ipv6_local_address"), "[::1]:0");
  EXPECT_EQ(property(wc, "transport/tcp", "use_multicast"), "<missing>");

  // 6 participants in domain 7 -> 2 * 6 + 8 = 20 participant IDs starting at 7400 + 250 * 7 + 10
  const std::string addrs = property(wc, "rtps_discovery/disc", "SpdpSendAddrs");
  EXPECT_EQ(addrs.find("127.0.0.1:9160,127.0.0.1:9162,"), 0u);
  EXPECT_NE(addrs.find("127.0.0.1:9198"), std::string::npos);
  EXPECT_EQ(addrs.find("127.0.0.1:9200"), std::string::npos);
}

TEST(LocalOnlyTest, AddsCommonSection)
{
  ScenarioPrototype sp;
  sp.any_node.length(1);
  sp.any_node[0].config = "w.json";
  sp.any_node[0].count = 1;

  std::map<std::string, std::string> configs;
  configs["w.json"] = "{ \"process\": { \"participants\": [ { \"name\": \"p\", \"domain\": 0 } ] } }";
  ScenarioManager::apply_local_only(sp, configs);

  EXPECT_EQ(property(parse(configs["w.json"]), "common", "DCPSDefaultAddress"), "127.0.0.1");
}
