/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader::DataSampleHeader()
  : message_id_(0)
  , submessage_id_(0)
  , byte_order_(ACE_CDR_BYTE_ORDER)
  , coherent_change_(0)
  , historic_sample_(0)
  , lifespan_duration_(0)
  , group_coherent_(0)
  , content_filter_(0)
  , sequence_repair_(0)
  , more_fragments_(0)
  , cdr_encapsulation_(0)
  , key_fields_only_(0)
  , reserved_1(0)
  , reserved_2(0)
  , reserved_3(0)
  , reserved_4(0)
  , reserved_5(0)
  , reserved_6(0)
  , message_length_(0)
  , sequence_()
  , source_timestamp_sec_(0)
  , source_timestamp_nanosec_(0)
  , lifespan_duration_sec_(0)
  , lifespan_duration_nanosec_(0)
  , publication_id_(GUID_UNKNOWN)
  , publisher_id_(GUID_UNKNOWN)
  , marshaled_size_(0)
{
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader::DataSampleHeader(ACE_Message_Block& buffer)
  : message_id_(0)
  , submessage_id_(0)
  , byte_order_(ACE_CDR_BYTE_ORDER)
  , coherent_change_(0)
  , historic_sample_(0)
  , lifespan_duration_(0)
  , group_coherent_(0)
  , content_filter_(0)
  , sequence_repair_(0)
  , more_fragments_(0)
  , cdr_encapsulation_(0)
  , key_fields_only_(0)
  , reserved_1(0)
  , reserved_2(0)
  , reserved_3(0)
  , reserved_4(0)
  , reserved_5(0)
  , reserved_6(0)
  , message_length_(0)
  , sequence_()
  , source_timestamp_sec_(0)
  , source_timestamp_nanosec_(0)
  , lifespan_duration_sec_(0)
  , lifespan_duration_nanosec_(0)
  , publication_id_(GUID_UNKNOWN)
  , publisher_id_(GUID_UNKNOWN)
{
  this->init(&buffer);
}

ACE_INLINE
OpenDDS::DCPS::DataSampleHeader&
OpenDDS::DCPS::DataSampleHeader::operator=(ACE_Message_Block& buffer)
{
  this->init(&buffer);
  return *this;
}

ACE_INLINE
size_t
OpenDDS::DCPS::DataSampleHeader::marshaled_size() const
{
  return marshaled_size_;
}

ACE_INLINE
size_t
OpenDDS::DCPS::DataSampleHeader::max_marshaled_size()
{
  return 1 + // message_id_;
         1 + // submessage_id_;
         2 + // flags
         4 + // message_length_;
         8 + // sequence_;
         4 + // source_timestamp_sec_;
         4 + // source_timestamp_nanosec_;
         4 + // lifespan_duration_sec_;
         4 + // lifespan_duration_nanosec_;
        16 + // publication_id_;
        16 ; // publisher_id_;
  // content_filter_entries_ is not marsahled into the same Data Block
  // so it is not part of the max_marshaled_size() which is used to allocate
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
  char* base = buffer->base();

  // verify sufficient length exists:
  if (static_cast<size_t>(buffer->end() - base) < FLAGS_OFFSET + 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::clear_flag: ")
               ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")));
    return;
  }

  base[FLAGS_OFFSET] &= ~mask_flag(flag);
}

ACE_INLINE
void
OpenDDS::DCPS::DataSampleHeader::set_flag(DataSampleHeaderFlag flag,
                                          ACE_Message_Block* buffer)
{
  char* base = buffer->base();

  // verify sufficient length exists:
  if (static_cast<size_t>(buffer->end() - base) < FLAGS_OFFSET + 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::set_flag: ")
               ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")));
    return;
  }

  base[FLAGS_OFFSET] |= mask_flag(flag);
}

ACE_INLINE
bool
OpenDDS::DCPS::DataSampleHeader::test_flag(DataSampleHeaderFlag flag,
                                           const ACE_Message_Block* buffer)
{
  char* base = buffer->base();

  // verify sufficient length exists:
  if (static_cast<size_t>(buffer->end() - base) < FLAGS_OFFSET + 1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: DataSampleHeader::set_flag: ")
                      ACE_TEXT("ACE_Message_Block too short (missing flags octet).\n")), false);
  }

  // Test flag bit.
  return base[FLAGS_OFFSET] & mask_flag(flag);
}
