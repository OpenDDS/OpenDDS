#include "RelayHandler.h"

#include "lib/RelayTypeSupportImpl.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
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

#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/SecurityRegistry.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpDataLink.h>
#endif

namespace RtpsRelay {

// This macro makes it easier to make sure the stats are updated when an error is logged
#define HANDLER_ERROR(X) { ACE_ERROR (X); stats_reporter_.error(now); }
#define HANDLER_WARNING(X) { if (config_.log_warnings()) { ACE_ERROR (X); }; stats_reporter_.error(now); }

#ifdef OPENDDS_SECURITY
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
#endif

RelayHandler::RelayHandler(const Config& config,
                           const std::string& name,
                           ACE_Reactor* reactor,
                           HandlerStatisticsReporter& stats_reporter)
  : ACE_Event_Handler(reactor)
  , config_(config)
  , name_(name)
  , stats_reporter_(stats_reporter)
{
}

int RelayHandler::open(const ACE_INET_Addr& address)
{
  if (socket_.open(address) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::open %C failed to open socket on '%C'\n"),
               name_.c_str(), OpenDDS::DCPS::LogAddr(address).c_str()));
    return -1;
  }
  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::open %C failed to enable ACE_NONBLOCK\n"), name_.c_str()));
    return -1;
  }

  int send_buffer_size = 16384;
#ifdef ACE_DEFAULT_MAX_SOCKET_BUFSIZ
  send_buffer_size = ACE_DEFAULT_MAX_SOCKET_BUFSIZ;
#endif

  if (socket_.set_option(SOL_SOCKET,
                         SO_SNDBUF,
                         (void *) &send_buffer_size,
                         sizeof(send_buffer_size)) < 0
      && errno != ENOTSUP) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::open %C failed to set the send buffer size to %d errno %m\n"), name_.c_str(), send_buffer_size));
    return -1;
  }

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::open %C failed to register READ_MASK handler\n"), name_.c_str()));
    return -1;
  }

  return 0;
}

int RelayHandler::handle_input(ACE_HANDLE handle)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  ACE_INET_Addr remote;
  int inlen = 65536; // Default to maximum datagram size.

#ifdef FIONREAD
  if (ACE_OS::ioctl (handle,
                     FIONREAD,
                     &inlen) == -1) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::handle_input %C failed to get available byte count: %m\n"), name_.c_str()));
    return 0;
  }
#else
  ACE_UNUSED_ARG(handle);
#endif

  if (inlen < 0) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::handle_input %C available byte count is negative\n"), name_.c_str()));
    return 0;
  }

  // Allocate at least one byte so that recv cannot return early.
  OpenDDS::DCPS::Message_Block_Shared_Ptr buffer(new ACE_Message_Block(std::max(inlen, 1)));

  const auto bytes = socket_.recv(buffer->wr_ptr(), buffer->space(), remote);

  if (bytes < 0) {
    if (errno == ECONNRESET) {
      // Sending to a non-existent client may result in an ICMP message that is delievered as connection reset.
      return 0;
    }

    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::handle_input %C failed to recv: %m\n"), name_.c_str()));
    return 0;
  } else if (bytes == 0) {
    // Okay.  Empty datagram.
    HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: RelayHandler::handle_input %C received an empty datagram from %C\n"),
                     name_.c_str(), OpenDDS::DCPS::LogAddr(remote).c_str()));
    return 0;
  }

  buffer->length(bytes);
  const CORBA::ULong generated_messages = process_message(remote, now, buffer);
  stats_reporter_.max_gain(generated_messages, now);
  stats_reporter_.input_message(static_cast<size_t>(bytes), OpenDDS::DCPS::MonotonicTimePoint::now() - now, now);

  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, outgoing_mutex_, 0);

  if (!outgoing_.empty()) {
    const auto& out = outgoing_.front();

    const int BUFFERS_SIZE = 2;
    iovec buffers[BUFFERS_SIZE];
    size_t total_bytes = 0;

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

    const auto bytes = socket_.send(buffers, idx, out.address, 0);

    if (bytes < 0) {
      HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayHandler::handle_output %C failed to send to %C: %m\n"),
                     name_.c_str(), OpenDDS::DCPS::LogAddr(out.address).c_str()));
      const auto new_now = OpenDDS::DCPS::MonotonicTimePoint::now();
      stats_reporter_.dropped_message(total_bytes, new_now - now, new_now - out.timestamp, now);
    } else {
      const auto new_now = OpenDDS::DCPS::MonotonicTimePoint::now();
      stats_reporter_.output_message(total_bytes, new_now - now, new_now - out.timestamp, now);
    }

    outgoing_.pop();
  }

  if (outgoing_.empty()) {
    reactor()->remove_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::enqueue_message(const ACE_INET_Addr& addr,
                                   const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                   const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outgoing_mutex_);

  const auto empty = outgoing_.empty();

  outgoing_.push(Element(addr, msg, now));
  stats_reporter_.max_queue_size(outgoing_.size(), now);
  if (empty) {
    reactor()->register_handler(this, WRITE_MASK);
  }
}


