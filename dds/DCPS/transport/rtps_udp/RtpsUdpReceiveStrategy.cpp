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

#include "dds/DCPS/RTPS/MessageUtils.h"
#include "dds/DCPS/RTPS/MessageTypes.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Util.h>

#include "dds/DCPS/transport/framework/TransportDebug.h"

#include <dds/OpenDDSConfigWrapper.h>

#include "ace/Reactor.h"

#include <algorithm>
#include <cstring>


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpReceiveStrategy::RtpsUdpReceiveStrategy(RtpsUdpDataLink* link,
                                               const GuidPrefix_t& local_prefix,
                                               ThreadStatusManager& thread_status_manager)
  : BaseReceiveStrategy(link->config(), BUFFER_COUNT)
  , link_(link)
  , last_received_()
  , recvd_sample_(0)
  , fragment_size_(0)
  , total_frags_(0)
  , reassembly_(link->config()->fragment_reassembly_timeout())
  , receiver_(local_prefix)
  , thread_status_manager_(thread_status_manager)
#if OPENDDS_CONFIG_SECURITY
  , secure_sample_()
  , encoded_rtps_(false)
  , encoded_submsg_(false)
#endif
{
  // Since BUFFER_COUNT is 1, the index will always be 0
  const size_t INDEX = 0;

  if (receive_buffers_[INDEX] == 0) {
    ACE_NEW_MALLOC(
      receive_buffers_[INDEX],
      (ACE_Message_Block*) mb_allocator_.malloc(sizeof(ACE_Message_Block)),
      ACE_Message_Block(
        RECEIVE_DATA_BUFFER_SIZE,           // Buffer size
        ACE_Message_Block::MB_DATA,         // Default
        0,                                  // Start with no continuation
        0,                                  // Let the constructor allocate
        &data_allocator_,                   // Our buffer cache
        &receive_lock_,                     // Our locking strategy
        ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY, // Default
        ACE_Time_Value::zero,               // Default
        ACE_Time_Value::max_time,           // Default
        &db_allocator_,                     // Our data block cache
        &mb_allocator_                      // Our message block cache
      ));
  }

#if OPENDDS_CONFIG_SECURITY
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
#endif
}

int
RtpsUdpReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  ThreadStatusManager::Event ev(thread_status_manager_);

  // Since BUFFER_COUNT is 1, the index will always be 0
  const size_t INDEX = 0;

  ACE_Message_Block* const cur_rb = receive_buffers_[INDEX];
  cur_rb->reset();

  iovec iov;
