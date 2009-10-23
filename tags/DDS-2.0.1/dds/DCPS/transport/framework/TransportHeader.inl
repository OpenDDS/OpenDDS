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
  : length_(0)
{
  DBG_ENTRY_LVL("TransportHeader","TransportHeader",6);

  this->byte_order_ = TAO_ENCAP_BYTE_ORDER;
  this->packet_id_[0] = supported_id_[0];
  this->packet_id_[1] = supported_id_[1];
  this->packet_id_[2] = supported_id_[2];
  this->packet_id_[3] = supported_id_[3];
  this->packet_id_[4] = supported_id_[4];
  this->packet_id_[5] = supported_id_[5];
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
  return sizeof(this->byte_order_) + sizeof(this->packet_id_) + sizeof(this->length_) ;
}

ACE_INLINE
bool
OpenDDS::DCPS::TransportHeader::valid() const
{
  DBG_ENTRY_LVL("TransportHeader","valid",6);

  const char* valid_bytes = reinterpret_cast<const char*>(supported_id_);
  const char* check_bytes = reinterpret_cast<const char*>(this->packet_id_);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "supported_id_ bytes == [%X] [%X] [%X] [%X] [%X] [%X]\n",
        valid_bytes[0],valid_bytes[1],valid_bytes[2],
        valid_bytes[3],valid_bytes[4],valid_bytes[5]));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
        "this->packet_id_ bytes == [%X] [%X] [%X] [%X] [%X] [%X]\n",
        check_bytes[0],check_bytes[1],check_bytes[2],
        check_bytes[3],check_bytes[4],check_bytes[5]));

  // Only return true if the check_bytes match the valid_bytes.
  return 0 == ACE_OS::strncmp(valid_bytes, check_bytes, sizeof(supported_id_));
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
  reader.read_octet_array(this->packet_id_, sizeof(this->packet_id_));

  if (reader.good_bit() != true) {
    return;
  }

  // Extract the length_ value.
  reader >> this->length_;
}
