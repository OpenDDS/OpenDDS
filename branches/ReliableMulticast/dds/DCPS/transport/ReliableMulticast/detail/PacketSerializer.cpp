// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "PacketSerializer.h"
#include "Packet.h"
#include "ace/Auto_Ptr.h"
#include "ace/CDR_Stream.h"

#if !defined (__ACE_INLINE__)
#include "PacketSerializer.inl"
#endif /* __ACE_INLINE__ */

typedef TAO::DCPS::ReliableMulticast::detail::Packet Packet;

char*
TAO::DCPS::ReliableMulticast::detail::PacketSerializer::getBuffer(
  const Packet& packet,
  size_t& size
  ) const
{
  size = 0;
  size += 4 + 4;
  if (packet.type_ == Packet::NACK)
  {
    size += 4 + 4;
  }
  else if (
    packet.type_ == Packet::DATA_INTERMEDIATE ||
    packet.type_ == Packet::DATA_END_OF_MESSAGE
    )
  {
    size += 4;
    size += packet.payload_.size();
  }
  return new char[size];
}

void
TAO::DCPS::ReliableMulticast::detail::PacketSerializer::serializeFromTo(
  const Packet& packet,
  char* buffer,
  size_t size
  ) const
{
  ACE_OutputCDR output(buffer, size);

  output << packet.id_;
  output << ACE_CDR::Long(packet.type_);
  if (packet.type_ == Packet::NACK)
  {
    output << packet.nack_begin_;
    output << packet.nack_end_;
  }
  else if (
    packet.type_ == Packet::DATA_INTERMEDIATE ||
    packet.type_ == Packet::DATA_END_OF_MESSAGE
    )
  {
    output << packet.payload_.size();
    output.write_char_array(packet.payload_.data(), packet.payload_.size());
  }
}

void
TAO::DCPS::ReliableMulticast::detail::PacketSerializer::serializeFromTo(
  const char* buffer,
  size_t size,
  Packet& packet
  ) const
{
  ACE_InputCDR input(buffer, size);
  ACE_CDR::Long type;

  packet.payload_.clear();
  input >> packet.id_;
  input >> type;
  packet.type_ = Packet::PacketType(type);
  if (packet.type_ == Packet::NACK)
  {
    input >> packet.nack_begin_;
    input >> packet.nack_end_;
  }
  else if (
    packet.type_ == Packet::DATA_INTERMEDIATE ||
    packet.type_ == Packet::DATA_END_OF_MESSAGE
    )
  {
    size_t arraysize = 0;
    input >> arraysize;
    ACE_Auto_Basic_Array_Ptr<char> safe_array(new char[arraysize]);
    input.read_char_array(safe_array.get(), arraysize);
    packet.payload_.assign(safe_array.get(), arraysize);
  }
}
