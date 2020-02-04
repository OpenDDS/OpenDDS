/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATASAMPLEHEADER_H
#define OPENDDS_DCPS_DATASAMPLEHEADER_H

#include "Definitions.h"
#include "GuidUtils.h"
#include "PoolAllocationBase.h"
#include "SequenceNumber.h"
#include "RepoIdTypes.h"
#include "Message_Block_Ptr.h"
#include <iosfwd>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;

/// One byte message id (<256)
enum MessageId {
  SAMPLE_DATA,
  DATAWRITER_LIVELINESS,
  INSTANCE_REGISTRATION,
  UNREGISTER_INSTANCE,
  DISPOSE_INSTANCE,
  GRACEFUL_DISCONNECT,
  REQUEST_ACK,
  SAMPLE_ACK,
  END_COHERENT_CHANGES,
  TRANSPORT_CONTROL,
  DISPOSE_UNREGISTER_INSTANCE,
  END_HISTORIC_SAMPLES,
  MESSAGE_ID_MAX // must be the last enumerator
};

enum SubMessageId {
  SUBMESSAGE_NONE,
  MULTICAST_SYN,
  MULTICAST_SYNACK,
  MULTICAST_NAK,
  MULTICAST_NAKACK,
  SUBMESSAGE_ID_MAX // must be the last enumerator
};

enum DataSampleHeaderFlag {
  BYTE_ORDER_FLAG,
  COHERENT_CHANGE_FLAG,
  HISTORIC_SAMPLE_FLAG,
  LIFESPAN_DURATION_FLAG,
  GROUP_COHERENT_FLAG,
  CONTENT_FILTER_FLAG,
  SEQUENCE_REPAIR_FLAG,
  MORE_FRAGMENTS_FLAG
};

enum DataSampleHeaderFlag2 {
  CDR_ENCAP_FLAG,
  KEY_ONLY_FLAG
};

/// The header message of a data sample.
/// This header and the data sample are in different
/// message block and will be chained together.
struct OpenDDS_Dcps_Export DataSampleHeader : public PoolAllocationBase {
  enum {
    MESSAGE_ID_OFFSET = 0,
    SUBMESSAGE_ID_OFFSET = 1,
    FLAGS_OFFSET = 2 // message_id_ + submessage_id_
  };

  /// The enum MessageId.
  char message_id_;

  /// Implementation-specific sub-message Ids.
  char submessage_id_;

  /// 0 -  Message encoded using big-endian byte order. (see ace/CDR_Base.h)
  /// 1 -  Message encoded using little-endian byte order.
  bool byte_order_ : 1;

  /// The flag indicates the sample belongs to a coherent
  /// change set (i.e. PRESENTATION coherent_access == true).
  bool coherent_change_ : 1;

  /// This flag indicates a sample has been resent from a
  /// non-VOLATILE DataWriter.
  bool historic_sample_ : 1;

  /// This flag indicates the sample header contains non-default
  /// LIFESPAN duration fields.
  bool lifespan_duration_ : 1;

  bool group_coherent_ : 1;

  /// The publishing side has applied content filtering, and the optional
  /// content_filter_entries_ field is present in the marshaled header.
  bool content_filter_ : 1;

  /// Due to content filtering, a gap in the sequence numbers may be an
  /// expected condition.  If this bit is set, assume prior sequence numbers
  /// were filtered-out and are not missing.
  bool sequence_repair_ : 1;

  /// The current "Data Sample" needs reassembly before further processing.
  bool more_fragments_ : 1;

  // bools above this line are in the first flags byte, below this line are
  // in the second flags byte.  To avoid complicating the implementation of
  // partial(), flags that impact the size of serialized DataSampleHeader
  // should go in the first flags byte.

  /// The data payload uses CDR encapsulation and alignment rules, as defined
  /// by the RTPS specification formal/2010-11-01.
  bool cdr_encapsulation_ : 1;

  /// Only the key fields of the data sample are present in the payload.
  bool key_fields_only_ : 1;

  bool reserved_1 : 1;
  bool reserved_2 : 1;
  bool reserved_3 : 1;
  bool reserved_4 : 1;
  bool reserved_5 : 1;
  bool reserved_6 : 1;

  /// The size of the data sample (without header).  After this header is
  /// demarshaled, the transport expects to see this many bytes in the stream
  /// before the start of the next header (or end of the Transport PDU).
  ACE_UINT32 message_length_;

  /// The sequence number is obtained from the Publisher
  /// associated with the DataWriter based on the PRESENTATION
  /// requirement for the sequence value (access_scope == GROUP).
  SequenceNumber sequence_;

