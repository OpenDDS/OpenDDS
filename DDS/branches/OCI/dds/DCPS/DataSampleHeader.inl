/// -*- C++ -*-
///
/// $Id$

ACE_INLINE
DataSampleHeader::DataSampleHeader ()
  : message_id_( 0)
  , last_sample_( 0)
  , byte_order_( 0)
  , reserved_1( 0)
  , reserved_2( 0)
  , reserved_3( 0)
  , reserved_4( 0)
  , reserved_5( 0)
  , reserved_6( 0)
  , message_length_( 0)
  , sequence_( 0)
  , source_timestamp_sec_( 0)
  , source_timestamp_nanosec_( 0)
  , coherency_group_( 0)
  , publication_id_( 0)
  , marshaled_size_( 0)
{
}

ACE_INLINE
DataSampleHeader::DataSampleHeader (ACE_Message_Block* buffer)
  : message_id_( 0)
  , last_sample_( 0)
  , byte_order_( 0)
  , reserved_1( 0)
  , reserved_2( 0)
  , reserved_3( 0)
  , reserved_4( 0)
  , reserved_5( 0)
  , reserved_6( 0)
  , message_length_( 0)
  , sequence_( 0)
  , source_timestamp_sec_( 0)
  , source_timestamp_nanosec_( 0)
  , coherency_group_( 0)
  , publication_id_( 0)
{
  this->init( buffer) ;
}

ACE_INLINE
DataSampleHeader::DataSampleHeader (ACE_Message_Block& buffer)
  : message_id_( 0)
  , last_sample_( 0)
  , byte_order_( 0)
  , reserved_1( 0)
  , reserved_2( 0)
  , reserved_3( 0)
  , reserved_4( 0)
  , reserved_5( 0)
  , reserved_6( 0)
  , message_length_( 0)
  , sequence_( 0)
  , source_timestamp_sec_( 0)
  , source_timestamp_nanosec_( 0)
  , coherency_group_( 0)
  , publication_id_( 0)
{
  this->init( &buffer) ;
}

ACE_INLINE
DataSampleHeader&
DataSampleHeader::operator= (ACE_Message_Block* buffer)
{
  this->init( buffer) ;
  return *this ;
}

ACE_INLINE
DataSampleHeader&
DataSampleHeader::operator= (ACE_Message_Block& buffer)
{
  this->init( &buffer) ;
  return *this ;
}

ACE_INLINE
size_t
DataSampleHeader::marshaled_size()
{
  return marshaled_size_ ;
}

ACE_INLINE
size_t
DataSampleHeader::max_marshaled_size()
{
  return sizeof( this->message_id_)
       + sizeof(char) // Flags are a single byte.
       + sizeof( this->message_length_)
       + sizeof( this->sequence_)
       + sizeof( this->source_timestamp_sec_)
       + sizeof( this->source_timestamp_nanosec_)
       + sizeof( this->coherency_group_)
       + 1 + sizeof( this->publication_id_) ;
             // Encoding of PublicationId can add an additional byte to
             // the representation.
}

