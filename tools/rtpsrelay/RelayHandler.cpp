#include "RelayHandler.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <ace/Global_Macros.h>
#include <ace/Reactor.h>

#include <array>
#include <cstring>
#include <sstream>
#include <string>

#include <dds/DCPS/security/framework/SecurityRegistry.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpDataLink.h>
#include <dds/DCPS/security/AuthenticationBuiltInImpl.h>

namespace RtpsRelay {

// This macro makes it easier to make sure the stats are updated when an error is logged
#define HANDLER_ERROR(X) { ACE_ERROR (X); stats_reporter_.error(now); }
#define HANDLER_WARNING(X) { if (config_.log_warnings()) { ACE_ERROR (X); }; stats_reporter_.error(now); }

namespace {
  OpenDDS::STUN::Message make_bad_request_error_response(const OpenDDS::STUN::Message& a_message,
                                                         const std::string& a_reason)
  {
    OpenDDS::STUN::Message response;
    response.class_ = OpenDDS::STUN::ERROR_RESPONSE;
    response.method = a_message.method;
    std::memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(OpenDDS::STUN::make_error_code(OpenDDS::STUN::BAD_REQUEST, a_reason));
    response.append_attribute(OpenDDS::STUN::make_fingerprint());
    return response;
  }

  OpenDDS::STUN::Message make_unknown_attributes_error_response(const OpenDDS::STUN::Message& a_message,
                                                                const std::vector<OpenDDS::STUN::AttributeType>& a_unknown_attributes)
  {
    OpenDDS::STUN::Message response;
    response.class_ = OpenDDS::STUN::ERROR_RESPONSE;
    response.method = a_message.method;
    std::memcpy(response.transaction_id.data, a_message.transaction_id.data, sizeof(a_message.transaction_id.data));
    response.append_attribute(OpenDDS::STUN::make_error_code(OpenDDS::STUN::UNKNOWN_ATTRIBUTE, "Unknown Attributes"));
    response.append_attribute(OpenDDS::STUN::make_unknown_attributes(a_unknown_attributes));
    response.append_attribute(OpenDDS::STUN::make_fingerprint());
    return response;
  }
}

RelayHandler::RelayHandler(const Config& config,
                           const std::string& name,
                           Port port,
                           ACE_Reactor* reactor,
                           HandlerStatisticsReporter& stats_reporter,
                           OpenDDS::DCPS::Lockable_Message_Block_Ptr::Lock_Policy message_block_locking)
  : ACE_Event_Handler(reactor)
  , config_(config)
  , name_(name)
  , port_(port)
  , stats_reporter_(stats_reporter)
  , message_block_locking_(message_block_locking)
{
}

int RelayHandler::open(const ACE_INET_Addr& address)
{
  if (socket_.open(address) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::open %C failed to open socket on '%C'\n",
               name_.c_str(), OpenDDS::DCPS::LogAddr(address).c_str()));
    return -1;
  }
  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::open %C failed to enable ACE_NONBLOCK\n", name_.c_str()));
    return -1;
  }

  int buffer_size = config_.buffer_size();

  if (socket_.set_option(SOL_SOCKET,
                         SO_SNDBUF,
                         (void *) &buffer_size,
                         sizeof(buffer_size)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::open %C failed to set the send buffer size to %d errno %m\n", name_.c_str(), buffer_size));
    return -1;
  }

  if (socket_.set_option(SOL_SOCKET,
                         SO_RCVBUF,
                         (void *) &buffer_size,
                         sizeof(buffer_size)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::open %C failed to set the receive buffer size to %d errno %m\n", name_.c_str(), buffer_size));
    return -1;
  }

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::open %C failed to register READ_MASK handler\n", name_.c_str()));
    return -1;
  }

  return 0;
}

int RelayHandler::handle_input(ACE_HANDLE handle)
{
  OpenDDS::DCPS::ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager(), READ_MASK, handle_to_int(handle));

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  ACE_INET_Addr remote;
  int inlen = 65536; // Default to maximum datagram size.

#ifdef FIONREAD
  if (ACE_OS::ioctl (handle,
                     FIONREAD,
                     &inlen) == -1) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::handle_input %C failed to get available byte count: %m\n", name_.c_str()));
    return 0;
  }
#else
  ACE_UNUSED_ARG(handle);
