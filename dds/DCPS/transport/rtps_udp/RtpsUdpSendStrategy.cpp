/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

#include "dds/DCPS/Serializer.h"

#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpSendStrategy::RtpsUdpSendStrategy(RtpsUdpDataLink* link,
                                         const TransportInst_rch& inst,
                                         const GuidPrefix_t& local_prefix)
  : TransportSendStrategy(0, inst,
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>()),
    link_(link),
    override_dest_(0),
    override_single_dest_(0),
    rtps_header_db_(RTPS::RTPSHDR_SZ, ACE_Message_Block::MB_DATA,
                    rtps_header_data_, 0, 0, ACE_Message_Block::DONT_DELETE, 0),
    rtps_header_mb_(&rtps_header_db_, ACE_Message_Block::DONT_DELETE)
{
  rtps_header_.prefix[0] = 'R';
  rtps_header_.prefix[1] = 'T';
  rtps_header_.prefix[2] = 'P';
  rtps_header_.prefix[3] = 'S';
  rtps_header_.version = OpenDDS::RTPS::PROTOCOLVERSION;
  rtps_header_.vendorId = OpenDDS::RTPS::VENDORID_OPENDDS;
  std::memcpy(rtps_header_.guidPrefix, local_prefix,
              sizeof(GuidPrefix_t));
  Serializer writer(&rtps_header_mb_);
  // byte order doesn't matter for the RTPS Header
  writer << rtps_header_;
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  if (override_single_dest_) {
    return send_single_i(iov, n, *override_single_dest_);
  }

  if (override_dest_) {
    return send_multi_i(iov, n, *override_dest_);
  }

  // determine destination address(es) from TransportQueueElement in progress
  TransportQueueElement* elem = current_packet_first_element();
  if (!elem) {
    errno = ENOTCONN;
    return -1;
  }

  const RepoId remote_id = elem->subscription_id();
  OPENDDS_SET(ACE_INET_Addr) addrs;

  if (remote_id != GUID_UNKNOWN) {
    const ACE_INET_Addr remote = link_->get_locator(remote_id);
    if (remote != ACE_INET_Addr()) {
      addrs.insert(remote);
    }
  }

  if (addrs.empty()) {
    link_->get_locators(elem->publication_id(), addrs);
  }

  if (addrs.empty()) {
    errno = ENOTCONN;
    return -1;
  }

  return send_multi_i(iov, n, addrs);
}

RtpsUdpSendStrategy::OverrideToken
RtpsUdpSendStrategy::override_destinations(const ACE_INET_Addr& destination)
{
  override_single_dest_ = &destination;
  return OverrideToken(this);
}

RtpsUdpSendStrategy::OverrideToken
RtpsUdpSendStrategy::override_destinations(const OPENDDS_SET(ACE_INET_Addr)& dest)
{
  override_dest_ = &dest;
  return OverrideToken(this);
}

RtpsUdpSendStrategy::OverrideToken::~OverrideToken()
{
  outer_->override_single_dest_ = 0;
  outer_->override_dest_ = 0;
}

bool
RtpsUdpSendStrategy::marshal_transport_header(ACE_Message_Block* mb)
{
  Serializer writer(mb); // byte order doesn't matter for the RTPS Header
  return writer.write_octet_array(reinterpret_cast<ACE_CDR::Octet*>(rtps_header_data_),
    RTPS::RTPSHDR_SZ);
}

void
RtpsUdpSendStrategy::send_rtps_control(ACE_Message_Block& submessages,
                                       const ACE_INET_Addr& addr)
{
  rtps_header_mb_.cont(&submessages);

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(rtps_header_mb_, iov);
  const ssize_t result = send_single_i(iov, num_blocks, addr);
  if (result < 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }

  rtps_header_mb_.cont(0);
}

void
RtpsUdpSendStrategy::send_rtps_control(ACE_Message_Block& submessages,
                                       const OPENDDS_SET(ACE_INET_Addr)& addrs)
{
  rtps_header_mb_.cont(&submessages);

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(rtps_header_mb_, iov);
  const ssize_t result = send_multi_i(iov, num_blocks, addrs);
  if (result < 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }

  rtps_header_mb_.cont(0);
}

