/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpTransport.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Util.h"

#include "ace/Reactor.h"

#include <algorithm>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpReceiveStrategy::RtpsUdpReceiveStrategy(RtpsUdpDataLink* link, const GuidPrefix_t& local_prefix)
  : link_(link)
  , last_received_()
  , recvd_sample_(0)
  , receiver_(local_prefix)
#ifdef OPENDDS_SECURITY
  , secure_sample_(0)
  , encoded_rtps_(false)
  , encoded_submsg_(false)
#endif
{
#ifdef OPENDDS_SECURITY
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
#endif
}

int
RtpsUdpReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  return handle_dds_input(fd);
}

ssize_t
RtpsUdpReceiveStrategy::receive_bytes_helper(iovec iov[],
                                             int n,
                                             const ACE_SOCK_Dgram& socket,
                                             ACE_INET_Addr& remote_address,
                                             ICE::Endpoint* endpoint,
                                             bool& stop)
{
  ACE_INET_Addr local_address;
  const ssize_t ret = socket.recv(iov, n, remote_address, 0
#ifdef ACE_RECVPKTINFO
                                  , &local_address
#endif
  );

  if (ret == -1) {
    return ret;
  }

#ifdef OPENDDS_SECURITY
  if (endpoint && n > 0 && ret > 0 && iov[0].iov_len >= 4 && memcmp(iov[0].iov_base, "RTPS", 4) != 0) {
# ifndef ACE_RECVPKTINFO
    ACE_ERROR((LM_ERROR, "ERROR: RtpsUdpReceiveStrategy::receive_bytes_helper potential STUN message "
               "received but this version of the ACE library doesn't support the local_address "
               "extension in ACE_SOCK_Dgram::recv\n"));
    ACE_UNUSED_ARG(stop);
    ACE_NOTSUP_RETURN(-1);
# else
    // Assume STUN
    stop = true;
    size_t bytes = ret;
    size_t block_size = std::min(bytes, static_cast<size_t>(iov[0].iov_len));
    ACE_Message_Block* head = new ACE_Message_Block(static_cast<const char*>(iov[0].iov_base), block_size);
    head->length(block_size);
    bytes -= block_size;

    ACE_Message_Block* tail = head;
    for (int i = 1; i < n && bytes != 0; ++i) {
      block_size = std::min(bytes, static_cast<size_t>(iov[i].iov_len));
      ACE_Message_Block* mb = new ACE_Message_Block(static_cast<const char*>(iov[i].iov_base), block_size);
      mb->length(block_size);
      tail->cont(mb);
      tail = mb;
      bytes -= block_size;
    }

    DCPS::Serializer serializer(head, DCPS::Serializer::SWAP_BE);
    STUN::Message message;
    message.block = head;
    if (serializer >> message) {
      ICE::Agent::instance()->receive(endpoint, local_address, remote_address, message);
    }
    head->release();
# endif
  }
#else
  ACE_UNUSED_ARG(endpoint);
  ACE_UNUSED_ARG(stop);
#endif

  return ret;
}

#ifdef OPENDDS_SECURITY
namespace {
  ssize_t recv_err(const char* msg, const ACE_INET_Addr& remote, bool& stop)
  {
    if (security_debug.encdec_error) {
      ACE_TCHAR addr_buff[256] = {};
      remote.addr_to_string(addr_buff, 256);
      ACE_ERROR((LM_ERROR, "(%P|%t) {encdec_error} RtpsUdpReceiveStrategy::receive_bytes - "
                 "from %s secure RTPS processing failed: %C\n", addr_buff, msg));
    }
    stop = true;
    return 0;
  }
}
#endif

