/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTICAST_MANAGER_H
#define OPENDDS_DCPS_MULTICAST_MANAGER_H

#include "dcps_export.h"

#include "NetworkConfigMonitor.h"

#include <ace/SOCK_Dgram_Mcast.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MulticastManager {
public:
  /// Returns true if at least one group was joined.
  bool process(InternalDataReader<NetworkInterfaceAddress>::SampleSequence& samples,
               InternalSampleInfoSequence& infos,
               const OPENDDS_STRING& multicast_interface,
               ACE_Reactor* reactor,
               ACE_Event_Handler* event_handler,
               const NetworkAddress& multicast_group_address,
               ACE_SOCK_Dgram_Mcast& multicast_socket
               #ifdef ACE_HAS_IPV6
               , const NetworkAddress& ipv6_multicast_group_address,
               ACE_SOCK_Dgram_Mcast& ipv6_multicast_socket
               #endif
               );
private:
  OPENDDS_SET(OPENDDS_STRING) joined_interfaces_;
#ifdef ACE_HAS_IPV6
  OPENDDS_SET(OPENDDS_STRING) ipv6_joined_interfaces_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_MULTICAST_MANAGER_H
