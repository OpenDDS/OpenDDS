/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsSampleHeader.h"
#include "RtpsUdpSendStrategy.h"

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DisjointSequence.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"

#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"

#include <cstring>

#ifndef __ACE_INLINE__
#include "RtpsSampleHeader.inl"
#endif

namespace {
  const OpenDDS::RTPS::StatusInfo_t STATUS_INFO_REGISTER = { { 0, 0, 0, 0 } },
    STATUS_INFO_DISPOSE = { { 0, 0, 0, 1 } },
    STATUS_INFO_UNREGISTER = { { 0, 0, 0, 2 } },
    STATUS_INFO_DISPOSE_UNREGISTER = { { 0, 0, 0, 3 } };
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

#ifndef OPENDDS_SAFETY_PROFILE
inline bool
operator==(const StatusInfo_t& lhs, const StatusInfo_t& rhs)
{
  return
    (lhs.value[3] == rhs.value[3]) &&
    (lhs.value[2] == rhs.value[2]) &&
    (lhs.value[1] == rhs.value[1]) &&
    (lhs.value[0] == rhs.value[0]);
}
#endif

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

#if defined(OPENDDS_SECURITY)
    // Each submessage type introduced by the Security spec is treated
    // as an opaque octet sequence at this layer.
    case SEC_BODY:
    case SEC_PREFIX:
    case SEC_POSTFIX:
    case SRTPS_PREFIX:
    case SRTPS_POSTFIX: {
    SecuritySubmessage submessage;
    if (ser >> submessage) {
      octetsToNextHeader = submessage.smHeader.submessageLength;
      submessage_.security_sm(submessage);
      submessage_._d(kind);
      valid_ = true;
    }
    break;
    }
#endif

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
  OPENDDS_STRING output("into_received_data_sample(): ");
  output += to_dds_string(iqos.length());
  output += " inline QoS parameters\n";
  for (CORBA::ULong index = 0; index < iqos.length(); ++index) {
    output += "  parameter type = ";
    output += to_dds_string(iqos[index]._d());
    output += "\n";
  }
  ACE_DEBUG((LM_DEBUG, "%C", output.c_str()));
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
            rds.sample_.reset(new ACE_Message_Block(20));
            // CDR_BE encapsuation scheme (endianness is not used for key hash)
            rds.sample_->copy("\x00\x00\x00\x00", 4);
            const CORBA::Octet* data = rtps.inlineQos[i].key_hash().value;
            rds.sample_->copy(reinterpret_cast<const char*>(data), 16);
            opendds.message_length_ =
              static_cast<ACE_UINT32>(rds.sample_->length());
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
      opendds.byte_order_ = payload_byte_order(rds);
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
    // opendds.byte_order_ set in RtpsUdpReceiveStrategy::reassemble().

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

bool RtpsSampleHeader::payload_byte_order(const ReceivedDataSample& rds)
{
  return rds.sample_->rd_ptr()[1] & RTPS::FLAG_E;
}

namespace {
  void add_timestamp(RTPS::SubmessageSeq& subm, ACE_CDR::Octet flags,
    const DataSampleHeader& header)
  {
    using namespace OpenDDS::RTPS;
    const DDS::Time_t st = {header.source_timestamp_sec_,
                            header.source_timestamp_nanosec_};
    const InfoTimestampSubmessage ts = {
      {INFO_TS, flags, INFO_TS_SZ},
      {static_cast<ACE_UINT32>(st.sec), DCPS::nanoseconds_to_uint32_fractional_seconds(st.nanosec)}
    };
    const CORBA::ULong i = subm.length();
    subm.length(i + 1);
    subm[i].info_ts_sm(ts);
  }
}

void
RtpsSampleHeader::populate_data_sample_submessages(
  RTPS::SubmessageSeq& subm,
  const DataSampleElement& dsle,
  bool requires_inline_qos)
{
  using namespace OpenDDS::RTPS;

  const ACE_CDR::Octet flags = dsle.get_header().byte_order_;
  add_timestamp(subm, flags, dsle.get_header());
  CORBA::ULong i = subm.length();

  EntityId_t readerId = ENTITYID_UNKNOWN;
  if (dsle.get_num_subs() == 1) {
    readerId = dsle.get_sub_id(0).entityId;
    InfoDestinationSubmessage idest;
    idest.smHeader.submessageId = INFO_DST;
    idest.smHeader.flags = flags;
    idest.smHeader.submessageLength = INFO_DST_SZ;
    std::memcpy(idest.guidPrefix, dsle.get_sub_id(0).guidPrefix,
                sizeof(GuidPrefix_t));
    subm.length(i + 1);
    subm[i++].info_dst_sm(idest);
  } else {
    //Not durability resend, but could have inline gaps
    for (CORBA::ULong x = 0; x < i; ++x) {
      if (subm[x]._d() == INFO_DST) {
        //Need to add INFO_DST
        InfoDestinationSubmessage idest;
        idest.smHeader.submessageId = INFO_DST;
        idest.smHeader.flags = flags;
        idest.smHeader.submessageLength = INFO_DST_SZ;
        std::memcpy(idest.guidPrefix, GUIDPREFIX_UNKNOWN,
                    sizeof(GuidPrefix_t));
        subm.length(i + 1);
        subm[i++].info_dst_sm(idest);
        break;
      }
    }
  }

  DataSubmessage data = {
    {DATA, flags, 0},
    0,
    DATA_OCTETS_TO_IQOS,
    readerId,
    dsle.get_pub_id().entityId,
    {dsle.get_header().sequence_.getHigh(), dsle.get_header().sequence_.getLow()},
    ParameterList()
  };
  const char message_id = dsle.get_header().message_id_;
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
    dsle.get_send_listener()->retrieve_inline_qos_data(qos_data);

    populate_inline_qos(qos_data, data.inlineQos);
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
  const ACE_CDR::Octet flags = header.byte_order_;
  add_timestamp(subm, flags, header);
  CORBA::ULong i = subm.length();

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
               "RtpsSampleHeader::populate_data_control_submessages(): "
               "Non-sample messages seen, message_id = %d\n",
               header.message_id_));
    break;
  }

  if (requires_inline_qos) {
    TransportSendListener::InlineQosData qos_data;
    tsce.listener()->retrieve_inline_qos_data(qos_data);

    populate_inline_qos(qos_data, data.inlineQos);
  }

  if (data.inlineQos.length() > 0) {
    data.smHeader.flags |= FLAG_Q;
  }

  subm.length(i + 1);
  subm[i].data_sm(data);
}

