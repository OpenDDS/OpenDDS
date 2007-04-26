// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "SenderLogic.h"

#if !defined (__ACE_INLINE__)
#include "SenderLogic.inl"
#endif /* __ACE_INLINE__ */

void
TAO::DCPS::ReliableMulticast::detail::SenderLogic::receive(
  const TAO::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& redelivered
  ) const
{
  redelivered.clear();
  if (p.type_ == TAO::DCPS::ReliableMulticast::detail::Packet::NACK)
  {
    for (
      TAO::DCPS::ReliableMulticast::detail::Packet::id_type id = p.nack_begin_;
      id != p.nack_end_;
      ++id
      )
    {
      BufferType::const_iterator iter = buffer_.find(id);

      if (iter != buffer_.end())
      {
        redelivered.push_back(iter->second);
      }
      else
      {
        redelivered.push_back(TAO::DCPS::ReliableMulticast::detail::Packet(
          id,
          TAO::DCPS::ReliableMulticast::detail::Packet::DATA_NOT_AVAILABLE
          ));
      }
    }
  }
}

void
TAO::DCPS::ReliableMulticast::detail::SenderLogic::send(
  const TAO::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& delivered
  )
{
  delivered.clear();
  if (
    p.type_ == TAO::DCPS::ReliableMulticast::detail::Packet::DATA_INTERMEDIATE ||
    p.type_ == TAO::DCPS::ReliableMulticast::detail::Packet::DATA_END_OF_MESSAGE
    )
  {
    buffer_packet(p, delivered);
  }
}

void
TAO::DCPS::ReliableMulticast::detail::SenderLogic::make_heartbeat(
  TAO::DCPS::ReliableMulticast::detail::Packet& p
  )
{
  p = TAO::DCPS::ReliableMulticast::detail::Packet(
    current_id_ - 1,
    TAO::DCPS::ReliableMulticast::detail::Packet::HEARTBEAT
    );
}

void
TAO::DCPS::ReliableMulticast::detail::SenderLogic::buffer_packet(
  const TAO::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& delivered
  )
{
  TAO::DCPS::ReliableMulticast::detail::Packet tmp(p);

  if (buffersize() == sender_history_size_)
  {
    buffer_.erase(current_id_ - sender_history_size_);
  }
  tmp.id_ = current_id_;
  ++current_id_;
  buffer_.insert(std::make_pair(tmp.id_, tmp));
  delivered.push_back(tmp);
}

bool
TAO::DCPS::ReliableMulticast::detail::SenderLogic::is_buffered(
  const TAO::DCPS::ReliableMulticast::detail::Packet& p
  ) const
{
  return buffer_.find(p.id_) != buffer_.end();
}

size_t
TAO::DCPS::ReliableMulticast::detail::SenderLogic::buffersize() const
{
  return buffer_.size();
}
