// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleHeader.h"
#include "Serializer.h"


#if ! defined (__ACE_INLINE__)
#include "DataSampleHeader.inl"
#endif /* __ACE_INLINE__ */

void
OpenDDS::DCPS::DataSampleHeader::init (ACE_Message_Block* buffer)
{
  this->marshaled_size_ = 0;

  TAO::DCPS::Serializer reader( buffer )  ;
  // TODO: Now it's ok to serialize the message_id before flag byte
  // since the message_id_ is defined as char. If the message_id_ 
  // is changed to be defined as a type with multiple bytes then 
  // we need define it after the flag byte or serialize flag byte before
  // serializing the message_id_. I think the former approach is simpler
  // than the latter approach.
  reader >> this->message_id_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->message_id_) ;

  // Extract the flag values.
  ACE_CDR::Octet byte ;
  reader >> ACE_InputCDR::to_octet(byte) ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( byte) ;

  this->byte_order_  = ((byte & 0x01) != 0) ;
  this->last_sample_ = ((byte & 0x02) != 0) ;
  this->reserved_1   = ((byte & 0x04) != 0) ;
  this->reserved_2   = ((byte & 0x08) != 0) ;
  this->reserved_3   = ((byte & 0x10) != 0) ;
  this->reserved_4   = ((byte & 0x20) != 0) ;
  this->reserved_5   = ((byte & 0x40) != 0) ;
  this->reserved_6   = ((byte & 0x80) != 0) ;

  // Set swap_bytes flag to the Serializer if data sample from
  // the publisher is in different byte order.
  reader.swap_bytes (this->byte_order_ != TAO_ENCAP_BYTE_ORDER);

  reader >> this->message_length_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->message_length_) ;

  reader >> this->sequence_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->sequence_) ;

  reader >> this->source_timestamp_sec_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->source_timestamp_sec_) ;

  reader >> this->source_timestamp_nanosec_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->source_timestamp_nanosec_) ;

  reader >> this->coherency_group_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->coherency_group_) ;

  // Publication ID is variable length, so read it byte by byte.
  this->publication_id_ = 0;

  do {
    reader >> ACE_InputCDR::to_octet(byte) ;
    this->publication_id_ = (this->publication_id_<<7) | (byte & 0x7f) ;
    if( reader.good_bit() != true) return ;
    this->marshaled_size_ += sizeof( byte) ;
  } while( byte & 0x80) ;

}

ACE_CDR::Boolean
operator<< (ACE_Message_Block*& buffer, OpenDDS::DCPS::DataSampleHeader& value)
{
  TAO::DCPS::Serializer writer( buffer, value.byte_order_ != TAO_ENCAP_BYTE_ORDER) ;

  writer << value.message_id_ ;

  // Write the flags as a single byte.
  ACE_CDR::Octet flags = (value.byte_order_  << 0)
                       | (value.last_sample_ << 1)
                       | (value.reserved_1   << 2)
                       | (value.reserved_2   << 3)
                       | (value.reserved_3   << 4)
                       | (value.reserved_4   << 5)
                       | (value.reserved_5   << 6)
                       | (value.reserved_6   << 7)
                       ;
  writer << ACE_OutputCDR::from_octet (flags);
  writer << value.message_length_ ;
  writer << value.sequence_ ;
  writer << value.source_timestamp_sec_ ;
  writer << value.source_timestamp_nanosec_ ;
  writer << value.coherency_group_ ;

  //
  // We need to copy the publication ID here since we need to modify the
  // value as we encode it.
  //
  OpenDDS::DCPS::PublicationId id = value.publication_id_ ;

  // Encode.
  const size_t amount = (((sizeof(OpenDDS::DCPS::PublicationId)*8)%7)? 2: 1)
                      +  ((sizeof(OpenDDS::DCPS::PublicationId)*8)/7) ;
  char  encoder[ amount] ;
  char* current = encoder + amount - 1 ;
  char* start   = current ;
  ACE_CDR::Octet continue_bit = 0x00 ;
  while (current >= encoder)
    {
      // The current 7 bit chunk we are encoding.
      unsigned char chunk = ( 0x7f & id ) ;

      // Move the beginning of the encoded value if we encoded anything.
      if( chunk != 0) start = current ;

      // Assign the 8 bit encoded value.
      *current-- = continue_bit | chunk ;

      // Move to the next 7 bits to encode.
      id >>= 7 ;

      // Continuation encoding is always bit7 set for non-terminal bytes.
      continue_bit = 0x80 ;
    }

  // Insert.
  writer.write_char_array( start, amount-(start-encoder)) ;

  return writer.good_bit() ;
}