ParticipantStatisticsReporter&
GuidAddrSet::Proxy::record_activity(const ACE_INET_Addr& remote_address,
                                    const OpenDDS::DCPS::MonotonicTimePoint& now,
                                    const OpenDDS::DCPS::RepoId& src_guid,
                                    const size_t& msg_len,
                                    RelayHandler& handler)
{
  return gas_.record_activity(remote_address, now, src_guid, msg_len, handler);
}

ParticipantStatisticsReporter&
GuidAddrSet::record_activity(const ACE_INET_Addr& remote_address,
                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                             const OpenDDS::DCPS::RepoId& src_guid,
                             const size_t& msg_len,
                             RelayHandler& handler)
{
  {
    const auto before = guid_addr_set_map_.size();
    const auto res = handler.select_addr_set(guid_addr_set_map_[src_guid])->insert(remote_address);
    if (res.second) {
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity %C %C is at %C total=%B/%B pending=%B/%B\n"),
                   handler.name().c_str(), guid_to_string(src_guid).c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str(),
                   guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
      }
      relay_stats_reporter_.new_address(now);
      const auto after = guid_addr_set_map_.size();
      if (before != after) {
        relay_stats_reporter_.local_active_participants(after, now);
        *handler.select_stats_reporter(guid_addr_set_map_[src_guid]) = ParticipantStatisticsReporter(repoid_to_guid(src_guid), handler.name());
      }
    }
  }

  ParticipantStatisticsReporter& stats_reporter = *handler.select_stats_reporter(guid_addr_set_map_[src_guid]);
  {
    // Record the participant stats only if a valid message was received
    stats_reporter.message_from(msg_len, now);

    const GuidAddr ga(src_guid, remote_address);

    // Compute the new expiration time for this GuidAddr.
    const auto expiration = now + config_.lifespan();
    const auto res = guid_addr_expiration_map_.insert(std::make_pair(ga, expiration));
    if (!res.second) {
      // The GuidAddr already exists.  Remove the previous expiration.
      const auto previous_expiration = res.first->second;
      auto r = expiration_guid_addr_map_.equal_range(previous_expiration);
      while (r.first != r.second && r.first->second != ga) {
        ++r.first;
      }
      expiration_guid_addr_map_.erase(r.first);
      // Assign the new expiration time.
      res.first->second = expiration;
    }
    // Assign the new expiration time.
    expiration_guid_addr_map_.insert(std::make_pair(expiration, ga));
  }

  return stats_reporter;
}

void GuidAddrSet::process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  for (auto pos = expiration_guid_addr_map_.begin(), limit = expiration_guid_addr_map_.end(); pos != limit && pos->first < now;) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::process_expirations %C %C expired at %d.%d now=%d.%d total=%B/%B pending=%B/%B\n"),
        guid_to_string(pos->second.guid).c_str(), OpenDDS::DCPS::LogAddr(pos->second.address).c_str(),
        pos->first.value().sec(), pos->first.value().usec(), now.value().sec(), now.value().usec(),
        guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
    }
    relay_stats_reporter_.expired_address(now);
    AddrSetStats& addr_stats = guid_addr_set_map_[pos->second.guid];
    addr_stats.spdp_addr_set.erase(pos->second.address);
    addr_stats.sedp_addr_set.erase(pos->second.address);
    addr_stats.data_addr_set.erase(pos->second.address);
    if (addr_stats.empty()) {
      addr_stats.spdp_stats_reporter.report(now, true);
      addr_stats.sedp_stats_reporter.report(now, true);
      addr_stats.data_stats_reporter.report(now, true);
      guid_addr_set_map_.erase(pos->second.guid);
      pending_.erase(pos->second.guid);
      relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);
      spdp_vertical_handler_->purge(pos->second.guid);
      sedp_vertical_handler_->purge(pos->second.guid);
      data_vertical_handler_->purge(pos->second.guid);
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::process_expirations %C removed total=%B/%B pending=%B/%B\n"), guid_to_string(pos->second.guid).c_str(), guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
      }
    }
    guid_addr_expiration_map_.erase(pos->second);
    expiration_guid_addr_map_.erase(pos++);
  }

  for (auto pos = pending_expiration_map_.begin(), limit = pending_expiration_map_.end(); pos != limit && pos->first < now;) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::process_expirations %C pending expired at %d.%d now=%d.%d total=%B/%B pending=%B/%B\n"), guid_to_string(pos->second).c_str(), pos->first.value().sec(), pos->first.value().usec(), now.value().sec(), now.value().usec(), guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
    }
    relay_stats_reporter_.expired_pending(now);
    pending_.erase(pos->second);
    pending_expiration_map_.erase(pos++);
  }
}