#endif

  if (inlen < 0) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::handle_input %C available byte count is negative\n", name_.c_str()));
    return 0;
  }

  // Allocate at least one byte so that recv cannot return early.
  const auto n = static_cast<size_t>(std::max(inlen, 1));
  OpenDDS::DCPS::Lockable_Message_Block_Ptr buffer(new ACE_Message_Block(n), message_block_locking_);

  const auto bytes = socket_.recv(buffer->wr_ptr(), buffer->space(), remote);

  if (bytes < 0) {
    if (errno == ECONNRESET) {
      // Sending to a non-existent client may result in an ICMP message that is delievered as connection reset.
      return 0;
    }

    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::handle_input %C failed to recv: %m\n", name_.c_str()));
    return 0;
  } else if (bytes == 0) {
    // Okay.  Empty datagram.
    HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: RelayHandler::handle_input %C received an empty datagram from %C\n",
                     name_.c_str(), OpenDDS::DCPS::LogAddr(remote).c_str()));
    return 0;
  }

  buffer->length(static_cast<size_t>(bytes));
  MessageType type = MessageType::Unknown;
  const CORBA::ULong generated_messages = process_message(remote, now, buffer, type);
  stats_reporter_.max_gain(generated_messages, now);
  stats_reporter_.input_message(static_cast<size_t>(bytes),
    OpenDDS::DCPS::MonotonicTimePoint::now() - now, now, type);

  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE handle)
{
  auto& statusManager = TheServiceParticipant->get_thread_status_manager();
  OpenDDS::DCPS::ThreadStatusManager::Event ev(statusManager, WRITE_MASK, handle_to_int(handle));

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, outgoing_mutex_, 0);
  OpenDDS::DCPS::ThreadStatusManager::Event evLocked(statusManager, WRITE_MASK | DONT_CALL, handle_to_int(handle));

  if (!outgoing_.empty()) {
    const auto& out = outgoing_.front();
    size_t total_bytes;

    if (send_i(out, total_bytes) < 0) {
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::handle_output %C failed to send to %C: %m\n",
                     name_.c_str(), OpenDDS::DCPS::LogAddr(out.address).c_str()));
      const auto new_now = OpenDDS::DCPS::MonotonicTimePoint::now();
      stats_reporter_.dropped_message(total_bytes, new_now - now, new_now - out.timestamp, now, out.type);
    } else {
      const auto new_now = OpenDDS::DCPS::MonotonicTimePoint::now();
      stats_reporter_.output_message(total_bytes, new_now - now, new_now - out.timestamp, now, out.type);
    }

    outgoing_.pop();
  }

  if (outgoing_.empty()) {
    reactor()->remove_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::enqueue_message(const ACE_INET_Addr& addr,
                                   const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                   const OpenDDS::DCPS::MonotonicTimePoint& now,
                                   MessageType type)
{
  const Element out(addr, msg, now, type);
  if (config_.synchronous_output()) {
    size_t total_bytes;

    if (send_i(out, total_bytes) < 0) {
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayHandler::enqueue_message %C failed to send to %C: %m\n",
                     name_.c_str(), OpenDDS::DCPS::LogAddr(out.address).c_str()));
      stats_reporter_.dropped_message(total_bytes, OpenDDS::DCPS::TimeDuration::zero_value, OpenDDS::DCPS::TimeDuration::zero_value, now, out.type);
    } else {
      stats_reporter_.output_message(total_bytes, OpenDDS::DCPS::TimeDuration::zero_value, OpenDDS::DCPS::TimeDuration::zero_value, now, out.type);
    }

  } else {
    ACE_GUARD(ACE_Thread_Mutex, g, outgoing_mutex_);

    const auto empty = outgoing_.empty();

    outgoing_.push(out);
    stats_reporter_.max_queue_size(outgoing_.size(), now);
    if (empty) {
      reactor()->register_handler(this, WRITE_MASK);
    }
  }
}

ssize_t RelayHandler::send_i(const Element& out,
                             size_t& total_bytes)
{
  const int BUFFERS_SIZE = 2;
  iovec buffers[BUFFERS_SIZE];
  total_bytes = 0;

  int idx = 0;
  for (ACE_Message_Block* block = out.message_block.get(); block && idx < BUFFERS_SIZE; block = block->cont(), ++idx) {
    buffers[idx].iov_base = block->rd_ptr();
#ifdef _MSC_VER
#pragma warning(push)
    // iov_len is 32-bit on 64-bit VC++, but we don't want a cast here
    // since on other platforms iov_len is 64-bit
#pragma warning(disable : 4267)
#endif
    buffers[idx].iov_len = block->length();
    total_bytes += buffers[idx].iov_len;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  }

  return socket_.send(buffers, idx, out.address, 0);
}

