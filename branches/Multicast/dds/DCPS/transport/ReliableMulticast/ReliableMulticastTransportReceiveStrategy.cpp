/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastTransportReceiveStrategy.h"
#include "ReliableMulticastTransportImpl.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastTransportReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;
typedef OpenDDS::DCPS::ReliableMulticast::detail::ReactivePacketReceiver ReactivePacketReceiver;
typedef OpenDDS::DCPS::ReliableMulticastTransportImpl ReliableMulticastTransportImpl;

namespace {

#if defined (_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning (disable:4996) //std::copy OK here
#endif
/// Fill an iovec as much as possible with data from a packet.
/// Increment iovidx if we hit the end of an iovec,
/// erase data from packets accordingly and increment packetidx if
/// we run out of data in the packet.
size_t fillIovec(
  iovec iov,
  int& iovidx,
  Packet& packet,
  size_t& packetidx)
{
  size_t to_fill = std::min(size_t(iov.iov_len), packet.payload_.size());

  if (to_fill == 0) {
    return 0;
  }

  // start filling at (char*)(iov.iov_base) + iov.iov_len
  char* iov_cbase = reinterpret_cast<char*>(iov.iov_base);
  std::copy(
    packet.payload_.data(),
    packet.payload_.data() + to_fill,
    iov_cbase);
  iov.iov_base = iov_cbase + to_fill;
  iov.iov_len -= to_fill;
  packet.payload_.erase(packet.payload_.begin(), packet.payload_.begin() + to_fill);

  if (iov.iov_len == 0) {
    ++iovidx;
  }

  if (packet.payload_.empty()) {
    ++packetidx;
  }

  return to_fill;
}
#if defined (_MSC_VER) && (_MSC_VER >= 1400)
#pragma warning (default:4996)
#endif

size_t fillIovecArray(
  std::vector<Packet>& packets,
  iovec iov[],
  int n)
{
  size_t filled = 0;
  size_t packetidx = 0;
  int iovidx = 0;
  bool sent_end = false;

  while ((iovidx < n) && (packetidx < packets.size()) && !sent_end) {
    Packet& packet = packets[packetidx];
    filled += fillIovec(iov[iovidx], iovidx, packets[packetidx], packetidx);
    sent_end = packet.type_ == Packet::DATA_END_OF_MESSAGE;
  }

  // Clean up
  while (!packets.empty() && packets.begin()->payload_.empty()) {
    packets.erase(packets.begin());
  }

  return filled;
}

} // namespace

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::configure(
  ACE_Reactor* reactor,
  const ACE_INET_Addr& multicast_group_address,
  size_t receiver_buffer_size)
{
  receiver_.reset(new ReactivePacketReceiver(
                    multicast_group_address,
                    *this,
                    receiver_buffer_size));
  receiver_->reactor(reactor);
  receiver_->open();
}

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::teardown()
{
  if (receiver_.get() != 0) {
    receiver_->close();
    receiver_.reset();
  }
}

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::received_packets(
  const std::vector<Packet>& packets)
{
  buffered_packets_.insert(
    buffered_packets_.end(),
    packets.begin(),
    packets.end());

  std::vector<Packet>::iterator iter = buffered_packets_.begin();

  while (iter != buffered_packets_.end()) {
    if (iter->type_ == Packet::DATA_END_OF_MESSAGE) {
      // we have received one complete "message" from the upper layer
      handle_input();
      // reset our iterator as things have gotten erased!
      iter = buffered_packets_.begin();

    } else {
      ++iter;
    }
  }
}

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::reliability_compromised()
{
  gracefully_disconnected_ = true;
}

ssize_t
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::receive_bytes(
  iovec iov[],
  int n,
  ACE_INET_Addr& remote_address)
{
  ACE_UNUSED_ARG(remote_address);

  return fillIovecArray(buffered_packets_, iov, n);
}

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::deliver_sample(
  ReceivedDataSample& sample,
  const ACE_INET_Addr& remote_address)
{
  ACE_UNUSED_ARG(remote_address);

  if (sample.header_.message_id_ == GRACEFUL_DISCONNECT) {
    gracefully_disconnected_ = true;

  } else if (sample.header_.message_id_ == FULLY_ASSOCIATED) {
    ReliableMulticastTransportImpl_rch transport = data_link_.get_transport_impl();
    transport->demarshal_acks(
      sample.sample_,
      sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);

  } else {
    data_link_.data_received(sample);
  }
}

int
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::start_i()
{
  return 0;
}

void
OpenDDS::DCPS::ReliableMulticastTransportReceiveStrategy::stop_i()
{
  teardown();
}