  //{@
  /// The SOURCE_TIMESTAMP field is generated from the DataWriter
  /// or supplied by the application at the time of the write.
  /// This value is derived from the local hosts system clock,
  /// which is assumed to be synchronized with the clocks on other
  /// hosts within the domain.  This field is required for
  /// DESTINATION_ORDER and LIFESPAN policy behaviors of subscriptions.
  /// It is also required to be present for all data in the
  /// SampleInfo structure supplied along with each data sample.
  ACE_INT32 source_timestamp_sec_;
  ACE_UINT32 source_timestamp_nanosec_; // Corresponding IDL is unsigned.
  //@}

  //{@
  /// The LIFESPAN duration field is generated from the DataWriter
  /// or supplied by the application at the time of the write. This
  /// field is used to determine if a given sample is considered
  /// 'stale' and should be discarded by associated DataReader.
  /// These fields are optional and are controlled by the
  /// lifespan_duration_ flag.
  ACE_INT32 lifespan_duration_sec_;
  ACE_UINT32 lifespan_duration_nanosec_;  // Corresponding IDL is unsigned.
  //@}

  /// Identify the DataWriter that produced the sample data being
  /// sent.
  PublicationId publication_id_;

  /// Id representing the coherent group.  Optional field that's only present if
  /// the flag for group_coherent_ is set.
  RepoId publisher_id_;

  /// Optional field present if the content_filter_ flag bit is set.
  /// Indicates which readers should not receive the data.
  GUIDSeq content_filter_entries_;

  static ACE_UINT8 mask_flag(DataSampleHeaderFlag  flag) { return 1 << flag; }
  static ACE_UINT8 mask_flag(DataSampleHeaderFlag2 flag) { return 1 << flag; }

  static void clear_flag(DataSampleHeaderFlag flag,
                         ACE_Message_Block* buffer);

  static void set_flag(DataSampleHeaderFlag flag,
                       ACE_Message_Block* buffer);

  static bool test_flag(DataSampleHeaderFlag flag,
                        const ACE_Message_Block* buffer);

  /// Does the data in this mb constitute a partial Sample Header?
  static bool partial(const ACE_Message_Block& mb);

  /// Marshal the "guids" as an optional header chained as to the continuation
  /// of "mb" (which must already be a valid DataSampleHeader serialization).
  /// Any existing payload of "mb" (its continuation) will be chained after the
  /// new optional header part.  "guids" may be null, same serialization as 0.
  static void add_cfentries(const GUIDSeq* guids, ACE_Message_Block* mb);

  /// Create two new serialized headers (owned by caller), the "head" having at
  /// most "size" bytes (header + data) and the "tail" having the rest.
  static void split(const ACE_Message_Block& orig, size_t size,
                    Message_Block_Ptr& head, Message_Block_Ptr& tail);

  /// If "first" and "second" are two fragments of the same original message
  /// (as created by split()), return true and set up the "result" header to
  /// match the original header.  Joining the data payload is the
  /// responsibility of the caller (manipulate the continuation chain).
  static bool join(const DataSampleHeader& first,
                   const DataSampleHeader& second, DataSampleHeader& result);

  DataSampleHeader();

  /// Construct with values extracted from a buffer.
  explicit DataSampleHeader(ACE_Message_Block& buffer);

  /// Assignment from an ACE_Message_Block.
  DataSampleHeader& operator=(ACE_Message_Block& buffer);

  /// Amount of data read when initializing from a buffer.
  size_t marshaled_size() const;

  /// Similar to IDL compiler generated methods.
  static size_t max_marshaled_size();

  /// Implement load from buffer.
  void init(ACE_Message_Block* buffer);

  bool into_received_data_sample(ReceivedDataSample& rds);

  ACE_UINT32 message_length() const { return this->message_length_; }

  bool more_fragments() const { return this->more_fragments_; }

  void pdu_remaining(size_t) { /* ignored, only RTPS uses this */ }

  static ACE_Message_Block* alloc_msgblock(const ACE_Message_Block& mb,
                                           size_t size, bool use_data_alloc);

  static void split_payload(const ACE_Message_Block& orig, size_t size,
                            Message_Block_Ptr& head, Message_Block_Ptr& tail);

  /// Returns false if the sample is dispose and/or unregister,
  bool valid_data() const;

private:
  /// Keep track of the amount of data read from a buffer.
  size_t marshaled_size_;
};

const char* to_string(MessageId value);
const char* to_string(SubMessageId value);
OPENDDS_STRING to_string(const DataSampleHeader& value);

/// Marshal/Insertion into a buffer.
OpenDDS_Dcps_Export
bool operator<<(ACE_Message_Block&, const DataSampleHeader& value);

#ifndef OPENDDS_SAFETY_PROFILE
/// Message Id enumeration insertion onto an ostream.
OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& os, MessageId value);

/// Sub-Message Id enumeration insertion onto an ostream.
OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& os, SubMessageId value);

/// Message header insertion onto an ostream.
OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const DataSampleHeader& value);
#endif //OPENDDS_SAFETY_PROFILE

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "DataSampleHeader.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLEHEADER_H */
