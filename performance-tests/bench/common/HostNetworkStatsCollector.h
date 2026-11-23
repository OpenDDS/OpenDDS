#ifndef BENCH_HOST_NETWORK_STATS_COLLECTOR_HEADER
#define BENCH_HOST_NETWORK_STATS_COLLECTOR_HEADER

#include "Bench_Common_Export.h"

#include <cstddef>
#include <cstdint>
#include <istream>
#include <map>
#include <string>

namespace Bench {

class Bench_Common_Export HostNetworkStatsCollector {
public:
  // counter name -> value
  typedef std::map<std::string, uint64_t> CounterMap;
  // interface identifier -> counters
  typedef std::map<std::string, CounterMap> InterfaceCounterMap;

  static const char* const counter_names[];
  static const size_t counter_count;
  static const char* const available_property_name;

  // Collect interface-level counters with platform-neutral meanings.  A
  // counter is omitted when the host platform doesn't expose an equivalent;
  // an unavailable counter must never be reported as zero.  Loopback
  // interfaces are excluded.  Construction takes the baseline snapshot.
  HostNetworkStatsCollector();

  // Returns false if the counters could not be read, either at construction
  // or now (including on unsupported platforms).  Otherwise fills result with
  // the change in each available counter since construction.
  bool deltas(CounterMap& result) const;

  // Platform-specific snapshot of the current counters.
  static bool read_counters(InterfaceCounterMap& counters);

  // Parser for the format of Linux's /proc/net/dev.
  static bool parse_linux_net_dev(std::istream& input, InterfaceCounterMap& counters);

  // Sums per-interface changes.  Counters narrower than 64 bits are assumed
  // to have wrapped at most once; a 64-bit counter that decreased was reset
  // and that interface's value is skipped.  Interfaces missing from either
  // snapshot are skipped.
  static CounterMap compute_deltas(const InterfaceCounterMap& before,
                                   const InterfaceCounterMap& after,
                                   unsigned counter_bits);

private:
  InterfaceCounterMap baseline_;
  bool baseline_valid_;
};

}

#endif