bool GuidAddrSet::ignore(const OpenDDS::DCPS::GUID_t& guid,
                         const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  // Client has already been admitted.
  if (guid_addr_set_map_.count(guid) != 0) {
    return false;
  }

  if (config_.static_limit() != 0 &&
      guid_addr_set_map_.size() >= config_.static_limit()) {
    // Too many clients to admit another.
    return true;
  }

  if (config_.max_pending() != 0 &&
      pending_.size() >= config_.max_pending()) {
    // Too many new clients to admit another.
    return true;
  }

  pending_.insert(guid);
  pending_expiration_map_.insert(std::make_pair(now + config_.lifespan(), guid));

  return false;
}

void GuidAddrSet::remove(const OpenDDS::DCPS::RepoId& guid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  AddrSetStats& addr_stats = guid_addr_set_map_[guid];
  spdp_vertical_handler_->select_stats_reporter(addr_stats)->report(now, true);
  sedp_vertical_handler_->select_stats_reporter(addr_stats)->report(now, true);
  data_vertical_handler_->select_stats_reporter(addr_stats)->report(now, true);

  remove_helper(guid, addr_stats.spdp_addr_set);
  remove_helper(guid, addr_stats.sedp_addr_set);
  remove_helper(guid, addr_stats.data_addr_set);

  guid_addr_set_map_.erase(guid);
  pending_.erase(guid);
  relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);
  spdp_vertical_handler_->purge(guid);
  sedp_vertical_handler_->purge(guid);
  data_vertical_handler_->purge(guid);

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::remove %C removed total=%B/%B pending=%B/%B\n"), guid_to_string(guid).c_str(), guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
  }
}

void GuidAddrSet::remove_helper(const OpenDDS::DCPS::RepoId& guid, const AddrSet& addr_set)
{
  for (const auto& addr : addr_set) {
    const GuidAddr ga(guid, addr);
    const auto pos = guid_addr_expiration_map_.find(ga);
    if (pos != guid_addr_expiration_map_.end()) {
      for (auto pos2 = expiration_guid_addr_map_.lower_bound(pos->second), limit2 = expiration_guid_addr_map_.upper_bound(pos->second); pos2 != limit2; ) {
        if (pos2->second == ga) {
          expiration_guid_addr_map_.erase(pos2++);
        } else {
          ++pos2;
        }
      }
      guid_addr_expiration_map_.erase(pos);
    }
  }
}

VerticalHandler::VerticalHandler(const Config& config,
                                 const std::string& name,
                                 const ACE_INET_Addr& horizontal_address,
                                 ACE_Reactor* reactor,
                                 const GuidPartitionTable& guid_partition_table,
                                 const RelayPartitionTable& relay_partition_table,
                                 GuidAddrSet& guid_addr_set,
                                 const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                                 const CRYPTO_TYPE& crypto,
                                 const ACE_INET_Addr& application_participant_addr,
                                 HandlerStatisticsReporter& stats_reporter)
  : RelayHandler(config, name, reactor, stats_reporter)
  , guid_partition_table_(guid_partition_table)
  , relay_partition_table_(relay_partition_table)
  , guid_addr_set_(guid_addr_set)
  , horizontal_handler_(nullptr)
  , application_participant_addr_(application_participant_addr)
  , horizontal_address_(horizontal_address)
  , horizontal_address_str_(OpenDDS::DCPS::LogAddr(horizontal_address).c_str())
  , rtps_discovery_(rtps_discovery)
