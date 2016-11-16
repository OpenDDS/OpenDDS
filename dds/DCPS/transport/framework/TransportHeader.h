/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTHEADER_H
#define OPENDDS_DCPS_TRANSPORTHEADER_H

#include "ace/Basic_Types.h"
#include "ace/CDR_Base.h"

#include "dds/DCPS/Definitions.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// If TransportHeader changes size, must recalculate
// serialized size and update static variable here
static const ACE_UINT32 TRANSPORT_HDR_SERIALIZED_SZ(28);

/**
 * @struct TransportHeader
 *
 * @brief Defines class that represents a transport packet header.
 *
 * The TransportHeader is the transport packet header.  Each packet
 * sent by the transport will always start with a transport packet
 * header, followed by one or more data samples (all belonging to the
 * same transport packet).
 */
struct OpenDDS_Dcps_Export TransportHeader {
  static const ACE_CDR::Octet DCPS_PROTOCOL[6];

  /// Default constructor.
  TransportHeader();

  /// Construct with values extracted from a buffer.
  explicit TransportHeader(ACE_Message_Block& buffer);

  /// Assignment from an ACE_Message_Block.
  TransportHeader& operator=(ACE_Message_Block& buffer);

  void incomplete(ACE_Message_Block&) {}

  /// Determine if the serializer should swap bytes.
  bool swap_bytes() const;

  /// Determine if this is a valid packet header.
  bool valid() const;

  /// The protocol of the packet being transmitted.
  ACE_CDR::Octet protocol_[6];

  /// Flags: marshaled as a single byte (ACE_CDR::Octet)
  bool byte_order_;
  bool first_fragment_;
  bool last_fragment_;

  /// Constants for bit masking the marshaled flags byte.
  /// This needs to match the 'Flags' above.
  enum { BYTE_ORDER_FLAG, FIRST_FRAGMENT_FLAG, LAST_FRAGMENT_FLAG };

  /// Reserved for future use (provides padding for preamble).
  ACE_CDR::Octet reserved_;

  /// The size of the message following this header, not including the
  /// bytes used by this TransportHeader.
  ACE_UINT32 length_;

  /// The sequence number of the packet identified by this header; this
  /// value is guaranteed to be a monotonically increasing number per
  /// transport instance.
  SequenceNumber sequence_;

  /// A transport-specific identification number which uniquely
  /// identifies the source of the packet.
  ACE_INT64 source_;

  /// Similar to IDL compiler generated methods.
  static size_t max_marshaled_size();

  /// Demarshal transport packet from ACE_Message_Block.
  void init(ACE_Message_Block* buffer);

  bool first_fragment() { return this->first_fragment_; }
  bool last_fragment()  { return this->last_fragment_; }
  void last_fragment(bool frag) { this->last_fragment_ = frag; }
  const SequenceNumber& sequence() { return this->sequence_; }

  static ACE_UINT32 get_length(const char* marshaled_transport_header);

private:
  struct no_init_t {};
  static const no_init_t no_init;
  explicit TransportHeader(const no_init_t&);
};

OpenDDS_Dcps_Export
bool operator<<(ACE_Message_Block&, const TransportHeader& value);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "TransportHeader.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTHEADER_H */