ssize_t
RtpsUdpReceiveStrategy::receive_bytes(iovec iov[],
                                      int n,
                                      ACE_INET_Addr& remote_address,
                                      ACE_HANDLE fd,
                                      bool& stop)
{
  const ACE_SOCK_Dgram& socket =
    (fd == link_->unicast_socket().get_handle())
    ? link_->unicast_socket() : link_->multicast_socket();
#ifdef ACE_LACKS_SENDMSG
  ACE_UNUSED_ARG(stop);
  char buffer[0x10000];
  ssize_t scatter = socket.recv(buffer, sizeof buffer, remote_address);
  char* iter = buffer;
  for (int i = 0; scatter > 0 && i < n; ++i) {
    const size_t chunk = std::min(static_cast<size_t>(iov[i].iov_len), // int on LynxOS
                                  static_cast<size_t>(scatter));
    std::memcpy(iov[i].iov_base, iter, chunk);
    scatter -= chunk;
    iter += chunk;
  }
  const ssize_t ret = (scatter < 0) ? scatter : (iter - buffer);
#else
  const ssize_t ret = receive_bytes_helper(iov, n, socket, remote_address, link_->get_ice_endpoint(), stop);
#endif
  remote_address_ = remote_address;

#ifdef OPENDDS_SECURITY
  if (stop) {
    return ret;
  }

  using namespace DDS::Security;
  const ParticipantCryptoHandle receiver = link_->local_crypto_handle();
  if (ret > 0 && receiver != DDS::HANDLE_NIL) {
    encoded_rtps_ = false;

    const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
    if (!crypto) {
      return recv_err("no crypto plugin", remote_address, stop);
    }

    if (ret < RTPS::RTPSHDR_SZ + RTPS::SMHDR_SZ) {
      return recv_err("message too short", remote_address, stop);
    }

    const unsigned int encLen = static_cast<unsigned int>(ret);
    DDS::OctetSeq encoded(encLen);
    encoded.length(encLen);
    unsigned char* const encBuf = encoded.get_buffer();
    size_t copied = 0;
    for (int i = 0; i < n && copied < encLen; ++i) {
      const size_t chunk = std::min(static_cast<size_t>(iov[i].iov_len),
                                    static_cast<size_t>(encLen - copied));
      std::memcpy(encBuf + copied, iov[i].iov_base, chunk);
      copied += chunk;
    }

    if (copied != encLen) {
      return recv_err("received bytes didn't fit in iovec array", remote_address, stop);
    }

    if (encoded[RTPS::RTPSHDR_SZ] != RTPS::SRTPS_PREFIX) {
      return ret;
    }

    GUID_t peer;
    static const int GuidPrefixOffset = 8; // "RTPS", Version(2), Vendor(2)
    std::memcpy(peer.guidPrefix, encBuf + GuidPrefixOffset, sizeof peer.guidPrefix);
    peer.entityId = RTPS::ENTITYID_PARTICIPANT;
    const ParticipantCryptoHandle sender = link_->peer_crypto_handle(peer);
    if (sender == DDS::HANDLE_NIL) {
      if (security_debug.encdec_warn) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::receive_bytes: ")
          ACE_TEXT("decode_rtps_message no remote participant crypto handle, dropping\n")));
      }
      stop = true;
      return ret;
    }

    DDS::OctetSeq plain;
    SecurityException ex = {"", 0, 0};
    if (!crypto->decode_rtps_message(plain, encoded, receiver, sender, ex)) {
      if (security_debug.encdec_warn) {
        ACE_ERROR((LM_WARNING, "(%P|%t) {encdec_warn} decode_rtps_message SecurityException [%d.%d]: %C\n",
                   ex.code, ex.minor_code, ex.message.in()));
      }
      if (ex.code == OPENDDS_EXCEPTION_CODE_NO_KEY && ex.minor_code == OPENDDS_EXCEPTION_MINOR_CODE_NO_KEY) {
        if (security_debug.encdec_warn) {
          ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::receive_bytes: ")
            ACE_TEXT("decode_rtps_message remote participant has crypto handle but no key, dropping\n")));
        }
        stop = true;
        return ret;
      }
      return recv_err("decode_rtps_message failed", remote_address, stop);
    }

    copied = 0;
    const size_t plainLen = plain.length();
    const unsigned char* const plainBuf = plain.get_buffer();
    for (int i = 0; i < n && copied < plainLen; ++i) {
      const size_t chunk = std::min(static_cast<size_t>(iov[i].iov_len),
                                    plainLen - copied);
      std::memcpy(iov[i].iov_base, plainBuf + copied, chunk);
      copied += chunk;
    }

    if (copied != plainLen) {
      return recv_err("plaintext doesn't fit in iovec array", remote_address, stop);
    }

    encoded_rtps_ = true;
    return plainLen;
  }
#endif

  return ret;
}

bool RtpsUdpReceiveStrategy::check_encoded(const EntityId_t& sender)
{
#ifdef OPENDDS_SECURITY
  using namespace DDS::Security;
  GUID_t sendGuid;
  std::memcpy(sendGuid.guidPrefix, receiver_.source_guid_prefix_, sizeof sendGuid.guidPrefix);
  sendGuid.entityId = sender;

  if (link_->local_crypto_handle() != DDS::HANDLE_NIL
      && !encoded_rtps_ && !RtpsUdpDataLink::separate_message(sender)) {
    if (security_debug.encdec_warn) {
      const GuidConverter conv(sendGuid);
      ACE_ERROR((LM_WARNING, "(%P|%t) RtpsUdpReceiveStrategy::check_encoded "
                 "Full message from %C requires protection, dropping\n",
                 OPENDDS_STRING(conv).c_str()));
    }
    return false;
  }

  const EndpointSecurityAttributesMask esa = link_->security_attributes(sendGuid);
  static const EndpointSecurityAttributesMask MASK_PROTECT_SUBMSG =
    ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID | ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED;

  if ((esa & MASK_PROTECT_SUBMSG) == MASK_PROTECT_SUBMSG && !encoded_submsg_) {
    if (security_debug.encdec_warn) {
      const GuidConverter conv(sendGuid);
      ACE_ERROR((LM_WARNING, "(%P|%t) RtpsUdpReceiveStrategy::check_encoded "
                 "Submessage from %C requires protection, dropping\n",
                 OPENDDS_STRING(conv).c_str()));
    }
    return false;
  }
#else
  ACE_UNUSED_ARG(sender);
#endif
  return true;
}

