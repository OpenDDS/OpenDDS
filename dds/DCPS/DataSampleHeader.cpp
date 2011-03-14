/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleHeader.h"
#include "Serializer.h"
#include "RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include <iomanip>
#include <iostream>

#if !defined (__ACE_INLINE__)
#include "DataSampleHeader.inl"
#endif /* __ACE_INLINE__ */

namespace {
  struct AMB_Releaser {
    explicit AMB_Releaser(ACE_Message_Block* p) : p_(p) {}
    ~AMB_Releaser() { p_->release(); }
    ACE_Message_Block* p_;
  };

  bool mb_copy(char& dest, const ACE_Message_Block& mb, size_t offset, bool)
  {
    dest = mb.rd_ptr()[offset];
    return true;
  }

  template <typename T>
  bool mb_copy(T& dest, const ACE_Message_Block& mb, size_t offset, bool swap)
  {
    ACE_Message_Block* temp = mb.duplicate();
    if (!temp) { // couldn't allocate
      return false;
    }
    AMB_Releaser r(temp);
    temp->rd_ptr(offset);
    if (temp->total_length() < sizeof(T)) {
      return false;
    }
    OpenDDS::DCPS::Serializer ser(temp, swap);
    ser.buffer_read(reinterpret_cast<char*>(&dest), sizeof(T), swap);
    return true;
  }

  // Skip "offset" bytes from the mb and copy the subsequent data
  // (sizeof(T) bytes) into dest.  Return false if there is not enough data
  // in the mb to complete the operation.  Continuation pointers are followed.
  template <typename T>
  bool mb_peek(T& dest, const ACE_Message_Block& mb, size_t offset, bool swap)
  {
    for (const ACE_Message_Block* iter = &mb; iter; iter = iter->cont()) {
      const size_t len = iter->length();
      if (len > offset) {
        return mb_copy(dest, *iter, offset, swap);
      }
      offset -= len;
    }
    return false;
  }

}

namespace OpenDDS {
namespace DCPS {

bool DataSampleHeader::partial(const ACE_Message_Block& mb)
{
  static const unsigned int LIFESPAN_MASK = mask_flag(LIFESPAN_DURATION_FLAG),
    LIFESPAN_LENGTH = 8,
    COHERENT_MASK = mask_flag(GROUP_COHERENT_FLAG),
    COHERENT_LENGTH = 16,
    CONTENT_FILT_MASK = mask_flag(CONTENT_FILTER_FLAG),
    BYTE_ORDER_MASK = mask_flag(BYTE_ORDER_FLAG);

  const size_t len = mb.total_length();

  if (len <= FLAGS_OFFSET) return true;

  char flags;
  if (!mb_peek(flags, mb, FLAGS_OFFSET, false /*swap ignored for char*/)) {
    return true;
  }

  size_t expected = max_marshaled_size();
  if (!(flags & LIFESPAN_MASK)) expected -= LIFESPAN_LENGTH;
  if (!(flags & COHERENT_MASK)) expected -= COHERENT_LENGTH;

  if (flags & CONTENT_FILT_MASK) {
    CORBA::ULong seqLen;
    const bool swap = (flags & BYTE_ORDER_MASK) != TAO_ENCAP_BYTE_ORDER;
    if (!mb_peek(seqLen, mb, expected, swap)) {
      return true;
    }
    expected += sizeof(seqLen) + gen_find_size(GUID_t()) * seqLen;
  }

  return len < expected;
}

void
DataSampleHeader::init(ACE_Message_Block* buffer)
{
  this->marshaled_size_ = 0;

  Serializer reader(buffer);
  // TODO: Now it's ok to serialize the message_id before flag byte
  // since the message_id_ is defined as char. If the message_id_
  // is changed to be defined as a type with multiple bytes then
  // we need define it after the flag byte or serialize flag byte before
  // serializing the message_id_. I think the former approach is simpler
  // than the latter approach.
  reader >> this->message_id_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->message_id_);

  reader >> this->submessage_id_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->submessage_id_);

  // Extract the flag values.
  ACE_CDR::Octet byte;
  reader >> ACE_InputCDR::to_octet(byte);

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(byte);

  this->byte_order_         = byte & mask_flag(BYTE_ORDER_FLAG);
  this->coherent_change_    = byte & mask_flag(COHERENT_CHANGE_FLAG);
  this->historic_sample_    = byte & mask_flag(HISTORIC_SAMPLE_FLAG);
  this->lifespan_duration_  = byte & mask_flag(LIFESPAN_DURATION_FLAG);
  this->group_coherent_     = byte & mask_flag(GROUP_COHERENT_FLAG);
  this->content_filter_     = byte & mask_flag(CONTENT_FILTER_FLAG);
  this->sequence_repair_    = byte & mask_flag(SEQUENCE_REPAIR_FLAG);
  this->reserved_4          = byte & mask_flag(RESERVED_4_FLAG);

