/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PACKETRECEIVERCALLBACK_H
#define OPENDDS_DCPS_PACKETRECEIVERCALLBACK_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "Packet.h"
#include <vector>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

/// Specifies an interface only!
class ReliableMulticast_Export PacketReceiverCallback {
public:
  virtual ~PacketReceiverCallback() {}

  virtual void received_packets(
    const std::vector<Packet>& packets) = 0;

  virtual void reliability_compromised() = 0;
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_PACKETRECEIVERCALLBACK_H */
