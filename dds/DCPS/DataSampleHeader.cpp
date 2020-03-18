/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleHeader.h"
#include "Serializer.h"
#include "GuidConverter.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include <cstdio>

#if !defined (__ACE_INLINE__)
#include "DataSampleHeader.inl"
#endif /* __ACE_INLINE__ */

namespace {

  bool mb_copy(char& dest, const ACE_Message_Block& mb, size_t offset, bool)
  {
    dest = mb.rd_ptr()[offset];
    return true;
  }

  template <typename T>
  bool mb_copy(T& dest, const ACE_Message_Block& mb, size_t offset, bool swap)
  {
    if (mb.length() >= sizeof(T)) {
      // Avoid creating ACE_Message_Block from the heap if we just need one.
      ACE_Message_Block temp(mb.data_block (), ACE_Message_Block::DONT_DELETE);
      temp.rd_ptr(mb.rd_ptr()+offset);
      temp.wr_ptr(mb.wr_ptr());
      OpenDDS::DCPS::Serializer ser(&temp, swap);
      ser.buffer_read(reinterpret_cast<char*>(&dest), sizeof(T), swap);
      return true;
    }

    OpenDDS::DCPS::Message_Block_Ptr temp(mb.duplicate());
    if (!temp) { // couldn't allocate
      return false;
    }
    temp->rd_ptr(offset);
    if (temp->total_length() < sizeof(T)) {
      return false;
    }
    OpenDDS::DCPS::Serializer ser(temp.get(), swap);
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// Allocate a new message block using the allocators from an existing
// message block, "mb".  Use of mb's data_allocator_ is optional.
ACE_Message_Block*
DataSampleHeader::alloc_msgblock(const ACE_Message_Block& mb,
                                 size_t size, bool use_data_alloc)
{
  enum { DATA, DB, MB, N_ALLOC };
  ACE_Allocator* allocators[N_ALLOC];
  // It's an ACE bug that access_allocators isn't const
  ACE_Message_Block& mut_mb = const_cast<ACE_Message_Block&>(mb);
  mut_mb.access_allocators(allocators[DATA], allocators[DB], allocators[MB]);
  if (allocators[MB]) {
    ACE_Message_Block* result;
    ACE_NEW_MALLOC_RETURN(result,
      static_cast<ACE_Message_Block*>(
        allocators[MB]->malloc(sizeof(ACE_Message_Block))),
      ACE_Message_Block(size,
                        ACE_Message_Block::MB_DATA,
                        0, // cont
                        0, // data
                        use_data_alloc ? allocators[DATA] : 0,
                        mut_mb.locking_strategy(), // locking_strategy
                        ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                        ACE_Time_Value::zero,
                        ACE_Time_Value::max_time,
                        allocators[DB],
                        allocators[MB]),
      0);
    return result;
  } else {
    return new ACE_Message_Block(size,
                                  ACE_Message_Block::MB_DATA,
                                  0, // cont
                                  0, // data
                                  use_data_alloc ? allocators[DATA] : 0,
                                  mut_mb.locking_strategy(), // locking_strategy
                                  ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                  ACE_Time_Value::zero,
                                  ACE_Time_Value::max_time,
                                  allocators[DB]);
  }
}

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

  unsigned char msg_id;
  if (!mb_peek(msg_id, mb, MESSAGE_ID_OFFSET,
               false /*swap ignored for char*/)
      || int(msg_id) >= MESSAGE_ID_MAX) {
    // This check, and the similar one below for submessage id, are actually
    // indicating an invalid header (and not a partial header) but we can
    // treat it the same as partial for the sake of the TransportRecvStrategy.
    return true;
  }

  if (!mb_peek(msg_id, mb, SUBMESSAGE_ID_OFFSET,
               false /*swap ignored for char*/)
      || int(msg_id) >= SUBMESSAGE_ID_MAX) {
    return true;
  }

  char flags;
  if (!mb_peek(flags, mb, FLAGS_OFFSET, false /*swap ignored for char*/)) {
    return true;
  }

  size_t expected = max_marshaled_size();
  if (!(flags & LIFESPAN_MASK)) expected -= LIFESPAN_LENGTH;
  if (!(flags & COHERENT_MASK)) expected -= COHERENT_LENGTH;

  if (flags & CONTENT_FILT_MASK) {
    CORBA::ULong seqLen;
    const bool swap = (flags & BYTE_ORDER_MASK) != ACE_CDR_BYTE_ORDER;
    if (!mb_peek(seqLen, mb, expected, swap)) {
      return true;
    }
    size_t guidsize = 0, padding = 0;
    gen_find_size(GUID_t(), guidsize, padding);
    expected += sizeof(seqLen) + guidsize * seqLen;
  }

  return len < expected;
}

void
DataSampleHeader::init(ACE_Message_Block* buffer)
{
  this->marshaled_size_ = 0;

  Serializer reader(buffer);

  // Only byte-sized reads until we get the byte_order_ flag.

  if (!(reader >> this->message_id_)) {
    return;
  }

  this->marshaled_size_ += sizeof(this->message_id_);

  if (!(reader >> this->submessage_id_)) {
    return;
  }

  this->marshaled_size_ += sizeof(this->submessage_id_);

  // Extract the flag values.
  ACE_CDR::Octet byte;
  if (!(reader >> ACE_InputCDR::to_octet(byte))) {
    return;
  }

  this->marshaled_size_ += sizeof(byte);

  this->byte_order_         = byte & mask_flag(BYTE_ORDER_FLAG);
  this->coherent_change_    = byte & mask_flag(COHERENT_CHANGE_FLAG);
  this->historic_sample_    = byte & mask_flag(HISTORIC_SAMPLE_FLAG);
  this->lifespan_duration_  = byte & mask_flag(LIFESPAN_DURATION_FLAG);
  this->group_coherent_     = byte & mask_flag(GROUP_COHERENT_FLAG);
  this->content_filter_     = byte & mask_flag(CONTENT_FILTER_FLAG);
  this->sequence_repair_    = byte & mask_flag(SEQUENCE_REPAIR_FLAG);
  this->more_fragments_     = byte & mask_flag(MORE_FRAGMENTS_FLAG);

  // Set swap_bytes flag to the Serializer if data sample from
  // the publisher is in different byte order.
  reader.swap_bytes(this->byte_order_ != ACE_CDR_BYTE_ORDER);

  if (!(reader >> ACE_InputCDR::to_octet(byte))) {
    return;
  }

  this->marshaled_size_ += sizeof(byte);

  this->cdr_encapsulation_ = byte & mask_flag(CDR_ENCAP_FLAG);
  this->key_fields_only_   = byte & mask_flag(KEY_ONLY_FLAG);

  if (!(reader >> this->message_length_)) {
    return;
  }

  this->marshaled_size_ += sizeof(this->message_length_);

  if (!(reader >> this->sequence_)) {
    return;
  }

  size_t padding = 0;
  gen_find_size(this->sequence_, this->marshaled_size_, padding);

  if (!(reader >> this->source_timestamp_sec_)) {
    return;
  }

  this->marshaled_size_ += sizeof(this->source_timestamp_sec_);

  if (!(reader >> this->source_timestamp_nanosec_)) {
    return;
  }

  this->marshaled_size_ += sizeof(this->source_timestamp_nanosec_);

  if (this->lifespan_duration_) {
    if (!(reader >> this->lifespan_duration_sec_)) {
      return;
    }

    this->marshaled_size_ += sizeof(this->lifespan_duration_sec_);

    if (!(reader >> this->lifespan_duration_nanosec_)) {
      return;
    }

    this->marshaled_size_ += sizeof(this->lifespan_duration_nanosec_);
  }

  if (!(reader >> this->publication_id_)) {
    return;
  }

  gen_find_size(this->publication_id_, this->marshaled_size_, padding);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (this->group_coherent_) {
    if (!(reader >> this->publisher_id_)) {
      return;
    }
    gen_find_size(this->publisher_id_, this->marshaled_size_, padding);
  }
#endif

  if (this->content_filter_) {
    if (!(reader >> this->content_filter_entries_)) {
      return;
    }
    gen_find_size(this->content_filter_entries_, this->marshaled_size_, padding);
  }
}

bool
operator<<(ACE_Message_Block& buffer, const DataSampleHeader& value)
{
  Serializer writer(&buffer, value.byte_order_ != ACE_CDR_BYTE_ORDER);

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
                         | (value.more_fragments_     << MORE_FRAGMENTS_FLAG)
                         ;
  writer << ACE_OutputCDR::from_octet(flags);

  flags = (value.cdr_encapsulation_ << CDR_ENCAP_FLAG)
        | (value.key_fields_only_   << KEY_ONLY_FLAG)
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

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (value.group_coherent_) {
    writer << value.publisher_id_;
  }
#endif

  // content_filter_entries_ is deliberately not marshaled here.
  // It's variable sized, so it won't fit into our pre-allocated data block.
  // It may be customized per-datalink so it will be handled later with a
  // a chained (continuation) ACE_Message_Block.

  return writer.good_bit();
}

void
DataSampleHeader::add_cfentries(const GUIDSeq* guids, ACE_Message_Block* mb)
{
  size_t size = 0;
  if (guids) {
    size_t padding = 0; // GUIDs are always aligned
    gen_find_size(*guids, size, padding);
  } else {
    size = sizeof(CORBA::ULong);
  }
  ACE_Message_Block* optHdr = alloc_msgblock(*mb, size, false);

  const bool swap = (ACE_CDR_BYTE_ORDER != test_flag(BYTE_ORDER_FLAG, mb));
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

void
DataSampleHeader::split_payload(const ACE_Message_Block& orig, size_t size,
                                Message_Block_Ptr& head,
                                Message_Block_Ptr& tail)
{
  if (!head) {
    head.reset(orig.duplicate());
  }

  ACE_Message_Block* frag = head.get();
  size_t frag_remain = size;
  for (; frag_remain > frag->length(); frag = frag->cont()) {
    frag_remain -= frag->length();
  }

  if (frag_remain == frag->length()) { // split at ACE_Message_Block boundary
    tail.reset(frag->cont());
  } else {
    tail.reset(frag->duplicate());
    frag->wr_ptr(frag->wr_ptr() - frag->length() + frag_remain);
    ACE_Message_Block::release(frag->cont());
    tail->rd_ptr(frag_remain);
  }
  frag->cont(0);
}

void
DataSampleHeader::split(const ACE_Message_Block& orig, size_t size,
                        Message_Block_Ptr& head, Message_Block_Ptr& tail)
{
  Message_Block_Ptr dup (orig.duplicate());

  const size_t length = dup->total_length();
  DataSampleHeader hdr(*dup); // deserialize entire header (with cfentries)
  const size_t hdr_len = length - dup->total_length();

  ACE_Message_Block* payload = dup.get();
  //skip zero length message blocks
  ACE_Message_Block* prev = 0;
  for (; payload->length() == 0; payload = payload->cont()) {
    prev = payload;
  }
  prev->cont(0);
  Message_Block_Ptr payload_head(payload);

  if (size < hdr_len) { // need to fragment the content_filter_entries_
    head.reset(alloc_msgblock(*dup, max_marshaled_size(), true));
    hdr.more_fragments_ = true;
    hdr.message_length_ = 0; // no room for payload data
    *head << hdr;
    const size_t avail = size - head->length() - 4 /* sequence length */;
    const CORBA::ULong n_entries =
      static_cast<CORBA::ULong>(avail / gen_max_marshaled_size(GUID_t()));
    GUIDSeq entries(n_entries);
    entries.length(n_entries);
    // remove from the end of hdr's entries (order doesn't matter)
    for (CORBA::ULong i(0), x(hdr.content_filter_entries_.length());
         i < n_entries; ++i) {
      entries[i] = hdr.content_filter_entries_[--x];
      hdr.content_filter_entries_.length(x);
    }
    add_cfentries(&entries, head.get());

    tail.reset(alloc_msgblock(*dup, max_marshaled_size(), true));
    hdr.more_fragments_ = false;
    hdr.content_filter_ = (hdr.content_filter_entries_.length() > 0);
    hdr.message_length_ = static_cast<ACE_UINT32>(payload->total_length());
    *tail << hdr;
    tail->cont(payload_head.release());
    if (hdr.content_filter_) {
      add_cfentries(&hdr.content_filter_entries_, tail.get());
    }
    return;
  }

  Message_Block_Ptr payload_tail;
  split_payload(*payload, size - hdr_len, payload_head, payload_tail);

  hdr.more_fragments_ = true;
  hdr.message_length_ = static_cast<ACE_UINT32>(payload_head->total_length());

  head.reset(alloc_msgblock(*dup, max_marshaled_size(), true));
  *head << hdr;
  head->cont(payload_head.release());
  if (hdr.content_filter_) {
    add_cfentries(&hdr.content_filter_entries_, head.get());
  }

  hdr.more_fragments_ = false;
  hdr.content_filter_ = false;
  hdr.message_length_ = static_cast<ACE_UINT32>(payload_tail->total_length());

  tail.reset(alloc_msgblock(*dup, max_marshaled_size(), true));
  *tail << hdr;
  tail->cont(payload_tail.release());
}

bool
DataSampleHeader::join(const DataSampleHeader& first,
                       const DataSampleHeader& second, DataSampleHeader& result)
{
  if (!first.more_fragments_ || first.sequence_ != second.sequence_) {
    return false;
  }
  result = second;
  result.message_length_ += first.message_length_;
  if (first.content_filter_) {
    result.content_filter_ = true;
    const CORBA::ULong entries = first.content_filter_entries_.length();
    CORBA::ULong x = result.content_filter_entries_.length();
    result.content_filter_entries_.length(x + entries);
    for (CORBA::ULong i(entries); i > 0;) {
      result.content_filter_entries_[x++] = first.content_filter_entries_[--i];
    }
  }
  return true;
}

const char* to_string(MessageId value)
{
  switch (value) {
  case SAMPLE_DATA:
    return "SAMPLE_DATA";
  case DATAWRITER_LIVELINESS:
    return "DATAWRITER_LIVELINESS";
  case INSTANCE_REGISTRATION:
    return "INSTANCE_REGISTRATION";
  case UNREGISTER_INSTANCE:
    return "UNREGISTER_INSTANCE";
  case DISPOSE_INSTANCE:
    return "DISPOSE_INSTANCE";
  case GRACEFUL_DISCONNECT:
    return "GRACEFUL_DISCONNECT";
  case REQUEST_ACK:
    return "REQUEST_ACK";
  case SAMPLE_ACK:
    return "SAMPLE_ACK";
  case END_COHERENT_CHANGES:
    return "END_COHERENT_CHANGES";
  case TRANSPORT_CONTROL:
    return "TRANSPORT_CONTROL";
  case DISPOSE_UNREGISTER_INSTANCE:
    return "DISPOSE_UNREGISTER_INSTANCE";
  case END_HISTORIC_SAMPLES:
    return "END_HISTORIC_SAMPLES";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: to_string(MessageId): ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));
    return "Invalid MessageId";
  }
}