void
RtpsUdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                       const ACE_INET_Addr& /*remote_address*/)
{
  using namespace RTPS;

  if (std::memcmp(receiver_.dest_guid_prefix_, link_->local_prefix(),
                  sizeof(GuidPrefix_t))) {
    // Not our message, we may be on multicast listening to all the others.
    return;
  }

  const RtpsSampleHeader& rsh = received_sample_header();

#ifdef OPENDDS_SECURITY
  const SubmessageKind kind = rsh.submessage_._d();

  if (secure_prefix_.smHeader.submessageId == SEC_PREFIX && kind != SEC_POSTFIX) {
    // secure envelope in progress, defer processing
    secure_submessages_.push_back(rsh.submessage_);
    if (kind == DATA) {
      secure_sample_ = sample;
    }
    return;
  }

  encoded_submsg_ = false;
#endif

  deliver_sample_i(sample, rsh.submessage_);
}

void
RtpsUdpReceiveStrategy::deliver_sample_i(ReceivedDataSample& sample,
                                         const RTPS::Submessage& submessage)
{
  using namespace RTPS;
  const SubmessageKind kind = submessage._d();

  switch (kind) {
  case INFO_SRC:
  case INFO_REPLY_IP4:
  case INFO_DST:
  case INFO_REPLY:
  case INFO_TS:
    // No-op: the INFO_* submessages only modify the state of the
    // MessageReceiver (see check_header()), they are not passed up to DCPS.
    break;

  case DATA: {
    receiver_.fill_header(sample.header_);
    const DataSubmessage& data = submessage.data_sm();
    if (!check_encoded(data.writerId)) {
      break;
    }

    RepoIdSet directedWriteReaders;
    getDirectedWriteReaders(directedWriteReaders, data);

    recvd_sample_ = &sample;
    readers_selected_.clear();
    readers_withheld_.clear();
    // If this sample should be withheld from some readers in order to maintain
    // in-order delivery, link_->received() will add it to readers_withheld_ otherwise
    // it will be added to readers_selected_
    link_->received(data, receiver_.source_guid_prefix_);
    recvd_sample_ = 0;

    link_->filterBestEffortReaders(sample, readers_selected_, readers_withheld_);

    if (data.readerId != ENTITYID_UNKNOWN) {
      RepoId reader;
      std::memcpy(reader.guidPrefix, link_->local_prefix(),
                  sizeof(GuidPrefix_t));
      reader.entityId = data.readerId;
      if (!readers_withheld_.count(reader) &&
          (directedWriteReaders.empty() || directedWriteReaders.find(reader) != directedWriteReaders.end())) {
        if (Transport_debug_level > 5) {
          GuidConverter reader_conv(reader);
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
            ACE_TEXT("calling DataLink::data_received for seq: %q to reader %C\n"),
            this, sample.header_.sequence_.getValue(), OPENDDS_STRING(reader_conv).c_str()));
        }
#ifdef OPENDDS_SECURITY
        if (decode_payload(sample, data)) {
          link_->data_received(sample, reader);
        }
#else
        link_->data_received(sample, reader);
#endif
      }
    } else {
      if (Transport_debug_level > 5) {
        OPENDDS_STRING included_ids;
        bool first = true;
        RepoIdSet::iterator iter = readers_selected_.begin();
        while (iter != readers_selected_.end()) {
          included_ids += (first ? "" : "\n") + OPENDDS_STRING(GuidConverter(*iter));
          first = false;
          ++iter;
        }
        OPENDDS_STRING excluded_ids;
        first = true;
        RepoIdSet::iterator iter2 = this->readers_withheld_.begin();
        while (iter2 != readers_withheld_.end()) {
          excluded_ids += (first ? "" : "\n") + OPENDDS_STRING(GuidConverter(*iter2));
          first = false;
          ++iter2;
        }
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t)  - RtpsUdpReceiveStrategy[%@]::deliver_sample_i:\n")
          ACE_TEXT("  readers_selected ids:\n%C\n")
          ACE_TEXT("  readers_withheld ids:\n%C\n"),
          this, included_ids.c_str(), excluded_ids.c_str()));
      }

      if (readers_withheld_.empty() && readers_selected_.empty()) {
#ifdef OPENDDS_SECURITY
        if (decode_payload(sample, data)) {
#endif
          if (directedWriteReaders.empty()) {
            if (Transport_debug_level > 5) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
                ACE_TEXT("calling DataLink::data_received for seq: %q TO ALL, no exclusion or inclusion\n"),
                this, sample.header_.sequence_.getValue()));
            }
            link_->data_received(sample);
          } else {
            if (Transport_debug_level > 5) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
                ACE_TEXT("calling DataLink::data_received_include for seq: %q to directedWriteReaders\n"),
                this, sample.header_.sequence_.getValue()));
            }
            link_->data_received_include(sample, directedWriteReaders);
          }