#ifdef _MSC_VER
#pragma warning(push)
// iov_len is 32-bit on 64-bit VC++, but we don't want a cast here
// since on other platforms iov_len is 64-bit
#pragma warning(disable : 4267)
#endif
  iov.iov_len = cur_rb->space();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  iov.iov_base = cur_rb->wr_ptr();

  ACE_INET_Addr remote_address;
  bool stop = false;
  ssize_t bytes_remaining = receive_bytes(&iov,
                                          1,
                                          remote_address,
                                          fd,
                                          stop);

  if (stop) {
    return 0;
  }

  if (bytes_remaining < 0) {
    relink();
    return -1;
  }

  cur_rb->wr_ptr(bytes_remaining);

  if (bytes_remaining == 0) {
    if (gracefully_disconnected_) {
      return -1;
    } else {
      relink();
      return -1;
    }
  }

  if (!pdu_remaining_) {
    receive_transport_header_.length_ = static_cast<ACE_UINT32>(bytes_remaining);
  }

  receive_transport_header_ = *cur_rb;
  if (!receive_transport_header_.valid()) {
    cur_rb->reset();
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: RtpsUdpReceiveStrategy::handle_input: TransportHeader invalid.\n")));
    }
    return 0;
  }

  bytes_remaining = receive_transport_header_.length_;
  if (!check_header(receive_transport_header_)) {
    return 0;
  }

  {
    const ScopedHeaderProcessing shp(*this);
    while (bytes_remaining > 0) {
      data_sample_header_.pdu_remaining(bytes_remaining);
      data_sample_header_ = *cur_rb;
      bytes_remaining -= data_sample_header_.get_serialized_size();
      if (!check_header(data_sample_header_)) {
        return 0;
      }
      ReceivedDataSample rds = data_sample_header_.message_length() ? ReceivedDataSample(*cur_rb) : ReceivedDataSample();
      if (data_sample_header_.into_received_data_sample(rds)) {

        if (data_sample_header_.more_fragments() || receive_transport_header_.last_fragment()) {
          VDBG((LM_DEBUG,"(%P|%t) DBG:   Attempt reassembly of fragments\n"));

          if (reassemble(rds)) {
            VDBG((LM_DEBUG,"(%P|%t) DBG:   Reassembled complete message\n"));
            deliver_sample(rds, remote_address);
          }
          // If reassemble() returned false, it takes ownership of the data
          // just like deliver_sample() does.

        } else {
          deliver_sample(rds, remote_address);
        }
      }
      cur_rb->rd_ptr(data_sample_header_.message_length());
      bytes_remaining -= data_sample_header_.message_length();

      // For the reassembly algorithm, the 'last_fragment_' header bit only
      // applies to the first DataSampleHeader in the TransportHeader
      receive_transport_header_.last_fragment(false);
    }
  }

  // If newly selected buffer index still has a reference count, we'll need to allocate a new one for the read
  if (receive_buffers_[INDEX]->data_block()->reference_count() > 1) {

    if (log_level >= LogLevel::Info) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: RtpsUdpReceiveStrategy::handle_input: reallocating primary receive buffer based on reference count\n"));
    }

    ACE_DES_FREE(
      receive_buffers_[INDEX],
      mb_allocator_.free,
      ACE_Message_Block);

    ACE_NEW_MALLOC_RETURN(
      receive_buffers_[INDEX],
      (ACE_Message_Block*) mb_allocator_.malloc(sizeof(ACE_Message_Block)),
      ACE_Message_Block(
        RECEIVE_DATA_BUFFER_SIZE,           // Buffer size
        ACE_Message_Block::MB_DATA,         // Default
        0,                                  // Start with no continuation
        0,                                  // Let the constructor allocate
        &data_allocator_,                   // Our buffer cache
        &receive_lock_,                     // Our locking strategy
        ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY, // Default
        ACE_Time_Value::zero,               // Default
        ACE_Time_Value::max_time,           // Default
        &db_allocator_,                     // Our data block cache
        &mb_allocator_                      // Our message block cache
      ),
      -1);
  }

  return 0;
}

ssize_t
RtpsUdpReceiveStrategy::receive_bytes_helper(iovec iov[],
                                             int n,
                                             const ACE_SOCK_Dgram& socket,
                                             ACE_INET_Addr& remote_address,
#if OPENDDS_CONFIG_SECURITY
                                             DCPS::RcHandle<ICE::Agent> ice_agent,
                                             DCPS::WeakRcHandle<ICE::Endpoint> endpoint,
#endif
                                             RtpsUdpTransport& tport,
                                             bool& stop)
{
  ACE_INET_Addr local_address;
  const ssize_t ret = socket.recv(iov, n, remote_address, 0
#if defined(ACE_RECVPKTINFO) || defined(ACE_RECVPKTINFO6)
                                  , &local_address
#endif
  );

  if (ret == -1) {
    return ret;
  }

  if (remote_address.get_size() > remote_address.get_addr_size()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpReceiveStrategy::receive_bytes_helper - invalid address size\n"));
    return 0;
  }

  const NetworkAddress ra(remote_address);

  if (n > 0 && ret > 0 && iov[0].iov_len >= 4 && std::memcmp(iov[0].iov_base, "RTPS", 4) == 0) {
    tport.core().recv(ra, MCK_RTPS, ret);
    return ret;
  }

#if OPENDDS_CONFIG_SECURITY
  // Assume STUN
# ifndef ACE_RECVPKTINFO
  ACE_ERROR((LM_ERROR, "ERROR: RtpsUdpReceiveStrategy::receive_bytes_helper potential STUN message "
             "received but this version of the ACE library doesn't support the local_address "
             "extension in ACE_SOCK_Dgram::recv\n"));
  ACE_UNUSED_ARG(stop);
  ACE_NOTSUP_RETURN(-1);
# else

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

  DCPS::Serializer serializer(head, STUN::encoding);
  STUN::Message message;
  message.block = head;
  if (serializer >> message) {
    tport.core().recv(ra, MCK_STUN, ret);

    if (tport.relay_srsm().is_response(message)) {
      tport.process_relay_sra(tport.relay_srsm().receive(message));
#if OPENDDS_CONFIG_SECURITY
    } else if (endpoint) {
      ice_agent->receive(endpoint, local_address, remote_address, message);
#endif
    }
  }
  head->release();
# endif
#else
  ACE_UNUSED_ARG(stop);
#endif

  return ret;
}

