/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPSENDSTRATEGY_RCH_H
#define OPENDDS_TCPSENDSTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpSendStrategy;

typedef RcHandle<TcpSendStrategy> TcpSendStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TCPSENDSTRATEGY_RCH_H */
