/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

//TODO: we should really use the ACE_CDR_BYTE_ORDER instead of
//      TAO_ENCAP_BYTE_ORDER, but since the ACE_CDR_BYTE_ORDER
//      is currently defined under the ACE_HAS_WCHAR guard which
//      is different from DOC's version (I'm not sure why) and
//      the TAO_ENCAP_BYTE_ORDER is currently consistent with
//      the ACE_CDR_BYTE_ORDER, it's ok to use
//      TAO_ENCAP_BYTE_ORDER for now.
ACE_INLINE
OpenDDS::DCPS::DataSampleHeader::DataSampleHeader()
  : message_id_(0)
  , submessage_id_(0)
  , byte_order_(TAO_ENCAP_BYTE_ORDER)
  , coherent_change_(0)
  , historic_sample_(0)
  , lifespan_duration_(0)
  , reserved_1(0)
  , reserved_2(0)
  , reserved_3(0)
  , reserved_4(0)
  , message_length_(0)
  , sequence_(0)
  , source_timestamp_sec_(0)
  , source_timestamp_nanosec_(0)
  , publication_id_(GUID_UNKNOWN)
  , marshaled_size_(0)
{
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader::DataSampleHeader(ACE_Message_Block* buffer)
  : message_id_(0)
  , submessage_id_(0)
  , byte_order_(TAO_ENCAP_BYTE_ORDER)
  , coherent_change_(0)
  , historic_sample_(0)
  , lifespan_duration_(0)
  , reserved_1(0)
  , reserved_2(0)
  , reserved_3(0)
  , reserved_4(0)
  , message_length_(0)
  , sequence_(0)
  , source_timestamp_sec_(0)
  , source_timestamp_nanosec_(0)
  , publication_id_(GUID_UNKNOWN)
{
  this->init(buffer) ;
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader::DataSampleHeader(ACE_Message_Block& buffer)
  : message_id_(0)
  , submessage_id_(0)
  , byte_order_(TAO_ENCAP_BYTE_ORDER)
  , coherent_change_(0)
  , historic_sample_(0)
  , lifespan_duration_(0)
  , reserved_1(0)
  , reserved_2(0)
  , reserved_3(0)
  , reserved_4(0)
  , message_length_(0)
  , sequence_(0)
  , source_timestamp_sec_(0)
  , source_timestamp_nanosec_(0)
  , publication_id_(GUID_UNKNOWN)
{
  this->init(&buffer) ;
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader&
OpenDDS::DCPS::DataSampleHeader::operator= (ACE_Message_Block* buffer)
{
  this->init(buffer) ;
  return *this ;
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader&
OpenDDS::DCPS::DataSampleHeader::operator= (ACE_Message_Block& buffer)
{
  this->init(&buffer) ;
  return *this ;
}

ACE_INLINE
size_t
OpenDDS::DCPS::DataSampleHeader::marshaled_size()
{
  return marshaled_size_ ;
}

ACE_INLINE
size_t
OpenDDS::DCPS::DataSampleHeader::max_marshaled_size()
{
  return sizeof(this->message_id_)
         + sizeof(this->submessage_id_)
         + sizeof(char) // Flags are a single byte.
         + sizeof(this->message_length_)
         + sizeof(this->sequence_)
         + sizeof(this->source_timestamp_sec_)
         + sizeof(this->source_timestamp_nanosec_)
         + sizeof(this->lifespan_duration_sec_)
         + sizeof(this->lifespan_duration_nanosec_)
         + sizeof(this->publication_id_) ;
}

ACE_INLINE
long
OpenDDS::DCPS::DataSampleHeader::mask_flag(DataSampleHeaderFlag flag)
{
  return 1 << flag;
}

/// The clear_flag and set_flag methods are a hack to update the
/// header flags after a sample has been serialized without
/// deserializing the entire message. This method will break if
/// the current Serializer behavior changes.

ACE_INLINE
void
OpenDDS::DCPS::DataSampleHeader::clear_flag(DataSampleHeaderFlag flag,
                                            ACE_Message_Block* buffer)
{
  char *base = buffer->base();

  // The flags octet will always be the second byte;
  // verify sufficient length exists:
  if (buffer->end() - base < 2) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::clear_flag: ")
               ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")));
    return;
  }

  // Twiddle flag bit.
  *(base + 1) ^= mask_flag(flag);
}

ACE_INLINE
void
OpenDDS::DCPS::DataSampleHeader::set_flag(DataSampleHeaderFlag flag,
                                          ACE_Message_Block* buffer)
{
  char *base = buffer->base();

  // The flags octet will always be the second byte;
  // verify sufficient length exists:
  if (buffer->end() - base < 2) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::set_flag: ")
               ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")));
    return;
  }

  // Twiddle flag bit.
  *(base + 1) |= mask_flag(flag);
}

ACE_INLINE
bool
OpenDDS::DCPS::DataSampleHeader::test_flag(DataSampleHeaderFlag flag,
                                           ACE_Message_Block* buffer)
{
  char *base = buffer->base();

  // The flags octet will always be the second byte;
  // verify sufficient length exists:
  if (buffer->end() - base < 2) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::set_flag: ")
                      ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")), false);
  }

  // Test flag bit.
  return *(base + 1) & mask_flag(flag);
}
