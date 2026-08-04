#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

class HostNetworkStatsCollector {
public:
  typedef std::map<std::string, uint64_t> CounterMap;

  // Collect interface-level counters with platform-neutral meanings.  A
  // counter is omitted when the host platform doesn't expose an equivalent;
  // an unavailable counter must never be reported as zero.
  explicit HostNetworkStatsCollector(const std::string& proc_root = "/proc");

  CounterMap deltas() const;

  static const char* const counter_names[];
  static const size_t counter_count;

private:
  CounterMap read_counters() const;

  std::string proc_root_;
  CounterMap baseline_;
};