#ifdef OPENDDS_SECURITY
        }
#endif
      } else {
#ifdef OPENDDS_SECURITY
        if (decode_payload(sample, data)) {
#endif
          if (directedWriteReaders.empty()) {
            if (Transport_debug_level > 5) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
                ACE_TEXT("calling DataLink::data_received_include for seq: %q to readers_selected_\n"),
                this, sample.header_.sequence_.getValue()));
            }
            link_->data_received_include(sample, readers_selected_);
          } else {
            if (Transport_debug_level > 5) {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
                ACE_TEXT("calling DataLink::data_received_include for seq: %q to intersection of readers\n"),
                this, sample.header_.sequence_.getValue()));
            }
            set_intersect(directedWriteReaders, readers_selected_, GUID_tKeyLessThan());
            link_->data_received_include(sample, directedWriteReaders);
          }
#ifdef OPENDDS_SECURITY
        }
#endif
      }
    }
    break;
  }
  case GAP:
    if (!check_encoded(submessage.gap_sm().writerId)) {
      break;
    }
    link_->received(submessage.gap_sm(), receiver_.source_guid_prefix_);
    break;

  case HEARTBEAT:
    if (!check_encoded(submessage.heartbeat_sm().writerId)) {
      break;
    }
    link_->received(submessage.heartbeat_sm(),
                    receiver_.source_guid_prefix_);
    if (submessage.heartbeat_sm().smHeader.flags & FLAG_L) {
      // Liveliness has been asserted.  Create a DATAWRITER_LIVELINESS message.
      sample.header_.message_id_ = DATAWRITER_LIVELINESS;
      receiver_.fill_header(sample.header_);
      sample.header_.publication_id_.entityId = submessage.heartbeat_sm().writerId;
      link_->data_received(sample);
    }
    break;

  case ACKNACK:
    if (!check_encoded(submessage.acknack_sm().readerId)) {
      break;
    }
    link_->received(submessage.acknack_sm(),
                    receiver_.source_guid_prefix_);
    break;

  case HEARTBEAT_FRAG:
    if (!check_encoded(submessage.hb_frag_sm().writerId)) {
      break;
    }
    link_->received(submessage.hb_frag_sm(),
                    receiver_.source_guid_prefix_);
    break;

  case NACK_FRAG:
    if (!check_encoded(submessage.nack_frag_sm().readerId)) {
      break;
    }
    link_->received(submessage.nack_frag_sm(),
                    receiver_.source_guid_prefix_);
    break;

  /* no case DATA_FRAG: by the time deliver_sample() is called, reassemble()
     has successfully reassembled the fragments and we now have a DATA submsg
   */

#ifdef OPENDDS_SECURITY
  case SEC_PREFIX:
    secure_prefix_ = submessage.security_sm();
    break;

  case SEC_POSTFIX:
    deliver_from_secure(submessage);
    break;
#endif

  default:
    break;
  }
}

