/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSSAMPLEHEADER_H
#define DCPS_RTPSSAMPLEHEADER_H

#include "ace/Basic_Types.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;
struct DataSampleListElement;

/// Adapt the TransportReceiveStrategy for RTPS's "sample" (submessage) Header
struct RtpsSampleHeader {

  // This is not really the max_marshaled_size, but it's used for determining
  // how much of the submessage to show in debugging hexdumps.  Since we don't
  // know which kind of submessage we have, we can only count on the 4-byte
  // SubmessageHeader being present.
  static size_t max_marshaled_size() { return 4; }

  // We never have partial "sample" headers since this is UDP.
  // (The header could fail to parse in init(), but unlike the TCP case we
  // never want to go back to the reactor and wait for more bytes to arrive.)
  static bool partial(const ACE_Message_Block&) { return false; }

  RtpsSampleHeader();
  explicit RtpsSampleHeader(ACE_Message_Block& mb);
  RtpsSampleHeader& operator=(ACE_Message_Block& mn);

  void pdu_remaining(size_t size);
  size_t marshaled_size();
  ACE_UINT32 message_length();

  bool valid() const;

  void into_received_data_sample(ReceivedDataSample& rds);

  bool more_fragments() const;

  void init(ACE_Message_Block& mb);

  OpenDDS::RTPS::Submessage submessage_;
  bool valid_;
  size_t marshaled_size_, message_length_;

  // Unlike the rest of this class, which is used with the
  // TransportReceiveStrategy, this function does the inverse of
  // into_received_data_sample() so it is used on the sending side:
  // translating from an OpenDDS data structure to the RTPS format.
  static void populate_submessages(OpenDDS::RTPS::SubmessageSeq& subm,
                                   const DataSampleListElement& dsle);
};

}
}

#ifdef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

#endif
