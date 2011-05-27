/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTRCHANDLES_H
#define OPENDDS_DCPS_RELIABLEMULTICASTRCHANDLES_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class ReliableMulticastDataLink;
class ReliableMulticastTransportImpl;
class ReliableMulticastTransportConfiguration;

typedef RcHandle<ReliableMulticastDataLink> ReliableMulticastDataLink_rch;
typedef RcHandle<ReliableMulticastTransportImpl> ReliableMulticastTransportImpl_rch;
typedef RcHandle<ReliableMulticastTransportConfiguration> ReliableMulticastTransportConfiguration_rch;

} // namespace DCPS
} // namespace OpenDDS

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTRCHANDLES_H */
