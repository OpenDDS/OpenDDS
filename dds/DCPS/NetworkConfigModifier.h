/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H
#define OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H

#include "ace/config.h"

// ACE_HAS_GETIFADDRS is not set on android but is available in API >= 24
#if ((!defined (ACE_LINUX) && defined(ACE_HAS_GETIFADDRS))  || (defined(ACE_ANDROID) && !defined ACE_LACKS_IF_NAMEINDEX)) && !defined(OPENDDS_SAFETY_PROFILE)

#define OPENDDS_NETWORK_CONFIG_MODIFIER

#include "NetworkConfigMonitor.h"
#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export NetworkConfigModifier : public NetworkConfigMonitor {
public:
  bool open();
  bool close();
  void update_interfaces();

  void add_interface(const OPENDDS_STRING& name);
  void remove_interface(const OPENDDS_STRING& name);
  void add_address(const OPENDDS_STRING& name,
                   bool can_multicast,
                   const ACE_INET_Addr& addr);
  void remove_address(const OPENDDS_STRING& name, const ACE_INET_Addr& addr);
};
typedef RcHandle<NetworkConfigModifier> NetworkConfigModifier_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_GETIFADDRS && ! OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H
