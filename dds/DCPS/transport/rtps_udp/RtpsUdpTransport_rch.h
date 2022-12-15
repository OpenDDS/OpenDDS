/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_RCH_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_RCH_H

#include <dds/DCPS/RcHandle_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpTransport;

typedef RcHandle<RtpsUdpTransport> RtpsUdpTransport_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPTRANSPORT_RCH_H */
