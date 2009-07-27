// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportSendStrategy.h"
#include "detail/Packetizer.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportSendStrategy.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;
typedef OpenDDS::DCPS::ReliableMulticast::detail::Packetizer Packetizer;
typedef OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketSender ReactivePacketSender;

void
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::configure(
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
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::teardown()
{
  if (sender_.get() != 0)
  {
    sender_->close();
    sender_.reset();
  }
}

void
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::stop_i()
{
  teardown();
}

ssize_t
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::send_bytes(const iovec iov[], int n, int& bp)
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
    sender_->send_packet(*packet);
  }

  return sent;
}

ACE_HANDLE
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::get_handle()
{
  return ACE_INVALID_HANDLE;
}

ACE_SOCK&
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::socket()
{
  static ACE_SOCK_IO nilSocket;

  if( this->sender_.get()) {
    return this->sender_->socket();

  } else {
    return nilSocket;
  }
}

ssize_t
OpenDDS::DCPS::ReliableMulticastTransportSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ACE_UNUSED_ARG(iov);
  ACE_UNUSED_ARG(n);

  return -1;
}