#if OPENDDS_CONFIG_SECURITY
namespace {
  ssize_t recv_err(const char* msg, const ACE_INET_Addr& remote, const DCPS::GUID_t& peer, bool& stop)
  {
    if (security_debug.encdec_warn) {
      ACE_ERROR((LM_WARNING, "(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::receive_bytes - "
                 "from %C %C secure RTPS processing failed: %C\n",
                 DCPS::LogAddr(remote).c_str(), LogGuid(peer).c_str(), msg));
    }
    stop = true;
    return 0;
  }
}
#endif

const ACE_SOCK_Dgram&
RtpsUdpReceiveStrategy::choose_recv_socket(ACE_HANDLE fd) const
{
#ifdef ACE_HAS_IPV6
  if (fd == link_->ipv6_multicast_socket().get_handle()) {
    return link_->ipv6_multicast_socket();
  }
  if (fd == link_->ipv6_unicast_socket().get_handle()) {
    return link_->ipv6_unicast_socket();
  }
#endif
  if (fd == link_->multicast_socket().get_handle()) {
    return link_->multicast_socket();
  }
  return link_->unicast_socket();
}

ssize_t
RtpsUdpReceiveStrategy::receive_bytes(iovec iov[],
                                      int n,
                                      ACE_INET_Addr& remote_address,
                                      ACE_HANDLE fd,
                                      bool& stop)
{
  const ACE_SOCK_Dgram& socket = choose_recv_socket(fd);
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
  const ssize_t ret = receive_bytes_helper(iov, n, socket, remote_address,
#if OPENDDS_CONFIG_SECURITY
                                           link_->get_ice_agent(), link_->get_ice_endpoint(),
#endif
                                           *link_->transport(), stop);
#endif
  remote_address_ = remote_address;

#if OPENDDS_CONFIG_SECURITY
  if (stop) {
    return ret;
  }

  using namespace DDS::Security;
  const ParticipantCryptoHandle receiver = link_->local_crypto_handle();
  if (ret > 0 && receiver != DDS::HANDLE_NIL) {
    encoded_rtps_ = false;

    GUID_t peer = GUID_UNKNOWN;

    const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
    if (!crypto) {
      return recv_err("no crypto plugin", remote_address, peer, stop);
    }

    if (ret < RTPS::RTPSHDR_SZ + RTPS::SMHDR_SZ) {
      return recv_err("message too short", remote_address, peer, stop);
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
      return recv_err("received bytes didn't fit in iovec array", remote_address, peer, stop);
    }

    if (encoded[RTPS::RTPSHDR_SZ] != RTPS::SRTPS_PREFIX) {
      return ret;
    }

    static const int GuidPrefixOffset = 8; // "RTPS", Version(2), Vendor(2)
    std::memcpy(peer.guidPrefix, encBuf + GuidPrefixOffset, sizeof peer.guidPrefix);
    peer.entityId = RTPS::ENTITYID_PARTICIPANT;
    const ParticipantCryptoHandle sender = equal_guid_prefixes(peer.guidPrefix, receiver_.local_) ?
      link_->local_crypto_handle() :
      link_->handle_registry()->get_remote_participant_crypto_handle(peer);
    if (sender == DDS::HANDLE_NIL) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::receive_bytes - decode error from %C\n", LogGuid(peer).c_str()));
      }
      if (security_debug.encdec_warn) {
        ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::receive_bytes: ")
                   ACE_TEXT("decode_rtps_message no remote participant crypto handle for %C, dropping\n"),
                   LogGuid(peer).c_str()));
      }
      stop = true;
      return ret;
    }

    DDS::OctetSeq plain;
    SecurityException ex = {"", 0, 0};
    if (!crypto->decode_rtps_message(plain, encoded, receiver, sender, ex)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::receive_bytes - decode error from %C\n", LogGuid(peer).c_str()));
      }
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
      return recv_err("decode_rtps_message failed", remote_address, peer, stop);
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
      return recv_err("plaintext doesn't fit in iovec array", remote_address, peer, stop);
    }

    encoded_rtps_ = true;
    return plainLen;
  }
#endif

  return ret;
}