VerticalHandler::VerticalHandler(const Config& config,
                                 const std::string& name,
                                 Port port,
                                 const ACE_INET_Addr& horizontal_address,
                                 ACE_Reactor* reactor,
                                 const GuidPartitionTable& guid_partition_table,
                                 const RelayPartitionTable& relay_partition_table,
                                 GuidAddrSet& guid_addr_set,
                                 const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                                 const DDS::Security::CryptoTransform_var& crypto,
                                 const ACE_INET_Addr& application_participant_addr,
                                 HandlerStatisticsReporter& stats_reporter,
                                 OpenDDS::DCPS::Lockable_Message_Block_Ptr::Lock_Policy message_block_locking)
  : RelayHandler(config, name, port, reactor, stats_reporter, message_block_locking)
  , guid_partition_table_(guid_partition_table)
  , relay_partition_table_(relay_partition_table)
  , guid_addr_set_(guid_addr_set)
  , horizontal_handler_(nullptr)
  , spdp_handler_(nullptr)
  , application_participant_addr_(application_participant_addr)
  , horizontal_address_(horizontal_address)
  , horizontal_address_str_(OpenDDS::DCPS::LogAddr(horizontal_address).c_str())
  , rtps_discovery_(rtps_discovery)
  , crypto_(crypto)
  , application_participant_crypto_handle_(rtps_discovery_->get_crypto_handle(config.application_domain(), config.application_participant_guid()))
{
  ACE_UNUSED_ARG(crypto);
}

void VerticalHandler::stop()
{
  reactor()->cancel_timer(this);
}

void VerticalHandler::venqueue_message(const ACE_INET_Addr& addr,
                                       ParticipantStatisticsReporter& to_psr,
                                       const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       MessageType type)
{
  enqueue_message(addr, msg, now, type);
  to_psr.output_message(msg->length(), type);
}

