// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReactivePacketReceiver.h"
#include "Packet.h"

#if !defined (__ACE_INLINE__)
#include "ReactivePacketReceiver.inl"
#endif /* __ACE_INLINE__ */

typedef TAO::DCPS::ReliableMulticast::detail::Packet Packet;

namespace
{
  void logError(
    const char* errMsg
    )
  {
  }
}

TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::ReactivePacketReceiver(
  const ACE_INET_Addr& multicast_group_address
  )
  : multicast_group_address_(multicast_group_address)
{
}

TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::~ReactivePacketReceiver(
  )
{
}

bool
TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::open(
  )
{
  if (socket_.join(
    multicast_group_address_
    ) == -1)
  {
    logError("ReactivePacketReceiver: failure to open");
    return false;
  }
  if (reactor()->register_handler(
    this,
    ACE_Event_Handler::READ_MASK
    ) == -1)
  {
    logError("ReactivePacketReceiver: failure to register_handler");
    socket_.close();
    return false;
  }
  if (reactor()->schedule_timer(
    this,
    0,
    ACE_Time_Value(1),
    ACE_Time_Value(1)
    ) == -1)
  {
    logError("ReactivePacketReceiver: failure to schedule_timer");
  }
  return true;
}

void
TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::receive(
  const Packet& packet,
  const ACE_INET_Addr& peer
  )
{
  std::vector<Packet> nacks;
  std::vector<Packet> delivered;

  bool schedule_timer = false;
  {
    ACE_Guard<ACE_Thread_Mutex> lock(nack_mutex_);

    receiver_logics_[peer].receive(packet, nacks, delivered);
    if (nacks.empty())
    {
      nacks_.erase(peer);
    }
    else
    {
      nacks_[peer] = nacks;
      schedule_timer = true;
    }
  }
  if (schedule_timer)
  {
    if (reactor()->schedule_timer(this, 0, ACE_Time_Value(0, 0)) == -1)
    {
      logError("Unable to schedule immediate timer");
    }
  }
}

int
TAO::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::handle_timeout(
  const ACE_Time_Value& current_time,
  const void*
  )
{
  ACE_UNUSED_ARG(current_time);

  PeerToPacketVectorMap nacks;
  {
    ACE_Guard<ACE_Thread_Mutex> lock(nack_mutex_);
    nacks = nacks_;
  }
  for (
    PeerToPacketVectorMap::iterator iter = nacks.begin();
    iter != nacks.end();
    ++iter
    )
  {
    send_many(iter->second, iter->first);
  }
  return 0;
}