#define PROCESS_INLINE_QOS(QOS_NAME, DEFAULT_QOS, WRITER_QOS) \
  if (WRITER_QOS.QOS_NAME != DEFAULT_QOS.QOS_NAME) {          \
    const int qos_len = plist.length();                       \
    plist.length(qos_len + 1);                                \
    plist[qos_len].QOS_NAME(WRITER_QOS.QOS_NAME);             \
  }

void
RtpsSampleHeader::populate_inline_qos(
  const TransportSendListener::InlineQosData& qos_data,
  RTPS::ParameterList& plist)
{
  using namespace OpenDDS::RTPS;

  // Always include topic name (per the spec)
  {
    const int qos_len = plist.length();
    plist.length(qos_len + 1);
    plist[qos_len].string_data(qos_data.topic_name.c_str());
    plist[qos_len]._d(PID_TOPIC_NAME);
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
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  PROCESS_INLINE_QOS(ownership_strength, default_dw_qos, qos_data.dw_qos);
#endif
  PROCESS_INLINE_QOS(liveliness, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(reliability, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(transport_priority, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(lifespan, default_dw_qos, qos_data.dw_qos);
  PROCESS_INLINE_QOS(destination_order, default_dw_qos, qos_data.dw_qos);
}

#undef PROCESS_INLINE_QOS

// simple marshaling helpers for RtpsSampleHeader::split()
namespace {
  void write(const Message_Block_Ptr& mb, ACE_CDR::UShort s, bool swap_bytes)
  {
    const char* ps = reinterpret_cast<const char*>(&s);
    if (swap_bytes) {
      ACE_CDR::swap_2(ps, mb->wr_ptr());
      mb->wr_ptr(2);
    } else {
      mb->copy(ps, 2);
    }
  }

  void write(const Message_Block_Ptr& mb, ACE_CDR::ULong i, bool swap_bytes)
  {
    const char* pi = reinterpret_cast<const char*>(&i);
    if (swap_bytes) {
      ACE_CDR::swap_4(pi, mb->wr_ptr());
      mb->wr_ptr(4);
    } else {
      mb->copy(pi, 4);
    }
  }

  // read without advancing rd_ptr()
  void peek(ACE_CDR::UShort& target, const char* src, bool swap_bytes)
  {
    if (swap_bytes) {
      ACE_CDR::swap_2(src, reinterpret_cast<char*>(&target));
    } else {
      std::memcpy(&target, src, sizeof(ACE_CDR::UShort));
    }
  }

  void peek(ACE_CDR::ULong& target, const char* src, bool swap_bytes)
  {
    if (swap_bytes) {
      ACE_CDR::swap_4(src, reinterpret_cast<char*>(&target));
    } else {
      std::memcpy(&target, src, sizeof(ACE_CDR::ULong));
    }
  }

  const size_t FRAG_START_OFFSET = 24, FRAG_SAMPLE_SIZE_OFFSET = 32;
}

SequenceRange
RtpsSampleHeader::split(const ACE_Message_Block& orig, size_t size,
                        Message_Block_Ptr& head, Message_Block_Ptr& tail)
{
  using namespace RTPS;
  static const SequenceRange unknown_range(SequenceNumber::SEQUENCENUMBER_UNKNOWN(),
                                           SequenceNumber::SEQUENCENUMBER_UNKNOWN());
  size_t data_offset = 0;
  const char* rd = orig.rd_ptr();
  ACE_CDR::ULong starting_frag, sample_size;
  ACE_CDR::Octet flags;
  bool swap_bytes;

  // Find the start of the DATA | DATA_FRAG submessage in the orig msg block.
  // The submessages from the start of the msg block to this point (data_offset)
  // will be copied to both the head and tail fragments.
  while (true) {
    flags = rd[data_offset + 1];
    swap_bytes = ACE_CDR_BYTE_ORDER != bool(flags & FLAG_E);
    bool found_data = false;

    switch (rd[data_offset]) {
    case DATA:
      if ((flags & (FLAG_D | FLAG_K_IN_DATA)) == 0) {
        if (Transport_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) RtpsSampleHeader::split() ERROR - "
            "attempting to fragment a Data submessage with no payload.\n"));
        }
        return unknown_range;
      }
      found_data = true;
      starting_frag = 1;
      sample_size = static_cast<ACE_CDR::ULong>(orig.cont()->total_length());
      break;
    case DATA_FRAG:
      found_data = true;
      peek(starting_frag, rd + data_offset + FRAG_START_OFFSET, swap_bytes);
      peek(sample_size, rd + data_offset + FRAG_SAMPLE_SIZE_OFFSET, swap_bytes);
      break;
    }

    if (found_data) {
      break;
    }

    // Scan for next submessage in orig
    ACE_CDR::UShort octetsToNextHeader;
    peek(octetsToNextHeader, rd + data_offset + 2, swap_bytes);

    data_offset += octetsToNextHeader + SMHDR_SZ;
    if (data_offset >= orig.length()) {
      if (Transport_debug_level) {
        ACE_ERROR((LM_ERROR, "(%P|%t) RtpsSampleHeader::split() ERROR - "
          "invalid octetsToNextHeader encountered while fragmenting.\n"));
      }
      return unknown_range;
    }
  }

  // Create the "head" message block (of size "sz") containing DATA_FRAG
  size_t sz = orig.length();
  ACE_CDR::Octet new_flags = flags;
  size_t iqos_offset = data_offset + 8 + DATA_FRAG_OCTETS_TO_IQOS;
  if (rd[data_offset] == DATA) {
    sz += 12; // DATA_FRAG is 12 bytes larger than DATA
    iqos_offset -= 12;
    new_flags = flags & (FLAG_E | FLAG_Q);
    if (flags & FLAG_K_IN_DATA) {
      new_flags |= FLAG_K_IN_FRAG;
    }
    if (flags & FLAG_N_IN_DATA) {
      new_flags |= FLAG_N_IN_FRAG;
    }

  }
  head.reset(DataSampleHeader::alloc_msgblock(orig, sz, false));

  head->copy(rd, data_offset);

  head->wr_ptr()[0] = DATA_FRAG;
  head->wr_ptr()[1] = new_flags;
  head->wr_ptr(2);

  std::memset(head->wr_ptr(), 0, 4); // octetsToNextHeader, extraFlags
  head->wr_ptr(4);

  write(head, DATA_FRAG_OCTETS_TO_IQOS, swap_bytes);

  head->copy(rd + data_offset + 8, 16); // readerId, writerId, sequenceNum

  write(head, starting_frag, swap_bytes);
  const size_t max_data = size - sz, orig_payload = orig.cont()->total_length();
  const ACE_CDR::UShort frags =
    static_cast<ACE_CDR::UShort>(std::min(max_data, orig_payload) / FRAG_SIZE);
  write(head, frags, swap_bytes);
  write(head, FRAG_SIZE, swap_bytes);
  write(head, sample_size, swap_bytes);

  if (flags & FLAG_Q) {
    head->copy(rd + iqos_offset, orig.length() - iqos_offset);
  }

  // Create the "tail" message block containing DATA_FRAG with Q=0
  tail.reset(DataSampleHeader::alloc_msgblock(orig, data_offset + 36, false));

  tail->copy(rd, data_offset);

  tail->wr_ptr()[0] = DATA_FRAG;
  tail->wr_ptr()[1] = new_flags & ~FLAG_Q;
  tail->wr_ptr(2);

  std::memset(tail->wr_ptr(), 0, 4); // octetsToNextHeader, extraFlags
  tail->wr_ptr(4);

  write(tail, DATA_FRAG_OCTETS_TO_IQOS, swap_bytes);
  tail->copy(rd + data_offset + 8, 16); // readerId, writerId, sequenceNum

  write(tail, starting_frag + frags, swap_bytes);
  const size_t tail_data = orig_payload - frags * FRAG_SIZE;
  const ACE_CDR::UShort tail_frags =
    static_cast<ACE_CDR::UShort>((tail_data + FRAG_SIZE - 1) / FRAG_SIZE);
  write(tail, tail_frags, swap_bytes);
  write(tail, FRAG_SIZE, swap_bytes);
  write(tail, sample_size, swap_bytes);

  Message_Block_Ptr payload_head;
  Message_Block_Ptr payload_tail;
  DataSampleHeader::split_payload(*orig.cont(), frags * FRAG_SIZE,
                                  payload_head, payload_tail);
  head->cont(payload_head.release());
  tail->cont(payload_tail.release());

  return SequenceRange(starting_frag + frags - 1,
                       starting_frag + frags + tail_frags - 1);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