CORBA::ULong VerticalHandler::process_message(const ACE_INET_Addr& remote_address,
                                              const OpenDDS::DCPS::MonotonicTimePoint& now,
                                              const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                              MessageType& type)
{
  auto& statusManager = TheServiceParticipant->get_thread_status_manager();
  const auto handle_as_int = handle_to_int(get_handle());
  const auto msg_len = msg->length();
  {
    GuidAddrSet::Proxy proxy(guid_addr_set_);
    OpenDDS::DCPS::ThreadStatusManager::Event evLocked(statusManager, READ_MASK | DONT_CALL, handle_as_int);
    proxy.maintain_admission_queue(now);
    if (!proxy.check_address(remote_address)) {
      stats_reporter_.ignored_message(msg_len, now, type);
      return 0;
    }
  }

  AddrPort addr_port(remote_address, port());

  if (msg_len >= 4 && ACE_OS::memcmp(msg->rd_ptr(), "RTPS", 4) == 0) {
    type = MessageType::Rtps;

    OpenDDS::RTPS::MessageParser mp(*msg);
    OpenDDS::DCPS::GUID_t src_guid;
    GuidSet to;

    if (!parse_message(mp, msg, src_guid, to, true, now)) {
      HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C failed to parse_message from %C\n",
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      return 0;
    }

    const bool from_application_participant =
      (remote_address == application_participant_addr_) &&
      (src_guid == config_.application_participant_guid());

    GuidAddrSet::Proxy proxy(guid_addr_set_);
    OpenDDS::DCPS::ThreadStatusManager::Event evLocked(statusManager, READ_MASK | SIGNAL_MASK, handle_as_int);
    record_activity(proxy, addr_port, now, src_guid, type, msg_len, from_application_participant);

    cache_message(proxy, src_guid, to, msg, now);

    bool admitted = false;
    if (proxy.ignore_rtps(from_application_participant, src_guid, now, admitted)) {
      stats_reporter_.ignored_message(msg_len, now, type);
      return 0;
    }

    CORBA::ULong sent = 0;

    if (admitted && spdp_handler_) {
      sent += spdp_handler_->send_to_application_participant(proxy, src_guid, now);
    }

    bool send_to_application_participant = false;
    if (do_normal_processing(proxy, remote_address, src_guid, to, admitted, send_to_application_participant, msg, now, sent)) {
      StringSet to_partitions;
      guid_partition_table_.lookup(to_partitions, src_guid);
      sent += send(proxy, src_guid, to_partitions, to, send_to_application_participant, msg, now);
    }
    return sent;
  } else {
    // Assume STUN.
    type = MessageType::Stun;

    OpenDDS::DCPS::Serializer serializer(msg.get(), OpenDDS::STUN::encoding);
    OpenDDS::STUN::Message message;
    message.block = msg.get();
    if (!(serializer >> message)) {
      HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C Could not deserialize STUN message from %C\n",
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      return 0;
    }

    std::vector<OpenDDS::STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

    if (!unknown_attributes.empty()) {
      HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C Unknown comprehension required attributes from %C\n",
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      send(remote_address, make_unknown_attributes_error_response(message, unknown_attributes), now);
      return 1;
    }

    if (!message.has_fingerprint()) {
      HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C No FINGERPRINT attribute from %C\n",
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      send(remote_address, make_bad_request_error_response(message, "Bad Request: FINGERPRINT must be pesent"), now);
      return 1;
    }

    bool has_guid = false;
    OpenDDS::DCPS::GUID_t src_guid;
    if (message.get_guid_prefix(src_guid.guidPrefix)) {
      src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      has_guid = true;
    }

    OpenDDS::STUN::Message response;
    bool response_needed = true;

    switch (message.method) {
    case OpenDDS::STUN::BINDING:
      {
        if (message.class_ == OpenDDS::STUN::REQUEST) {
          response.class_ = OpenDDS::STUN::SUCCESS_RESPONSE;
          response.method = OpenDDS::STUN::BINDING;
          std::memcpy(response.transaction_id.data, message.transaction_id.data, sizeof(message.transaction_id.data));
          response.append_attribute(OpenDDS::STUN::make_mapped_address(remote_address));
          response.append_attribute(OpenDDS::STUN::make_xor_mapped_address(remote_address));
          response.append_attribute(OpenDDS::STUN::make_fingerprint());
          response_needed = true;
        } else if (message.class_ == OpenDDS::STUN::INDICATION) {
          // Do nothing.
        } else {
          HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C Unknown STUN message class from %C\n",
            name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
        }
        break;
      }

    default:
      // Unknown method.  Stop processing.
      HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::process_message %C Unknown STUN method from %C\n",
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      response = make_bad_request_error_response(message, "Bad Request: Unknown method");
      response_needed = true;
      break;
    }

    CORBA::ULong messages_sent = 0;

    if (has_guid) {
      GuidAddrSet::Proxy proxy(guid_addr_set_);
      OpenDDS::DCPS::ThreadStatusManager::Event evLocked(statusManager, READ_MASK | GROUP_QOS_MASK, handle_as_int);

      const bool from_application_participant =
        (remote_address == application_participant_addr_) &&
        (src_guid == config_.application_participant_guid());
      bool allow_stun_responses = true;

      ParticipantStatisticsReporter& from_psr =
        record_activity(proxy, addr_port, now, src_guid, type, msg_len, from_application_participant, &allow_stun_responses);

      if (allow_stun_responses && response_needed) {
        const auto bytes_sent = send(remote_address, std::move(response), now);
        ++messages_sent;
        if (bytes_sent) {
          from_psr.output_message(bytes_sent, type);
        }
      }

      bool admitted = false;
      proxy.ignore_rtps(from_application_participant, src_guid, now, admitted);
      if (admitted && spdp_handler_) {
        messages_sent += spdp_handler_->send_to_application_participant(proxy, src_guid, now);
      }
    } else if (response_needed) {
      send(remote_address, std::move(response), now);
      ++messages_sent;
    }

    return messages_sent;
  }
}

ParticipantStatisticsReporter&
VerticalHandler::record_activity(GuidAddrSet::Proxy& proxy,
                                 const AddrPort& remote_address,
                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                 const OpenDDS::DCPS::GUID_t& src_guid,
                                 MessageType msg_type,
                                 const size_t& msg_len,
                                 bool from_application_participant,
                                 bool* allow_stun_responses)
{
  return proxy.record_activity(remote_address, now, src_guid, msg_type, msg_len, from_application_participant, allow_stun_responses, *this);
}

