/*
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPRECEIVESTRATEGY_RCH_H
#define OPENDDS_TCPRECEIVESTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpReceiveStrategy;

typedef RcHandle<TcpReceiveStrategy> TcpReceiveStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_TCPRECEIVESTRATEGY_RCH_H */