ssize_t
RtpsUdpSendStrategy::send_multi_i(const iovec iov[], int n,
                                  const OPENDDS_SET(ACE_INET_Addr)& addrs)
{
  ssize_t result = -1;
  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = addrs.begin(); iter != addrs.end(); ++iter) {
    const ssize_t result_per_dest = send_single_i(iov, n, *iter);
    if (result_per_dest >= 0) {
      result = result_per_dest;
    }
  }
  return result;
}

ssize_t
RtpsUdpSendStrategy::send_single_i(const iovec iov[], int n,
                                   const ACE_INET_Addr& addr)
{
#ifdef ACE_LACKS_SENDMSG
  char buffer[UDP_MAX_MESSAGE_SIZE];
  char *iter = buffer;
  for (int i = 0; i < n; ++i) {
    if (size_t(iter - buffer + iov[i].iov_len) > UDP_MAX_MESSAGE_SIZE) {
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
                 "message too large at index %d size %d\n", i, iov[i].iov_len));
      return -1;
    }
    std::memcpy(iter, iov[i].iov_base, iov[i].iov_len);
    iter += iov[i].iov_len;
  }
  const ssize_t result = link_->unicast_socket().send(buffer, iter - buffer, addr);
#else
  const ssize_t result = link_->unicast_socket().send(iov, n, addr);
#endif
  if (result < 0) {
    ACE_TCHAR addr_buff[256] = {};
    int err = errno;
    addr.addr_to_string(addr_buff, 256, 0);
    errno = err;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
      "destination %s failed %p\n", addr_buff, ACE_TEXT("send")));
  }
  return result;
}

void
RtpsUdpSendStrategy::add_delayed_notification(TransportQueueElement* element)
{
  if (!link_->add_delayed_notification(element)) {
    TransportSendStrategy::add_delayed_notification(element);
  }
}

RemoveResult
RtpsUdpSendStrategy::do_remove_sample(const RepoId& pub_id,
  const TransportQueueElement::MatchCriteria& criteria,
  void* context)
{
  ACE_Guard<ACE_Thread_Mutex>* guard =
    static_cast<ACE_Guard<ACE_Thread_Mutex>*>(context);
  link_->do_remove_sample(pub_id, criteria, *guard);
  return TransportSendStrategy::do_remove_sample(pub_id, criteria, 0);
}

namespace {
  bool contentsDiffer(const DDS::OctetSeq& lhs, const DDS::OctetSeq& rhs)
  {
    return lhs.length() != rhs.length() ||
      std::memcmp(lhs.get_buffer(), rhs.get_buffer(), rhs.length());
  }
}

void
RtpsUdpSendStrategy::encode_payload(const RepoId& pub_id,
                                    Message_Block_Ptr& payload,
                                    RTPS::SubmessageSeq& /*submessages*/)
{
  const DDS::Security::DatawriterCryptoHandle writer_crypto_handle =
    link_->writer_crypto_handle(pub_id);
  DDS::Security::CryptoTransform_var crypto =
    link_->security_config()->get_crypto_transform();

  if (writer_crypto_handle == DDS::HANDLE_NIL || !crypto) {
    return;
  }

  DDS::OctetSeq encoded, plain, iQos;
  plain.length(payload->total_length());
  ACE_Message_Block* mb(payload.get());
  for (CORBA::ULong i = 0; mb; mb = mb->cont()) {
    std::memcpy(plain.get_buffer() + i, mb->rd_ptr(), mb->length());
    i += mb->length();
  }

  DDS::Security::SecurityException ex = {"", 0, 0};
  if (crypto->encode_serialized_payload(encoded, iQos, plain,
                                        writer_crypto_handle, ex)) {
    // extra_inline_qos not currently used by plugin
    // when supported: find the DATA submessage inside 'submessages'
    // and append the (deserialized) iQos to data's inline qos (set FLAG_Q)
    if (contentsDiffer(encoded, plain)) {
      payload.reset(new ACE_Message_Block(encoded.length()));
      const char* raw = reinterpret_cast<const char*>(encoded.get_buffer());
      payload->copy(raw, encoded.length());
    }
  }
}

