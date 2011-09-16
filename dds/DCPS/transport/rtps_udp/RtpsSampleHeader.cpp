/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsSampleHeader.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"

#ifndef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

namespace {
  enum { FLAG_E = 1, FLAG_D = 4, FLAG_K = 8 };
}

namespace OpenDDS {
namespace DCPS {

void
RtpsSampleHeader::init(ACE_Message_Block& mb)
{
  using namespace OpenDDS::RTPS;

  // Manually grab the first two bytes for the SubmessageKind and the byte order
  if (mb.length() == 0) {
    valid_ = false;
    return;
  }

  const SubmessageKind kind = static_cast<SubmessageKind>(*mb.rd_ptr());

  ACE_CDR::Octet flags = 0;

  if (mb.length() > 1) {
    flags = mb.rd_ptr()[1];
  } else if (mb.cont() && mb.cont()->length() > 0) {
    flags = mb.cont()->rd_ptr()[0];
  } else {
    valid_ = false;
    return;
  }

  const bool little_endian = flags & FLAG_E;
  const size_t starting_length = mb.total_length();
  Serializer ser(&mb, ACE_CDR_BYTE_ORDER != little_endian,
                 Serializer::ALIGN_CDR);

  ACE_CDR::UShort octetsToNextHeader = 0;

#define CASE_SMKIND(kind, class, name) case kind: {               \
    class submessage;                                             \
    if (ser >> submessage) {                                      \
      octetsToNextHeader = submessage.smHeader.submessageLength;  \
      submessage_.name##_(submessage);                            \
      valid_ = true;                                              \
    }                                                             \
    break;                                                        \
  }

  switch (kind) {
  CASE_SMKIND(PAD, PadSubmessage, pad)
  CASE_SMKIND(ACKNACK, AckNackSubmessage, acknack)
  CASE_SMKIND(HEARTBEAT, HeartBeatSubmessage, heartbeat)
  CASE_SMKIND(GAP, GapSubmessage, gap)
  CASE_SMKIND(INFO_TS, InfoTimestampSubmessage, info_ts)
  CASE_SMKIND(INFO_SRC, InfoSourceSubmessage, info_src)
  CASE_SMKIND(INFO_REPLY_IP4, InfoReplyIp4Submessage, info_reply_ipv4)
  CASE_SMKIND(INFO_DST, InfoDestinationSubmessage, info_dst)
  CASE_SMKIND(INFO_REPLY, InfoReplySubmessage, info_reply)
  CASE_SMKIND(NACK_FRAG, NackFragSubmessage, nack_frag)
  CASE_SMKIND(HEARTBEAT_FRAG, HeartBeatFragSubmessage, hb_frag)
  CASE_SMKIND(DATA, DataSubmessage, data)
  CASE_SMKIND(DATA_FRAG, DataFragSubmessage, data_frag)
  default:
    {
      SubmessageHeader submessage;
      if (ser >> submessage) {
        octetsToNextHeader = submessage.submessageLength;
        submessage_.unknown_(submessage);
        valid_ = true;
      }
      break;
    }
  }
#undef CASE_SMKIND

  const ACE_CDR::UShort SMHDR_SZ = 4; // size of SubmessageHeader

  if (valid_) {

    // marshaled_size_ is # of bytes of submessage we have read from "mb"
    marshaled_size_ = starting_length - mb.total_length();

    if (octetsToNextHeader == 0 && kind != PAD && kind != INFO_TS) {
      // see RTPS v2.1 section 9.4.5.1.3
      // In this case the current Submessage extends to the end of Message,
      // so we will use the message_length_ that was set in pdu_remaining().
      octetsToNextHeader = static_cast<ACE_CDR::UShort>(message_length_);
    }

    if ((kind == DATA && (flags & (FLAG_D | FLAG_K))) || kind == DATA_FRAG) {
      // These Submessages have a payload which we haven't deserialized yet.
      // The TransportReceiveStrategy will know this via message_length().
      // octetsToNextHeader does not count the SubmessageHeader (4 bytes)
      message_length_ = octetsToNextHeader + SMHDR_SZ - marshaled_size_;
    } else {
      // These Submessages _could_ have extra data that we don't know about
      // (from a newer minor version of the RTPS spec).  Either way, indicate
      // to the TransportReceiveStrategy that there is no data payload here.
      message_length_ = 0;
      if (octetsToNextHeader + SMHDR_SZ > marshaled_size_) {
        valid_ = ser.skip(octetsToNextHeader + SMHDR_SZ -
                          static_cast<ACE_CDR::UShort>(marshaled_size_));
      }
    }
  }
}

void
RtpsSampleHeader::into_received_data_sample(ReceivedDataSample& rds)
{
  using namespace OpenDDS::RTPS;
  DataSampleHeader& opendds = rds.header_;

  switch (submessage_._d()) {
  case DATA: {
    const OpenDDS::RTPS::DataSubmessage& rtps = submessage_.data_();
    opendds.cdr_encapsulation_ = true;
    opendds.message_length_ = message_length();
    opendds.sequence_.setValue(rtps.writerSN.high, rtps.writerSN.low);
    opendds.publication_id_.entityId = rtps.writerId;

    opendds.message_id_ = SAMPLE_DATA;

    for (CORBA::ULong i = 0; i < rtps.inlineQos.length(); ++i) {
      if (rtps.inlineQos[i]._d() == PID_STATUS_INFO) {
        const ACE_CDR::Octet flags = rtps.inlineQos[i].status_info_().value[3];
        if (flags & 1) {
          opendds.message_id_ = DISPOSE_INSTANCE;
        } else if (flags & 2) {
          opendds.message_id_ = UNREGISTER_INSTANCE;
          //TODO: handle both DISPOSE and UNREGISTER
          // (need to synthesize a second ReceivedDataSample for DCPS layer?)
        } else {
          opendds.message_id_ = INSTANCE_REGISTRATION;
        }
      }
    }

    if (rtps.smHeader.flags & FLAG_K) {
      opendds.key_fields_only_ = true;
    } else if (!(rtps.smHeader.flags & (FLAG_D | FLAG_K))) {
      // Handle the case of D = 0 and K = 0 (used for Coherent Sets see 8.7.5)
    }

    if (rtps.smHeader.flags & (FLAG_D | FLAG_K)) {
      opendds.byte_order_ = rds.sample_->rd_ptr()[1] & FLAG_E;
    }

    //TODO: all the rest...
    break;
  }
  default:
    break;
  }
}


}
}
