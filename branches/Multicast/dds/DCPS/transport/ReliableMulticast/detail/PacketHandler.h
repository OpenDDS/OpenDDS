/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PACKETHANDLER_H
#define OPENDDS_DCPS_PACKETHANDLER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "EventHandler.h"
#include "Packet.h"
#include <cstring>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

class ReliableMulticast_Export PacketHandler
  : public OpenDDS::DCPS::ReliableMulticast::detail::EventHandler {
public:
  template <typename Container> void send_many(
    const Container& container,
    const ACE_INET_Addr& dest) {
    for (
      typename Container::const_iterator iter = container.begin();
      iter != container.end();
      ++iter) {
      send_packet_to(*iter, dest);
    }
  }

  virtual void send_packet_to(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& packet,
    const ACE_INET_Addr& dest);

  virtual void receive(
    const char* buffer,
    size_t size,
    const ACE_INET_Addr& peer);

  virtual void receive_packet_from(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& packet,
    const ACE_INET_Addr& peer) = 0;
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "PacketHandler.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_PACKETHANDLER_H */
