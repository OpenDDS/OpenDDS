/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsSampleHeader.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"

#include <sstream>

#ifndef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

namespace {
  enum { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4, FLAG_K = 8 };
  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_REGISTER   = { { 0, 0, 0, 0 } };
  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_DISPOSE    = { { 0, 0, 0, 1 } };
  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_UNREGISTER = { { 0, 0, 0, 2 } };
  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_DISPOSE_UNREGISTER = { { 0, 0, 0, 3 } };

}

namespace OpenDDS {
  namespace RTPS {
    inline bool operator==(const OpenDDS::RTPS::StatusInfo_t& lhs,
                           const OpenDDS::RTPS::StatusInfo_t& rhs)
    {
      return
        (lhs.value[3] == rhs.value[3]) &&
        (lhs.value[2] == rhs.value[2]) &&
        (lhs.value[1] == rhs.value[1]) &&
        (lhs.value[0] == rhs.value[0]);
    }
  }
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
      submessage_.name##_sm(submessage);                          \
      valid_ = true;                                              \
    }                                                             \
    break;                                                        \
  }

  valid_ = false;
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
        submessage_.unknown_sm(submessage);
        valid_ = true;
      }
      break;
    }
  }
#undef CASE_SMKIND

  if (valid_) {

    // marshaled_size_ is # of bytes of submessage we have read from "mb"
    marshaled_size_ = starting_length - mb.total_length();

    if (octetsToNextHeader == 0 && kind != PAD && kind != INFO_TS) {
      // see RTPS v2.1 section 9.4.5.1.3
      // In this case the current Submessage extends to the end of Message,
      // so we will use the message_length_ that was set in pdu_remaining().
      octetsToNextHeader =
        static_cast<ACE_CDR::UShort>(message_length_ - SMHDR_SZ);
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
      ACE_CDR::UShort marshaled = static_cast<ACE_CDR::UShort>(marshaled_size_);
      if (octetsToNextHeader + SMHDR_SZ > marshaled) {
        valid_ = ser.skip(octetsToNextHeader + SMHDR_SZ - marshaled);
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
    const OpenDDS::RTPS::DataSubmessage& rtps = submessage_.data_sm();
    opendds.cdr_encapsulation_ = true;
    opendds.message_length_ = message_length();
    opendds.sequence_.setValue(rtps.writerSN.high, rtps.writerSN.low);
    opendds.publication_id_.entityId = rtps.writerId;

    opendds.message_id_ = SAMPLE_DATA;

#if defined(OPENDDS_TEST_INLINE_QOS)
    std::stringstream os;
    os << "into_received_data_sample(): " << rtps.inlineQos.length()
       << " inline QoS parameters\n";
    for (CORBA::ULong index = 0; index < rtps.inlineQos.length(); ++index) {
      os << "  parameter type = " << rtps.inlineQos[index]._d() << "\n";
    }
    ACE_DEBUG((LM_DEBUG, "%s", os.str().c_str()));
#endif
    for (CORBA::ULong i = 0; i < rtps.inlineQos.length(); ++i) {
      if (rtps.inlineQos[i]._d() == PID_STATUS_INFO) {
        if (rtps.inlineQos[i].status_info() == STATUS_INFO_DISPOSE) {
          opendds.message_id_ = DISPOSE_INSTANCE;
        } else if (rtps.inlineQos[i].status_info() == STATUS_INFO_UNREGISTER) {
          opendds.message_id_ = UNREGISTER_INSTANCE;
        } else if (rtps.inlineQos[i].status_info() == STATUS_INFO_DISPOSE_UNREGISTER) {
          opendds.message_id_ = DISPOSE_UNREGISTER_INSTANCE;
        } else if (rtps.inlineQos[i].status_info() == STATUS_INFO_REGISTER) {
          // TODO: Remove this case if we decide not to send Register messages
          opendds.message_id_ = INSTANCE_REGISTRATION;
        }
#if defined(OPENDDS_TEST_INLINE_QOS)
      } else if (rtps.inlineQos[i]._d() == PID_TOPIC_NAME) {
        ACE_DEBUG((LM_DEBUG, "topic_name = %s\n", rtps.inlineQos[i].string_data()));
      } else if (rtps.inlineQos[i]._d() == PID_PRESENTATION) {
        DDS::PresentationQosPolicy pres_qos = rtps.inlineQos[i].presentation();
        ACE_DEBUG((LM_DEBUG, "presentation qos, access_scope = %d, "
                   "coherent_access = %d, ordered_access = %d\n",
                   pres_qos.access_scope, pres_qos.coherent_access, pres_qos.ordered_access));
      } else if (rtps.inlineQos[i]._d() == PID_PARTITION) {
        DDS::PartitionQosPolicy part_qos = rtps.inlineQos[i].partition();
        ACE_DEBUG((LM_DEBUG, "partition qos(%d): ", part_qos.name.length()));
        for (size_t i = 0; i < part_qos.name.length(); i++) {
          ACE_DEBUG((LM_DEBUG, "'%s'  ", part_qos.name[i].in()));
        }
        ACE_DEBUG((LM_DEBUG, "\n"));
#endif
      }
    }

    if (rtps.smHeader.flags & FLAG_K) {
      opendds.key_fields_only_ = true;
    } else if (!(rtps.smHeader.flags & (FLAG_D | FLAG_K))) {
      // TODO: Handle the case of D = 0 and K = 0 (used for Coherent Sets see 8.7.5)
    }

    if (rtps.smHeader.flags & (FLAG_D | FLAG_K)) {
      // Peek at the byte order from the encapsulation containing the payload.
      opendds.byte_order_ = rds.sample_->rd_ptr()[1] & FLAG_E;
    }

    //TODO: all the rest...
    break;
  }
  default:
    break;
  }
}

