// -*- C++ -*-
//
// $Id$
#include  "dds/DCPS/Serializer.h"
#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::TransportHeader::TransportHeader()
  : length_(0)
{
  DBG_SUB_ENTRY("TransportHeader","TransportHeader",1);

  this->packet_id_[0] = supported_id_[0];
  this->packet_id_[1] = supported_id_[1];
  this->packet_id_[2] = supported_id_[2];
  this->packet_id_[3] = supported_id_[3];
  this->packet_id_[4] = supported_id_[4];
  this->packet_id_[5] = supported_id_[5];
}


ACE_INLINE
TAO::DCPS::TransportHeader::TransportHeader(ACE_Message_Block* buffer)
{
  DBG_SUB_ENTRY("TransportHeader","TransportHeader",2);
  this->init(buffer);
}


ACE_INLINE
TAO::DCPS::TransportHeader::TransportHeader(ACE_Message_Block& buffer)
{
  DBG_SUB_ENTRY("TransportHeader","TransportHeader",3);
  this->init(&buffer);
}


ACE_INLINE
TAO::DCPS::TransportHeader&
TAO::DCPS::TransportHeader::operator=(ACE_Message_Block* buffer)
{
  DBG_SUB_ENTRY("TransportHeader","operator=",1);
  this->init(buffer);
  return *this;
}


ACE_INLINE
TAO::DCPS::TransportHeader&
TAO::DCPS::TransportHeader::operator=(ACE_Message_Block& buffer)
{
  DBG_SUB_ENTRY("TransportHeader","operator=",2);
  this->init(&buffer);
  return *this;
}


ACE_INLINE
size_t
TAO::DCPS::TransportHeader::max_marshaled_size()
{
  DBG_ENTRY("TransportHeader","max_marshaled_size");
  // Representation takes no extra space for encoding.
  return sizeof(this->packet_id_) + sizeof(this->length_) ;
}


ACE_INLINE
bool
TAO::DCPS::TransportHeader::valid() const
{
  DBG_ENTRY("TransportHeader","valid");

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
TAO::DCPS::TransportHeader::init(ACE_Message_Block* buffer)
{
  DBG_ENTRY("TransportHeader","init");

  TAO::DCPS::Serializer reader(buffer);

  // Extract the packet_id_ octet values.
  reader.read_octet_array(this->packet_id_, sizeof(this->packet_id_));

  if (reader.good_bit() != true)
    {
      return;
    }

  // Extract the length_ value.
  reader >> this->length_;
}