bool RtpsUdpReceiveStrategy::check_encoded(const EntityId_t& sender)
{
#if OPENDDS_CONFIG_SECURITY
  using namespace DDS::Security;
  const GUID_t sendGuid = make_id(receiver_.source_guid_prefix_, sender);
  const GuidConverter conv(sendGuid);

  if (link_->local_crypto_handle() != DDS::HANDLE_NIL
      && !encoded_rtps_ && !RtpsUdpDataLink::separate_message(sender)) {
    if (security_debug.encdec_warn) {
      ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsUdpReceiveStrategy::check_encoded "
                 "Full message from %C requires protection, dropping\n",
                 OPENDDS_STRING(conv).c_str()));
    }
    return false;
  }

  const EndpointSecurityAttributesMask esa = RTPS::security_attributes_to_bitmask(
    conv.isReader() ?
    link_->handle_registry()->get_remote_datareader_security_attributes(sendGuid) :
    link_->handle_registry()->get_remote_datawriter_security_attributes(sendGuid));
  static const EndpointSecurityAttributesMask MASK_PROTECT_SUBMSG =
    ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID | ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_SUBMESSAGE_PROTECTED;
  if ((esa & MASK_PROTECT_SUBMSG) == MASK_PROTECT_SUBMSG && !encoded_submsg_) {
    if (security_debug.encdec_warn) {
      ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsUdpReceiveStrategy::check_encoded "
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
                                       const ACE_INET_Addr& remote_address)
{
  using namespace RTPS;

  if (std::memcmp(receiver_.dest_guid_prefix_, link_->local_prefix(),
                  sizeof(GuidPrefix_t))) {
    // Not our message, we may be on multicast listening to all the others.
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample - not destination\n"));
    }
    return;
  }

  const RtpsSampleHeader& rsh = received_sample_header();

  if (transport_debug.log_messages) {
    DCPS::push_back(message_.submessages, rsh.submessage_);
  }

#if OPENDDS_CONFIG_SECURITY
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

  deliver_sample_i(sample, rsh.submessage_, NetworkAddress(remote_address));
}

void
RtpsUdpReceiveStrategy::deliver_sample_i(ReceivedDataSample& sample,
                                         const RTPS::Submessage& submessage,
                                         const NetworkAddress& remote_addr)
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
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }

#if OPENDDS_CONFIG_SECURITY
    if (!decode_payload(sample, data)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
#endif

    RepoIdSet directedWriteReaders;
    getDirectedWriteReaders(directedWriteReaders, data);

    recvd_sample_ = &sample;
    readers_selected_.clear();
    readers_withheld_.clear();
    // If this sample should be withheld from some readers in order to maintain
    // in-order delivery, link_->received() will add it to readers_withheld_ otherwise
    // it will be added to readers_selected_
    link_->received(data, receiver_.source_guid_prefix_, remote_addr);
    recvd_sample_ = 0;

    link_->filterBestEffortReaders(sample, readers_selected_, readers_withheld_);

    if (data.readerId != ENTITYID_UNKNOWN) {
      GUID_t reader;
      std::memcpy(reader.guidPrefix, link_->local_prefix(),
                  sizeof(GuidPrefix_t));
      reader.entityId = data.readerId;
      if (!readers_withheld_.count(reader) &&
          (directedWriteReaders.empty() || directedWriteReaders.find(reader) != directedWriteReaders.end())) {
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i - ")
            ACE_TEXT("calling DataLink::data_received for seq: %q to reader %C\n"),
            this, sample.header_.sequence_.getValue(), LogGuid(reader).c_str()));
        }
        link_->data_received(sample, reader);
      }
    } else {
      if (Transport_debug_level > 5) {
        OPENDDS_STRING included_ids;
        bool first = true;
        RepoIdSet::iterator iter = readers_selected_.begin();
        while (iter != readers_selected_.end()) {
          included_ids += (first ? "" : "\n") + LogGuid(*iter).conv_;
          first = false;
          ++iter;
        }
        OPENDDS_STRING excluded_ids;
        first = true;
        RepoIdSet::iterator iter2 = this->readers_withheld_.begin();
        while (iter2 != readers_withheld_.end()) {
          excluded_ids += (first ? "" : "\n") + LogGuid(*iter2).conv_;
          first = false;
          ++iter2;
        }
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpReceiveStrategy[%@]::deliver_sample_i:")
          ACE_TEXT(" readers_selected ids: %C\n")
          ACE_TEXT(" readers_withheld ids: %C\n"),
          this, included_ids.c_str(), excluded_ids.c_str()));
      }

      if (readers_withheld_.empty() && readers_selected_.empty()) {
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
     } else {
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
      }
    }
    break;
  }
  case GAP:
    if (!check_encoded(submessage.gap_sm().writerId)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
    link_->received(submessage.gap_sm(), receiver_.source_guid_prefix_, receiver_.directed_, remote_addr);
    break;

  case HEARTBEAT:
    if (!check_encoded(submessage.heartbeat_sm().writerId)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
    link_->received(submessage.heartbeat_sm(), receiver_.source_guid_prefix_, receiver_.directed_, remote_addr);
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
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
    link_->received(submessage.acknack_sm(), receiver_.source_guid_prefix_, remote_addr);
    break;

  case HEARTBEAT_FRAG:
    if (!check_encoded(submessage.hb_frag_sm().writerId)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
    link_->received(submessage.hb_frag_sm(), receiver_.source_guid_prefix_, receiver_.directed_, remote_addr);
    break;

  case NACK_FRAG:
    if (!check_encoded(submessage.nack_frag_sm().readerId)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpReceiveStrategy::deliver_sample_i - decode error\n"));
      }
      break;
    }
    link_->received(submessage.nack_frag_sm(), receiver_.source_guid_prefix_, remote_addr);
    break;

  /* no case DATA_FRAG: by the time deliver_sample() is called, reassemble()
     has successfully reassembled the fragments and we now have a DATA submsg
   */

#if OPENDDS_CONFIG_SECURITY
  case SEC_PREFIX:
    secure_prefix_ = submessage.security_sm();
    break;

  case SEC_POSTFIX:
    deliver_from_secure(submessage, remote_addr);
    break;
#endif

  default:
    break;
  }
}

