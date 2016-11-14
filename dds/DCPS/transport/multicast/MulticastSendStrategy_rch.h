/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSENDSTRATEGY_RCH_H
#define DCPS_MULTICASTSENDSTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastSendStrategy;

typedef RcHandle<MulticastSendStrategy> MulticastSendStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTSENDSTRATEGY_RCH_H */
