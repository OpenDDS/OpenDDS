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
  : protocol_(DCPS_PROTOCOL),
    version_major_(DCPS_VERSION_MAJOR),
    version_minor_(DCPS_VERSION_MINOR),
    source_(0),
    sequence_(0),
    length_(0)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);
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
         sizeof(this->version_major_) +
         sizeof(this->version_minor_) +
         sizeof(this->source_) +
         sizeof(this->sequence_) +
         sizeof(this->length_);
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::swap_bytes() const
{
  DBG_ENTRY_LVL("TransportHeader","swap_bytes",6);

  return this->protocol_ == DCPS_PROTOCOL_SWAPPED;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::valid() const
{
  DBG_ENTRY_LVL("TransportHeader","valid",6);

  return (this->protocol_ == DCPS_PROTOCOL || swap_bytes()) &&
         this->version_major_ == DCPS_VERSION_MAJOR &&
         this->version_minor_ == DCPS_VERSION_MINOR;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportHeader::init(ACE_Message_Block* buffer)
{
  DBG_ENTRY_LVL("TransportHeader","init",6);

  TAO::DCPS::Serializer reader(buffer);

  // Extract the protocol_ value.
  reader >> this->protocol_;

  reader.swap_bytes(swap_bytes());

  // Extract the version_major_ value.
  reader >> ACE_InputCDR::to_octet(this->version_major_);

  // Extract the version_minor_ value.
  reader >> ACE_InputCDR::to_octet(this->version_minor_);

  // Extract the source_id_ value.
  reader >> this->source_;

  // Extract the sequence_ value.
  reader >> this->sequence_;

  // Extract the length_ value.
  reader >> this->length_;
}