#ifdef OPENDDS_SECURITY
  , crypto_(crypto)
  , application_participant_crypto_handle_(rtps_discovery_->get_crypto_handle(config.application_domain(), config.application_participant_guid()))
#endif
{
  ACE_UNUSED_ARG(crypto);
}

void VerticalHandler::stop()
{
  reactor()->cancel_timer(this);
}

void VerticalHandler::venqueue_message(const ACE_INET_Addr& addr,
                                       ParticipantStatisticsReporter& to_psr,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  enqueue_message(addr, msg, now);
  to_psr.message_to(msg->length(), now);
}

CORBA::ULong VerticalHandler::process_message(const ACE_INET_Addr& remote_address,
                                              const OpenDDS::DCPS::MonotonicTimePoint& now,
                                              const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  guid_addr_set_.process_expirations(now);

  const auto msg_len = msg->length();
  if (msg_len >= 4 && ACE_OS::memcmp(msg->rd_ptr(), "RTPS", 4) == 0) {
    OpenDDS::RTPS::MessageParser mp(*msg);
    OpenDDS::DCPS::RepoId src_guid;
    GuidSet to;

    if (!parse_message(mp, msg, src_guid, to, true, now)) {
      HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C failed to parse_message from %C\n"),
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      return 0;
    }

    if (guid_addr_set_.ignore(src_guid, now)) {
      stats_reporter_.ignored_message(msg_len, now);
      return 0;
    }

    GuidAddrSet::Proxy proxy(guid_addr_set_);
    record_activity(proxy, remote_address, now, src_guid, msg_len);
    bool send_to_application_participant = false;

    CORBA::ULong sent = 0;
    if (do_normal_processing(proxy, remote_address, src_guid, to, send_to_application_participant, msg, now, sent)) {
      StringSet to_partitions;
      guid_partition_table_.lookup(to_partitions, src_guid);
      sent += send(proxy, src_guid, to_partitions, to, send_to_application_participant, msg, now);
    }
    return sent;
  } else {
    // Assume STUN.
    OpenDDS::DCPS::Serializer serializer(msg.get(), OpenDDS::STUN::encoding);
    OpenDDS::STUN::Message message;
    message.block = msg.get();
    if (!(serializer >> message)) {
      HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C Could not deserialize STUN message from %C\n"),
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      return 0;
    }

    std::vector<OpenDDS::STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

    if (!unknown_attributes.empty()) {
      HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C Unknown comprehension required attributes from %C\n"),
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      send(remote_address, make_unknown_attributes_error_response(message, unknown_attributes), now);
      return 1;
    }

    if (!message.has_fingerprint()) {
      HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C No FINGERPRINT attribute from %C\n"),
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      send(remote_address, make_bad_request_error_response(message, "Bad Request: FINGERPRINT must be pesent"), now);
      return 1;
    }

    bool has_guid = false;
    OpenDDS::DCPS::RepoId src_guid;
    if (message.get_guid_prefix(src_guid.guidPrefix)) {
      src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      has_guid = true;

      if (guid_addr_set_.ignore(src_guid, now)) {
        stats_reporter_.ignored_message(msg_len, now);
        return 0;
      }
    }

    size_t bytes_sent = 0;

    switch (message.method) {
    case OpenDDS::STUN::BINDING:
      {
        if (message.class_ == OpenDDS::STUN::REQUEST) {
          OpenDDS::STUN::Message response;
          response.class_ = OpenDDS::STUN::SUCCESS_RESPONSE;
          response.method = OpenDDS::STUN::BINDING;
          std::memcpy(response.transaction_id.data, message.transaction_id.data, sizeof(message.transaction_id.data));
          response.append_attribute(OpenDDS::STUN::make_mapped_address(remote_address));
          response.append_attribute(OpenDDS::STUN::make_xor_mapped_address(remote_address));
          response.append_attribute(OpenDDS::STUN::make_fingerprint());
          bytes_sent = send(remote_address, response, now);
        } else if (message.class_ == OpenDDS::STUN::INDICATION) {
          // Do nothing.
        } else {
          HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C Unknown STUN message class from %C\n"),
            name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
        }
        break;
      }

    default:
      // Unknown method.  Stop processing.
      HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::process_message %C Unknown STUN method from %C\n"),
        name_.c_str(), OpenDDS::DCPS::LogAddr(remote_address).c_str()));
      bytes_sent = send(remote_address, make_bad_request_error_response(message, "Bad Request: Unknown method"), now);
      break;
    }

    if (has_guid) {
      GuidAddrSet::Proxy proxy(guid_addr_set_);
      ParticipantStatisticsReporter& from_psr = record_activity(proxy, remote_address, now, src_guid, msg_len);
      if (bytes_sent) {
        from_psr.message_to(bytes_sent, now);
      }
    }

    return bytes_sent ? 0 : 1;
  }
}

