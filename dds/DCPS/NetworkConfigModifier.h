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
};
typedef RcHandle<NetworkConfigModifier> NetworkConfigModifier_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_GETIFADDRS && ! OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H
