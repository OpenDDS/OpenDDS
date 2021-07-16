/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"

#include <dds/DCPS/LogAddr.h>

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/Logging.h"

#include "dds/DCPS/Serializer.h"

#include "dds/DdsDcpsGuidTypeSupportImpl.h"

#ifdef OPENDDS_SECURITY
#include "dds/DCPS/RTPS/SecurityHelpers.h"
#include <vector>
#endif

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  const Encoding encoding_unaligned_native(Encoding::KIND_UNALIGNED_CDR);
}

RtpsUdpSendStrategy::RtpsUdpSendStrategy(RtpsUdpDataLink* link,
                                         const GuidPrefix_t& local_prefix)
  : TransportSendStrategy(0, link->impl(),
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>()),
    link_(link),
    override_dest_(0),
    override_single_dest_(0),
    max_message_size_(link->config().max_message_size_),
    rtps_header_db_(RTPS::RTPSHDR_SZ, ACE_Message_Block::MB_DATA,
                    rtps_header_data_, 0, 0, ACE_Message_Block::DONT_DELETE, 0),
    rtps_header_mb_(&rtps_header_db_, ACE_Message_Block::DONT_DELETE),
    network_is_unreachable_(false)
{
  std::memcpy(rtps_message_.hdr.prefix, RTPS::PROTOCOL_RTPS, sizeof RTPS::PROTOCOL_RTPS);
  rtps_message_.hdr.version = OpenDDS::RTPS::PROTOCOLVERSION;
  rtps_message_.hdr.vendorId = OpenDDS::RTPS::VENDORID_OPENDDS;
  std::memcpy(rtps_message_.hdr.guidPrefix, local_prefix,
              sizeof(GuidPrefix_t));
  Serializer writer(&rtps_header_mb_, encoding_unaligned_native);
  // byte order doesn't matter for the RTPS Header
  writer << rtps_message_.hdr;
}

namespace {
  bool shouldWarn(int code) {
    return code == EPERM || code == EACCES || code == EINTR || code == ENOBUFS
      || code == ENOMEM || code == EADDRNOTAVAIL || code == ENETUNREACH;
  }
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ssize_t result = send_bytes_i_helper(iov, n);

  if (result == -1 && shouldWarn(errno)) {
    // Make the framework think this was a successful send to avoid
    // putting the send strategy in suspended mode. If reliability
    // is enabled, the data may be resent later.
    ssize_t b = 0;
    for (int i = 0; i < n; ++i) {
      b += iov[i].iov_len;
    }
    result = b;
  }

  return result;
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i_helper(const iovec iov[], int n)
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

  OPENDDS_SET(ACE_INET_Addr) addrs;
  if (elem->subscription_id() != GUID_UNKNOWN) {
    addrs = link_->get_addresses(elem->publication_id(), elem->subscription_id());
  } else {
    addrs = link_->get_addresses(elem->publication_id());
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
  if (transport_debug.log_messages) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, rtps_message_mutex_, false);
    RTPS::log_message("(%P|%t) {transport_debug.log_messages} %C\n", rtps_message_.hdr.guidPrefix, true, rtps_message_);
    rtps_message_.submessages.length(0);
  }

  Serializer writer(mb, encoding_unaligned_native); // byte order doesn't matter for the RTPS Header
  return writer.write_octet_array(reinterpret_cast<ACE_CDR::Octet*>(rtps_header_data_),
    RTPS::RTPSHDR_SZ);
}

namespace {
  struct AMB_Continuation {
    AMB_Continuation(ACE_Thread_Mutex& mutex, ACE_Message_Block& head, ACE_Message_Block& tail)
      : lock_(mutex), head_(head) { head_.cont(&tail); }
    ~AMB_Continuation() { head_.cont(0); }
    ACE_Guard<ACE_Thread_Mutex> lock_;
    ACE_Message_Block& head_;
  };
}

void
RtpsUdpSendStrategy::send_rtps_control(RTPS::Message& message,
                                       ACE_Message_Block& submessages,
                                       const ACE_INET_Addr& addr)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, rtps_message_mutex_);
    message.hdr = rtps_message_.hdr;
  }

  const AMB_Continuation cont(rtps_header_mb_lock_, rtps_header_mb_, submessages);