#if OPENDDS_CONFIG_SECURITY
void
RtpsUdpReceiveStrategy::deliver_from_secure(const RTPS::Submessage& submessage,
                                            const NetworkAddress& remote_addr)
{
  using namespace DDS::Security;

  const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
  if (!crypto) {
    // security not enabled for this datalink -- this can be reached
    // when a secure message is seen on the same multicast group
    return;
  }

  const GUID_t peer = make_id(receiver_.source_guid_prefix_, ENTITYID_PARTICIPANT);
  const ParticipantCryptoHandle peer_pch = equal_guid_prefixes(peer.guidPrefix, receiver_.local_) ?
    link_->local_crypto_handle() :
    link_->handle_registry()->get_remote_participant_crypto_handle(peer);

  DDS::OctetSeq encoded_submsg, plain_submsg;
  if (!sec_submsg_to_octets(encoded_submsg, submessage)) {
    if (security_debug.encdec_warn) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy: ")
                 ACE_TEXT("deliver_from_secure failed to encode submessage %C RPCH %d\n"),
                 LogGuid(peer).c_str(), peer_pch));
    }
    return;
  }
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
  secure_sample_ = ReceivedDataSample();

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
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::deliver_from_secure ")
                 ACE_TEXT("failed remote %C RPCH %d, [%d.%d]: %C\n"),
                 LogGuid(peer).c_str(), peer_pch, ex.code, ex.minor_code, ex.message.in()));
    }
    return;
  }

  if (!ok) {
    bool dw = category == DATAWRITER_SUBMESSAGE;
    if (security_debug.encdec_warn) {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) {encdec_warn} RtpsUdpReceiveStrategy::deliver_from_secure ")
                 ACE_TEXT("decode %C submessage failed [%d.%d]: \"%C\" ")
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
    ReceivedDataSample plain_sample(mb);
    if (rsh.into_received_data_sample(plain_sample)) {
      if (rsh.more_fragments()) {
        VDBG((LM_DEBUG, "(%P|%t) DBG:   Attempt reassembly of decoded fragments\n"));
        if (reassemble_i(plain_sample, rsh)) {
          VDBG((LM_DEBUG, "(%P|%t) DBG:   Reassembled complete message from decoded\n"));
          encoded_submsg_ = true;
          if (transport_debug.log_messages) {
            // Pop the secure envelope.
            message_.submessages.length(message_.submessages.length() - 3);
            DCPS::push_back(message_.submessages, rsh.submessage_);
          }
          deliver_sample_i(plain_sample, rsh.submessage_, remote_addr);
          return;
        }
      }
      encoded_submsg_ = true;
      if (transport_debug.log_messages) {
        // Pop the secure envelope.
        message_.submessages.length(message_.submessages.length() - 3);
        DCPS::push_back(message_.submessages, rsh.submessage_);
      }
      deliver_sample_i(plain_sample, rsh.submessage_, remote_addr);
    }
  }
}

