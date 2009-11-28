/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PACKET_H
#define OPENDDS_DCPS_PACKET_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "ace/Basic_Types.h"
#include <string>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

struct ReliableMulticast_Export Packet {
  typedef ACE_UINT32 id_type;

  enum PacketType {
    DATA_INTERMEDIATE,
    DATA_END_OF_MESSAGE,
    NACK,
    DATA_NOT_AVAILABLE,
    HEARTBEAT
  };

  Packet(
    id_type id = 0,
    const PacketType& type = DATA_INTERMEDIATE,
    id_type begin = 0,
    id_type end = 0);

  bool operator<(
    const Packet& rhs) const;

  bool operator==(
    const Packet& rhs) const;

  bool operator!=(
    const Packet& rhs) const;

  id_type id_;
  PacketType type_;
  id_type nack_begin_;
  id_type nack_end_;
  std::string payload_;
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "Packet.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_PACKET_H */
