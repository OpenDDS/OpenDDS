/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTRECEIVESTRATEGY_RCH_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTRECEIVESTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastReceiveStrategy;

typedef RcHandle<MulticastReceiveStrategy> MulticastReceiveStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTRECEIVESTRATEGY_RCH_H */
