/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTHEADER_H
#define OPENDDS_DCPS_TRANSPORTHEADER_H

#include "ace/Basic_Types.h"

#include "dds/DCPS/Definitions.h"

namespace OpenDDS {
namespace DCPS {

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
  static const ACE_INT32      DCPS_PROTOCOL;
  static const ACE_INT32      DCPS_PROTOCOL_SWAPPED;

  static const ACE_CDR::Octet DCPS_VERSION_MAJOR;
  static const ACE_CDR::Octet DCPS_VERSION_MINOR;

  /// Default constructor.
  TransportHeader();

  /// Construct with values extracted from a buffer.
  TransportHeader(ACE_Message_Block* buffer);
  TransportHeader(ACE_Message_Block& buffer);

  /// Assignment from an ACE_Message_Block.
  TransportHeader& operator=(ACE_Message_Block* buffer);
  TransportHeader& operator=(ACE_Message_Block& buffer);

  /// Determine if the serializer should swap bytes.
  bool swap_bytes() const;

  /// Determine if this is a valid packet header.
  bool valid() const;

  /// The protocol of the packet being transmitted. This value also
  /// indicates the endianess of the source.
  ACE_INT32 protocol_;

  /// The major version of the protocol.
  ACE_CDR::Octet version_major_;

  /// The minor version of the protocol.
  ACE_CDR::Octet version_minor_;

  /// A transport-specific identification number which uniquely
  /// identifies the source of the packet.
  ACE_INT32 source_;

  /// The sequence number of the packet identified by this header; this
  /// value is guaranteed to be a monotonically increasing number per
  /// transport instance.
  ACE_INT16 sequence_;

  /// The size of the message following this header, not including the
  /// 11 bytes used by this TransportHeader.
  ACE_UINT16 length_;

  /// Similar to IDL compiler generated methods.
  size_t max_marshaled_size() ;

private:
  /// Demarshall transport packet from ACE_Message_Block.
  void init(ACE_Message_Block* buffer);
};

} // namespace DCPS
} // namespace OpenDDS

extern
ACE_CDR::Boolean
operator<<(ACE_Message_Block&, OpenDDS::DCPS::TransportHeader& value);

extern
ACE_CDR::Boolean
operator<<(ACE_Message_Block*&, OpenDDS::DCPS::TransportHeader& value);

#if defined(__ACE_INLINE__)
#include "TransportHeader.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTHEADER_H */
