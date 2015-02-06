/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPCONNECTION_RCH_H
#define OPENDDS_TCPCONNECTION_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class TcpConnection;

typedef RcHandle<TcpConnection> TcpConnection_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_TCPCONNECTION_RCH_H */
