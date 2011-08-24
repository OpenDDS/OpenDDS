/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPDATALINK_RCH_H
#define OPENDDS_TCPDATALINK_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class TcpDataLink;

typedef RcHandle<TcpDataLink> TcpDataLink_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DATALINK_RCH_H */