  // Set swap_bytes flag to the Serializer if data sample from
  // the publisher is in different byte order.
  reader.swap_bytes(this->byte_order_ != TAO_ENCAP_BYTE_ORDER);

  reader >> this->message_length_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->message_length_);

  reader >> this->sequence_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->sequence_);

  reader >> this->source_timestamp_sec_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->source_timestamp_sec_);

  reader >> this->source_timestamp_nanosec_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += sizeof(this->source_timestamp_nanosec_);

  if (this->lifespan_duration_) {
    reader >> this->lifespan_duration_sec_;

    if (!reader.good_bit()) return;
    this->marshaled_size_ += sizeof(this->lifespan_duration_sec_);

    reader >> this->lifespan_duration_nanosec_;

    if (!reader.good_bit()) return;
    this->marshaled_size_ += sizeof(this->lifespan_duration_nanosec_);
  }

  reader >> this->publication_id_;

  if (!reader.good_bit()) return;
  this->marshaled_size_ += gen_find_size(this->publication_id_);

  if (this->group_coherent_) {
    reader >> this->publisher_id_;
    if (!reader.good_bit()) return;
    this->marshaled_size_ += gen_find_size(this->publisher_id_);
  }

  if (this->content_filter_) {
    reader >> this->content_filter_entries_;
    if (!reader.good_bit()) return;
    this->marshaled_size_ += gen_find_size(this->content_filter_entries_);
  }
}

ACE_CDR::Boolean
operator<<(ACE_Message_Block*& buffer, DataSampleHeader& value)
{
  Serializer writer(buffer, value.byte_order_ != TAO_ENCAP_BYTE_ORDER);

  writer << value.message_id_;
  writer << value.submessage_id_;

  // Write the flags as a single byte.
  ACE_CDR::Octet flags = (value.byte_order_           << BYTE_ORDER_FLAG)
                         | (value.coherent_change_    << COHERENT_CHANGE_FLAG)
                         | (value.historic_sample_    << HISTORIC_SAMPLE_FLAG)
                         | (value.lifespan_duration_  << LIFESPAN_DURATION_FLAG)
                         | (value.group_coherent_     << GROUP_COHERENT_FLAG)
                         | (value.content_filter_     << CONTENT_FILTER_FLAG)
                         | (value.sequence_repair_    << SEQUENCE_REPAIR_FLAG)
                         | (value.reserved_4          << RESERVED_4_FLAG)
                         ;
  writer << ACE_OutputCDR::from_octet(flags);
  writer << value.message_length_;
  writer << value.sequence_;
  writer << value.source_timestamp_sec_;
  writer << value.source_timestamp_nanosec_;

  if (value.lifespan_duration_) {
    writer << value.lifespan_duration_sec_;
    writer << value.lifespan_duration_nanosec_;
  }

  writer << value.publication_id_;

  if (value.group_coherent_) {
    writer << value.publisher_id_;
  }

  // content_filter_entries_ is deliberately not marshaled here.
  // It's variable sized, so it won't fit into our pre-allocated data block.
  // It may be customized per-datalink so it will be handled later with a
  // a chained (continuation) ACE_Message_Block.

  return writer.good_bit();
}

void
DataSampleHeader::add_cfentries(const GUIDSeq* guids, ACE_Message_Block* mb)
{
  enum { DATA, DB, MB, N_ALLOC };
  ACE_Allocator* allocators[N_ALLOC];
  mb->access_allocators(allocators[DATA], allocators[DB], allocators[MB]);
  ACE_Message_Block* optHdr;
  ACE_NEW_MALLOC(optHdr,
    static_cast<ACE_Message_Block*>(
      allocators[MB]->malloc(sizeof(ACE_Message_Block))),
    ACE_Message_Block(guids ? gen_find_size(*guids) : sizeof(CORBA::ULong),
                      ACE_Message_Block::MB_DATA,
                      0, // cont
                      0, // data
                      0, // data allocator: leave as default
                      0, // locking_strategy
                      ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                      ACE_Time_Value::zero,
                      ACE_Time_Value::max_time,
                      allocators[DB],
                      allocators[MB]));

  const bool swap = (TAO_ENCAP_BYTE_ORDER != test_flag(BYTE_ORDER_FLAG, mb));
  Serializer ser(optHdr, swap);
  if (guids) {
    ser << *guids;
  } else {
    ser << CORBA::ULong(0);
  }

  // New chain: mb (DataSampleHeader), optHdr (GUIDSeq), data (Foo or control)
  optHdr->cont(mb->cont());
  mb->cont(optHdr);
}