bool VerticalHandler::parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                                    const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                    OpenDDS::DCPS::GUID_t& src_guid,
                                    GuidSet& to,
                                    bool check_submessages,
                                    const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_UNUSED_ARG(msg);

  if (!message_parser.parseHeader()) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C failed to deserialize RTPS header\n", name_.c_str()));
    return false;
  }

  const auto& header = message_parser.header();
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  bool valid_info_dst = false;
  bool all_valid_info_dst = true;

  while (message_parser.parseSubmessageHeader()) {
    const auto submessage_header = message_parser.submessageHeader();

    // Check that every non-info submessage has a "valid" (not unknown) destination.
    switch (submessage_header.submessageId) {
    case OpenDDS::RTPS::INFO_DST: {
      OpenDDS::DCPS::GUID_t dest = OpenDDS::DCPS::GUID_UNKNOWN;
      OpenDDS::DCPS::GuidPrefix_t_forany guidPrefix(dest.guidPrefix);
      if (!(message_parser >> guidPrefix)) {
        HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C failed to deserialize INFO_DST from %C\n", name_.c_str(), guid_to_string(src_guid).c_str()));
        return false;
      }
      valid_info_dst = dest != OpenDDS::DCPS::GUID_UNKNOWN;
      if (valid_info_dst) {
        dest.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
        to.insert(dest);
      }
      break;
    }
    case OpenDDS::RTPS::INFO_TS:
    case OpenDDS::RTPS::INFO_SRC:
    case OpenDDS::RTPS::INFO_REPLY_IP4:
    case OpenDDS::RTPS::INFO_REPLY:
      break;
    default:
      all_valid_info_dst = all_valid_info_dst && valid_info_dst;
    }

    if (check_submessages) {
      switch (submessage_header.submessageId) {
      case OpenDDS::RTPS::SRTPS_PREFIX:
        {
          if (application_participant_crypto_handle_ == DDS::HANDLE_NIL) {
            HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C no crypto handle for application participant\n", name_.c_str()));
            return false;
          }

          const DDS::Security::ParticipantCryptoHandle remote_crypto_handle = rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid(), src_guid);
          if (remote_crypto_handle == DDS::HANDLE_NIL) {
            HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::parse_message %C no crypto handle for message from %C\n", name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }

          DDS::OctetSeq encoded_buffer, plain_buffer;
          DDS::Security::SecurityException ex;

          if (msg->cont() != nullptr) {
            HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C does not support message block chaining\n", name_.c_str()));
            return false;
          }

          encoded_buffer.length(static_cast<CORBA::ULong>(msg->length()));
          std::memcpy(encoded_buffer.get_buffer(), msg->rd_ptr(), msg->length());

          if (!crypto_->decode_rtps_message(plain_buffer, encoded_buffer, application_participant_crypto_handle_, remote_crypto_handle, ex)) {
            HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::parse_message %C message from %C could not be verified [%d.%d]: \"%C\"\n", name_.c_str(), guid_to_string(src_guid).c_str(), ex.code, ex.minor_code, ex.message.in()));
            return false;
          }

          OpenDDS::RTPS::MessageParser mp(plain_buffer);
          to.clear();
          return parse_message(mp, msg, src_guid, to, false, now);
        }
        break;
      case OpenDDS::RTPS::DATA:
      case OpenDDS::RTPS::DATA_FRAG:
        {
          unsigned short extraFlags;
          unsigned short octetsToInlineQos;
          if (!(message_parser >> extraFlags) ||
              !(message_parser >> octetsToInlineQos)) {
            HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n", name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }
        }
        // Fall through.
      case OpenDDS::RTPS::HEARTBEAT:
      case OpenDDS::RTPS::HEARTBEAT_FRAG:
      case OpenDDS::RTPS::GAP:
      case OpenDDS::RTPS::ACKNACK:
      case OpenDDS::RTPS::NACK_FRAG:
        {
          OpenDDS::DCPS::EntityId_t readerId;
          OpenDDS::DCPS::EntityId_t writerId;
          if (!(message_parser >> readerId) ||
              !(message_parser >> writerId)) {
            HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n", name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }
          if (rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid()) != DDS::HANDLE_NIL &&
              !(OpenDDS::DCPS::RtpsUdpDataLink::separate_message(writerId) ||
                writerId == OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER)) {
            HANDLER_WARNING((LM_WARNING, "(%P|%t) WARNING: VerticalHandler::parse_message %C submessage from %C with id %d could not be verified writerId=%02X%02X%02X%02X\n", name_.c_str(), guid_to_string(src_guid).c_str(), submessage_header.submessageId, writerId.entityKey[0], writerId.entityKey[1], writerId.entityKey[2], writerId.entityKind));
            return false;
          }
        }
        break;
      default:
        break;
      }
    }

    message_parser.skipSubmessageContent();
  }

  if (message_parser.remaining() != 0) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: VerticalHandler::parse_message %C trailing bytes from %C\n", name_.c_str(), guid_to_string(src_guid).c_str()));
    return false;
  }

  if (!all_valid_info_dst) {
    to.clear();
  }

  return true;
}