bool
RtpsUdpReceiveStrategy::sec_submsg_to_octets(DDS::OctetSeq& encoded,
                                             const RTPS::Submessage& postfix)
{
  const Encoding encoding(Encoding::KIND_XCDR1, ENDIAN_BIG);
  size_t size = serialized_size(encoding, secure_prefix_);

  for (size_t i = 0; i < secure_submessages_.size(); ++i) {
    serialized_size(encoding, size, secure_submessages_[i]);
    const RTPS::SubmessageKind kind = secure_submessages_[i]._d();
    if (kind == RTPS::DATA || kind == RTPS::DATA_FRAG) {
      size += secure_sample_.data_length();
    }
    align(size, RTPS::SMHDR_SZ);
  }
  serialized_size(encoding, size, postfix);

  ACE_Message_Block mb(size);
  Serializer ser(&mb, encoding);
  if (!(ser << secure_prefix_)) {
    return false;
  }

  if (!ser.align_r(RTPS::SMHDR_SZ)) {
    return false;
  }

  for (size_t i = 0; i < secure_submessages_.size(); ++i) {
    if (!(ser << secure_submessages_[i])) {
      return false;
    }
    const RTPS::SubmessageKind kind = secure_submessages_[i]._d();
    if (kind == RTPS::DATA || kind == RTPS::DATA_FRAG) {
      if (!secure_sample_.write_data(ser)) {
        return false;
      }
    }
    if (!ser.align_r(RTPS::SMHDR_SZ)) {
      return false;
    }
  }
  if (!(ser << postfix)) {
    return false;
  }

  encoded.length(static_cast<unsigned int>(mb.length()));
  std::memcpy(encoded.get_buffer(), mb.rd_ptr(), mb.length());
  secure_submessages_.resize(0);

  return true;
}

