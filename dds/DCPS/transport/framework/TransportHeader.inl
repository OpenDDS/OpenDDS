/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/Serializer.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportHeader::TransportHeader()
  : version_(DCPS_VERSION),
    source_(0),
    sequence_(0),
    length_(0)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);

  this->byte_order_ = TAO_ENCAP_BYTE_ORDER;
  this->protocol_[0] = DCPS_PROTOCOL[0];  // D
  this->protocol_[1] = DCPS_PROTOCOL[1];  // C
  this->protocol_[2] = DCPS_PROTOCOL[2];  // P
  this->protocol_[3] = DCPS_PROTOCOL[3];  // S
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
  return sizeof(this->byte_order_) +
         sizeof(this->protocol_) +
         sizeof(this->version_) +
         sizeof(this->source_) +
         sizeof(this->sequence_) +
         sizeof(this->length_);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::valid() const
{
  DBG_ENTRY_LVL("TransportHeader","valid",6);

  return this->protocol_[0] == DCPS_PROTOCOL[0] &&  // D
         this->protocol_[1] == DCPS_PROTOCOL[1] &&  // C
         this->protocol_[2] == DCPS_PROTOCOL[2] &&  // P
         this->protocol_[3] == DCPS_PROTOCOL[3] &&  // S
         this->version_ == DCPS_VERSION;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportHeader::init(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","init",6);

  TAO::DCPS::Serializer reader(buffer);

  // Extract the byte order for the transport header.
  reader >> ACE_InputCDR::to_octet(this->byte_order_);

  reader.swap_bytes(this->byte_order_ != TAO_ENCAP_BYTE_ORDER);

  // Extract the packet_id_ octet values.
  reader.read_octet_array(this->protocol_, sizeof(this->protocol_));
  if (reader.good_bit() != true) return;  // bad reader

  // Extract the version_ value.
  reader >> ACE_InputCDR::to_octet(this->version_);

  // Extract the source_id_ value.
  reader >> this->source_;

  // Extract the sequence_ value.
  reader >> this->sequence_;

  // Extract the length_ value.
  reader >> this->length_;
}
