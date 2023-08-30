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
    if (sp_default != NetworkAddress::default_IPV4) {
      set(NetworkInterfaceAddress("", true, sp_default));
      return true;
    }

    set(NetworkInterfaceAddress("", true, NetworkAddress(u_short(0), "0.0.0.0")));
#ifdef ACE_HAS_IPV6
    set(NetworkInterfaceAddress("", true, NetworkAddress(u_short(0), "::")));
#endif

    return true;
  }

  bool close() { return true; }
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_DEFAULT_NETWORK_CONFIG_MONITOR_H