#ifdef OPENDDS_SECURITY
  const Message_Block_Ptr alternate(pre_send_packet(&rtps_header_mb_));
  if (!alternate) {
    VDBG((LM_DEBUG, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control () - "
          "pre_send_packet returned NULL, dropping.\n"));
    return;
  }
  ACE_Message_Block& use_mb = *alternate;
#else
  ACE_Message_Block& use_mb = rtps_header_mb_;
#endif

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(use_mb, iov);
  const ssize_t result = send_single_i(iov, num_blocks, addr);
  if (result < 0 && !network_is_unreachable_) {
    const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
    ACE_ERROR((prio, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }
}

void
RtpsUdpSendStrategy::send_rtps_control(RTPS::Message& message,
                                       ACE_Message_Block& submessages,
                                       const OPENDDS_SET(ACE_INET_Addr)& addrs)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, rtps_message_mutex_);
    message.hdr = rtps_message_.hdr;
  }

  const AMB_Continuation cont(rtps_header_mb_lock_, rtps_header_mb_, submessages);

#ifdef OPENDDS_SECURITY
  const Message_Block_Ptr alternate(pre_send_packet(&rtps_header_mb_));
  if (!alternate) {
    VDBG((LM_DEBUG, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control () - "
          "pre_send_packet returned NULL, dropping.\n"));
    return;
  }
  ACE_Message_Block& use_mb = *alternate;
#else
  ACE_Message_Block& use_mb = rtps_header_mb_;
#endif

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(use_mb, iov);
  const ssize_t result = send_multi_i(iov, num_blocks, addrs);
  if (result < 0 && !network_is_unreachable_) {
    const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
    ACE_ERROR((prio, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }
}

void
RtpsUdpSendStrategy::append_submessages(const RTPS::SubmessageSeq& submessages)
{
  ACE_GUARD(ACE_Thread_Mutex, g, rtps_message_mutex_);
  for (ACE_CDR::ULong idx = 0; idx != submessages.length(); ++idx) {
    DCPS::push_back(rtps_message_.submessages, submessages[idx]);
  }
}

ssize_t
RtpsUdpSendStrategy::send_multi_i(const iovec iov[], int n,
                                  const OPENDDS_SET(ACE_INET_Addr)& addrs)
{
  ssize_t result = -1;
  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = addrs.begin(); iter != addrs.end(); ++iter) {
    if (*iter == ACE_INET_Addr()) {
      continue;
    }
    const ssize_t result_per_dest = send_single_i(iov, n, *iter);
    if (result_per_dest >= 0) {
      result = result_per_dest;
    }
  }
  return result;
}

const ACE_SOCK_Dgram&
RtpsUdpSendStrategy::choose_send_socket(const ACE_INET_Addr& addr) const
{
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    return link_->ipv6_unicast_socket();
  }
#endif
  ACE_UNUSED_ARG(addr);
  return link_->unicast_socket();
}

