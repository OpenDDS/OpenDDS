/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPRECEIVESTRATEGY_RCH_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPRECEIVESTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpReceiveStrategy;

typedef RcHandle<RtpsUdpReceiveStrategy> RtpsUdpReceiveStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPRECEIVESTRATEGY_RCH_H */
