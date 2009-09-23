/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReceiverLogic.h"

#if !defined (__ACE_INLINE__)
#include "ReceiverLogic.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;

void
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::receive(
  const Packet& p,
  PacketVector& nacks,
  PacketVector& delivered)
{
  delivered.clear();

  // todo: validate
  if (!seen_last_delivered_) {
    if (
      p.type_ == Packet::DATA_INTERMEDIATE ||
      p.type_ == Packet::DATA_END_OF_MESSAGE) {
      last_delivered_id_ = p.id_ - 1;
      seen_last_delivered_ = true;

    } else {
      return;
    }
  }

  if (
    p.type_ == Packet::DATA_INTERMEDIATE ||
    p.type_ == Packet::DATA_END_OF_MESSAGE ||
    p.type_ == Packet::DATA_NOT_AVAILABLE) {
    bool prior_nack_canceled = nacker_.cancel(p.id_);

    if (in_range(p.id_, 1, receiver_buffer_size_ + receiver_buffer_size_)) {
      if (p.id_ == last_delivered_id_ + 1) {
        Packet tmp_packet;

        deliver(delivered, p);

        while (get_and_remove_buffered_packet(
                 last_delivered_id_ + 1,
                 tmp_packet)) {
          deliver(delivered, tmp_packet);
        }

      } else if (!is_buffered(p)) {
        if (
          p.type_ == Packet::DATA_INTERMEDIATE ||
          p.type_ == Packet::DATA_END_OF_MESSAGE) {
          buffer_packet(p, delivered);

          if (!prior_nack_canceled) {
            nacker_.nack_range(find_previous_received(p.id_) + 1, p.id_);
          }

        } else if (p.type_ == Packet::DATA_NOT_AVAILABLE) {
          handle_unreliable_operation(delivered);
        }
      }
    }

  } else if (p.type_ == Packet::HEARTBEAT) {
    if (!in_range(p.id_, 0 - 2 * receiver_buffer_size_, 0)) {
      // NACK the last packet, which will send it along and
      // then trigger the above NACK code...
      nacker_.nack_range(p.id_, p.id_ + 1);
    }
  }

  nacker_.get_nacks(nacks);
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::in_range(
  const Packet::id_type& id,
  int minadd,
  int maxadd)
{
  Packet::id_type min = last_delivered_id_ + minadd;
  Packet::id_type max = last_delivered_id_ + maxadd;
  bool valid = false;

  if (max < min) {
    valid = (id >= min) || (id <= max);

  } else {
    valid = (id >= min) && (id <= max);
  }

  return valid;
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::get_and_remove_buffered_packet(
  const Packet::id_type& id,
  Packet& p)
{
  bool found = false;
  BufferType::iterator iter = buffer_.find(id);
  found = (iter != buffer_.end());

  if (iter != buffer_.end()) {
    p = iter->second;
    buffer_.erase(iter);
  }

  return found;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::deliver(
  PacketVector& delivered,
  const Packet& p)
{
  delivered.push_back(p);
  last_delivered_id_ = p.id_;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::buffer_packet(
  const Packet& p,
  PacketVector& delivered)
{
  buffer_.insert(std::make_pair(p.id_, p));

  if (buffersize() == receiver_buffer_size_) {
    handle_unreliable_operation(delivered);
  }
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::is_buffered(
  const Packet& p) const
{
  return buffer_.find(p.id_) != buffer_.end();
}

Packet::id_type
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::find_previous_received(
  const Packet::id_type& id) const
{
  Packet::id_type result = last_delivered_id_;
  BufferType::const_iterator iter = buffer_.find(id);

  if (iter == buffer_.end()) {
    throw std::runtime_error("expected data not buffered");

  } else if (iter != buffer_.begin()) {
    --iter;
    result = iter->first;

  } else if (buffer_.size() != 1) { // we have wrapped around
    iter = buffer_.end();
    --iter;
    result = iter->first;
  }

  return result;
}

size_t
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::buffersize() const
{
  return buffer_.size();
}

Packet::id_type
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::find_beginning_of_consecutive_range(
  const Packet::id_type& end) const
{
  Packet::id_type begin = end;

  for (size_t idx = 0; idx < buffer_.size(); ++idx) {
    if (buffer_.find(begin - 1) == buffer_.end()) {
      return begin;
    }

    --begin;
  }

  return begin;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReceiverLogic::handle_unreliable_operation(
  PacketVector& delivered)
{
  // for "hard" reliability:
  // throw
  // for "soft" reliability:
  // send last consecutive buffered group of packets we have (latest data)
  // clear buffer
  if (reliability_ == HARD_RELIABILITY) {
    throw std::runtime_error("disconnected");

  } else if (reliability_ == SOFT_RELIABILITY) {
    if (!buffer_.empty()) {
      BufferType::const_iterator iter = buffer_.end();
      --iter;

      // handle wraparound
      while (
        iter->first - buffer_.begin()->first >=
        receiver_buffer_size_ + receiver_buffer_size_) {
        --iter;
      }

      Packet::id_type last_id = iter->first + 1;
      Packet::id_type first_id =
        find_beginning_of_consecutive_range(last_id - 1);

      for (Packet::id_type id = first_id; id != last_id; ++id) {
        Packet tmp_packet;

        get_and_remove_buffered_packet(id, tmp_packet);
        deliver(delivered, tmp_packet);
      }

      buffer_.clear();
      nacker_.cancel_all();
    }
  }
}
