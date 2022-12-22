/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DEFAULT_NETWORK_CONFIG_MONITOR_H
#define OPENDDS_DCPS_DEFAULT_NETWORK_CONFIG_MONITOR_H

#include "ace/config.h"

#include "NetworkConfigMonitor.h"
#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DefaultNetworkConfigMonitor : public NetworkConfigMonitor {
public:
  bool open()
  {
    const NetworkAddress sp_default = TheServiceParticipant->default_address();
    if (sp_default != NetworkAddress()) {
      set(NetworkInterfaceAddress("", true, sp_default));
      return true;
    }

    static const u_short port_zero = 0;
    ACE_INET_Addr addr(port_zero, "0.0.0.0");
    set(NetworkInterfaceAddress("", true, NetworkAddress(addr)));
#ifdef ACE_HAS_IPV6
    ACE_INET_Addr addr2(port_zero, "::");
    set(NetworkInterfaceAddress("", true, NetworkAddress(addr2)));
#endif

    return true;
  }

  bool close() { return true; }
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_DEFAULT_NETWORK_CONFIG_MONITOR_H