namespace {
  DDS::OctetSeq toSeq(Serializer& ser1, CORBA::Octet msgId, CORBA::Octet flags,
                      CORBA::UShort octetsToNextHeader, EntityId_t receiverId,
                      CORBA::ULong u2, EntityId_t senderId, size_t rem)
  {
    const bool shortMsg = (msgId == RTPS::PAD || msgId == RTPS::INFO_TS);
    CORBA::ULong size = 4 +
      ((octetsToNextHeader == 0 && !shortMsg) ? rem : octetsToNextHeader);
    DDS::OctetSeq out(size);
    out.length(size);
    ACE_Message_Block mb(reinterpret_cast<const char*>(out.get_buffer()), size);
    Serializer ser2(&mb, ser1.swap_bytes(), Serializer::ALIGN_CDR);
    ser2 << ACE_OutputCDR::from_octet(msgId);
    ser2 << ACE_OutputCDR::from_octet(flags);
    ser2 << octetsToNextHeader;
    if (msgId == RTPS::DATA || msgId == RTPS::DATA_FRAG) {
      ser2 << u2;
    }
    if (msgId != RTPS::ACKNACK && msgId != RTPS::NACK_FRAG) {
      ser2 << receiverId;
    }
    ser2 << senderId;
    ser1.read_octet_array(reinterpret_cast<CORBA::Octet*>(mb.wr_ptr()),
                          mb.space());
    return out;
  }

  struct Chunk {
    char* start_;
    unsigned int length_;
    DDS::OctetSeq encoded_;
  };
}

