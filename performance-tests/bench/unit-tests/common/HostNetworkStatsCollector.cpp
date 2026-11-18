/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "HostNetworkStatsCollector.h"

#include <ace/config-all.h>

#include <gtest/gtest.h>

#include <sstream>

using Bench::HostNetworkStatsCollector;

namespace {

const char* const net_dev_sample =
  "Inter-|   Receive                                                |  Transmit\n"
  " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
  "    lo: 1000 10 7 7 0 0 0 0 1000 10 7 7 0 0 0 0\n"
  "  eth0: 5000 50 1 2 0 0 0 3 6000 60 3 4 0 0 0 0\n"
  "docker0:7000 70 5 6 0 0 0 0 8000 80 7 8 0 0 0 0\n";

HostNetworkStatsCollector::CounterMap make_counters(uint64_t rx_err, uint64_t rx_drop, uint64_t tx_err, uint64_t tx_drop)
{
  HostNetworkStatsCollector::CounterMap result;
  result["network_receive_errors"] = rx_err;
  result["network_receive_dropped"] = rx_drop;
  result["network_transmit_errors"] = tx_err;
  result["network_transmit_dropped"] = tx_drop;
  return result;
}

}

TEST(bench_host_network_stats_collector, parse_linux_net_dev)
{
  std::istringstream input(net_dev_sample);
  HostNetworkStatsCollector::InterfaceCounterMap counters;
  ASSERT_TRUE(HostNetworkStatsCollector::parse_linux_net_dev(input, counters));

  ASSERT_EQ(counters.size(), 2u);
  EXPECT_EQ(counters.count("lo"), 0u);
  EXPECT_EQ(counters["eth0"], make_counters(1, 2, 3, 4));
  EXPECT_EQ(counters["docker0"], make_counters(5, 6, 7, 8));
}

TEST(bench_host_network_stats_collector, parse_linux_net_dev_only_loopback)
{
  std::istringstream input("    lo: 1000 10 7 7 0 0 0 0 1000 10 7 7 0 0 0 0\n");
  HostNetworkStatsCollector::InterfaceCounterMap counters;
  EXPECT_TRUE(HostNetworkStatsCollector::parse_linux_net_dev(input, counters));
  EXPECT_TRUE(counters.empty());
}

TEST(bench_host_network_stats_collector, parse_linux_net_dev_garbage)
{
  std::istringstream input("not the expected: format\n");
  HostNetworkStatsCollector::InterfaceCounterMap counters;
  EXPECT_FALSE(HostNetworkStatsCollector::parse_linux_net_dev(input, counters));
  EXPECT_TRUE(counters.empty());
}

TEST(bench_host_network_stats_collector, compute_deltas_sums_interfaces)
{
  HostNetworkStatsCollector::InterfaceCounterMap before, after;
  before["a"] = make_counters(1, 2, 3, 4);
  before["b"] = make_counters(10, 20, 30, 40);
  after["a"] = make_counters(2, 4, 6, 8);
  after["b"] = make_counters(11, 21, 31, 41);

  EXPECT_EQ(HostNetworkStatsCollector::compute_deltas(before, after, 64), make_counters(2, 3, 4, 5));
}

TEST(bench_host_network_stats_collector, compute_deltas_skips_interfaces_not_in_both)
{
  HostNetworkStatsCollector::InterfaceCounterMap before, after;
  before["a"] = make_counters(1, 1, 1, 1);
  before["gone"] = make_counters(100, 100, 100, 100);
  after["a"] = make_counters(2, 2, 2, 2);
  after["new"] = make_counters(100, 100, 100, 100);

  EXPECT_EQ(HostNetworkStatsCollector::compute_deltas(before, after, 64), make_counters(1, 1, 1, 1));
}

TEST(bench_host_network_stats_collector, compute_deltas_omits_missing_counters)
{
  HostNetworkStatsCollector::InterfaceCounterMap before, after;
  before["a"]["network_receive_errors"] = 1;
  after["a"]["network_receive_errors"] = 3;

  const HostNetworkStatsCollector::CounterMap deltas = HostNetworkStatsCollector::compute_deltas(before, after, 64);
  ASSERT_EQ(deltas.size(), 1u);
  EXPECT_EQ(deltas.at("network_receive_errors"), 2u);
}

TEST(bench_host_network_stats_collector, compute_deltas_32_bit_wrap)
{
  HostNetworkStatsCollector::InterfaceCounterMap before, after;
  before["a"]["network_receive_errors"] = 0xFFFFFFFEu;
  after["a"]["network_receive_errors"] = 3;

  EXPECT_EQ(HostNetworkStatsCollector::compute_deltas(before, after, 32).at("network_receive_errors"), 5u);
}

TEST(bench_host_network_stats_collector, compute_deltas_64_bit_reset)
{
  HostNetworkStatsCollector::InterfaceCounterMap before, after;
  before["a"]["network_receive_errors"] = 10;
  before["b"]["network_receive_errors"] = 10;
  after["a"]["network_receive_errors"] = 3; // reset, skipped
  after["b"]["network_receive_errors"] = 12;

  EXPECT_EQ(HostNetworkStatsCollector::compute_deltas(before, after, 64).at("network_receive_errors"), 2u);
}

TEST(bench_host_network_stats_collector, live_counters)
{
  HostNetworkStatsCollector collector;
  HostNetworkStatsCollector::CounterMap deltas;
#if defined ACE_LINUX || defined ACE_WIN32 || defined ACE_HAS_MAC_OSX
  EXPECT_TRUE(collector.deltas(deltas));
#else
  EXPECT_FALSE(collector.deltas(deltas));
#endif
}
