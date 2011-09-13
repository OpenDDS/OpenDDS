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

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {

class ReceivedDataSample;

/// Adapt the TransportReceiveStrategy for RTPS's "sample" (submessage) Header
struct RtpsSampleHeader {

  static size_t max_marshaled_size();
  static bool partial(const ACE_Message_Block& mb);

  RtpsSampleHeader();
  explicit RtpsSampleHeader(ACE_Message_Block& mb);
  RtpsSampleHeader& operator=(ACE_Message_Block& mn);

  size_t marshaled_size();

  bool valid() const;

  void into_received_data_sample(ReceivedDataSample& rds);

  ACE_UINT32 message_length();

  bool more_fragments() const;

};

}
}

#ifdef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

#endif
