#include "HostNetworkStatsCollector.h"

#include <ace/config-all.h>

#include <fstream>
#include <sstream>
#include <vector>

#ifdef ACE_WIN32
#  include <windows.h>
#  include <iphlpapi.h>
#elif defined ACE_HAS_MAC_OSX
#  include <ifaddrs.h>
#  include <net/if.h>
#  include <net/if_dl.h>
#endif

const char* const HostNetworkStatsCollector::counter_names[] = {
  "network_receive_dropped",
  "network_transmit_dropped",
  "network_receive_errors",
  "network_transmit_errors"
};

const size_t HostNetworkStatsCollector::counter_count =
  sizeof counter_names / sizeof counter_names[0];

HostNetworkStatsCollector::HostNetworkStatsCollector(const std::string& proc_root)
  : proc_root_(proc_root)
  , baseline_(read_counters())
{
}

HostNetworkStatsCollector::CounterMap HostNetworkStatsCollector::deltas() const
{
  const CounterMap current = read_counters();
  CounterMap result;
  for (size_t i = 0; i < counter_count; ++i) {
    const std::string name(counter_names[i]);
    const CounterMap::const_iterator before = baseline_.find(name);
    const CounterMap::const_iterator after = current.find(name);
    if (before != baseline_.end() && after != current.end() && after->second >= before->second) {
      result[name] = after->second - before->second;
    }
  }
  return result;
}

HostNetworkStatsCollector::CounterMap HostNetworkStatsCollector::read_counters() const
{
  CounterMap counters;

#if defined ACE_LINUX
  {
    std::ifstream input((proc_root_ + "/net/dev").c_str());
    std::string line;
    while (std::getline(input, line)) {
      const size_t colon = line.find(':');
      if (colon == std::string::npos) {
        continue;
      }
      std::string interface_name = line.substr(0, colon);
      const size_t first = interface_name.find_first_not_of(" \t");
      const size_t last = interface_name.find_last_not_of(" \t");
      interface_name = first == std::string::npos ? "" : interface_name.substr(first, last - first + 1);
      if (interface_name.empty() || interface_name == "lo") {
        continue;
      }

      std::istringstream fields(line.substr(colon + 1));
      uint64_t rx_bytes, rx_packets, rx_errors, rx_dropped, rx_fifo, rx_frame, rx_compressed, rx_multicast;
      uint64_t tx_bytes, tx_packets, tx_errors, tx_dropped;
      if (fields >> rx_bytes >> rx_packets >> rx_errors >> rx_dropped >> rx_fifo >> rx_frame
          >> rx_compressed >> rx_multicast >> tx_bytes >> tx_packets >> tx_errors >> tx_dropped) {
        counters["network_receive_errors"] += rx_errors;
        counters["network_receive_dropped"] += rx_dropped;
        counters["network_transmit_errors"] += tx_errors;
        counters["network_transmit_dropped"] += tx_dropped;
      }
    }
  }
#elif defined ACE_WIN32
  HMODULE library = LoadLibraryA("iphlpapi.dll");
  if (library) {
    typedef DWORD (WINAPI *GetIfTableFn)(PMIB_IFTABLE, PULONG, BOOL);
    GetIfTableFn get_if_table = reinterpret_cast<GetIfTableFn>(GetProcAddress(library, "GetIfTable"));
    if (get_if_table) {
      ULONG size = 0;
      if (get_if_table(0, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<unsigned char> storage(size);
        PMIB_IFTABLE table = reinterpret_cast<PMIB_IFTABLE>(&storage[0]);
        if (get_if_table(table, &size, FALSE) == NO_ERROR) {
          for (DWORD i = 0; i < table->dwNumEntries; ++i) {
            const MIB_IFROW& row = table->table[i];
            if (row.dwType != IF_TYPE_SOFTWARE_LOOPBACK) {
              counters["network_receive_errors"] += row.dwInErrors;
              counters["network_receive_dropped"] += row.dwInDiscards;
              counters["network_transmit_errors"] += row.dwOutErrors;
              counters["network_transmit_dropped"] += row.dwOutDiscards;
            }
          }
        }
      }
    }
    FreeLibrary(library);
  }
#elif defined ACE_HAS_MAC_OSX
  ifaddrs* interfaces = 0;
  if (getifaddrs(&interfaces) == 0) {
    for (const ifaddrs* interface = interfaces; interface; interface = interface->ifa_next) {
      if (interface->ifa_addr && interface->ifa_addr->sa_family == AF_LINK &&
          interface->ifa_data && !(interface->ifa_flags & IFF_LOOPBACK)) {
        const if_data* data = static_cast<const if_data*>(interface->ifa_data);
        counters["network_receive_errors"] += data->ifi_ierrors;
        counters["network_receive_dropped"] += data->ifi_iqdrops;
        counters["network_transmit_errors"] += data->ifi_oerrors;
      }
    }
    freeifaddrs(interfaces);
  }
#endif

  return counters;
}
