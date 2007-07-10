// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "PacketHandler.h"
#include "PacketSerializer.h"
#include "ace/Auto_Ptr.h"

#if !defined (__ACE_INLINE__)
#include "PacketHandler.inl"
#endif /* __ACE_INLINE__ */

typedef OpenDDS::DCPS::ReliableMulticast::detail::Packet Packet;
typedef OpenDDS::DCPS::ReliableMulticast::detail::PacketSerializer PacketSerializer;

void
OpenDDS::DCPS::ReliableMulticast::detail::PacketHandler::send_packet_to(
  const Packet& packet,
  const ACE_INET_Addr& dest
  )
{
  size_t buffer_size = 0;
  PacketSerializer packetSerializer;
  ACE_Auto_Basic_Array_Ptr<char> serialized(
    packetSerializer.getBuffer(packet, buffer_size)
    );
  char* begin = packetSerializer.serializeFromTo(packet, serialized.get(), buffer_size);

  OpenDDS::DCPS::ReliableMulticast::detail::EventHandler::send(
    begin,
    buffer_size - (begin - serialized.get()),
    dest
    );
}

void
OpenDDS::DCPS::ReliableMulticast::detail::PacketHandler::receive(
  const char* buffer,
  size_t size,
  const ACE_INET_Addr& peer
  )
{
  PacketSerializer packetSerializer;
  Packet packet;

  packetSerializer.serializeFromTo(buffer, size, packet);
  receive_packet_from(packet, peer);
}
