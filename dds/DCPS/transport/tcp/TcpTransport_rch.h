/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TCP_TCPTRANSPORT_RCH_H
#define OPENDDS_DCPS_TRANSPORT_TCP_TCPTRANSPORT_RCH_H

#include <dds/DCPS/RcHandle_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpTransport;

typedef RcHandle<TcpTransport> TcpTransport_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_TCP_TCPTRANSPORT_RCH_H */
