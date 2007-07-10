// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "NackGenerator.h"

#if !defined (__ACE_INLINE__)
#include "NackGenerator.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;

bool
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::cancel(
  Packet::id_type id
  )
{
  bool result = false;
  Packet packet(id, Packet::NACK);
  PacketSet::iterator iter =
    find_nack_containing(packet);

  if (iter != nacks_.end())
  {
    Packet begin(iter->nack_begin_, Packet::NACK, iter->nack_begin_, id);
    Packet end(id + 1, Packet::NACK, id + 1, iter->nack_end_);

    nacks_.erase(iter);
    result = true;
    if (begin.nack_begin_ != begin.nack_end_)
    {
      nacks_.insert(begin);
    }
    if (end.nack_begin_ != end.nack_end_)
    {
      nacks_.insert(end);
    }
  }
  return result;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::cancel_all(
  )
{
  nacks_.clear();
}

void
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::nack_range(
  Packet::id_type begin,
  Packet::id_type end
  )
{
  if (begin == end)
  {
    return;
  }
  Packet nack(begin, Packet::NACK, begin, end);
  PacketSet::iterator beg =
    find_nack_containing(nack);

  //if (beg == nacks.end())
  {
    nacks_.insert(nack);
    beg = nacks_.find(begin);
  }

  PacketSet::iterator beg_prev =
    beg;

  if (beg == nacks_.begin())
  {
    beg_prev = nacks_.end();
  }
  --beg_prev;
  beg = join_nacks(beg_prev, beg);

  PacketSet::iterator beg_next =
    beg;

  ++beg_next;
  if (beg_next == nacks_.end())
  {
    beg_next = nacks_.begin();
  }
  join_nacks(beg, beg_next);
}

void
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::get_nacks(
  std::vector<Packet>& nacks
  )
{
  nacks.clear();
  std::copy(nacks_.begin(), nacks_.end(), std::back_inserter(nacks));
}

OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::PacketSet::iterator
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::find_nack_containing(
  const Packet& packet
  )
{
  PacketSet::iterator result = nacks_.upper_bound(packet);

  if (nacks_.empty())
  {
    return result;
  }
  if (result == nacks_.begin())
  {
    if (result->nack_end_ >= result->nack_begin_)
    {
      return nacks_.end();
    }
  }
  else
  {
    --result;
    if (packet.id_ >= result->nack_end_)
    {
      if (result->nack_end_ >= result->nack_begin_)
      {
        return nacks_.end();
      }
    }
  }
  return result;
}

OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::PacketSet::iterator
OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::join_nacks(
  OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::PacketSet::iterator first,
  OpenDDS::DCPS::ReliableMulticast::detail::NackGenerator::PacketSet::iterator second
  )
{
  if (second == nacks_.begin())
  {
    return second;
  }
  else if (second == nacks_.end())
  {
    return first;
  }

  if (
    (first->nack_end_ >= second->nack_begin_) ||
    (
      first->nack_begin_ <= second->nack_begin_ &&
      first->nack_begin_ > first->nack_end_
      )
    )
  {
    Packet nack(*first);

    nack.nack_end_ = second->nack_end_;
    nacks_.erase(first);
    nacks_.erase(second);
    return nacks_.insert(nack).first;
  }
  return second;
}
