// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleHeader.h"
#include "Serializer.h"
#include "RepoIdConverter.h"

#include <iostream>

#if ! defined (__ACE_INLINE__)
#include "DataSampleHeader.inl"
#endif /* __ACE_INLINE__ */


bool OpenDDS::DCPS::DataSampleHeader::partial(ACE_Message_Block& mb)
{
  static const unsigned int LIFESPAN_MASK = 0x08, LIFESPAN_LENGTH = 8;
  const size_t full_header = DataSampleHeader().max_marshaled_size();

  size_t len = mb.total_length();
  if (len < 2) return true;

  char buffer[2];

  switch (mb.length())
    {
    case 0:
      if (!mb.cont()) return true;
      memcpy(buffer, mb.cont()->rd_ptr(), 2);
      break;
    case 1:
      if (!mb.cont()) return true;
      buffer[0] = *mb.rd_ptr();
      buffer[1] = *mb.cont()->rd_ptr();
      break;
    default:
      memcpy(buffer, mb.rd_ptr(), 2);
    }

  if (buffer[1] & LIFESPAN_MASK) return len < full_header;
  else return len < full_header - LIFESPAN_LENGTH;
}

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

  this->byte_order_         = ((byte & 0x01) != 0) ;
  this->last_sample_        = ((byte & 0x02) != 0) ;
  this->historic_sample_    = ((byte & 0x04) != 0) ;
  this->lifespan_duration_  = ((byte & 0x08) != 0) ;
  this->reserved_1          = ((byte & 0x10) != 0) ;
  this->reserved_2          = ((byte & 0x20) != 0) ;
  this->reserved_3          = ((byte & 0x40) != 0) ;
  this->reserved_4          = ((byte & 0x80) != 0) ;

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

  if (this->lifespan_duration_) {
    reader >> this->lifespan_duration_sec_;
    if (!reader.good_bit()) return;
    this->marshaled_size_ += sizeof (this->lifespan_duration_sec_);
    
    reader >> this->lifespan_duration_nanosec_;
    if (!reader.good_bit()) return;
    this->marshaled_size_ += sizeof (this->lifespan_duration_nanosec_);
  }

  reader >> this->coherency_group_ ;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += sizeof( this->coherency_group_) ;

  reader >> this->publication_id_;
  if( reader.good_bit() != true) return ;
  this->marshaled_size_ += _dcps_find_size( this->publication_id_) ;

}

/// The update_flag method is a hack which updates the sample
/// header flags after a sample has been serialized without
/// deserializing the message. This method will break if the
/// current Serializer behavior changes.

void
OpenDDS::DCPS::DataSampleHeader::update_flag (ACE_Message_Block* buffer,
  DataSampleHeaderFlag flag)
{
  char *base = buffer->base();

  // The flags octet will always be the second byte;
  // verify sufficient length exists:
  if (buffer->end() - base < 2) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::update_flag: ")
      ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")));
  }

  // Twiddle flag bit.
  *(base + 1) |= (1 << flag);
}

ACE_CDR::Boolean
operator<< (ACE_Message_Block*& buffer, OpenDDS::DCPS::DataSampleHeader& value)
{
  TAO::DCPS::Serializer writer( buffer, value.byte_order_ != TAO_ENCAP_BYTE_ORDER) ;

  writer << value.message_id_ ;

  // Write the flags as a single byte.
  ACE_CDR::Octet flags = (value.byte_order_         << 0)
                       | (value.last_sample_        << 1)
                       | (value.historic_sample_    << 2)
                       | (value.lifespan_duration_  << 3)
                       | (value.reserved_1          << 4)
                       | (value.reserved_2          << 5)
                       | (value.reserved_3          << 6)
                       | (value.reserved_4          << 7)
                       ;
  writer << ACE_OutputCDR::from_octet (flags);
  writer << value.message_length_ ;
  writer << value.sequence_ ;
  writer << value.source_timestamp_sec_ ;
  writer << value.source_timestamp_nanosec_ ;

  if (value.lifespan_duration_) {
    writer << value.lifespan_duration_sec_;
    writer << value.lifespan_duration_nanosec_;
  }

  writer << value.coherency_group_ ;

  writer << value.publication_id_;

  return writer.good_bit() ;
}

/// Message Id enumeration insertion onto an ostream.
std::ostream& operator<<( std::ostream& str, const OpenDDS::DCPS::MessageId value)
{
  switch( value) {
    case OpenDDS::DCPS::SAMPLE_DATA:           return str << "SAMPLE_DATA";
    case OpenDDS::DCPS::DATAWRITER_LIVELINESS: return str << "DATAWRITER_LIVELINESS";
    case OpenDDS::DCPS::INSTANCE_REGISTRATION: return str << "INSTANCE_REGISTRATION";
    case OpenDDS::DCPS::UNREGISTER_INSTANCE:   return str << "UNREGISTER_INSTANCE";
    case OpenDDS::DCPS::DISPOSE_INSTANCE:      return str << "DISPOSE_INSTANCE";
    case OpenDDS::DCPS::GRACEFUL_DISCONNECT:   return str << "GRACEFUL_DISCONNECT";
    case OpenDDS::DCPS::FULLY_ASSOCIATED:      return str << "FULLY_ASSOCIATED";
    case OpenDDS::DCPS::REQUEST_ACK:           return str << "REQUEST_ACK";
    case OpenDDS::DCPS::SAMPLE_ACK:            return str << "SAMPLE_ACK";
    default:                                   return str << "UNSPECIFIED(" << int(value) << ")";
  }
}

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<( std::ostream& str, const OpenDDS::DCPS::DataSampleHeader& value)
{
  str << "[";

  str << OpenDDS::DCPS::MessageId( value.message_id_) << ", ";
  if( value.last_sample_ == 1) {
    str << "last sample, ";
  }
  if( value.byte_order_ == 1) {
    str << "network order, ";
  } else {
    str << "little endian, ";
  }
  str << std::dec << value.message_length_ << ", ";
  str << "0x" << std::hex << value.sequence_ << ", ";
  str << "(" << std::dec << value.source_timestamp_sec_ << "/";
  str << std::dec << value.source_timestamp_nanosec_ << "), ";
  str << "(" << std::dec << value.lifespan_duration_sec_ << "/";
  str << std::dec << value.lifespan_duration_nanosec_ << "), ";
  str << std::dec << value.coherency_group_ << ", ";
  str << OpenDDS::DCPS::RepoIdConverter(value.publication_id_);

  str << "]";
  return str;
}