#ifdef OPENDDS_SECURITY
void
RtpsUdpReceiveStrategy::deliver_from_secure(const RTPS::Submessage& submessage)
{
  using namespace DDS::Security;

  const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
  if (!crypto) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpReceiveStrategy SEC_POSTFIX no CryptoTransform\n"));
    return;
  }

  RepoId peer;
  RTPS::assign(peer.guidPrefix, receiver_.source_guid_prefix_);
  peer.entityId = ENTITYID_PARTICIPANT;
  const ParticipantCryptoHandle peer_pch = link_->peer_crypto_handle(peer);

  DDS::OctetSeq encoded_submsg, plain_submsg;
  sec_submsg_to_octets(encoded_submsg, submessage);
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
  secure_sample_ = ReceivedDataSample(0);

  DatawriterCryptoHandle dwch = DDS::HANDLE_NIL;
  DatareaderCryptoHandle drch = DDS::HANDLE_NIL;
  SecureSubmessageCategory_t category = INFO_SUBMESSAGE;
  SecurityException ex = {"", 0, 0};

  bool ok = crypto->preprocess_secure_submsg(dwch, drch, category, encoded_submsg,
                                             link_->local_crypto_handle(), peer_pch, ex);

  if (ok) {
    VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy::deliver_from_secure ")
      ACE_TEXT("dwch is %d and drch is %d\n"), dwch, drch), 4);
  }

  if (ok && category == DATAWRITER_SUBMESSAGE) {
    ok = crypto->decode_datawriter_submessage(plain_submsg, encoded_submsg,
                                              drch, dwch, ex);

  } else if (ok && category == DATAREADER_SUBMESSAGE) {
    ok = crypto->decode_datareader_submessage(plain_submsg, encoded_submsg,
                                              dwch, drch, ex);

  } else if (ok && category == INFO_SUBMESSAGE) {
    return;

  } else {
    if (security_debug.encdec_warn) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy: ")
                 ACE_TEXT("preprocess_secure_submsg failed RPCH %d, [%d.%d]: %C\n"),
                 peer_pch, ex.code, ex.minor_code, ex.message.in()));
    }
    return;
  }

  if (!ok) {
    bool dw = category == DATAWRITER_SUBMESSAGE;
    if (security_debug.encdec_warn) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy: ")
                 ACE_TEXT("decode_data%C_submessage failed [%d.%d]: \"%C\" ")
                 ACE_TEXT("(rpch: %u, local d%cch: %u, remote d%cch: %u)\n"),
                 dw ? "writer" : "reader",
                 ex.code, ex.minor_code, ex.message.in(),
                 peer_pch,
                 dw ? 'r' : 'w',
                 dw ? drch : dwch,
                 dw ? 'w' : 'r',
                 dw ? dwch : drch
                 ));
    }
    return;
  }

  ACE_Message_Block mb(plain_submsg.length());
  mb.copy(reinterpret_cast<const char*>(plain_submsg.get_buffer()), mb.size());

  if (Transport_debug_level > 5) {
    ACE_HEX_DUMP((LM_DEBUG, mb.rd_ptr(), mb.length(),
                  category == DATAWRITER_SUBMESSAGE ?
                  ACE_TEXT("RtpsUdpReceiveStrategy: decoded writer submessage") :
                  ACE_TEXT("RtpsUdpReceiveStrategy: decoded reader submessage")));
  }

  RtpsSampleHeader rsh(mb);
  if (check_header(rsh)) {
    ReceivedDataSample plain_sample(mb.duplicate());
    if (rsh.into_received_data_sample(plain_sample)) {
      encoded_submsg_ = true;
      deliver_sample_i(plain_sample, rsh.submessage_);
    }
  }
}

void
RtpsUdpReceiveStrategy::sec_submsg_to_octets(DDS::OctetSeq& encoded,
                                             const RTPS::Submessage& postfix)
{
  size_t size = 0, padding = 0;
  gen_find_size(secure_prefix_, size, padding);

  for (size_t i = 0; i < secure_submessages_.size(); ++i) {
    gen_find_size(secure_submessages_[i], size, padding);
    const RTPS::SubmessageKind kind = secure_submessages_[i]._d();
    if (kind == RTPS::DATA || kind == RTPS::DATA_FRAG) {
      size += secure_sample_.sample_->size();
    }
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
  }
  gen_find_size(postfix, size, padding);

  ACE_Message_Block mb(size + padding);
  Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser << secure_prefix_;
  ser.align_r(4);

  for (size_t i = 0; i < secure_submessages_.size(); ++i) {
    ser << secure_submessages_[i];
    const RTPS::SubmessageKind kind = secure_submessages_[i]._d();
    if (kind == RTPS::DATA || kind == RTPS::DATA_FRAG) {
      const CORBA::Octet* sample_bytes =
        reinterpret_cast<const CORBA::Octet*>(secure_sample_.sample_->rd_ptr());
      ser.write_octet_array(sample_bytes,
                            static_cast<unsigned int>(secure_sample_.sample_->length()));
    }
    ser.align_r(4);
  }
  ser << postfix;

  encoded.length(static_cast<unsigned int>(mb.length()));
  std::memcpy(encoded.get_buffer(), mb.rd_ptr(), mb.length());
  secure_submessages_.resize(0);
}