/// Message Id enumeration insertion onto an ostream.
std::ostream& operator<<(std::ostream& str, const MessageId value)
{
  switch (value) {
  case SAMPLE_DATA:
    return str << "SAMPLE_DATA";
  case DATAWRITER_LIVELINESS:
    return str << "DATAWRITER_LIVELINESS";
  case INSTANCE_REGISTRATION:
    return str << "INSTANCE_REGISTRATION";
  case UNREGISTER_INSTANCE:
    return str << "UNREGISTER_INSTANCE";
  case DISPOSE_INSTANCE:
    return str << "DISPOSE_INSTANCE";
  case GRACEFUL_DISCONNECT:
    return str << "GRACEFUL_DISCONNECT";
  case FULLY_ASSOCIATED:
    return str << "FULLY_ASSOCIATED";
  case REQUEST_ACK:
    return str << "REQUEST_ACK";
  case SAMPLE_ACK:
    return str << "SAMPLE_ACK";
  case END_COHERENT_CHANGES:
    return str << "END_COHERENT_CHANGES";
  case TRANSPORT_CONTROL:
    return str << "TRANSPORT_CONTROL";
  default:
    return str << "Unknown";
  }
}

/// Sub-Message Id enumeration insertion onto an ostream.
std::ostream& operator<<(std::ostream& os, const SubMessageId rhs)
{
  switch (rhs) {
  case SUBMESSAGE_NONE:
    return os << "SUBMESSAGE_NONE";
  case MULTICAST_SYN:
    return os << "MULTICAST_SYN";
  case MULTICAST_SYNACK:
    return os << "MULTICAST_SYNACK";
  case MULTICAST_NAK:
    return os << "MULTICAST_NAK";
  case MULTICAST_NAKACK:
    return os << "MULTICAST_NAKACK";
  default:
    return os << "Unknown";
  }
}

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const DataSampleHeader& value)
{
  if (value.submessage_id_ != SUBMESSAGE_NONE) {
    str << SubMessageId(value.submessage_id_)
      << " (0x" << std::hex << std::setw(2) << std::setfill('0')
      << unsigned(value.submessage_id_) << "), ";

  } else {
    str << MessageId(value.message_id_)
        << " (0x" << std::hex << std::setw(2) << std::setfill('0')
        << unsigned(value.message_id_) << "), ";
  }

  str << "Length: " << std::dec << value.message_length_ << ", ";

  str << "Byte order: " << (value.byte_order_ == 1 ? "Little" : "Big")
      << " Endian (0x" << std::hex << unsigned(value.byte_order_) << ")";

  if (value.message_id_ != TRANSPORT_CONTROL) {
    str << ", ";

    if (value.coherent_change_ == 1) str << "Coherent, ";
    if (value.historic_sample_ == 1) str << "Historic, ";
    if (value.lifespan_duration_ == 1) str << "Lifespan, ";
    if (value.group_coherent_ == 1) str << "Group-Coherent, ";
    if (value.content_filter_ == 1) str << "Content-Filtered, ";
    if (value.sequence_repair_ == 1) str << "Sequence Repair, ";

    str << "Sequence: 0x" << std::hex << std::setw(4) << std::setfill('0')
        << value.sequence_ << ", ";

    str << "Timestamp: " << std::dec << value.source_timestamp_sec_ << "."
        << std::dec << value.source_timestamp_nanosec_ << ", ";

    if (value.lifespan_duration_) {
      str << "Lifespan: " << std::dec << value.lifespan_duration_sec_ << "."
          << std::dec << value.lifespan_duration_nanosec_ << ", ";
    }

    str << "Publication: " << RepoIdConverter(value.publication_id_);
    if (value.group_coherent_) {
      str << ", Publisher: " << RepoIdConverter(value.publisher_id_);
    }

    if (value.content_filter_) {
      const CORBA::ULong len = value.content_filter_entries_.length();
      str << ", Content-Filter Entries (" << len << "): [";
      for (CORBA::ULong i(0); i < len; ++i) {
        str << RepoIdConverter(value.content_filter_entries_[i]) << ' ';
      }
      str << ']';
    }
  }

  return str;
}

} // namespace DCPS
} // namespace OpenDDS
