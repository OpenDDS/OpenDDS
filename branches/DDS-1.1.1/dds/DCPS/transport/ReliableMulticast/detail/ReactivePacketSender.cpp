// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReactivePacketSender.h"
#include "Packet.h"

#if !defined (__ACE_INLINE__)
#include "ReactivePacketSender.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::PacketHandler PacketHandler;
typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;

namespace
{
  void logError(
    const char* errMsg
    )
  {
    ACE_ERROR((LM_ERROR, errMsg));
  }
}

OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::ReactivePacketSender(
  const ACE_INET_Addr& local_address,
  const ACE_INET_Addr& multicast_group_address,
  size_t sender_history_size
  )
  : sender_logic_(sender_history_size)
  , local_address_(local_address)
  , multicast_group_address_(multicast_group_address)
{
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::open(
  )
{
  if (socket_.ACE_SOCK_Dgram::open(
    local_address_
    ) == -1)
  {
    logError("ReactivePacketSender: failure to open\n");
    return false;
  }
  if (reactor()->register_handler(
    this,
    ACE_Event_Handler::READ_MASK
    ) == -1)
  {
    logError("ReactivePacketSender: failure to register_handler\n");
    return false;
  }
  if (reactor()->schedule_timer(
    this,
    0,
    ACE_Time_Value(1),
    ACE_Time_Value(1)
    ) == -1)
  {
    logError("ReactivePacketSender: failure to schedule_timer\n");
  }
  return true;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::close()
{
  if (reactor()->cancel_timer(this) == -1)
  {
    logError("ReactivePacketSender: failure to cancel_timer\n");
  }
  OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::close();
  reactor(0);
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::send_packet(
  const Packet& p
  )
{
  std::vector<Packet> to_deliver;

  {
    ACE_Guard<ACE_Thread_Mutex> lock(heartbeat_mutex_);
    sender_logic_.send(p, to_deliver);
  }
  send_many(to_deliver, multicast_group_address_);
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::receive_packet_from(
  const Packet& packet,
  const ACE_INET_Addr& peer
  )
{
  std::vector<Packet> to_redeliver;
  ACE_UNUSED_ARG(peer);

  {
    ACE_Guard<ACE_Thread_Mutex> lock(heartbeat_mutex_);
    sender_logic_.receive(packet, to_redeliver);
  }
  send_many(to_redeliver, multicast_group_address_);
}

int
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender::handle_timeout(
  const ACE_Time_Value& current_time,
  const void*
  )
{
  ACE_UNUSED_ARG(current_time);

  Packet heartbeat;
  {
    ACE_Guard<ACE_Thread_Mutex> lock(heartbeat_mutex_);
    sender_logic_.make_heartbeat(heartbeat);
  }
  PacketHandler::send_packet_to(heartbeat, multicast_group_address_);
  return 0;
}
