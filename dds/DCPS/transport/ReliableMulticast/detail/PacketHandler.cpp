// -*- C++ -*-
//
// $Id$

#include "PacketHandler.h"

#if !defined (__ACE_INLINE__)
#include "PacketHandler.inl"
#endif /* __ACE_INLINE__ */

typedef TAO::DCPS::ReliableMulticast::detail::Packet Packet;

namespace
{
  class SafeArray
  {
  public:
    SafeArray(size_t size)
      : buffer_(new char[size])
      , size_(size)
    {
    }

    ~SafeArray()
    {
      delete [] buffer_;
    }

    inline char* buffer() const
    {
      return buffer_;
    }

    inline size_t size() const
    {
      return size_;
    }

  private:
    SafeArray(const SafeArray& rhs);

    void operator=(const SafeArray& rhs);

    char* buffer_;
    size_t size_;
  };
}

void
TAO::DCPS::ReliableMulticast::detail::PacketHandler::send(
  const Packet& packet,
  const ACE_INET_Addr& dest
  )
{
  const size_t buffer_size =
    sizeof(Packet::id_type) +
    sizeof(Packet::PacketType) +
    sizeof(Packet::id_type) +
    sizeof(Packet::id_type) +
    packet.payload_.size();
  SafeArray serialized(buffer_size);
  size_t offset = 0;

  std::memcpy(
    serialized.buffer() + offset,
    &packet.id_,
    sizeof(packet.id_)
    );
  offset += sizeof(packet.id_);
  std::memcpy(
    serialized.buffer() + offset,
    &packet.type_,
    sizeof(packet.type_)
    );
  offset += sizeof(packet.type_);
  std::memcpy(
    serialized.buffer() + offset,
    &packet.nack_begin_,
    sizeof(packet.nack_begin_)
    );
  offset += sizeof(packet.nack_begin_);
  std::memcpy(
    serialized.buffer() + offset,
    &packet.nack_end_,
    sizeof(packet.nack_end_)
    );
  offset += sizeof(packet.nack_end_);
  std::memcpy(
    serialized.buffer() + offset,
    packet.payload_.data(),
    packet.payload_.size()
    );
  TAO::DCPS::ReliableMulticast::detail::EventHandler::send(
    serialized.buffer(),
    serialized.size(),
    dest
    );
}

void
TAO::DCPS::ReliableMulticast::detail::PacketHandler::receive(
  char* buffer,
  size_t size,
  const ACE_INET_Addr& peer
  )
{
  Packet packet;
  size_t offset = 0;

  std::memcpy(
    &packet.id_,
    buffer + offset,
    sizeof(packet.id_)
    );
  offset += sizeof(packet.id_);
  std::memcpy(
    &packet.type_,
    buffer + offset,
    sizeof(packet.type_)
    );
  offset += sizeof(packet.type_);
  std::memcpy(
    &packet.nack_begin_,
    buffer + offset,
    sizeof(packet.nack_begin_)
    );
  offset += sizeof(packet.nack_begin_);
  std::memcpy(
    &packet.nack_end_,
    buffer + offset,
    sizeof(packet.nack_end_)
    );
  offset += sizeof(packet.nack_end_);
  packet.payload_.assign(
    buffer + offset,
    size - offset
    );
  receive(packet, peer);
}
