/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RECEIVERLOGIC_H
#define OPENDDS_DCPS_RECEIVERLOGIC_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/transport/ReliableMulticast/ReliableMulticast_Export.h"
#include "Packet.h"
#include "NackGenerator.h"
#include <map>
#include <stdexcept>
#include <vector>

namespace OpenDDS {
namespace DCPS {
namespace ReliableMulticast {
namespace detail {

class ReliableMulticast_Export ReceiverLogic {
public:
  enum ReliabilityMode {
    HARD_RELIABILITY,
    SOFT_RELIABILITY
  };

  typedef std::vector<
  OpenDDS::DCPS::ReliableMulticast::detail::Packet
  > PacketVector;

  ReceiverLogic(
    size_t receiver_buffer_size,
    const ReliabilityMode& reliability = HARD_RELIABILITY);

  void receive(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p,
    PacketVector& nacks,
    PacketVector& delivered);

private:
  bool in_range(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type& id,
    int minadd,
    int maxadd);

  bool get_and_remove_buffered_packet(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type& id,
    OpenDDS::DCPS::ReliableMulticast::detail::Packet& p);

  void deliver(
    PacketVector& delivered,
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p);

  void buffer_packet(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p,
    PacketVector& delivered);

  bool is_buffered(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p) const;

  OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type find_previous_received(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type& id) const;

  size_t buffersize() const;

  OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type find_beginning_of_consecutive_range(
    const OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type& end) const;

  void handle_unreliable_operation(
    PacketVector& delivered);

  typedef std::map<
  OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type,
  OpenDDS::DCPS::ReliableMulticast::detail::Packet
  > BufferType;

  size_t receiver_buffer_size_;
  ReliabilityMode reliability_;
  bool seen_last_delivered_;
  OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type last_delivered_id_;
  OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator nacker_;
  BufferType buffer_;
};

} // namespace detail
} // namespace ReliableMulticast
} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ReceiverLogic.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RECEIVERLOGIC_H */