ssize_t
RtpsUdpSendStrategy::send_single_i(const iovec iov[], int n,
                                   const ACE_INET_Addr& addr)
{
  OPENDDS_ASSERT(addr != ACE_INET_Addr());

  const ACE_SOCK_Dgram& socket = choose_send_socket(addr);

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
  const ssize_t result = socket.send(buffer, iter - buffer, addr);
#else
  const ssize_t result = socket.send(iov, n, addr);
#endif
  if (result < 0) {
    const int err = errno;
    if (err != ENETUNREACH || !network_is_unreachable_) {
      errno = err;
      const ACE_Log_Priority prio = shouldWarn(errno) ? LM_WARNING : LM_ERROR;
      ACE_ERROR((prio, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
                 "destination %C failed send: %m\n", DCPS::LogAddr(addr).c_str()));
      if (errno == EMSGSIZE) {
        for (int i = 0; i < n; ++i) {
          ACE_ERROR((prio, "(%P|%t) RtpsUdpSendStrategy::send_single_i: "
              "iovec[%d].iov_len = %B\n", i, size_t(iov[i].iov_len)));
        }
      }
    }
    if (err == ENETUNREACH) {
      network_is_unreachable_ = true;
    }
    // Reset errno since the rest of framework expects it.
    errno = err;
  } else {
    network_is_unreachable_ = false;
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

#ifdef OPENDDS_SECURITY
namespace {
  DDS::OctetSeq toSeq(const ACE_Message_Block* mb)
  {
    DDS::OctetSeq out;
    out.length(static_cast<unsigned int>(mb->total_length()));
    unsigned char* const buffer = out.get_buffer();
    for (unsigned int i = 0; mb; mb = mb->cont()) {
      std::memcpy(buffer + i, mb->rd_ptr(), mb->length());
      i += static_cast<unsigned int>(mb->length());
    }
    return out;
  }
}

void
RtpsUdpSendStrategy::encode_payload(const RepoId& pub_id,
                                    Message_Block_Ptr& payload,
                                    RTPS::SubmessageSeq& submessages)
{
  const DDS::Security::DatawriterCryptoHandle writer_crypto_handle =
    link_->handle_registry()->get_local_datawriter_crypto_handle(pub_id);
  DDS::Security::CryptoTransform_var crypto =
    link_->security_config()->get_crypto_transform();

  if (writer_crypto_handle == DDS::HANDLE_NIL || !crypto) {
    return;
  }

  const DDS::OctetSeq plain = toSeq(payload.get());
  DDS::OctetSeq encoded, iQos;
  DDS::Security::SecurityException ex = {"", 0, 0};

  if (crypto->encode_serialized_payload(encoded, iQos, plain, writer_crypto_handle, ex)) {
    if (encoded != plain) {
      payload.reset(new ACE_Message_Block(encoded.length()));
      const char* raw = reinterpret_cast<const char*>(encoded.get_buffer());
      payload->copy(raw, encoded.length());

      // Set FLAG_N flag
      for (CORBA::ULong i = 0; i < submessages.length(); ++i) {
          if (submessages[i]._d() == RTPS::DATA) {
              RTPS::DataSubmessage& data = submessages[i].data_sm();
              data.smHeader.flags |= RTPS::FLAG_N_IN_DATA;
          }
      }
    }

    const CORBA::ULong iQosLen = iQos.length();
    if (iQosLen > 3) {
      for (CORBA::ULong i = 0; i < submessages.length(); ++i) {
        if (submessages[i]._d() == RTPS::DATA) {
          // ParameterList must end in {1, 0, x, x} (LE) or {0, 1, x, x} (BE)
          // Check for this sentinel and use it for endianness detection
          if (iQos[iQosLen - 3] + iQos[iQosLen - 4] != 1) {
            VDBG_LVL((LM_WARNING, "(%P|%t) RtpsUdpSendStrategy::encode_payload "
                      "extra_inline_qos is not a valid ParameterList\n"), 2);
            break;
          }

          const bool swapPl = iQos[iQosLen - 4] != ACE_CDR_BYTE_ORDER;
          const char* rawIQos = reinterpret_cast<const char*>(iQos.get_buffer());
          ACE_Message_Block mbIQos(rawIQos, iQosLen);
          Serializer ser(&mbIQos, Encoding::KIND_XCDR1, swapPl);

          RTPS::DataSubmessage& data = submessages[i].data_sm();
          if (!(ser >> data.inlineQos)) { // appends to any existing inlineQos
            VDBG_LVL((LM_WARNING, "(%P|%t) RtpsUdpSendStrategy::encode_payload "
                      "extra_inline_qos deserialization failed\n"), 2);
            break;
          }
          data.smHeader.flags |= RTPS::FLAG_Q;
          break;
        }
      }
    } else if (iQosLen) {
      VDBG_LVL((LM_WARNING, "(%P|%t) RtpsUdpSendStrategy::encode_payload "
                "extra_inline_qos not enough bytes for ParameterList\n"), 2);
    }
  }
}

ACE_Message_Block*
RtpsUdpSendStrategy::pre_send_packet(const ACE_Message_Block* plain)
{
  const DDS::Security::CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
  if (!crypto) {
    return plain->duplicate();
  }

  bool stateless_or_volatile = false;
  Message_Block_Ptr submessages(encode_submessages(plain, crypto, stateless_or_volatile));

  if (!submessages || stateless_or_volatile || link_->local_crypto_handle() == DDS::HANDLE_NIL) {
    return submessages.release();
  }

  return encode_rtps_message(submessages.get(), crypto);
}

ACE_Message_Block*
RtpsUdpSendStrategy::encode_rtps_message(const ACE_Message_Block* plain, DDS::Security::CryptoTransform* crypto)
{
  using namespace DDS::Security;
  DDS::OctetSeq encoded_rtps_message;
  const DDS::OctetSeq plain_rtps_message = toSeq(plain);
  const ParticipantCryptoHandle send_handle = link_->local_crypto_handle();
  const ParticipantCryptoHandleSeq recv_handles; // unused
  int idx = 0; // unused
  SecurityException ex = {"", 0, 0};
  if (crypto->encode_rtps_message(encoded_rtps_message, plain_rtps_message,
                                  send_handle, recv_handles, idx, ex)) {
    Message_Block_Ptr out(new ACE_Message_Block(encoded_rtps_message.length()));
    const char* raw = reinterpret_cast<const char*>(encoded_rtps_message.get_buffer());
    out->copy(raw, encoded_rtps_message.length());
    return out.release();
  }
  if (ex.code == 0 && ex.minor_code == 0) {
    return plain->duplicate(); // send original pre-encoded msg
  }
  if (Transport_debug_level) {
    ACE_ERROR((LM_ERROR, "RtpsUdpSendStrategy::encode_rtps_message - ERROR "
               "plugin failed to encode RTPS message from handle %d [%d.%d]: %C\n",
               send_handle, ex.code, ex.minor_code, ex.message.in()));
  }
  return 0; // do not send pre-encoded msg
}

namespace {
  DDS::OctetSeq toSeq(Serializer& ser1, RTPS::SubmessageHeader smHdr, CORBA::ULong dataExtra,
                      EntityId_t readerId, EntityId_t writerId, unsigned int remain)
  {
    const int msgId = smHdr.submessageId;
    const unsigned int octetsToNextHeader = smHdr.submessageLength;
    const bool shortMsg = (msgId == RTPS::PAD || msgId == RTPS::INFO_TS);
    const CORBA::ULong size = RTPS::SMHDR_SZ + ((octetsToNextHeader == 0 && !shortMsg) ? remain : octetsToNextHeader);
    DDS::OctetSeq out(size);
    out.length(size);
    ACE_Message_Block mb(reinterpret_cast<const char*>(out.get_buffer()), size);
    Serializer ser2(&mb, ser1.encoding());
    ser2 << ACE_OutputCDR::from_octet(smHdr.submessageId);
    ser2 << ACE_OutputCDR::from_octet(smHdr.flags);
    ser2 << smHdr.submessageLength;
    if (msgId == RTPS::DATA || msgId == RTPS::DATA_FRAG) {
      ser2 << dataExtra;
    }
    ser2 << readerId;
    ser2 << writerId;
    ser1.read_octet_array(reinterpret_cast<CORBA::Octet*>(mb.wr_ptr()),
                          static_cast<unsigned int>(mb.space()));
    return out;
  }

  void log_encode_error(CORBA::Octet msgId,
                        DDS::Security::NativeCryptoHandle sender,
                        const DDS::Security::SecurityException& ex)
  {
    if (Transport_debug_level) {
      ACE_ERROR((LM_ERROR, "RtpsUdpSendStrategy::pre_send_packet - ERROR "
                 "plugin failed to encode submessage 0x%x from handle %d "
                 "[%d.%d]: %C\n", msgId, sender, ex.code, ex.minor_code,
                 ex.message.in()));
    }
  }

  void check_stateless_volatile(EntityId_t writerId, bool& stateless_or_volatile)
  {
    stateless_or_volatile |=
      writerId == RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER ||
      writerId == RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
  }
}

bool
RtpsUdpSendStrategy::encode_writer_submessage(const RepoId& receiver,
                                              OPENDDS_VECTOR(Chunk)& replacements,
                                              DDS::Security::CryptoTransform* crypto,
                                              const DDS::OctetSeq& plain,
                                              DDS::Security::DatawriterCryptoHandle sender_dwch,
                                              const char* submessage_start,
                                              CORBA::Octet msgId)
{
  using namespace DDS::Security;

  if (sender_dwch == DDS::HANDLE_NIL) {
    return true;
  }

  DatareaderCryptoHandleSeq readerHandles;
  if (std::memcmp(&GUID_UNKNOWN, &receiver, sizeof receiver)) {
    DatareaderCryptoHandle drch =
      link_->handle_registry()->get_remote_datareader_crypto_handle(receiver);
    if (drch != DDS::HANDLE_NIL) {
      readerHandles.length(1);
      readerHandles[0] = drch;
    }
  }

  CORBA::Long idx = 0;
  SecurityException ex = {"", 0, 0};
  replacements.resize(replacements.size() + 1);
  Chunk& c = replacements.back();
  if (crypto->encode_datawriter_submessage(c.encoded_, plain, sender_dwch, readerHandles, idx, ex)) {
    if (c.encoded_ != plain) {
      c.start_ = submessage_start;
      c.length_ = plain.length();
    } else {
      replacements.pop_back();
    }
  } else {
    log_encode_error(msgId, sender_dwch, ex);
    replacements.pop_back();
    return false;
  }
  return true;
}

bool
RtpsUdpSendStrategy::encode_reader_submessage(const RepoId& receiver,
                                              OPENDDS_VECTOR(Chunk)& replacements,
                                              DDS::Security::CryptoTransform* crypto,
                                              const DDS::OctetSeq& plain,
                                              DDS::Security::DatareaderCryptoHandle sender_drch,
                                              const char* submessage_start,
                                              CORBA::Octet msgId)
{
  using namespace DDS::Security;

  if (sender_drch == DDS::HANDLE_NIL) {
    return true;
  }

  DatawriterCryptoHandleSeq writerHandles;
  if (std::memcmp(&GUID_UNKNOWN, &receiver, sizeof receiver)) {
    DatawriterCryptoHandle dwch = link_->handle_registry()->get_remote_datawriter_crypto_handle(receiver);
    if (dwch != DDS::HANDLE_NIL) {
      writerHandles.length(1);
      writerHandles[0] = dwch;
    }
  }

  SecurityException ex = {"", 0, 0};
  replacements.resize(replacements.size() + 1);
  Chunk& c = replacements.back();
  if (crypto->encode_datareader_submessage(c.encoded_, plain, sender_drch, writerHandles, ex)) {
    if (c.encoded_ != plain) {
      c.start_ = submessage_start;
      c.length_ = plain.length();
    } else {
      replacements.pop_back();
    }
  } else {
    log_encode_error(msgId, sender_drch, ex);
    replacements.pop_back();
    return false;
  }
  return true;
}

ACE_Message_Block*
RtpsUdpSendStrategy::encode_submessages(const ACE_Message_Block* plain,
                                        DDS::Security::CryptoTransform* crypto,
                                        bool& stateless_or_volatile)
{
  // 'plain' contains a full RTPS Message on its way to the socket(s).
  // Let the crypto plugin examine each submessage and replace it with an
  // encoded version.  First, parse through the message using the 'plain'
  // message block chain.  Instead of changing the messsage in place,
  // modifications are stored in the 'replacements' which will end up
  // changing the message when the 'out' message block is created in the
  // helper method replace_chunks().
  RTPS::MessageParser parser(*plain);
  bool ok = parser.parseHeader();

  RepoId sender = GUID_UNKNOWN;
  assign(sender.guidPrefix, link_->local_prefix());

  RepoId receiver = GUID_UNKNOWN;

  OPENDDS_VECTOR(Chunk) replacements;

  while (ok && parser.remaining()) {

    const char* const submessage_start = parser.current();

    if (!parser.parseSubmessageHeader()) {
      ok = false;
      break;
    }

    const unsigned int remaining = static_cast<unsigned int>(parser.remaining());
    const RTPS::SubmessageHeader smhdr = parser.submessageHeader();

    CORBA::ULong dataExtra = 0;

    switch (smhdr.submessageId) {
    case RTPS::INFO_DST: {
      GuidPrefix_t_forany guidPrefix(receiver.guidPrefix);
      if (!(parser >> guidPrefix)) {
        ok = false;
        break;
      }
      break;
    }
    case RTPS::DATA:
    case RTPS::DATA_FRAG:
      if (!(parser >> dataExtra)) { // extraFlags|octetsToInlineQos
        ok = false;
        break;
      }
      // fall-through
    case RTPS::HEARTBEAT:
    case RTPS::GAP:
    case RTPS::HEARTBEAT_FRAG: {
      if (!(parser >> receiver.entityId)) { // readerId
        ok = false;
        break;
      }
      if (!(parser >> sender.entityId)) { // writerId
        ok = false;
        break;
      }

      check_stateless_volatile(sender.entityId, stateless_or_volatile);
      DDS::OctetSeq plainSm(toSeq(parser.serializer(), smhdr, dataExtra, receiver.entityId, sender.entityId, remaining));
      if (!encode_writer_submessage(receiver, replacements, crypto, plainSm,
                                    link_->handle_registry()->get_local_datawriter_crypto_handle(sender), submessage_start, smhdr.submessageId)) {
        ok = false;
      }
      break;
    }
    case RTPS::ACKNACK:
    case RTPS::NACK_FRAG: {
      if (!(parser >> sender.entityId)) { // readerId
        ok = false;
        break;
      }
      if (!(parser >> receiver.entityId)) { // writerId
        ok = false;
        break;
      }

      check_stateless_volatile(receiver.entityId, stateless_or_volatile);
      DDS::OctetSeq plainSm(toSeq(parser.serializer(), smhdr, 0, sender.entityId, receiver.entityId, remaining));
      if (!encode_reader_submessage(receiver, replacements, crypto, plainSm,
                                    link_->handle_registry()->get_local_datareader_crypto_handle(sender), submessage_start, smhdr.submessageId)) {
        ok = false;
      }
      break;
    }
    default:
      break;
    }

    if (!ok || !parser.hasNextSubmessage()) {
      break;
    }

    if (!parser.skipToNextSubmessage()) {
      ok = false;
    }
  }

  if (!ok) {
    return 0;
  }

  if (replacements.empty()) {
    return plain->duplicate();
  }

  return replace_chunks(plain, replacements);
}

ACE_Message_Block*
RtpsUdpSendStrategy::replace_chunks(const ACE_Message_Block* plain,
                                    const OPENDDS_VECTOR(Chunk)& replacements)
{
  unsigned int out_size = static_cast<unsigned int>(plain->total_length());
  for (size_t i = 0; i < replacements.size(); ++i) {
    out_size += replacements[i].encoded_.length();
    out_size -= replacements[i].length_;
  }

  Message_Block_Ptr in(plain->duplicate());
  ACE_Message_Block* cur = in.get();
  Message_Block_Ptr out(new ACE_Message_Block(out_size));
  for (size_t i = 0; i < replacements.size(); ++i) {
    const Chunk& c = replacements[i];
    for (; cur && (c.start_ < cur->rd_ptr() || c.start_ >= cur->wr_ptr());
         cur = cur->cont()) {
      out->copy(cur->rd_ptr(), cur->length());
    }
    if (!cur) {
      return 0;
    }

    const size_t prefix = c.start_ - cur->rd_ptr();
    out->copy(cur->rd_ptr(), prefix);
    cur->rd_ptr(prefix);

    out->copy(reinterpret_cast<const char*>(c.encoded_.get_buffer()), c.encoded_.length());
    for (size_t n = c.length_; n; cur = cur->cont()) {
      if (cur->length() > n) {
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

  return out.release();
}
#endif

void
RtpsUdpSendStrategy::stop_i()
{
}

size_t RtpsUdpSendStrategy::max_message_size() const
{
  // TODO: Make this conditional on if the message actually needs to do this.
  return max_message_size_
#ifdef OPENDDS_SECURITY
    // Worst case scenario is full message encryption plus one submessage encryption.
    - MaxSecureSubmessageAdditionalSize - MaxSecureFullMessageAdditionalSize
#endif
    ;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
