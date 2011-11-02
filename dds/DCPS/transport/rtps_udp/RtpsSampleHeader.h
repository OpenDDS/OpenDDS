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
#include "dds/DCPS/transport/framework/TransportSendControlElement.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;
struct DataSampleListElement;

/// Adapt the TransportReceiveStrategy for RTPS's "sample" (submessage) Header
class RtpsSampleHeader {
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

  RtpsSampleHeader();
  explicit RtpsSampleHeader(ACE_Message_Block& mb);
  RtpsSampleHeader& operator=(ACE_Message_Block& mn);

  void pdu_remaining(size_t size);
  size_t marshaled_size();
  ACE_UINT32 message_length();

  bool valid() const;

  void into_received_data_sample(ReceivedDataSample& rds);

  bool more_fragments() const;

  OpenDDS::RTPS::Submessage submessage_;

private:
  void init(ACE_Message_Block& mb);

  bool valid_, frag_;
  size_t marshaled_size_, message_length_;

public:
  // Unlike the rest of this class, which is used with the
  // TransportReceiveStrategy, thes functions do the inverse of
  // into_received_data_sample() so they are used on the sending side:
  // translating from an OpenDDS data structure to the RTPS format.
  static void populate_data_sample_submessages(OpenDDS::RTPS::SubmessageSeq& subm,
                                               const DataSampleListElement& dsle,
                                               bool requires_inline_qos);
  static void populate_data_control_submessages(OpenDDS::RTPS::SubmessageSeq& subm,
                                                const TransportSendControlElement& tsce,
                                                bool requires_inline_qos);
  static void populate_inline_qos(const TransportSendListener::InlineQosData& qos_data,
                                  OpenDDS::RTPS::DataSubmessage& data);

private:
  static void process_iqos(DataSampleHeader& opendds,
                           const OpenDDS::RTPS::ParameterList& iqos);
};

}
}

#ifdef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

#endif
