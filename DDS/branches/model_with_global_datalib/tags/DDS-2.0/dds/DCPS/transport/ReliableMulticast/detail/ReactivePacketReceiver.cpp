/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReactivePacketReceiver.h"
#include "Packet.h"
#include "PacketReceiverCallback.h"

#if !defined (__ACE_INLINE__)
#include "ReactivePacketReceiver.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;
typedef OpenDDS::DCPS::ReliableMulticast::detail::PacketReceiverCallback PacketReceiverCallback;

namespace {

void logError(
  const char* errMsg)
{
  ACE_ERROR((LM_ERROR, errMsg));
}

} // namespace

OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::ReactivePacketReceiver(
  const ACE_INET_Addr& multicast_group_address,
  PacketReceiverCallback& callback,
  size_t receiver_buffer_size)
  : callback_(callback)
  , multicast_group_address_(multicast_group_address)
  , receiver_buffer_size_(receiver_buffer_size)
{
}

bool
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::open()
{
  if (socket_.join(
        multicast_group_address_) == -1) {
    logError("ReactivePacketReceiver: failure to open\n");
    return false;
  }

  if (reactor()->register_handler(
        this,
        ACE_Event_Handler::READ_MASK) == -1) {
    logError("ReactivePacketReceiver: failure to register_handler\n");
    socket_.close();
    return false;
  }

  if (reactor()->schedule_timer(
        this,
        0,
        ACE_Time_Value(1),
        ACE_Time_Value(1)) == -1) {
    logError("ReactivePacketReceiver: failure to schedule_timer\n");
  }

  return true;
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::close()
{
  if (reactor()->cancel_timer(this) == -1) {
    logError("ReactivePacketReceiver: failure to cancel_timer\n");
  }

  OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::close();
  reactor(0);
}

void
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::receive_packet_from(
  const Packet& packet,
  const ACE_INET_Addr& peer)
{
  std::vector<Packet> nacks;
  std::vector<Packet> delivered;

  bool schedule_timer = false;

  try {
    ACE_Guard<ACE_Thread_Mutex> lock(nack_mutex_);

    if (receiver_logics_[peer].get() == 0) {
      receiver_logics_[peer].reset(new ReceiverLogic(receiver_buffer_size_));
    }

    receiver_logics_[peer]->receive(packet, nacks, delivered);

    if (nacks.empty()) {
      nacks_.erase(peer);

    } else {
      nacks_[peer] = nacks;
      schedule_timer = true;
    }

  } catch (std::exception&) {
    callback_.reliability_compromised();
    return;
  }

  if (!delivered.empty()) {
    callback_.received_packets(delivered);
  }

  if (schedule_timer) {
    if (reactor()->schedule_timer(this, 0, ACE_Time_Value(0, 0)) == -1) {
      logError("ReactivePacketReceiver: Unable to schedule immediate timer\n");
    }
  }
}

int
OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver::handle_timeout(
  const ACE_Time_Value& current_time,
  const void*)
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
    ++iter) {
    send_many(iter->second, iter->first);
  }

  return 0;
}