ParticipantStatisticsReporter&
VerticalHandler::record_activity(GuidAddrSet::Proxy& proxy,
                                 const ACE_INET_Addr& remote_address,
                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                 const OpenDDS::DCPS::RepoId& src_guid,
                                 const size_t& msg_len)
{
  return proxy.record_activity(remote_address, now, src_guid, msg_len, *this);
}

bool VerticalHandler::parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                    OpenDDS::DCPS::RepoId& src_guid,
                                    GuidSet& to,
                                    bool check_submessages,
                                    const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_UNUSED_ARG(msg);

  if (!message_parser.parseHeader()) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C failed to deserialize RTPS header\n"), name_.c_str()));
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
      OpenDDS::DCPS::RepoId dest = OpenDDS::DCPS::GUID_UNKNOWN;
      OpenDDS::DCPS::GuidPrefix_t_forany guidPrefix(dest.guidPrefix);
      if (!(message_parser >> guidPrefix)) {
        HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C failed to deserialize INFO_DST from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
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
#ifdef OPENDDS_SECURITY
      switch (submessage_header.submessageId) {
      case OpenDDS::RTPS::SRTPS_PREFIX:
        {
          if (application_participant_crypto_handle_ == DDS::HANDLE_NIL) {
            HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C no crypto handle for application participant\n"), name_.c_str()));
            return false;
          }

          DDS::Security::ParticipantCryptoHandle remote_crypto_handle = rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid(), src_guid);
          if (remote_crypto_handle == DDS::HANDLE_NIL) {
            HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::parse_message %C no crypto handle for message from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }

          DDS::OctetSeq encoded_buffer, plain_buffer;
          DDS::Security::SecurityException ex;

          if (msg->cont() != nullptr) {
            HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C does not support message block chaining\n"), name_.c_str()));
            return false;
          }

          encoded_buffer.length(static_cast<CORBA::ULong>(msg->length()));
          std::memcpy(encoded_buffer.get_buffer(), msg->rd_ptr(), msg->length());

          if (!crypto_->decode_rtps_message(plain_buffer, encoded_buffer, application_participant_crypto_handle_, remote_crypto_handle, ex)) {
            HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::parse_message %C message from %C could not be verified [%d.%d]: \"%C\"\n"), name_.c_str(), guid_to_string(src_guid).c_str(), ex.code, ex.minor_code, ex.message.in()));
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
            HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
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
            HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }
          if (rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid()) != DDS::HANDLE_NIL &&
              !(OpenDDS::DCPS::RtpsUdpDataLink::separate_message(writerId) ||
                writerId == OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER)) {
            HANDLER_WARNING((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: VerticalHandler::parse_message %C submessage from %C with id %d could not be verified writerId=%02X%02X%02X%02X\n"), name_.c_str(), guid_to_string(src_guid).c_str(), submessage_header.submessageId, writerId.entityKey[0], writerId.entityKey[1], writerId.entityKey[2], writerId.entityKind));
            return false;
          }
        }
        break;
      default:
        break;
      }
#endif
    }

    message_parser.skipSubmessageContent();
  }

  if (message_parser.remaining() != 0) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: VerticalHandler::parse_message %C trailing bytes from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
    return false;
  }

  if (!all_valid_info_dst) {
    to.clear();
  }

  return true;
}

