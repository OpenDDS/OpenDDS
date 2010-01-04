/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/Serializer.h"
#include "EntryExit.h"

#include <algorithm>

ACE_INLINE
OpenDDS::DCPS::TransportHeader::TransportHeader()
  : byte_order_(TAO_ENCAP_BYTE_ORDER),
    reserved_(0),
    length_(0),
    sequence_(0),
    source_(0)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);

  std::copy(&DCPS_PROTOCOL[0], &DCPS_PROTOCOL[6], this->protocol_);
}

ACE_INLINE
OpenDDS::DCPS::TransportHeader::TransportHeader(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);
  this->init(buffer);
}

ACE_INLINE
OpenDDS::DCPS::TransportHeader::TransportHeader(ACE_Message_Block& buffer)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);
  this->init(&buffer);
}

ACE_INLINE
OpenDDS::DCPS::TransportHeader&
OpenDDS::DCPS::TransportHeader::operator=(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","operator=",6);
  this->init(buffer);
  return *this;
}

ACE_INLINE
OpenDDS::DCPS::TransportHeader&
OpenDDS::DCPS::TransportHeader::operator=(ACE_Message_Block& buffer)
{
  DBG_ENTRY_LVL("TransportHeader","operator=",6);
  this->init(&buffer);
  return *this;
}

ACE_INLINE
size_t
OpenDDS::DCPS::TransportHeader::max_marshaled_size()
{
  DBG_ENTRY_LVL("TransportHeader","max_marshaled_size",6);
  // Representation takes no extra space for encoding.
  return sizeof(this->protocol_) +
         sizeof(this->byte_order_) +
         sizeof(this->reserved_) +
         sizeof(this->length_) +
         sizeof(this->sequence_) +
         sizeof(this->source_);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::swap_bytes() const
{
  DBG_ENTRY_LVL("TransportHeader","swap_bytes",6);

  return this->byte_order_ != TAO_ENCAP_BYTE_ORDER;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::valid() const
{
  DBG_ENTRY_LVL("TransportHeader","valid",6);

  // Currently we do not support compatibility with other
  // versions of the protocol:
  return std::equal(&DCPS_PROTOCOL[0], &DCPS_PROTOCOL[6], this->protocol_);
}

ACE_INLINE
void
OpenDDS::DCPS::TransportHeader::init(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","init",6);

  TAO::DCPS::Serializer reader(buffer);

  reader.read_octet_array(this->protocol_, sizeof(this->protocol_));

  reader >> ACE_InputCDR::to_octet(this->byte_order_);
  reader >> ACE_InputCDR::to_octet(this->reserved_);

  reader.swap_bytes(swap_bytes());

  // Extract the length_ value.
  reader >> this->length_;

  // Extract the sequence_ value.
  reader >> this->sequence_;

  // Extract the source_id_ value.
  reader >> this->source_;
}
