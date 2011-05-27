/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "Packetizer.h"
#include "Packet.h"

#if !defined (__ACE_INLINE__)
#include "Packetizer.inl"
#endif /* __ACE_INLINE__ */

//We'd like the following #include to give us the function std::min()
//and not to get confused with any macros named min.
#ifdef min
#  undef min
#endif

#include <algorithm>

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;

namespace {

void accumulate(
  const void* buffer,
  size_t size_remaining,
  std::vector<Packet>& packets)
{
  size_t offset = 0;

  while (size_remaining > 0) {
    if (
      packets.empty() ||
      (packets[packets.size() - 1].payload_.size() == OpenDDS::DCPS::ReliableMulticast::detail::Packetizer::MAX_PAYLOAD_SIZE)) {
      packets.push_back(Packet(0, Packet::DATA_INTERMEDIATE));
    }

    Packet& packet = packets[packets.size() - 1];
    size_t room_left_in_packet =
      OpenDDS::DCPS::ReliableMulticast::detail::Packetizer::MAX_PAYLOAD_SIZE - packet.payload_.size();
    size_t to_copy = std::min(room_left_in_packet, size_remaining);
    packet.payload_.append(reinterpret_cast<const char*>(buffer) + offset, to_copy);
    size_remaining -= to_copy;
    offset += to_copy;
  }

  if (!packets.empty()) {
    packets[packets.size() - 1].type_ = Packet::DATA_END_OF_MESSAGE;
  }
}

} // namespace

void
OpenDDS::DCPS::ReliableMulticast::detail::Packetizer::packetize(
  const iovec iov[],
  int size,
  std::vector<Packet>& packets)
{
  packets.clear();

  for (int idx = 0; idx < size; ++idx) {
    accumulate(iov[idx].iov_base, iov[idx].iov_len, packets);
  }
}