bool RtpsUdpReceiveStrategy::decode_payload(ReceivedDataSample& sample,
                                            const RTPS::DataSubmessage& submsg)
{
  using namespace DDS::Security;
  const DatawriterCryptoHandle writer_crypto_handle = link_->writer_crypto_handle(sample.header_.publication_id_);
  const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();

  const EndpointSecurityAttributesMask esa = link_->security_attributes(sample.header_.publication_id_);
  static const EndpointSecurityAttributesMask MASK_PROTECT_PAYLOAD =
    ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID | ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED;
  const bool payload_protected = (esa & MASK_PROTECT_PAYLOAD) == MASK_PROTECT_PAYLOAD;

  if (writer_crypto_handle == DDS::HANDLE_NIL || !crypto || !payload_protected) {
    return true;
  }

  DDS::OctetSeq encoded, plain, iQos;
  encoded.length(static_cast<unsigned int>(sample.sample_->total_length()));
  unsigned char* const buffer = encoded.get_buffer();
  ACE_Message_Block* mb(sample.sample_.get());
  for (unsigned int i = 0; mb; mb = mb->cont()) {
    std::memcpy(buffer + i, mb->rd_ptr(), mb->length());
    i += static_cast<unsigned int>(mb->length());
  }

  size_t iQosSize = 0, iQosPadding = 0;
  gen_find_size(submsg.inlineQos, iQosSize, iQosPadding);
  iQos.length(static_cast<unsigned int>(iQosSize + iQosPadding));
  const char* iQos_raw = reinterpret_cast<const char*>(iQos.get_buffer());
  ACE_Message_Block iQosMb(iQos_raw, iQos.length());
  Serializer ser(&iQosMb, ACE_CDR_BYTE_ORDER != (submsg.smHeader.flags & 1),
                 Serializer::ALIGN_CDR);
  ser << submsg.inlineQos;

  SecurityException ex = {"", 0, 0};
  // DDS-Security: since origin authentication for payload is not yet supported
  // the reader's crypto handle is NIL here (could be multiple readers in this
  // participant)
  const bool ok = crypto->decode_serialized_payload(plain, encoded, iQos,
                                                    DDS::HANDLE_NIL,
                                                    writer_crypto_handle, ex);
  if (ok) {
    const unsigned int n = plain.length();

    // The sample.sample_ message block uses the transport's data block so it
    // can't be modified in-place, instead replace it with a new block.
    sample.sample_.reset(new ACE_Message_Block(n));
    const char* buffer_raw = reinterpret_cast<const char*>(plain.get_buffer());
    sample.sample_->copy(buffer_raw, n);

    if (n > 1) {
      sample.header_.byte_order_ = RtpsSampleHeader::payload_byte_order(sample);
    }

  } else if (security_debug.encdec_warn) {
    ACE_ERROR((LM_WARNING, "(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy: "
               "decode_serialized_payload failed [%d.%d]: %C\n",
               ex.code, ex.minor_code, ex.message.in()));
  }

  return ok;
}
#endif

int
RtpsUdpReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

#ifdef ACE_WIN32
  // By default Winsock will cause reads to fail with "connection reset"
  // when UDP sends result in ICMP "port unreachable" messages.
  // The transport framework is not set up for this since returning <= 0
  // from our receive_bytes causes the framework to close down the datalink
  // which in this case is used to receive from multiple peers.
  BOOL recv_udp_connreset = FALSE;
  link_->unicast_socket().control(SIO_UDP_CONNRESET, &recv_udp_connreset);
#endif

  if (reactor->register_handler(link_->unicast_socket().get_handle(), this,
                                ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for unicast ")
                      ACE_TEXT("socket %d\n"),
                      link_->unicast_socket().get_handle()),
                     -1);
  }

  if (link_->config().use_multicast_) {
    if (reactor->register_handler(link_->multicast_socket().get_handle(), this,
                                  ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                        ACE_TEXT("failed to register handler for multicast\n")),
                       -1);
    }
  }

  return 0;
}

void
RtpsUdpReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("RtpsUdpReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(link_->unicast_socket().get_handle(),
                          ACE_Event_Handler::READ_MASK);

  if (link_->config().use_multicast_) {
    reactor->remove_handler(link_->multicast_socket().get_handle(),
                            ACE_Event_Handler::READ_MASK);
  }
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsTransportHeader& header)
{
  receiver_.reset(remote_address_, header.header_);

#ifdef OPENDDS_SECURITY
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
#endif

  return header.valid();
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsSampleHeader& header)
{

#ifdef OPENDDS_SECURITY
  if (secure_prefix_.smHeader.submessageId) {
    return header.valid();
  }
#endif

  receiver_.submsg(header.submessage_);

  // save fragmentation details for use in reassemble()
  if (header.valid() && header.submessage_._d() == RTPS::DATA_FRAG) {
    const RTPS::DataFragSubmessage& rtps = header.submessage_.data_frag_sm();
    frags_.first = rtps.fragmentStartingNum.value;
    frags_.second = frags_.first + (rtps.fragmentsInSubmessage - 1);
  }

  return header.valid();
}

const ReceivedDataSample*
RtpsUdpReceiveStrategy::withhold_data_from(const RepoId& sub_id)
{
  readers_withheld_.insert(sub_id);
  return recvd_sample_;
}

void
RtpsUdpReceiveStrategy::do_not_withhold_data_from(const RepoId& sub_id)
{
  readers_selected_.insert(sub_id);
}

