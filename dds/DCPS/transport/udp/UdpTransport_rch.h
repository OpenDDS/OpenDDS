/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPTRANSPORT_RCH_H
#define DCPS_UDPTRANSPORT_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class UdpTransport;

typedef RcHandle<UdpTransport> UdpTransport_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPTRANSPORT_RCH_H */