ACE_Message_Block*
RtpsUdpSendStrategy::pre_send_packet(const ACE_Message_Block* plain)
{
  using DDS::Security::ParticipantCryptoHandle;
  using DDS::Security::ParticipantCryptoHandleSeq;
  using DDS::Security::DatawriterCryptoHandle;
  using DDS::Security::DatawriterCryptoHandleSeq;
  using DDS::Security::DatareaderCryptoHandle;
  using DDS::Security::DatareaderCryptoHandleSeq;
  using DDS::Security::CryptoTransform_var;

  CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
  const ParticipantCryptoHandle local_pch = link_->local_crypto_handle();
  if (!crypto || local_pch == DDS::HANDLE_NIL) {
    return 0;
  }

  DatawriterCryptoHandleSeq emptyWriterHandles;
  ParticipantCryptoHandleSeq emptyParticipantHandles;
  DDS::Security::SecurityException ex = {"", 0, 0};

  Message_Block_Ptr in(plain->duplicate());
  ACE_Message_Block* current = in.get();
  Serializer ser(current);
  RTPS::Header header;
  bool ok = ser >> header;

  RepoId sender = GUID_UNKNOWN;
  RTPS::assign(sender.guidPrefix, link_->local_prefix());

  RepoId receiver = GUID_UNKNOWN;

  std::vector<Chunk> replacements;

  while (ok && in->total_length()) {
    while (current && !current->length()) current = current->cont();
    char* submessage_start = current->rd_ptr();

    CORBA::Octet msgId, flags;
    if (!(ser >> ACE_InputCDR::to_octet(msgId)) ||
        !(ser >> ACE_InputCDR::to_octet(flags))) {
      ok = false;
      break;
    }

    ser.swap_bytes(ACE_CDR_BYTE_ORDER != (flags & 1 /*FLAG_E*/));
    CORBA::UShort octetsToNextHeader;
    if (!(ser >> octetsToNextHeader)) {
      ok = false;
      break;
    }

    const size_t remaining = in->total_length();
    int read = 0;
    CORBA::ULong u2;

    switch (msgId) {
    case RTPS::INFO_DST: {
      GuidPrefix_t_forany guidPrefix(receiver.guidPrefix);
      if (!(ser >> guidPrefix)) {
        ok = false;
        break;
      }
      read += 3;
      break;
    }
    case RTPS::DATA:
    case RTPS::DATA_FRAG:
      if (!(ser >> u2)) { // extraFlags|octetsToInlineQos
        ok = false;
        break;
      }
      ++read;
      // fall-through
    case RTPS::HEARTBEAT:
    case RTPS::GAP:
    case RTPS::HEARTBEAT_FRAG: {
      if (!(ser >> receiver.entityId)) { // readerId
        ok = false;
        break;
      }
      ++read;
      if (!(ser >> sender.entityId)) { // writerId
        ok = false;
        break;
      }
      ++read;
      DatawriterCryptoHandle sender_dwch = link_->writer_crypto_handle(sender);
      if (sender_dwch == DDS::HANDLE_NIL) {
        ok = false;
        break;
      }

      replacements.resize(replacements.size() + 1);
      Chunk& c = replacements.back();
      DDS::OctetSeq plain(toSeq(ser, msgId, flags, octetsToNextHeader,
                                receiver.entityId, u2,
                                sender.entityId, remaining));
      DatareaderCryptoHandleSeq readerHandles;
      if (std::memcmp(&GUID_UNKNOWN, &receiver, sizeof receiver)) {
        DatareaderCryptoHandle drch = link_->reader_crypto_handle(receiver);
        if (drch != DDS::HANDLE_NIL) {
          readerHandles.length(1);
          readerHandles[0] = drch;
        }
      }
      CORBA::Long idx = 0;
      if (crypto->encode_datawriter_submessage(c.encoded_, plain, sender_dwch,
                                               readerHandles, idx, ex)
          && contentsDiffer(c.encoded_, plain)) {
        c.start_ = submessage_start;
        c.length_ = plain.length();
      } else {
        replacements.pop_back();
        ok = false;
      }
      break;
    }
    case RTPS::ACKNACK:
    case RTPS::NACK_FRAG: {
      if (!(ser >> sender.entityId)) { // readerId
        ok = false;
        break;
      }
      ++read;
      DatareaderCryptoHandle sender_drch = link_->reader_crypto_handle(sender);
      if (sender_drch == DDS::HANDLE_NIL) {
        ok = false;
        break;
      }

      replacements.resize(replacements.size() + 1);
      Chunk& c = replacements.back();
      DDS::OctetSeq plain(toSeq(ser, msgId, flags, octetsToNextHeader,
                                ENTITYID_UNKNOWN, 0,
                                sender.entityId, remaining));
      if (crypto->encode_datareader_submessage(c.encoded_, plain, sender_drch,
                                               emptyWriterHandles, ex)
          && contentsDiffer(c.encoded_, plain)) {
        c.start_ = submessage_start;
        c.length_ = plain.length();
      } else {
        replacements.pop_back();
        ok = false;
      }
      break;
    }
    default:
      break;
    }

    if (!ok || (octetsToNextHeader == 0 && msgId != RTPS::PAD
                && msgId != RTPS::INFO_TS)) {
      break;
    }

    if (octetsToNextHeader > read * 4) {
      if (!ser.skip(octetsToNextHeader - read * 4)) {
        ok = false;
      }
    }
  }

  if (!ok || replacements.empty()) {
    return 0;
  }

  unsigned int out_size = plain->total_length();
  for (size_t i = 0; i < replacements.size(); ++i) {
    out_size += replacements[i].encoded_.length();
    out_size -= replacements[i].length_;
  }

  in.reset(plain->duplicate());
  ACE_Message_Block* cur = in.get();
  Message_Block_Ptr out(new ACE_Message_Block(out_size));
  for (size_t i = 0; i < replacements.size(); ++i) {
    const Chunk& c = replacements[i];
    for (; cur && (c.start_ < cur->rd_ptr() || c.start_ >= cur->wr_ptr());
         cur = cur->cont()) {
      out->copy(cur->rd_ptr(), cur->length());
    }
    if (!cur) return 0;

    const size_t prefix = c.start_ - cur->rd_ptr();
    out->copy(cur->rd_ptr(), prefix);
    cur->rd_ptr(prefix);

    out->copy(reinterpret_cast<const char*>(c.encoded_.get_buffer()),
              c.encoded_.length());
    for (size_t n = c.encoded_.length(); n; cur = cur->cont()) {
      if (cur->length() < n) {
        cur->rd_ptr(n);
        break;
      } else {
        n -= cur->length();
      }
    }
  }

  for (; cur; cur = cur->cont()) {
    out->copy(cur->rd_ptr(), cur->length());
  }

  //DDS-Security: SRTPS encoding
  return ok ? out.release() : 0;
}

void
RtpsUdpSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