CORBA::ULong VerticalHandler::send(GuidAddrSet::Proxy& proxy,
                                   const OpenDDS::DCPS::GUID_t& src_guid,
                                   const StringSet& to_partitions,
                                   const GuidSet& to_guids,
                                   bool send_to_application_participant,
                                   const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                   const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  AddressSet address_set;
  populate_address_set(address_set, to_partitions);
  const auto type = MessageType::Rtps;

  CORBA::ULong sent = 0;
  for (const auto& addr : address_set) {
    if (addr != horizontal_address_) {
      horizontal_handler_->enqueue_or_send_message(addr, to_partitions, to_guids, msg, now);
      ++sent;
    } else {
      // Local recipients.
      GuidSet guids;
      guid_partition_table_.lookup(guids, to_partitions, to_guids);
      for (const auto& guid : guids) {
        if (guid == src_guid) {
          continue;
        }
        auto p = proxy.find(guid);
        if (p != proxy.end()) {
          p->second.foreach_addr(port(),
                                 [&](const ACE_INET_Addr& address) {
                                   venqueue_message(address,
                                                    *p->second.select_stats_reporter(port()), msg, now, type);
                                   ++sent;
          });
        }
      }
    }
  }

  if (send_to_application_participant) {
    venqueue_message(application_participant_addr_,
      proxy.participant_statistics_reporter(config_.application_participant_guid(), now, port()),
      msg, now, type);
    ++sent;
  }

  return sent;
}

size_t VerticalHandler::send(const ACE_INET_Addr& addr,
                             OpenDDS::STUN::Message message,
                             const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::STUN;
  const auto type = MessageType::Stun;
  const size_t length = HEADER_SIZE + message.length();
  Message_Block_Shared_Ptr block(new ACE_Message_Block(length));
  Serializer serializer(block.get(), encoding);
  message.block = block.get();
  serializer << message;
  RelayHandler::enqueue_message(addr, block, now, type);
  return length;
}

void VerticalHandler::populate_address_set(AddressSet& address_set,
                                           const StringSet& to_partitions)
{
  relay_partition_table_.lookup(address_set, to_partitions, horizontal_handler_->name());
}

HorizontalHandler::HorizontalHandler(const Config& config,
                                     const std::string& name,
                                     Port port,
                                     ACE_Reactor* reactor,
                                     const GuidPartitionTable& guid_partition_table,
                                     HandlerStatisticsReporter& stats_reporter)
  : RelayHandler(config, name, port, reactor, stats_reporter)
  , guid_partition_table_(guid_partition_table)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::enqueue_or_send_message(const ACE_INET_Addr& addr,
                                                const StringSet& to_partitions,
                                                const GuidSet& to_guids,
                                                const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                                const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  using namespace OpenDDS::DCPS;

  const Encoding encoding(Encoding::KIND_XCDR1);

  RelayHeader relay_header;
  auto& tp = relay_header.to_partitions();
  for (const auto& p : to_partitions) {
    tp.push_back(p);
  }
  auto& tg = relay_header.to_guids();
  for (const auto& g : to_guids) {
    tg.push_back(rtps_guid_to_relay_guid(g));
  }

  const size_t size = serialized_size(encoding, relay_header);
  const size_t total_size = size + msg->length();
  if (total_size > TransportSendStrategy::UDP_MAX_MESSAGE_SIZE) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: HorizontalHandler::enqueue_message %C header and message too large (%B > %B)\n", name_.c_str(), total_size, static_cast<size_t>(TransportSendStrategy::UDP_MAX_MESSAGE_SIZE)));
    return;
  }

  Lockable_Message_Block_Ptr header_block(new ACE_Message_Block(size));
  Serializer ser(header_block.get(), encoding);
  ser << relay_header;
  header_block.lockable_cont(msg);
  RelayHandler::enqueue_message(addr, header_block, now, MessageType::Rtps);
}

CORBA::ULong HorizontalHandler::process_message(const ACE_INET_Addr&,
                                                const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                                MessageType& type)
{
  type = MessageType::Rtps;

  OpenDDS::RTPS::MessageParser mp(*msg);

  const size_t size_before_header = mp.remaining();

  RelayHeader relay_header;
  if (!(mp >> relay_header)) {
    HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: HorizontalHandler::process_message %C failed to deserialize Relay header\n", name_.c_str()));
    return 0;
  }

  const size_t size_after_header = mp.remaining();
  msg->rd_ptr(size_before_header - size_after_header);

  GuidSet guids;
  const auto to_guids = relay_guids_to_set(relay_header.to_guids());

  guid_partition_table_.lookup(guids, relay_header.to_partitions(), to_guids);
  GuidAddrSet::Proxy proxy(vertical_handler_->guid_addr_set());
  OpenDDS::DCPS::ThreadStatusManager::Event evLocked(TheServiceParticipant->get_thread_status_manager(),
    READ_MASK | DONT_CALL, handle_to_int(get_handle()));

  CORBA::ULong sent = 0;
  for (const auto& guid : guids) {
    const auto p = proxy.find(guid);
    if (p != proxy.end()) {
      p->second.foreach_addr(port(),
                             [&](const ACE_INET_Addr& addr) {
                               vertical_handler_->venqueue_message(addr,
                                                                   *p->second.select_stats_reporter(port()), msg, now, type);
                               ++sent;
                             });
    }
  }

  return sent;
}