CORBA::ULong VerticalHandler::send(GuidAddrSet::Proxy& proxy,
                                   const OpenDDS::DCPS::RepoId& src_guid,
                                   const StringSet& to_partitions,
                                   const GuidSet& to_guids,
                                   bool send_to_application_participant,
                                   const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                   const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  AddressSet address_set;
  populate_address_set(address_set, to_partitions);

  CORBA::ULong sent = 0;
  for (const auto& addr : address_set) {
    if (addr != horizontal_address_) {
      horizontal_handler_->enqueue_message(addr, to_partitions, to_guids, msg, now);
      ++sent;
    } else {
      // Local recipients.
      if (!to_guids.empty()) {
        for (const auto& guid : to_guids) {
          if (guid == src_guid) {
            continue;
          }
          auto p = proxy.find(guid);
          if (p != proxy.end()) {
            for (const auto& addr : *select_addr_set(p->second)) {
              venqueue_message(addr, *select_stats_reporter(p->second), msg, now);
              ++sent;
            }
          }
        }
      } else {
        GuidSet guids;
        guid_partition_table_.lookup(guids, to_partitions);
        for (const auto& guid : guids) {
          if (guid == src_guid) {
            continue;
          }
          auto p = proxy.find(guid);
          if (p != proxy.end()) {
            for (const auto& addr : *select_addr_set(p->second)) {
              venqueue_message(addr, *select_stats_reporter(p->second), msg, now);
              ++sent;
            }
          }
        }
      }
    }
  }

  if (send_to_application_participant) {
    venqueue_message(application_participant_addr_, proxy.participant_statistics_reporter(config_.application_participant_guid(), *this), msg, now);
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
  const size_t length = HEADER_SIZE + message.length();
  Message_Block_Shared_Ptr block(new ACE_Message_Block(length));
  Serializer serializer(block.get(), encoding);
  message.block = block.get();
  serializer << message;
  RelayHandler::enqueue_message(addr, block, now);
  const auto new_now = OpenDDS::DCPS::MonotonicTimePoint::now();
  stats_reporter_.output_message(length, new_now - now, new_now - now, now);
  return length;
}

void VerticalHandler::populate_address_set(AddressSet& address_set,
                                           const StringSet& to_partitions)
{
  relay_partition_table_.lookup(address_set, to_partitions, horizontal_handler_->name());
}

HorizontalHandler::HorizontalHandler(const Config& config,
                                     const std::string& name,
                                     ACE_Reactor* reactor,
                                     const GuidPartitionTable& guid_partition_table,
                                     HandlerStatisticsReporter& stats_reporter)
  : RelayHandler(config, name, reactor, stats_reporter)
  , guid_partition_table_(guid_partition_table)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::enqueue_message(const ACE_INET_Addr& addr,
                                        const StringSet& to_partitions,
                                        const GuidSet& to_guids,
                                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
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
    tg.push_back(repoid_to_guid(g));
  }

  size_t size = 0;
  serialized_size(encoding, size, relay_header);

  const size_t total_size = size + msg->length();
  if (total_size > TransportSendStrategy::UDP_MAX_MESSAGE_SIZE) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: HorizontalHandler::enqueue_message %C header and message too large (%B > %B)\n"), name_.c_str(), total_size, static_cast<size_t>(TransportSendStrategy::UDP_MAX_MESSAGE_SIZE)));
    return;
  }

  Message_Block_Shared_Ptr header_block(new ACE_Message_Block(size));
  Serializer ser(header_block.get(), encoding);
  ser << relay_header;
  header_block->cont(msg.get()->duplicate());
  RelayHandler::enqueue_message(addr, header_block, now);
}