void
RtpsSampleHeader::populate_data_sample_submessages(OpenDDS::RTPS::SubmessageSeq& subm,
                                                   const DataSampleListElement& dsle,
                                                   bool requires_inline_qos)
{
  using namespace OpenDDS::RTPS;

  ACE_CDR::Octet flags =
    DataSampleHeader::test_flag(BYTE_ORDER_FLAG, dsle.sample_);
  const ACE_CDR::UShort len = 8;
  const DDS::Time_t st = { dsle.header_.source_timestamp_sec_,
                           dsle.header_.source_timestamp_nanosec_ };
  const InfoTimestampSubmessage ts = { {INFO_TS, flags, len},
    {st.sec, static_cast<ACE_UINT32>(st.nanosec * NANOS_TO_RTPS_FRACS + .5)} };
  CORBA::ULong i = subm.length();
  subm.length(i + 1);
  subm[i++].info_ts_sm(ts);

  DataSubmessage data = {
    {DATA, flags, 0},
    0,
    DATA_OCTETS_TO_IQOS,
    ENTITYID_UNKNOWN,
    dsle.publication_id_.entityId,
    {dsle.header_.sequence_.getHigh(), dsle.header_.sequence_.getLow()},
    ParameterList()
  };
  char message_id = dsle.header_.message_id_;
  switch (message_id) {
  case SAMPLE_DATA:
    // Must be a data message
    data.smHeader.flags |= FLAG_D;
    break;
  default:
    // TODO: Figure out what to do here (if there are other cases)
    ACE_DEBUG((LM_INFO,
               "RtpsSampleHeader::populate_submessages(): Non-sample messages seen, message_id = %d\n",
               message_id));
    break;
  }

  if (requires_inline_qos) {
    TransportSendListener::InlineQosData qos_data;
    dsle.send_listener_->retrieve_inline_qos_data(qos_data);

    populate_inline_qos(qos_data, data);
  }

  if (data.inlineQos.length() > 0) {
    data.smHeader.flags |= FLAG_Q;
  }

  subm.length(i + 1);
  subm[i].data_sm(data);
}