SpdpHandler::SpdpHandler(const Config& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         const GuidPartitionTable& guid_partition_table,
                         const RelayPartitionTable& relay_partition_table,
                         GuidAddrSet& guid_addr_set,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const DDS::Security::CryptoTransform_var& crypto,
                         const ACE_INET_Addr& application_participant_addr,
                         HandlerStatisticsReporter& stats_reporter)
  : VerticalHandler(config, name, SPDP, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, application_participant_addr, stats_reporter,
                    OpenDDS::DCPS::Lockable_Message_Block_Ptr::Lock_Policy::Use_Lock)
{}

namespace {
  std::string extract_common_name(const ACE_Message_Block& msg,
                                  const OpenDDS::DCPS::GUID_t& src_guid)
  {
    OpenDDS::RTPS::MessageParser message_parser(msg);
    if (!message_parser.parseHeader()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() could not parse header\n"));
      return "";
    }

    while (message_parser.parseSubmessageHeader()) {
      const auto submessage_header = message_parser.submessageHeader();
      if (submessage_header.submessageId == OpenDDS::RTPS::DATA) {
        unsigned short extraFlags;
        unsigned short octetsToInlineQos;
        OpenDDS::DCPS::EntityId_t readerId;
        OpenDDS::DCPS::EntityId_t writerId;
        OpenDDS::RTPS::SequenceNumber_t writerSequenceNumber;

        if (!(message_parser >> extraFlags) ||
            !(message_parser >> octetsToInlineQos) ||
            !(message_parser >> readerId) ||
            !(message_parser >> writerId) ||
            !(message_parser >> writerSequenceNumber)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() could not parse submessage\n"));
          return "";
        }

        if (writerId != OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
          // Not our message: this could be the same multicast group used
          // for SEDP and other traffic.
          break;
        }

        if (!message_parser.serializer().skip(octetsToInlineQos - 16)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() could not parse submessage\n"));
          return "";
        }

        OpenDDS::RTPS::ParameterList inlineQos;
        if ((submessage_header.flags & OpenDDS::RTPS::FLAG_Q) && !(message_parser >> inlineQos)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() could not parse submessage\n"));
          return "";
        }

        if (submessage_header.flags & OpenDDS::RTPS::FLAG_D) {
          OpenDDS::RTPS::ParameterList plist;
          OpenDDS::DCPS::EncapsulationHeader encap;
          OpenDDS::DCPS::Encoding enc;
          if (!(message_parser >> encap) || !to_encoding(enc, encap, OpenDDS::DCPS::MUTABLE)
                                         || enc.kind() != OpenDDS::DCPS::Encoding::KIND_XCDR1) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() - failed to deserialize encapsulation header for SPDP from %C\n",
                       OpenDDS::DCPS::LogGuid(src_guid).c_str()));
            return "";
          }
          message_parser.serializer().encoding(enc);
          if (!(message_parser >> plist)) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: extract_common_name() - failed to deserialize data payload for SPDP\n"));
            return "";
          }

          for (CORBA::ULong i = 0; i != plist.length(); ++i) {
            const OpenDDS::RTPS::Parameter& param = plist[i];
            if (param._d() == DDS::Security::PID_IDENTITY_TOKEN) {
              const DDS::Security::IdentityToken& idt = param.identity_token();
              if (std::strcmp(OpenDDS::Security::Identity_Status_Token_Class_Id, idt.class_id.in()) == 0) {
                for (CORBA::ULong j = 0; j != idt.properties.length(); ++j) {
                  const DDS::Property_t& prop = idt.properties[j];
                  if (std::strcmp(OpenDDS::Security::dds_cert_sn, prop.name.in()) == 0) {
                    return std::string(prop.value.in());
                  }
                }
              }
            }
          }

        }
      }

      message_parser.skipSubmessageContent();
    }

    return "";
  }
}

void SpdpHandler::cache_message(GuidAddrSet::Proxy& proxy,
                                const OpenDDS::DCPS::GUID_t& src_guid,
                                const GuidSet& to,
                                const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  if (to.empty()) {
    const auto pos = proxy.find(src_guid);
    if (pos != proxy.end()) {
      if (!pos->second.seen_spdp_message) {
        pos->second.common_name = extract_common_name(*msg, src_guid);
        if (config_.log_activity()) {
          ACE_DEBUG((LM_INFO, "(%P|%t) INFO: SpdpHandler::cache_message %C got first SPDP %C into session dds.cert.sn %C\n",
                     guid_to_string(src_guid).c_str(),
                     pos->second.get_session_time(now).sec_str().c_str(),
                     pos->second.common_name.c_str()));
        }
        pos->second.spdp_message = msg;
        pos->second.seen_spdp_message = true;
      }
    }
  }
}

