// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportSendStrategy.h"
#include "detail/Packetizer.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

typedef TAO::DCPS::ReliableMulticast::detail::Packet Packet;
typedef TAO::DCPS::ReliableMulticast::detail::Packetizer Packetizer;
typedef TAO::DCPS::ReliableMulticast::detail::ReactivePacketSender ReactivePacketSender;

void
TAO::DCPS::ReliableMulticastTransportSendStrategy::configure(
  ACE_Reactor* reactor,
  const ACE_INET_Addr& local_address,
  const ACE_INET_Addr& multicast_group_address,
  size_t sender_history_size
  )
{
  sender_.reset(new ReactivePacketSender(
    local_address,
    multicast_group_address,
    sender_history_size
    ));
  sender_->reactor(reactor);
  sender_->open();
}

void
TAO::DCPS::ReliableMulticastTransportSendStrategy::teardown()
{
  if (sender_.get() != 0)
  {
    sender_->close();
    sender_.reset();
  }
}

void
TAO::DCPS::ReliableMulticastTransportSendStrategy::stop_i()
{
  teardown();
}

ssize_t
TAO::DCPS::ReliableMulticastTransportSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
{
  ACE_UNUSED_ARG(bp);

  ssize_t sent = 0;
  for (int idx = 0; idx < n; ++idx)
  {
    sent += iov[idx].iov_len;
  }

  Packetizer packetizer;
  std::vector<Packet> packets;

  packetizer.packetize(iov, n, packets);
  for (
    std::vector<Packet>::const_iterator packet = packets.begin();
    packet != packets.end();
    ++packet
    )
  {
    sender_->send(*packet);
  }

  return sent;
}

ACE_HANDLE
TAO::DCPS::ReliableMulticastTransportSendStrategy::get_handle()
{
  return ACE_INVALID_HANDLE;
}

ssize_t
TAO::DCPS::ReliableMulticastTransportSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ACE_UNUSED_ARG(iov);
  ACE_UNUSED_ARG(n);

  return -1;
}
