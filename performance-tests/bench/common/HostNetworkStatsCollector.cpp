#include "HostNetworkStatsCollector.h"

#include <ace/config-all.h>

#include <fstream>
#include <sstream>

#ifdef ACE_WIN32
// netioapi.h (via iphlpapi.h) only declares GetIfTable2 if ws2ipdef.h was included first
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  include <iphlpapi.h>
#elif defined ACE_HAS_MAC_OSX
#  include <ifaddrs.h>
#  include <net/if.h>
#  include <net/if_dl.h>
#  include <sys/socket.h>
#endif

namespace Bench {

namespace {

#ifdef ACE_HAS_MAC_OSX
// struct if_data counters are 32 bits wide
const unsigned platform_counter_bits = 32;
#else
const unsigned platform_counter_bits = 64;
#endif

}

const char* const HostNetworkStatsCollector::counter_names[] = {
  "network_receive_dropped",
  "network_transmit_dropped",
  "network_receive_errors",
  "network_transmit_errors"
};

const size_t HostNetworkStatsCollector::counter_count =
  sizeof counter_names / sizeof counter_names[0];

const char* const HostNetworkStatsCollector::available_property_name = "host_network_stats_available";

HostNetworkStatsCollector::HostNetworkStatsCollector()
  : baseline_valid_(read_counters(baseline_))
{
}

bool HostNetworkStatsCollector::deltas(CounterMap& result) const
{
  InterfaceCounterMap current;
  if (!baseline_valid_ || !read_counters(current)) {
    return false;
  }
  result = compute_deltas(baseline_, current, platform_counter_bits);
  return true;
}

HostNetworkStatsCollector::CounterMap HostNetworkStatsCollector::compute_deltas(
  const InterfaceCounterMap& before, const InterfaceCounterMap& after, unsigned counter_bits)
{
  const uint64_t mask = counter_bits >= 64 ? ~uint64_t(0) : (uint64_t(1) << counter_bits) - 1;
  CounterMap result;
  for (const auto& iface_after : after) {
    const auto iface_before = before.find(iface_after.first);
    if (iface_before == before.end()) {
      continue;
    }
    for (const auto& counter_after : iface_after.second) {
      const auto counter_before = iface_before->second.find(counter_after.first);
      if (counter_before == iface_before->second.end()) {
        continue;
      }
      if (counter_bits >= 64 && counter_after.second < counter_before->second) {
        continue;
      }
      result[counter_after.first] += (counter_after.second - counter_before->second) & mask;
    }
  }
  return result;
}

bool HostNetworkStatsCollector::parse_linux_net_dev(std::istream& input, InterfaceCounterMap& counters)
{
  bool parsed_any = false;
  std::string line;
  while (std::getline(input, line)) {
    const size_t colon = line.find(':');
    if (colon == std::string::npos) {
      continue; // header lines
    }
    std::string interface_name = line.substr(0, colon);
    const size_t first = interface_name.find_first_not_of(" \t");
    const size_t last = interface_name.find_last_not_of(" \t");
    interface_name = first == std::string::npos ? "" : interface_name.substr(first, last - first + 1);
    if (interface_name.empty()) {
      continue;
    }

    std::istringstream fields(line.substr(colon + 1));
    uint64_t rx_bytes, rx_packets, rx_errors, rx_dropped, rx_fifo, rx_frame, rx_compressed, rx_multicast;
    uint64_t tx_bytes, tx_packets, tx_errors, tx_dropped;
    if (fields >> rx_bytes >> rx_packets >> rx_errors >> rx_dropped >> rx_fifo >> rx_frame
        >> rx_compressed >> rx_multicast >> tx_bytes >> tx_packets >> tx_errors >> tx_dropped) {
      parsed_any = true;
      if (interface_name == "lo") {
        continue;
      }
      CounterMap& iface = counters[interface_name];
      iface["network_receive_errors"] = rx_errors;
      iface["network_receive_dropped"] = rx_dropped;
      iface["network_transmit_errors"] = tx_errors;
      iface["network_transmit_dropped"] = tx_dropped;
    }
  }
  return parsed_any;
}

bool HostNetworkStatsCollector::read_counters(InterfaceCounterMap& counters)
{
  counters.clear();

#if defined ACE_LINUX
  std::ifstream input("/proc/net/dev");
  return input && parse_linux_net_dev(input, counters);
#elif defined ACE_WIN32
  PMIB_IF_TABLE2 table = 0;
  if (GetIfTable2(&table) != NO_ERROR) {
    return false;
  }
  for (ULONG i = 0; i < table->NumEntries; ++i) {
    const MIB_IF_ROW2& row = table->Table[i];
    // Filter interfaces are NDIS filter layers stacked on another interface
    // and repeat that interface's counters.
    if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK || row.InterfaceAndOperStatusFlags.FilterInterface) {
      continue;
    }
    std::ostringstream luid;
    luid << row.InterfaceLuid.Value;
    CounterMap& iface = counters[luid.str()];
    iface["network_receive_errors"] = row.InErrors;
    iface["network_receive_dropped"] = row.InDiscards;
    iface["network_transmit_errors"] = row.OutErrors;
    iface["network_transmit_dropped"] = row.OutDiscards;
  }
  FreeMibTable(table);
  return true;
#elif defined ACE_HAS_MAC_OSX
  ifaddrs* interfaces = 0;
  if (getifaddrs(&interfaces) != 0) {
    return false;
  }
  for (const ifaddrs* ifa = interfaces; ifa; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK &&
        ifa->ifa_data && !(ifa->ifa_flags & IFF_LOOPBACK)) {
      const if_data* data = static_cast<const if_data*>(ifa->ifa_data);
      CounterMap& iface = counters[ifa->ifa_name];
      iface["network_receive_errors"] = data->ifi_ierrors;
      iface["network_receive_dropped"] = data->ifi_iqdrops;
      iface["network_transmit_errors"] = data->ifi_oerrors;
      // No portable transmit drop counter
    }
  }
  freeifaddrs(interfaces);
  return true;
#else
  return false;
#endif
}

}