bool RtpsUdpReceiveStrategy::getDirectedWriteReaders(RepoIdSet& directedWriteReaders, const RTPS::DataSubmessage& ds) const
{
  directedWriteReaders.clear();
  for (CORBA::ULong i = 0; i < ds.inlineQos.length(); ++i) {
    if (ds.inlineQos[i]._d() == RTPS::PID_DIRECTED_WRITE
        && receiver_.source_version_.minor >= 4) {
      directedWriteReaders.insert(ds.inlineQos[i].guid());
    }
  }
  return !directedWriteReaders.empty();
}

bool
RtpsUdpReceiveStrategy::reassemble(ReceivedDataSample& data)
{
  using namespace RTPS;
  receiver_.fill_header(data.header_); // set publication_id_.guidPrefix
  if (reassembly_.reassemble(frags_, data)) {

    // Reassembly was successful, replace DataFrag with Data.  This doesn't have
    // to be a fully-formed DataSubmessage, just enough for this class to use
    // in deliver_sample() which ends up calling RtpsUdpDataLink::received().
    // In particular we will need the SequenceNumber, but ignore the iQoS.

    // Peek at the byte order from the encapsulation containing the payload.
    data.header_.byte_order_ = data.sample_->rd_ptr()[1] & FLAG_E;

    RtpsSampleHeader& rsh = received_sample_header();
    const DataFragSubmessage& dfsm = rsh.submessage_.data_frag_sm();

    const CORBA::Octet data_flags = (data.header_.byte_order_ ? FLAG_E : 0)
      | (data.header_.key_fields_only_ ? FLAG_K_IN_DATA : FLAG_D);
    const DataSubmessage dsm = {
      {DATA, data_flags, 0}, 0, DATA_OCTETS_TO_IQOS,
      dfsm.readerId, dfsm.writerId, dfsm.writerSN, ParameterList()};
    rsh.submessage_.data_sm(dsm);
    return true;
  }
  return false;
}

bool
RtpsUdpReceiveStrategy::remove_frags_from_bitmap(CORBA::Long bitmap[],
                                                 CORBA::ULong num_bits,
                                                 const SequenceNumber& base,
                                                 const RepoId& pub_id)
{
  bool modified = false;
  for (CORBA::ULong i = 0, x = 0, bit = 0; i < num_bits; ++i, ++bit) {
    if (bit == 32) bit = 0;

    if (bit == 0) {
      x = static_cast<CORBA::ULong>(bitmap[i / 32]);
      if (x == 0) {
        // skip an entire Long if it's all 0's (adds 32 due to ++i)
        i += 31;
        bit = 31;
        //FUTURE: this could be generalized with something like the x86 "bsr"
        //        instruction using compiler intrinsics, VC++ _BitScanReverse()
        //        and GCC __builtin_clz()
        continue;
      }
    }

    const CORBA::ULong mask = 1 << (31 - bit);
    if ((x & mask) && reassembly_.has_frags(base + i, pub_id)) {
      x &= ~mask;
      bitmap[i / 32] = x;
      modified = true;
    }
  }
  return modified;
}

void
RtpsUdpReceiveStrategy::remove_fragments(const SequenceRange& range,
                                         const RepoId& pub_id)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    reassembly_.data_unavailable(sn, pub_id);
  }
}

bool
RtpsUdpReceiveStrategy::has_fragments(const SequenceRange& range,
                                      const RepoId& pub_id,
                                      FragmentInfo* frag_info)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    if (reassembly_.has_frags(sn, pub_id)) {
      if (frag_info) {
        std::pair<SequenceNumber, RTPS::FragmentNumberSet> p;
        p.first = sn;
        frag_info->push_back(p);
        RTPS::FragmentNumberSet& missing_frags = frag_info->back().second;
        missing_frags.numBits = 0; // make sure this is a valid number before passing to get_gaps
        missing_frags.bitmap.length(8); // start at max length
        missing_frags.bitmapBase.value =
          reassembly_.get_gaps(sn, pub_id, missing_frags.bitmap.get_buffer(),
                               8, missing_frags.numBits);
        // reduce length in case get_gaps() didn't need all that room
        missing_frags.bitmap.length((missing_frags.numBits + 31) / 32);
      } else {
        return true;
      }
    }
  }
  return frag_info ? !frag_info->empty() : false;
}


// MessageReceiver nested class

RtpsUdpReceiveStrategy::MessageReceiver::MessageReceiver(const GuidPrefix_t& local)
  : have_timestamp_(false)
{
  RTPS::assign(local_, local);
  source_version_.major = source_version_.minor = 0;
  source_vendor_.vendorId[0] = source_vendor_.vendorId[1] = 0;
  for (size_t i = 0; i < sizeof(GuidPrefix_t); ++i) {
    source_guid_prefix_[i] = 0;
    dest_guid_prefix_[i] = 0;
  }
  timestamp_.seconds = 0;
  timestamp_.fraction = 0;
}

