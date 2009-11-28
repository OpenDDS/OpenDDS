/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PACKETIZER_H
#define OPENDDS_DCPS_PACKETIZER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "ace/SOCK_IO.h"
#include <vector>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

struct Packet;

class ReliableMulticast_Export Packetizer {
public:
  enum {
    MAX_PAYLOAD_SIZE = 1024
  };

  void packetize(
    const iovec iov[],
    int size,
    std::vector<Packet>& packets);
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "Packetizer.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_PACKETIZER_H */