bool SpdpHandler::do_normal_processing(GuidAddrSet::Proxy& proxy,
                                       const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::GUID_t& src_guid,
                                       GuidSet& to,
                                       bool admitted,
                                       bool& send_to_application_participant,
                                       const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       CORBA::ULong& sent)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: SpdpHandler::do_normal_processing application participant imposter detected AP %C %C Remote %C %C\n",
                     guid_to_string(config_.application_participant_guid()).c_str(),
                     OpenDDS::DCPS::LogAddr(application_participant_addr_).c_str(),
                     guid_to_string(src_guid).c_str(),
                     OpenDDS::DCPS::LogAddr(remote).c_str()));
      proxy.reject_address(remote, now);
      return false;
    }

    // SPDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = proxy.find(guid);
        if (pos != proxy.end()) {
          pos->second.foreach_addr(port(),
                                   [&](const ACE_INET_Addr& addr) {
                                     venqueue_message(addr,
                                                      *pos->second.select_stats_reporter(port()), msg, now, MessageType::Rtps);
                                     ++sent;
          });
        }
      }
    } else {
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: SpdpHandler::do_normal_processing dropping non-directed SPDP message from application participant\n"));
      return false;
    }

    return false;
  }

  // SPDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    // Don't double send when admitted.
    send_to_application_participant = !admitted;
    to.erase(config_.application_participant_guid());
  }

  return true;
}

CORBA::ULong SpdpHandler::send_to_application_participant(GuidAddrSet::Proxy& proxy,
                                                          const OpenDDS::DCPS::GUID_t& guid,
                                                          const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  const auto pos = proxy.find(guid);
  if (pos == proxy.end()) {
    return 0;
  }

  if (!pos->second.spdp_message) {
    return 0;
  }

  const auto ret = send(proxy, guid, StringSet(), GuidSet(), true, pos->second.spdp_message, now);
  pos->second.spdp_message = OpenDDS::DCPS::Lockable_Message_Block_Ptr{};
  return ret;
}

SedpHandler::SedpHandler(const Config& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         const GuidPartitionTable& guid_partition_table,
                         const RelayPartitionTable& relay_partition_table,
                         GuidAddrSet& guid_addr_set,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const DDS::Security::CryptoTransform_var& crypto,
                         const ACE_INET_Addr& application_participant_addr,
                         HandlerStatisticsReporter& stats_reporter)
: VerticalHandler(config, name, SEDP, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, application_participant_addr, stats_reporter)
{}

bool SedpHandler::do_normal_processing(GuidAddrSet::Proxy& proxy,
                                       const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::GUID_t& src_guid,
                                       GuidSet& to,
                                       bool /*admitted*/,
                                       bool& send_to_application_participant,
                                       const OpenDDS::DCPS::Lockable_Message_Block_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       CORBA::ULong& sent)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: SedpHandler::do_normal_processing application participant imposter detected AP %C %C Remote %C %C\n",
                     guid_to_string(config_.application_participant_guid()).c_str(),
                     OpenDDS::DCPS::LogAddr(application_participant_addr_).c_str(),
                     guid_to_string(src_guid).c_str(),
                     OpenDDS::DCPS::LogAddr(remote).c_str()));
      proxy.reject_address(remote, now);
      return false;
    }

    // SEDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = proxy.find(guid);
        if (pos != proxy.end()) {
          pos->second.foreach_addr(port(),
                                   [&](const ACE_INET_Addr& addr) {
                                     venqueue_message(addr,
                                                      *pos->second.select_stats_reporter(port()), msg, now, MessageType::Rtps);
                                     ++sent;
                                   });
        }
      }
    } else {
      HANDLER_ERROR((LM_ERROR, "(%P|%t) ERROR: SedpHandler::do_normal_processing dropping non-directed SEDP message from application participant\n"));
      return false;
    }

    return false;
  }

  // SEDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    send_to_application_participant = true;
    to.erase(config_.application_participant_guid());
  }

  return true;
}

DataHandler::DataHandler(const Config& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         const GuidPartitionTable& guid_partition_table,
                         const RelayPartitionTable& relay_partition_table,
                         GuidAddrSet& guid_addr_set,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const DDS::Security::CryptoTransform_var& crypto,
                         HandlerStatisticsReporter& stats_reporter)
: VerticalHandler(config, name, DATA, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, ACE_INET_Addr(), stats_reporter)
{}

}