CORBA::ULong HorizontalHandler::process_message(const ACE_INET_Addr& from,
                                                const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  ACE_UNUSED_ARG(from);

  OpenDDS::RTPS::MessageParser mp(*msg);

  const size_t size_before_header = mp.remaining();

  RelayHeader relay_header;
  if (!(mp >> relay_header)) {
    HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: HorizontalHandler::process_message %C failed to deserialize Relay header\n"), name_.c_str()));
    return 0;
  }

  const size_t size_after_header = mp.remaining();

  msg->rd_ptr(size_before_header - size_after_header);

  GuidAddrSet::Proxy proxy(vertical_handler_->guid_addr_set());

  CORBA::ULong sent = 0;

  if (!relay_header.to_guids().empty()) {
    for (const auto& guid : relay_header.to_guids()) {
      const auto p = proxy.find(guid_to_repoid(guid));
      if (p != proxy.end()) {
        for (const auto& addr : *vertical_handler_->select_addr_set(p->second)) {
          vertical_handler_->venqueue_message(addr, *vertical_handler_->select_stats_reporter(p->second), msg, now);
          ++sent;
        }
      }
    }
  } else {
    GuidSet guids;
    guid_partition_table_.lookup(guids, relay_header.to_partitions());
    for (const auto& guid : guids) {
      const auto p = proxy.find(guid);
      if (p != proxy.end()) {
        for (const auto& addr : *vertical_handler_->select_addr_set(p->second)) {
          vertical_handler_->venqueue_message(addr, *vertical_handler_->select_stats_reporter(p->second), msg, now);
          ++sent;
        }
      }
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
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr,
                         HandlerStatisticsReporter& stats_reporter)
: VerticalHandler(config, name, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, application_participant_addr, stats_reporter)
{}

bool SpdpHandler::do_normal_processing(GuidAddrSet::Proxy& proxy,
                                       const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       bool& send_to_application_participant,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       CORBA::ULong& sent)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpHandler::do_normal_processing application participant imposter detected at %C\n"),
        OpenDDS::DCPS::LogAddr(remote).c_str()));
      return false;
    }

    // SPDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = proxy.find(guid);
        if (pos != proxy.end()) {
          for (const auto& addr : *select_addr_set(pos->second)) {
            venqueue_message(addr, *select_stats_reporter(pos->second), msg, now);
            ++sent;
          }
        }
      }
    } else {
      HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpHandler::do_normal_processing dropping non-directed SPDP message from application participant\n")));
      return false;
    }

    return false;
  }

  // SPDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    send_to_application_participant = true;
  }

  // Cache it.
  if (to.empty()) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, spdp_messages_mutex_, false);
    spdp_messages_[src_guid] = msg;
  }

  return true;
}

void SpdpHandler::purge(const OpenDDS::DCPS::RepoId& guid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, spdp_messages_mutex_);
  const auto pos = spdp_messages_.find(guid);
  if (pos != spdp_messages_.end()) {
    spdp_messages_.erase(pos);
  }
}

void SpdpHandler::replay(const StringSequence& partitions)
{
  ACE_GUARD(ACE_Thread_Mutex, g, replay_queue_mutex_);
  bool notify = replay_queue_.empty();
  replay_queue_.insert(partitions.begin(), partitions.end());
  if (notify) {
    reactor()->notify(this);
  }
}

int SpdpHandler::handle_exception(ACE_HANDLE /*fd*/)
{
  StringSet q;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, replay_queue_mutex_, 0);
    std::swap(q, replay_queue_);
  }

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  GuidSet guids;
  guid_partition_table_.lookup(guids, q);

  for (const auto& guid : guids) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, spdp_messages_mutex_, 0);
    const auto pos = spdp_messages_.find(guid);
    if (pos != spdp_messages_.end()) {
      GuidAddrSet::Proxy proxy(guid_addr_set_);
      send(proxy, guid, q, GuidSet(), false, pos->second, now);
    }
  }

  return 0;
}

SedpHandler::SedpHandler(const Config& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         const GuidPartitionTable& guid_partition_table,
                         const RelayPartitionTable& relay_partition_table,
                         GuidAddrSet& guid_addr_set,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr,
                         HandlerStatisticsReporter& stats_reporter)
: VerticalHandler(config, name, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, application_participant_addr, stats_reporter)
{}

bool SedpHandler::do_normal_processing(GuidAddrSet::Proxy& proxy,
                                       const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       bool& send_to_application_participant,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       CORBA::ULong& sent)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SedpHandler::do_normal_processing application participant imposter detected at %C\n"),
        OpenDDS::DCPS::LogAddr(remote).c_str()));
      return false;
    }

    // SEDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = proxy.find(guid);
        if (pos != proxy.end()) {
          for (const auto& addr : *select_addr_set(pos->second)) {
            venqueue_message(addr, *select_stats_reporter(pos->second), msg, now);
            ++sent;
          }
        }
      }
    } else {
      HANDLER_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SedpHandler::do_normal_processing dropping non-directed SEDP message from application participant\n")));
      return false;
    }

    return false;
  }

  // SEDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    send_to_application_participant = true;
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
                         const CRYPTO_TYPE& crypto,
                         HandlerStatisticsReporter& stats_reporter)
: VerticalHandler(config, name, address, reactor, guid_partition_table, relay_partition_table, guid_addr_set, rtps_discovery, crypto, ACE_INET_Addr(), stats_reporter)
{}

}