void
RtpsSampleHeader::populate_data_control_submessages(OpenDDS::RTPS::SubmessageSeq& subm,
                                                    const TransportSendControlElement& tsce,
                                                    bool requires_inline_qos)
{
  using namespace OpenDDS::RTPS;

  const DataSampleHeader& header = tsce.header();
  ACE_CDR::Octet flags = header.byte_order_ == true;
  const ACE_CDR::UShort len = 8;
  ACE_INT32  st_sec  = header.source_timestamp_sec_;
  ACE_UINT32 st_nsec = header.source_timestamp_nanosec_;
  const InfoTimestampSubmessage ts = { {INFO_TS, flags, len},
    {st_sec, static_cast<ACE_UINT32>(st_nsec * NANOS_TO_RTPS_FRACS + .5)} };
  CORBA::ULong i = subm.length();
  subm.length(i + 1);
  subm[i++].info_ts_sm(ts);

  DataSubmessage data = {
    {DATA, flags, 0},
    0,
    DATA_OCTETS_TO_IQOS,
    ENTITYID_UNKNOWN,
    header.publication_id_.entityId,
    {header.sequence_.getHigh(), header.sequence_.getLow()},
    ParameterList()
  };
  switch (header.message_id_) {
  case INSTANCE_REGISTRATION:
    {
      // TODO: Determine if we want to send a message here.
      // We should probably eat this message, but for now I am just going to
      // send it with none of the status info bits set (it is easier)
      data.smHeader.flags |= FLAG_K;
      int qos_len = data.inlineQos.length();
      data.inlineQos.length(qos_len+1);
      data.inlineQos[qos_len].status_info(STATUS_INFO_REGISTER);
    }
    break;
  case UNREGISTER_INSTANCE:
    {
      data.smHeader.flags |= FLAG_K;
      int qos_len = data.inlineQos.length();
      data.inlineQos.length(qos_len+1);
      data.inlineQos[qos_len].status_info(STATUS_INFO_UNREGISTER);
    }
    break;
  case DISPOSE_INSTANCE:
    {
      data.smHeader.flags |= FLAG_K;
      int qos_len = data.inlineQos.length();
      data.inlineQos.length(qos_len+1);
      data.inlineQos[qos_len].status_info(STATUS_INFO_DISPOSE);
    }
    break;
  case DISPOSE_UNREGISTER_INSTANCE:
    {
      data.smHeader.flags |= FLAG_K;
      int qos_len = data.inlineQos.length();
      data.inlineQos.length(qos_len+1);
      data.inlineQos[qos_len].status_info(STATUS_INFO_DISPOSE_UNREGISTER);
    }
    break;
  default:
    // TODO: Figure out what to do here (if there are other cases)
    ACE_DEBUG((LM_INFO,
               "RtpsSampleHeader::populate_control_submessages(): Non-sample messages seen, message_id = %d\n",
               header.message_id_));
    break;
  }

  if (requires_inline_qos) {
    TransportSendListener::InlineQosData qos_data;
    tsce.listener()->retrieve_inline_qos_data(qos_data);

    populate_inline_qos(qos_data, data);
  }

  if (data.inlineQos.length() > 0) {
    data.smHeader.flags |= FLAG_Q;
  }

  subm.length(i + 1);
  subm[i].data_sm(data);
}

#define PROCESS_INLINE_QOS(QOS_NAME, DEFAULT_QOS, WRITER_QOS) \
  if (WRITER_QOS.QOS_NAME != DEFAULT_QOS.QOS_NAME) {          \
    int qos_len = data.inlineQos.length();                    \
    data.inlineQos.length(qos_len+1);                         \
    data.inlineQos[qos_len].QOS_NAME(WRITER_QOS.QOS_NAME);    \
  }

void
RtpsSampleHeader::populate_inline_qos(const TransportSendListener::InlineQosData& qos_data,
                                      OpenDDS::RTPS::DataSubmessage& data)
{
  using namespace OpenDDS::RTPS;

  // Always include topic name (per the spec)
  int qos_len = data.inlineQos.length();
  data.inlineQos.length(qos_len+1);
  data.inlineQos[qos_len].string_data(qos_data.topic_name.c_str());
  data.inlineQos[qos_len]._d(PID_TOPIC_NAME);

  // Conditionally include other QoS inline when the differ from the
  // default value.
  DDS::PublisherQos default_pub_qos = PUBLISHER_QOS_DEFAULT;
  PROCESS_INLINE_QOS(presentation, default_pub_qos, qos_data.pub_qos);
  PROCESS_INLINE_QOS(partition, default_pub_qos, qos_data.pub_qos);

  DDS::DataWriterQos default_dw_qos = DATAWRITER_QOS_DEFAULT;
  PROCESS_INLINE_QOS(durability, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(deadline, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(latency_budget, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(ownership, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(ownership_strength, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(liveliness, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(reliability, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(transport_priority, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(lifespan, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(destination_order, default_dw_qos, qos_data.dw_qos);
}

}
}