const char* to_string(SubMessageId value)
{
  switch (value) {
  case SUBMESSAGE_NONE:
    return "SUBMESSAGE_NONE";
  case MULTICAST_SYN:
    return "MULTICAST_SYN";
  case MULTICAST_SYNACK:
    return "MULTICAST_SYNACK";
  case MULTICAST_NAK:
    return "MULTICAST_NAK";
  case MULTICAST_NAKACK:
    return "MULTICAST_NAKACK";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: to_string(SubMessageId): ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));
    return "Invalid SubMessageId";
  }
}

OPENDDS_STRING to_string(const DataSampleHeader& value)
{
  OPENDDS_STRING ret;
  if (value.submessage_id_ != SUBMESSAGE_NONE) {
    ret += to_string(SubMessageId(value.submessage_id_));
    ret += " 0x";
    ret += to_dds_string(unsigned(value.submessage_id_), true);
    ret += "), ";
  } else {
    ret += to_string(MessageId(value.message_id_));
    ret += " (0x";
    ret += to_dds_string(unsigned(value.message_id_), true);
    ret += "), ";
  }

  ret += "Length: ";
  ret += to_dds_string(value.message_length_);
  ret += ", ";

  ret += "Byte order: ";
  ret += (value.byte_order_ == 1 ? "Little" : "Big");
  ret += " Endian";

  if (value.message_id_ != TRANSPORT_CONTROL) {
    ret += ", ";

    if (value.coherent_change_ == 1) ret += "Coherent, ";
    if (value.historic_sample_ == 1) ret += "Historic, ";
    if (value.lifespan_duration_ == 1) ret += "Lifespan, ";
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
    if (value.group_coherent_ == 1) ret += "Group-Coherent, ";
#endif
    if (value.content_filter_ == 1) ret += "Content-Filtered, ";
    if (value.sequence_repair_ == 1) ret += "Sequence Repair, ";
    if (value.more_fragments_ == 1) ret += "More Fragments, ";
    if (value.cdr_encapsulation_ == 1) ret += "CDR Encapsulation, ";
    if (value.key_fields_only_ == 1) ret += "Key Fields Only, ";

    ret += "Sequence: 0x";
    ret += to_dds_string(unsigned(value.sequence_.getValue()), true);
    ret += ", ";

    ret += "Timestamp: ";
    ret += to_dds_string(value.source_timestamp_sec_);
    ret += ".";
    ret += to_dds_string(value.source_timestamp_nanosec_);
    ret += ", ";

    if (value.lifespan_duration_) {
      ret += "Lifespan: ";
      ret += to_dds_string(value.lifespan_duration_sec_);
      ret += ".";
      ret += to_dds_string(value.lifespan_duration_nanosec_);
      ret += ", ";
    }

    ret += "Publication: " + OPENDDS_STRING(GuidConverter(value.publication_id_));
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
    if (value.group_coherent_) {
      ret += ", Publisher: " + OPENDDS_STRING(GuidConverter(value.publisher_id_));
    }
#endif

    if (value.content_filter_) {
      const CORBA::ULong len = value.content_filter_entries_.length();
      ret += ", Content-Filter Entries (";
      ret += to_dds_string(len);
      ret += "): [";
      for (CORBA::ULong i(0); i < len; ++i) {
        ret += OPENDDS_STRING(GuidConverter(value.content_filter_entries_[i])) + ' ';
      }
      ret += ']';
    }
  }
  return ret;
}

