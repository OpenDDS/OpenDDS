// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "SenderLogic.h"

#if !defined (__ACE_INLINE__)
#include "SenderLogic.inl"
#endif /* __ACE_INLINE__ */

void
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::receive(
  const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& redelivered
  ) const
{
  redelivered.clear();
  if (p.type_ == OpenDDS::DCPS::ReliableMulticast::detail::Packet::NACK)
  {
    for (
      OpenDDS::DCPS::ReliableMulticast::detail::Packet::id_type id = p.nack_begin_;
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
        redelivered.push_back(OpenDDS::DCPS::ReliableMulticast::detail::Packet(
          id,
          OpenDDS::DCPS::ReliableMulticast::detail::Packet::DATA_NOT_AVAILABLE
          ));
      }
    }
  }
}

void
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::send(
  const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& delivered
  )
{
  delivered.clear();
  if (
    p.type_ == OpenDDS::DCPS::ReliableMulticast::detail::Packet::DATA_INTERMEDIATE ||
    p.type_ == OpenDDS::DCPS::ReliableMulticast::detail::Packet::DATA_END_OF_MESSAGE
    )
  {
    buffer_packet(p, delivered);
  }
}

void
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::make_heartbeat(
  OpenDDS::DCPS::ReliableMulticast::detail::Packet& p
  )
{
  p = OpenDDS::DCPS::ReliableMulticast::detail::Packet(
    current_id_ - 1,
    OpenDDS::DCPS::ReliableMulticast::detail::Packet::HEARTBEAT
    );
}

void
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::buffer_packet(
  const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p,
  PacketVector& delivered
  )
{
  OpenDDS::DCPS::ReliableMulticast::detail::Packet tmp(p);

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
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::is_buffered(
  const OpenDDS::DCPS::ReliableMulticast::detail::Packet& p
  ) const
{
  return buffer_.find(p.id_) != buffer_.end();
}

size_t
OpenDDS::DCPS::ReliableMulticast::detail::SenderLogic::buffersize() const
{
  return buffer_.size();
}
