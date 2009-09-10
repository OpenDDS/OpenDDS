/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATASAMPLEHEADER_H
#define OPENDDS_DCPS_DATASAMPLEHEADER_H

#include "Definitions.h"
#include "GuidUtils.h"

#include <iosfwd>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

/// One byte message id (<256)
enum MessageId {
  SAMPLE_DATA,
  DATAWRITER_LIVELINESS,
  INSTANCE_REGISTRATION,
  UNREGISTER_INSTANCE,
  DISPOSE_INSTANCE,
  GRACEFUL_DISCONNECT,
  FULLY_ASSOCIATED,
  REQUEST_ACK,
  SAMPLE_ACK,
  END_COHERENT_CHANGES
};

enum DataSampleHeaderFlag {
  BYTE_ORDER_FLAG,
  COHERENT_CHANGE_FLAG,
  HISTORIC_SAMPLE_FLAG,
  LIFESPAN_DURATION_FLAG,
  RESERVED_1_FLAG,
  RESERVED_2_FLAG,
  RESERVED_3_FLAG,
  RESERVED_4_FLAG
};

/// The header message of a data sample.
/// This header and the data sample are in different
/// message block and will be chained together.
struct OpenDDS_Dcps_Export DataSampleHeader {
  /// The enum MessageId.
  char message_id_;

  /// 0 -  Message encoded using little-endian byte order.
  /// 1 -  Message encoded using network byte order.
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

  /// reservered bits
  bool reserved_1   : 1;
  bool reserved_2   : 1;
  bool reserved_3   : 1;
  bool reserved_4   : 1;

  //The size of the message including the entire header.
  ACE_UINT32 message_length_;

  /// The sequence number is obtained from the Publisher
  /// associated with the DataWriter based on the PRESENTATION
  /// requirement for the sequence value (access_scope == GROUP).
  ACE_INT16   sequence_;

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

  /// The LIFESPAN duration field is generated from the DataWriter
  /// or supplied by the application at the time of the write. This
  /// field is used to determine if a given sample is considered
  /// 'stale' and should be discarded by associated DataReader.
  /// These fields are optional and are controlled by the
  /// lifespan_duration_ flag.
  ACE_INT32 lifespan_duration_sec_;
  ACE_UINT32 lifespan_duration_nanosec_;  // Corresponding IDL is unsigned.

  /// Identify the DataWriter that produced the sample data being
  /// sent.
  PublicationId  publication_id_;

  static long mask_flag(DataSampleHeaderFlag flag);

  static void clear_flag(DataSampleHeaderFlag flag,
                         ACE_Message_Block* buffer);

  static void set_flag(DataSampleHeaderFlag flag,
                       ACE_Message_Block* buffer);

  static bool test_flag(DataSampleHeaderFlag flag,
                        ACE_Message_Block* buffer);

  /// Does the data in this mb constitute a partial Sample Header?
  static bool partial(ACE_Message_Block& mb);

  /// Default constructor.
  DataSampleHeader() ;

  /// Construct with values extracted from a buffer.
  DataSampleHeader(ACE_Message_Block* buffer) ;
  DataSampleHeader(ACE_Message_Block& buffer) ;

  /// Assignment from an ACE_Message_Block.
  DataSampleHeader& operator= (ACE_Message_Block* buffer) ;
  DataSampleHeader& operator= (ACE_Message_Block& buffer) ;

  /// Amount of data read when initializing from a buffer.
  size_t marshaled_size() ;

  /// Similar to IDL compiler generated methods.
  size_t max_marshaled_size() ;

private:
  /// Implement load from buffer.
  void init(ACE_Message_Block* buffer) ;

  /// Keep track of the amount of data read from a buffer.
  size_t marshaled_size_ ;
};

/// Used to allocator the DataSampleHeader object.
typedef Cached_Allocator_With_Overflow<DataSampleHeader, ACE_Null_Mutex>
DataSampleHeaderAllocator;

#if defined(__ACE_INLINE__)
#include "DataSampleHeader.inl"

#endif /* __ACE_INLINE__ */

} // namespace DCPS
} // namespace OpenDDS

/// Marshal/Insertion into a buffer.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator<< (ACE_Message_Block*&, OpenDDS::DCPS::DataSampleHeader& value) ;

/// Message Id enumarion insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const OpenDDS::DCPS::MessageId value);

/// Message header insertion onto an ostream.
extern OpenDDS_Dcps_Export
std::ostream& operator<<(std::ostream& str, const OpenDDS::DCPS::DataSampleHeader& value);

#endif  /* OPENDDS_DCPS_DATASAMPLEHEADER_H */
