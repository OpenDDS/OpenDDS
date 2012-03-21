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
#include "dds/DCPS/Service_Participant.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"

#include <sstream>
#include <cstring>

#ifndef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

namespace {
  enum { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4,
         FLAG_K_IN_DATA = 8, FLAG_K_IN_FRAG = 4 };

  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_REGISTER = { { 0, 0, 0, 0 } },
          STATUS_INFO_DISPOSE = { { 0, 0, 0, 1 } },
          STATUS_INFO_UNREGISTER = { { 0, 0, 0, 2 } },
          STATUS_INFO_DISPOSE_UNREGISTER = { { 0, 0, 0, 3 } };
}

namespace OpenDDS {
namespace RTPS {

inline bool
operator==(const StatusInfo_t& lhs, const StatusInfo_t& rhs)
{
  return
    (lhs.value[3] == rhs.value[3]) &&
    (lhs.value[2] == rhs.value[2]) &&
    (lhs.value[1] == rhs.value[1]) &&
    (lhs.value[0] == rhs.value[0]);
}

}
namespace DCPS {

void
RtpsSampleHeader::init(ACE_Message_Block& mb)
{
  using namespace OpenDDS::RTPS;

  // valid_ is false here, it will only be set to true if there is a Submessage

  // Manually grab the first two bytes for the SubmessageKind and the byte order
  if (mb.length() == 0) {
    return;
  }

  const SubmessageKind kind = static_cast<SubmessageKind>(*mb.rd_ptr());

  ACE_CDR::Octet flags = 0;

  if (mb.length() > 1) {
    flags = mb.rd_ptr()[1];
  } else if (mb.cont() && mb.cont()->length() > 0) {
    flags = mb.cont()->rd_ptr()[0];
  } else {
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

    frag_ = (kind == DATA_FRAG);

    // marshaled_size_ is # of bytes of submessage we have read from "mb"
    marshaled_size_ = starting_length - mb.total_length();

    if (octetsToNextHeader == 0 && kind != PAD && kind != INFO_TS) {
      // see RTPS v2.1 section 9.4.5.1.3
      // In this case the current Submessage extends to the end of Message,
      // so we will use the message_length_ that was set in pdu_remaining().
      octetsToNextHeader =
        static_cast<ACE_CDR::UShort>(message_length_ - SMHDR_SZ);
    }

    if ((kind == DATA && (flags & (FLAG_D | FLAG_K_IN_DATA)))
        || kind == DATA_FRAG) {
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
        marshaled_size_ = octetsToNextHeader + SMHDR_SZ;
      }
    }
  }
}

void
RtpsSampleHeader::process_iqos(DataSampleHeader& opendds,
                               const RTPS::ParameterList& iqos)
{
  using namespace OpenDDS::RTPS;
#if defined(OPENDDS_TEST_INLINE_QOS)
  std::stringstream os;
  os << "into_received_data_sample(): " << iqos.length()
     << " inline QoS parameters\n";
  for (CORBA::ULong index = 0; index < iqos.length(); ++index) {
    os << "  parameter type = " << iqos[index]._d() << "\n";
  }
  ACE_DEBUG((LM_DEBUG, "%C", os.str().c_str()));
#endif
  for (CORBA::ULong i = 0; i < iqos.length(); ++i) {
    if (iqos[i]._d() == PID_STATUS_INFO) {
      if (iqos[i].status_info() == STATUS_INFO_DISPOSE) {
        opendds.message_id_ = DISPOSE_INSTANCE;
      } else if (iqos[i].status_info() == STATUS_INFO_UNREGISTER) {
        opendds.message_id_ = UNREGISTER_INSTANCE;
      } else if (iqos[i].status_info() == STATUS_INFO_DISPOSE_UNREGISTER) {
        opendds.message_id_ = DISPOSE_UNREGISTER_INSTANCE;
      } else if (iqos[i].status_info() == STATUS_INFO_REGISTER) {
        opendds.message_id_ = INSTANCE_REGISTRATION;
      }
    } else if (iqos[i]._d() == PID_ORIGINAL_WRITER_INFO) {
      opendds.historic_sample_ = true;
#if defined(OPENDDS_TEST_INLINE_QOS)
    } else if (iqos[i]._d() == PID_TOPIC_NAME) {
      ACE_DEBUG((LM_DEBUG, "topic_name = %C\n", iqos[i].string_data()));
    } else if (iqos[i]._d() == PID_PRESENTATION) {
      DDS::PresentationQosPolicy pres_qos = iqos[i].presentation();
      ACE_DEBUG((LM_DEBUG, "presentation qos, access_scope = %d, "
                 "coherent_access = %d, ordered_access = %d\n",
                 pres_qos.access_scope, pres_qos.coherent_access,
                 pres_qos.ordered_access));
    } else if (iqos[i]._d() == PID_PARTITION) {
      DDS::PartitionQosPolicy part_qos = iqos[i].partition();
      ACE_DEBUG((LM_DEBUG, "partition qos(%d): ", part_qos.name.length()));
      for (CORBA::ULong j = 0; j < part_qos.name.length(); j++) {
        ACE_DEBUG((LM_DEBUG, "'%C'  ", part_qos.name[j].in()));
      }
      ACE_DEBUG((LM_DEBUG, "\n"));
#endif
    }
  }
}

bool
RtpsSampleHeader::into_received_data_sample(ReceivedDataSample& rds)
{
  using namespace OpenDDS::RTPS;
  DataSampleHeader& opendds = rds.header_;

  switch (submessage_._d()) {
  case DATA: {
    const DataSubmessage& rtps = submessage_.data_sm();
    opendds.cdr_encapsulation_ = true;
    opendds.message_length_ = message_length();
    opendds.sequence_.setValue(rtps.writerSN.high, rtps.writerSN.low);
    opendds.publication_id_.entityId = rtps.writerId;
    opendds.message_id_ = SAMPLE_DATA;

    process_iqos(opendds, rtps.inlineQos);

    if (rtps.smHeader.flags & FLAG_K_IN_DATA) {
      opendds.key_fields_only_ = true;
    } else if (!(rtps.smHeader.flags & (FLAG_D | FLAG_K_IN_DATA))) {
      // Interoperability note: the Key may be hiding in the "key hash" param
      // in the InlineQos.  In order to make use of this Key, it mst be 16
      // bytes or less.  We have observed other DDS implementations only send
      // the MD5 hash of a >16 byte key, so we must limit this to Built-in
      // endpoints which are assumed to use GUIDs as keys.
      if ((rtps.writerId.entityKind & 0xC0) == 0xC0 // Only Built-in endpoints
          && (rtps.smHeader.flags & FLAG_Q) && !rds.sample_) {
        for (CORBA::ULong i = 0; i < rtps.inlineQos.length(); ++i) {
          if (rtps.inlineQos[i]._d() == PID_KEY_HASH) {
            rds.sample_ = new ACE_Message_Block(20);
            // CDR_BE encapsuation scheme (endianness is not used for key hash)
            rds.sample_->copy("\x00\x00\x00\x00", 4);
            const CORBA::Octet* data = rtps.inlineQos[i].key_hash().value;
            rds.sample_->copy(reinterpret_cast<const char*>(data), 16);
            opendds.message_length_ = rds.sample_->length();
            opendds.key_fields_only_ = true;
            if (Transport_debug_level) {
              ACE_DEBUG((LM_DEBUG,
                         "(%P|%t) RtpsSampleHeader::into_received_data_sample()"
                         " - used KeyHash data as the key-only payload\n"));
            }
            break;
          }
        }
      } else {
      // FUTURE: Handle the case of D = 0 and K = 0
      // used for Coherent Sets in PRESENTATION QoS (see 8.7.5)
        if (Transport_debug_level) {
          ACE_DEBUG((LM_WARNING,
                     "(%P|%t) RtpsSampleHeader::into_received_data_sample() - "
                     "Received a DATA Submessage with D = 0 and K = 0, "
                     "dropping\n"));
        }
        return false;
      }
    }

    if (rtps.smHeader.flags & (FLAG_D | FLAG_K_IN_DATA)) {
      // Peek at the byte order from the encapsulation containing the payload.
      opendds.byte_order_ = rds.sample_->rd_ptr()[1] & FLAG_E;
    }

    break;
  }
  case DATA_FRAG: {
    const DataFragSubmessage& rtps = submessage_.data_frag_sm();
    opendds.cdr_encapsulation_ = true;
    opendds.message_length_ = message_length();
    opendds.sequence_.setValue(rtps.writerSN.high, rtps.writerSN.low);
    opendds.publication_id_.entityId = rtps.writerId;
    opendds.message_id_ = SAMPLE_DATA;
    opendds.key_fields_only_ = (rtps.smHeader.flags & FLAG_K_IN_FRAG);

    // Peek at the byte order from the encapsulation containing the payload.
    opendds.byte_order_ = rds.sample_->rd_ptr()[1] & FLAG_E;

    process_iqos(opendds, rtps.inlineQos);

    const CORBA::ULong lastFragInSubmsg =
      rtps.fragmentStartingNum.value - 1 + rtps.fragmentsInSubmessage;
    if (lastFragInSubmsg * rtps.fragmentSize < rtps.sampleSize) {
      opendds.more_fragments_ = true;
    }
    break;
  }
  default:
    break;
  }

  return true;
}

void
RtpsSampleHeader::populate_data_sample_submessages(
  OpenDDS::RTPS::SubmessageSeq& subm,
  const DataSampleListElement& dsle,
  bool requires_inline_qos)
{
  using namespace OpenDDS::RTPS;

  const ACE_CDR::Octet flags =
    DataSampleHeader::test_flag(BYTE_ORDER_FLAG, dsle.sample_);
  const ACE_CDR::UShort len = 8;
  const DDS::Time_t st = { dsle.header_.source_timestamp_sec_,
                           dsle.header_.source_timestamp_nanosec_ };
  const InfoTimestampSubmessage ts = { {INFO_TS, flags, len},
    {st.sec, static_cast<ACE_UINT32>(st.nanosec * NANOS_TO_RTPS_FRACS + .5)} };
  CORBA::ULong i = subm.length();
  subm.length(i + 1);
  subm[i++].info_ts_sm(ts);

  EntityId_t readerId = ENTITYID_UNKNOWN;
  if (dsle.num_subs_ == 1) {
    readerId = dsle.subscription_ids_[0].entityId;
    InfoDestinationSubmessage idest;
    idest.smHeader.submessageId = INFO_DST;
    idest.smHeader.flags = flags;
    idest.smHeader.submessageLength = INFO_DST_SZ;
    std::memcpy(idest.guidPrefix, dsle.subscription_ids_[0].guidPrefix,
                sizeof(GuidPrefix_t));
    subm.length(i + 1);
    subm[i++].info_dst_sm(idest);
  }

  DataSubmessage data = {
    {DATA, flags, 0},
    0,
    DATA_OCTETS_TO_IQOS,
    readerId,
    dsle.publication_id_.entityId,
    {dsle.header_.sequence_.getHigh(), dsle.header_.sequence_.getLow()},
    ParameterList()
  };
  const char message_id = dsle.header_.message_id_;
  switch (message_id) {
  case SAMPLE_DATA:
    // Must be a data message
    data.smHeader.flags |= FLAG_D;
    break;
  default:
    ACE_DEBUG((LM_INFO, "(%P|%t) RtpsSampleHeader::populate_submessages(): "
               "Non-sample messages seen, message_id = %d\n", int(message_id)));
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

namespace {
  // Interoperability note:
  // used for discovery (SEDP) only, this helps interoperability because they
  // spec is unclear about the use of PID_KEY_HASH vs. the key as the payload
  void add_key_hash(RTPS::ParameterList& plist, const ACE_Message_Block* data)
  {
    RTPS::KeyHash_t kh;
    static const size_t offset = 8 /*skip encap (4) and plist hdr (4)*/;
    std::memcpy(kh.value, data->rd_ptr() + offset, sizeof(GUID_t));
    RTPS::Parameter p;
    p.key_hash(kh);
    const CORBA::ULong i = plist.length();
    plist.length(i + 1);
    plist[i] = p;
  }
}

void
RtpsSampleHeader::populate_data_control_submessages(
  OpenDDS::RTPS::SubmessageSeq& subm,
  const TransportSendControlElement& tsce,
  bool requires_inline_qos)
{
  using namespace OpenDDS::RTPS;

  const DataSampleHeader& header = tsce.header();
  const ACE_CDR::Octet flags = (header.byte_order_ == true);
  const ACE_CDR::UShort len = 8;
  const ACE_INT32  st_sec  = header.source_timestamp_sec_;
  const ACE_UINT32 st_nsec = header.source_timestamp_nanosec_;
  const InfoTimestampSubmessage ts = { {INFO_TS, flags, len},
    {st_sec, static_cast<ACE_UINT32>(st_nsec * NANOS_TO_RTPS_FRACS + .5)} };
  CORBA::ULong i = subm.length();
  subm.length(i + 1);
  subm[i++].info_ts_sm(ts);

  static const CORBA::Octet BUILT_IN_WRITER = 0xC2;

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
  case INSTANCE_REGISTRATION: {
    // NOTE: The RTPS spec is not entirely clear about instance registration.
    // We have decided to send a DATA Submessage containing the key and an
    // inlineQoS StatusInfo of zero.
    data.smHeader.flags |= FLAG_K_IN_DATA;
    const int qos_len = data.inlineQos.length();
    data.inlineQos.length(qos_len + 1);
    data.inlineQos[qos_len].status_info(STATUS_INFO_REGISTER);
    break;
  }
  case UNREGISTER_INSTANCE: {
    data.smHeader.flags |= FLAG_K_IN_DATA;
    const int qos_len = data.inlineQos.length();
    data.inlineQos.length(qos_len+1);
    data.inlineQos[qos_len].status_info(STATUS_INFO_UNREGISTER);
    if (header.publication_id_.entityId.entityKind == BUILT_IN_WRITER) {
      add_key_hash(data.inlineQos, tsce.msg_payload());
    }
    break;
  }
  case DISPOSE_INSTANCE: {
    data.smHeader.flags |= FLAG_K_IN_DATA;
    const int qos_len = data.inlineQos.length();
    data.inlineQos.length(qos_len + 1);
    data.inlineQos[qos_len].status_info(STATUS_INFO_DISPOSE);
    if (header.publication_id_.entityId.entityKind == BUILT_IN_WRITER) {
      add_key_hash(data.inlineQos, tsce.msg_payload());
    }
    break;
  }
  case DISPOSE_UNREGISTER_INSTANCE: {
    data.smHeader.flags |= FLAG_K_IN_DATA;
    const int qos_len = data.inlineQos.length();
    data.inlineQos.length(qos_len + 1);
    data.inlineQos[qos_len].status_info(STATUS_INFO_DISPOSE_UNREGISTER);
    if (header.publication_id_.entityId.entityKind == BUILT_IN_WRITER) {
      add_key_hash(data.inlineQos, tsce.msg_payload());
    }
    break;
  }
  // update control_message_supported() when adding new cases here
  default:
    ACE_DEBUG((LM_INFO,
               "RtpsSampleHeader::populate_control_submessages(): "
               "Non-sample messages seen, message_id = %d\n",
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
    const int qos_len = data.inlineQos.length();              \
    data.inlineQos.length(qos_len + 1);                       \
    data.inlineQos[qos_len].QOS_NAME(WRITER_QOS.QOS_NAME);    \
  }

void
RtpsSampleHeader::populate_inline_qos(
  const TransportSendListener::InlineQosData& qos_data,
  OpenDDS::RTPS::DataSubmessage& data)
{
  using namespace OpenDDS::RTPS;

  // Always include topic name (per the spec)
  {
    const int qos_len = data.inlineQos.length();
    data.inlineQos.length(qos_len + 1);
    data.inlineQos[qos_len].string_data(qos_data.topic_name.c_str());
    data.inlineQos[qos_len]._d(PID_TOPIC_NAME);
  }

  // Conditionally include other QoS inline when the differ from the
  // default value.
  DDS::PublisherQos default_pub_qos =
    TheServiceParticipant->initial_PublisherQos();
  PROCESS_INLINE_QOS(presentation, default_pub_qos, qos_data.pub_qos);
  PROCESS_INLINE_QOS(partition, default_pub_qos, qos_data.pub_qos);

  DDS::DataWriterQos default_dw_qos =
    TheServiceParticipant->initial_DataWriterQos();
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

#undef PROCESS_INLINE_QOS

}
}