void
RtpsUdpReceiveStrategy::MessageReceiver::reset(const ACE_INET_Addr& addr,
                                               const RTPS::Header& hdr)
{
  using namespace RTPS;
  // see RTPS spec v2.1 section 8.3.4 table 8.16 and section 8.3.6.4
  source_version_ = hdr.version;
  source_vendor_ = hdr.vendorId;

  assign(source_guid_prefix_, hdr.guidPrefix);
  assign(dest_guid_prefix_, local_);

  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0].kind = address_to_kind(addr);
  unicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  RTPS::address_to_bytes(unicast_reply_locator_list_[0].address, addr);

  multicast_reply_locator_list_.length(1);
  multicast_reply_locator_list_[0].kind = address_to_kind(addr);
  multicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  assign(multicast_reply_locator_list_[0].address, LOCATOR_ADDRESS_INVALID);

  have_timestamp_ = false;
  timestamp_ = TIME_INVALID;
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(const RTPS::Submessage& s)
{
  using namespace RTPS;

  switch (s._d()) {
  case INFO_TS:
    submsg(s.info_ts_sm());
    break;

  case INFO_SRC:
    submsg(s.info_src_sm());
    break;

  case INFO_REPLY_IP4:
    submsg(s.info_reply_ipv4_sm());
    break;

  case INFO_DST:
    submsg(s.info_dst_sm());
    break;

  case INFO_REPLY:
    submsg(s.info_reply_sm());
    break;

  default:
    break;
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const RTPS::InfoDestinationSubmessage& id)
{
  // see RTPS spec v2.1 section 8.3.7.7.4
  for (size_t i = 0; i < sizeof(GuidPrefix_t); ++i) {
    if (id.guidPrefix[i]) { // if some byte is > 0, it's not UNKNOWN
      RTPS::assign(dest_guid_prefix_, id.guidPrefix);
      return;
    }
  }
  RTPS::assign(dest_guid_prefix_, local_);
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(const RTPS::InfoReplySubmessage& ir)
{
  // see RTPS spec v2.1 section 8.3.7.8.4
  unicast_reply_locator_list_.length(ir.unicastLocatorList.length());
  for (CORBA::ULong i = 0; i < ir.unicastLocatorList.length(); ++i) {
    unicast_reply_locator_list_[i] = ir.unicastLocatorList[i];
  }

  if (ir.smHeader.flags & 2 /* MulticastFlag */) {
    multicast_reply_locator_list_.length(ir.multicastLocatorList.length());
    for (CORBA::ULong i = 0; i < ir.multicastLocatorList.length(); ++i) {
      multicast_reply_locator_list_[i] = ir.multicastLocatorList[i];
    }

  } else {
    multicast_reply_locator_list_.length(0);
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const RTPS::InfoReplyIp4Submessage& iri4)
{
  // see RTPS spec v2.1 sections 8.3.7.8.4 and 9.4.5.14
  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0].kind = RTPS::LOCATOR_KIND_UDPv4;
  unicast_reply_locator_list_[0].port = iri4.unicastLocator.port;
  RTPS::assign(unicast_reply_locator_list_[0].address, iri4.unicastLocator.address);

  if (iri4.smHeader.flags & 2 /* MulticastFlag */) {
    multicast_reply_locator_list_.length(1);
    multicast_reply_locator_list_[0].kind = RTPS::LOCATOR_KIND_UDPv4;
    multicast_reply_locator_list_[0].port = iri4.multicastLocator.port;
    RTPS::assign(multicast_reply_locator_list_[0].address, iri4.multicastLocator.address);
  } else {
    multicast_reply_locator_list_.length(0);
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const RTPS::InfoTimestampSubmessage& it)
{
  // see RTPS spec v2.1 section 8.3.7.9.10
  if (!(it.smHeader.flags & 2 /* InvalidateFlag */)) {
    have_timestamp_ = true;
    timestamp_ = it.timestamp;
  } else {
    have_timestamp_ = false;
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const RTPS::InfoSourceSubmessage& is)
{
  // see RTPS spec v2.1 section 8.3.7.9.4
  RTPS::assign(source_guid_prefix_, is.guidPrefix);
  source_version_ = is.version;
  source_vendor_ = is.vendorId;
  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0] = RTPS::LOCATOR_INVALID;
  multicast_reply_locator_list_.length(1);
  multicast_reply_locator_list_[0] = RTPS::LOCATOR_INVALID;
  have_timestamp_ = false;
}

void
RtpsUdpReceiveStrategy::MessageReceiver::fill_header(
  DataSampleHeader& header) const
{
  using namespace RTPS;
  if (have_timestamp_) {
    header.source_timestamp_sec_ = timestamp_.seconds;
    header.source_timestamp_nanosec_ =
      DCPS::uint32_fractional_seconds_to_nanoseconds(timestamp_.fraction);
  }
  assign(header.publication_id_.guidPrefix, source_guid_prefix_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
