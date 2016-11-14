/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSSAMPLEHEADER_H
#define DCPS_RTPSSAMPLEHEADER_H

#include "Rtps_Udp_Export.h"

#include "ace/Basic_Types.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/transport/framework/TransportSendControlElement.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;
class DataSampleElement;
class DisjointSequence;

/// Adapt the TransportReceiveStrategy for RTPS's "sample" (submessage) Header
class OpenDDS_Rtps_Udp_Export RtpsSampleHeader {
public:

  // This is not really the max_marshaled_size, but it's used for determining
  // how much of the submessage to show in debugging hexdumps.  Since we don't
  // know which kind of submessage we have, we can only count on the 4-byte
  // SubmessageHeader being present.
  static size_t max_marshaled_size() { return 4; }

  // We never have partial "sample" headers since this is UDP.
  // (The header could fail to parse in init(), but unlike the TCP case we
  // never want to go back to the reactor and wait for more bytes to arrive.)
  static bool partial(const ACE_Message_Block&) { return false; }

  /// Create two new serialized headers (owned by caller), the "head" having at
  /// most "size" bytes (header + data) and the "tail" having the rest.
  /// Returns a pair containing the largest fragment number in each new header.
  static SequenceRange split(const ACE_Message_Block& orig, size_t size,
                             ACE_Message_Block*& head,
                             ACE_Message_Block*& tail);

  RtpsSampleHeader();
  explicit RtpsSampleHeader(ACE_Message_Block& mb);
  RtpsSampleHeader& operator=(ACE_Message_Block& mn);

  void pdu_remaining(size_t size);
  size_t marshaled_size();
  ACE_UINT32 message_length();

  bool valid() const;

  bool into_received_data_sample(ReceivedDataSample& rds);

  bool more_fragments() const;

  RTPS::Submessage submessage_;

private:
  void init(ACE_Message_Block& mb);

  bool valid_, frag_;
  size_t marshaled_size_, message_length_;

public:
  // Unlike the rest of this class, which is used with the
  // TransportReceiveStrategy, these functions do the inverse of
  // into_received_data_sample() so they are used on the sending side:
  // translating from an OpenDDS data structure to the RTPS format.
  static void populate_data_sample_submessages(RTPS::SubmessageSeq& subm,
                                               const DataSampleElement& dsle,
                                               bool requires_inline_qos);
  static void populate_data_control_submessages(RTPS::SubmessageSeq& subm,
                                                const TransportSendControlElement& tsce,
                                                bool requires_inline_qos);
  static void populate_inline_qos(const TransportSendListener::InlineQosData& qos_data,
                                  RTPS::ParameterList& plist);
  static bool control_message_supported(char message_id);

  // All of the fragments we generate will use the FRAG_SIZE of 1024, which may
  // be the smallest allowed by the spec (8.4.14.1.1).  There is no practical
  // advantage to increasing this constant, since any number of 1024-byte
  // fragments may appear in a single DATA_FRAG submessage.  The spec is
  // ambiguous in its use of KB to mean either 1000 or 1024, and uses < instead
  // of <= (see issue 16966).
  static const ACE_CDR::UShort FRAG_SIZE = 1024;

private:
  static void process_iqos(DataSampleHeader& opendds,
                           const OpenDDS::RTPS::ParameterList& iqos);
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

#endif