#ifndef OPENDDS_SAFETY_PROFILE
/// Message Id enumeration insertion onto an ostream.
std::ostream& operator<<(std::ostream& os, const MessageId value)
{
  os << to_string(value);
  return os;
}

/// Sub-Message Id enumeration insertion onto an ostream.
std::ostream& operator<<(std::ostream& os, const SubMessageId value)
{
  os << to_string(value);
  return os;
}

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const DataSampleHeader& value)
{
  struct SaveAndRestoreStreamState {
    explicit SaveAndRestoreStreamState(std::ostream& s)
      : fill_(s.fill()), fmt_(s.flags()), s_(s) {}
    ~SaveAndRestoreStreamState()
    {
      s_.fill(fill_);
      s_.flags(fmt_);
    }
    char fill_;
    std::ios_base::fmtflags fmt_;
    std::ostream& s_;
  } stream_state(str);

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
      << " Endian";

  if (value.message_id_ != TRANSPORT_CONTROL) {
    str << ", ";

    if (value.coherent_change_ == 1) str << "Coherent, ";
    if (value.historic_sample_ == 1) str << "Historic, ";
    if (value.lifespan_duration_ == 1) str << "Lifespan, ";
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
    if (value.group_coherent_ == 1) str << "Group-Coherent, ";
#endif
    if (value.content_filter_ == 1) str << "Content-Filtered, ";
    if (value.sequence_repair_ == 1) str << "Sequence Repair, ";
    if (value.more_fragments_ == 1) str << "More Fragments, ";
    if (value.cdr_encapsulation_ == 1) str << "CDR Encapsulation, ";
    if (value.key_fields_only_ == 1) str << "Key Fields Only, ";

    str << "Sequence: 0x" << std::hex << std::setw(4) << std::setfill('0')
        << value.sequence_.getValue() << ", ";

    str << "Timestamp: " << std::dec << value.source_timestamp_sec_ << "."
        << std::dec << value.source_timestamp_nanosec_ << ", ";

    if (value.lifespan_duration_) {
      str << "Lifespan: " << std::dec << value.lifespan_duration_sec_ << "."
          << std::dec << value.lifespan_duration_nanosec_ << ", ";
    }

    str << "Publication: " << GuidConverter(value.publication_id_);
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
    if (value.group_coherent_) {
      str << ", Publisher: " << GuidConverter(value.publisher_id_);
    }
#endif

    if (value.content_filter_) {
      const CORBA::ULong len = value.content_filter_entries_.length();
      str << ", Content-Filter Entries (" << len << "): [";
      for (CORBA::ULong i(0); i < len; ++i) {
        str << GuidConverter(value.content_filter_entries_[i]) << ' ';
      }
      str << ']';
    }
  }

  return str;
}
#endif //OPENDDS_SAFETY_PROFILE


bool
DataSampleHeader::into_received_data_sample(ReceivedDataSample& rds)
{
  rds.header_ = *this;
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