bool RtpsUdpReceiveStrategy::decode_payload(ReceivedDataSample& sample,
                                            const RTPS::DataSubmessage& submsg)
{
  using namespace DDS::Security;

  static const EndpointSecurityAttributesMask MASK_PROTECT_PAYLOAD =
    ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_VALID | ENDPOINT_SECURITY_ATTRIBUTES_FLAG_IS_PAYLOAD_PROTECTED;
  const CryptoTransform_var crypto = link_->security_config()->get_crypto_transform();
  DatawriterCryptoHandle writer_crypto_handle;
  EndpointSecurityAttributesMask esa;

  if (equal_guid_prefixes(sample.header_.publication_id_.guidPrefix, receiver_.local_)) {
    writer_crypto_handle =
      link_->handle_registry()->get_local_datawriter_crypto_handle(sample.header_.publication_id_);
    esa =
      RTPS::security_attributes_to_bitmask(link_->handle_registry()->get_local_datawriter_security_attributes(sample.header_.publication_id_));
  } else {
    writer_crypto_handle =
      link_->handle_registry()->get_remote_datawriter_crypto_handle(sample.header_.publication_id_);
    esa =
      RTPS::security_attributes_to_bitmask(link_->handle_registry()->get_remote_datawriter_security_attributes(sample.header_.publication_id_));
  }

  const bool payload_protected = (esa & MASK_PROTECT_PAYLOAD) == MASK_PROTECT_PAYLOAD;

  if (writer_crypto_handle == DDS::HANDLE_NIL || !crypto || !payload_protected) {
    return true;
  }

  DDS::OctetSeq encoded = sample.copy_data(), plain, iQos;

  const Encoding encoding(Encoding::KIND_XCDR1,
    static_cast<Endianness>(submsg.smHeader.flags & 1));
  size_t iQosSize = 0;
  serialized_size(encoding, iQosSize, submsg.inlineQos);
  iQos.length(static_cast<unsigned int>(iQosSize));
  const char* iQos_raw = reinterpret_cast<const char*>(iQos.get_buffer());
  ACE_Message_Block iQosMb(iQos_raw, iQos.length());
  Serializer ser(&iQosMb, encoding);
  ser << submsg.inlineQos;

  SecurityException ex = {"", 0, 0};
  // DDS-Security: since origin authentication for payload is not yet supported
  // the reader's crypto handle is NIL here (could be multiple readers in this
  // participant)
  const bool ok = crypto->decode_serialized_payload(plain, encoded, iQos,
                                                    DDS::HANDLE_NIL,
                                                    writer_crypto_handle, ex);
  if (ok) {
    // The ReceivedDataSample's message block uses the transport's data block so it
    // can't be modified in-place, instead replace it with a new block.
    sample.clear();
    sample.append(reinterpret_cast<const char*>(plain.get_buffer()), plain.length());

    if (plain.length() > 1) {
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
  ReactorInterceptor_rch ri = link_->get_reactor_interceptor();
  ri->execute_or_enqueue(make_rch<RegisterHandler>(link_->unicast_socket().get_handle(), this, static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
  ri->execute_or_enqueue(make_rch<RegisterHandler>(link_->ipv6_unicast_socket().get_handle(), this, static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif

  return 0;
}

void
RtpsUdpReceiveStrategy::stop_i()
{
  ReactorInterceptor_rch ri = link_->get_reactor_interceptor();
  ri->execute_or_enqueue(make_rch<RemoveHandler>(link_->unicast_socket().get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
  ri->execute_or_enqueue(make_rch<RemoveHandler>(link_->ipv6_unicast_socket().get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif

  RtpsUdpInst_rch cfg = link_->config();
  if (cfg && cfg->use_multicast()) {
    ri->execute_or_enqueue(make_rch<RemoveHandler>(link_->multicast_socket().get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#ifdef ACE_HAS_IPV6
    ri->execute_or_enqueue(make_rch<RemoveHandler>(link_->ipv6_multicast_socket().get_handle(), static_cast<ACE_Reactor_Mask>(ACE_Event_Handler::READ_MASK)));
#endif
  }
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsTransportHeader& header)
{
  receiver_.reset(remote_address_, header.header_);
  if (transport_debug.log_messages) {
    message_.submessages.length(0);
    message_.hdr = header.header_;
  }

#if OPENDDS_CONFIG_SECURITY
  secure_prefix_.smHeader.submessageId = SUBMESSAGE_NONE;
#endif

  return header.valid();
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsSampleHeader& header)
{

#if OPENDDS_CONFIG_SECURITY
  if (secure_prefix_.smHeader.submessageId) {
    return header.valid();
  }
#endif

  receiver_.submsg(header.submessage_);

  // save fragmentation details for use in reassemble()
  if (header.valid() && header.submessage_._d() == RTPS::DATA_FRAG) {
    const RTPS::DataFragSubmessage& rtps = header.submessage_.data_frag_sm();
    frags_.first = rtps.fragmentStartingNum.value;
    frags_.second = RtpsSampleHeader::last_fragment(rtps);
    fragment_size_ = rtps.fragmentSize;
    total_frags_ = RtpsSampleHeader::total_fragments(rtps);
  }

  return header.valid();
}

void
RtpsUdpReceiveStrategy::begin_transport_header_processing()
{
  link_->enable_response_queue();
}

void
RtpsUdpReceiveStrategy::end_transport_header_processing()
{
  link_->disable_response_queue(false);
}

const ReceivedDataSample*
RtpsUdpReceiveStrategy::withhold_data_from(const GUID_t& sub_id)
{
  readers_withheld_.insert(sub_id);
  return recvd_sample_;
}

void
RtpsUdpReceiveStrategy::do_not_withhold_data_from(const GUID_t& sub_id)
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

bool RtpsUdpReceiveStrategy::reassemble(ReceivedDataSample& data)
{
  RtpsSampleHeader& rsh = received_sample_header();
  return reassemble_i(data, rsh);
}

bool RtpsUdpReceiveStrategy::reassemble_i(ReceivedDataSample& data, RtpsSampleHeader& rsh)
{
  using namespace RTPS;
  receiver_.fill_header(data.header_); // set publication_id_.guidPrefix
  data.fragment_size_ = fragment_size_;
  if (link_->is_target(data.header_.publication_id_) && reassembly_.reassemble(frags_, data, total_frags_)) {

    // Reassembly was successful, replace DataFrag with Data.  This doesn't have
    // to be a fully-formed DataSubmessage, just enough for this class to use
    // in deliver_sample() which ends up calling RtpsUdpDataLink::received().
    // In particular we will need the SequenceNumber, but ignore the iQoS.

    // Peek at the byte order from the encapsulation containing the payload.
    data.header_.byte_order_ = data.peek(1) & FLAG_E;

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
                                                 const GUID_t& pub_id,
                                                 ACE_CDR::ULong& cumulative_bits_added)
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
    if (x & mask) {
      const bool has_frags = reassembly_.has_frags(base + i, pub_id);
      if (has_frags) {
        x &= ~mask;
        bitmap[i / 32] = x;
        modified = true;
        --cumulative_bits_added;
      }
    }
  }
  return modified;
}

void
RtpsUdpReceiveStrategy::remove_fragments(const SequenceRange& range,
                                         const GUID_t& pub_id)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    reassembly_.data_unavailable(sn, pub_id);
  }
}

void
RtpsUdpReceiveStrategy::clear_completed_fragments(const GUID_t& pub_id)
{
  reassembly_.clear_completed(pub_id);
}

bool
RtpsUdpReceiveStrategy::has_fragments(const SequenceRange& range,
                                      const GUID_t& pub_id,
                                      FragmentInfo* frag_info)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    ACE_UINT32 total_frags = 0;
    if (reassembly_.has_frags(sn, pub_id, total_frags)) {
      if (frag_info) {
        if (total_frags > 256) {
          static const CORBA::Long empty_buffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
          OPENDDS_VECTOR(CORBA::Long) buffer(total_frags + 31 / 32, 0);
          ACE_UINT32 numBits = 0;
          size_t idx = 0;
          const ACE_UINT32 base = reassembly_.get_gaps(sn, pub_id, &buffer[0], static_cast<CORBA::ULong>(buffer.size()), numBits);
          const CORBA::ULong end = base + numBits;
          for (CORBA::ULong i = base; i <= end; i += 256) {
            const CORBA::ULong remain = end - i;
            const CORBA::ULong len = std::min(remain, static_cast<CORBA::ULong>(256));
            const CORBA::ULong len32 = (len + 31) / 32;
            const CORBA::ULong len8 = len32 * 4;
            if (std::memcmp(&buffer[idx], &empty_buffer[0], len8) != 0) {
              std::pair<SequenceNumber, RTPS::FragmentNumberSet> p;
              p.first = sn;
              p.second = RTPS::FragmentNumberSet();
              frag_info->push_back(p);
              RTPS::FragmentNumberSet& missing_frags = frag_info->back().second;
              missing_frags.numBits = len;
              missing_frags.bitmapBase.value = i;
              missing_frags.bitmap.length(len32);
              std::memcpy(missing_frags.bitmap.get_buffer(), &buffer[idx], len8);
            }
            idx += 8;
          }
        } else {
          std::pair<SequenceNumber, RTPS::FragmentNumberSet> p;
          p.first = sn;
          p.second = RTPS::FragmentNumberSet();
          frag_info->push_back(p);
          RTPS::FragmentNumberSet& missing_frags = frag_info->back().second;
          missing_frags.numBits = 0; // make sure this is a valid number before passing to get_gaps
          missing_frags.bitmap.length(8); // start at max length
          missing_frags.bitmapBase.value =
            reassembly_.get_gaps(sn, pub_id, missing_frags.bitmap.get_buffer(),
                                 8, missing_frags.numBits);
          // reduce length in case get_gaps() didn't need all that room
          missing_frags.bitmap.length((missing_frags.numBits + 31) / 32);
        }
      } else {
        return true;
      }
    }
  }
  return frag_info ? !frag_info->empty() : false;
}


// MessageReceiver nested class

RtpsUdpReceiveStrategy::MessageReceiver::MessageReceiver(const GuidPrefix_t& local)
  : directed_(false)
  , have_timestamp_(false)
{
  assign(local_, local);
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
  directed_ = false;

  unicast_reply_locator_list_.length(1);
  address_to_locator(unicast_reply_locator_list_[0], addr);
  unicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;

  multicast_reply_locator_list_.length(1);
  address_to_locator(multicast_reply_locator_list_[0], addr);
  multicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;

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
      assign(dest_guid_prefix_, id.guidPrefix);
      directed_ = true;
      return;
    }
  }
  assign(dest_guid_prefix_, local_);
  directed_ = false;
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
  assign(source_guid_prefix_, is.guidPrefix);
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
