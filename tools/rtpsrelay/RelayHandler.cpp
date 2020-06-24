#include "RelayHandler.h"

#include "lib/RelayTypeSupportImpl.h"

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

bool
operator<(const Duration_t& x, const Duration_t& y)
{
  if (x.sec() != y.sec()) {
    return x.sec() < y.sec();
  }
  return x.nanosec() < y.nanosec();
}

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

RelayHandler::RelayHandler(const RelayHandlerConfig& config,
                           const std::string& name,
                           ACE_Reactor* reactor,
                           Governor& governor)
  : ACE_Event_Handler(reactor)
  , governor_(governor)
  , config_(config)
  , name_(name)
{
  handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
  handler_statistics_.name(name);
}

int RelayHandler::open(const ACE_INET_Addr& address)
{
  if (socket_.open(address) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::open %C failed to open socket on '%C'\n"), name_.c_str(), addr_to_string(address).c_str()));
    return -1;
  }
  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::open %C failed to enable ACE_NONBLOCK\n"), name_.c_str()));
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::open %C failed to set the send buffer size to %d errno %m\n"), name_.c_str(), send_buffer_size));
    return -1;
  }

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::open %C failed to register READ_MASK handler\n"), name_.c_str()));
    return -1;
  }

  if (config_.handler_statistics_writer() || config_.publish_participant_statistics()) {
    reset_statistics(OpenDDS::DCPS::MonotonicTimePoint::now());
    if (reactor()->schedule_timer(this, &this->handler_statistics_, config_.statistics_interval().value(), config_.statistics_interval().value()) == -1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::open %C failed to register schedule statistics timer\n"), name_.c_str()));
      return -1;
    }
  }

  return 0;
}

int RelayHandler::handle_input(ACE_HANDLE handle)
{
  ACE_INET_Addr remote;
  int inlen = 65536; // Default to maximum datagram size.

#ifdef FIONREAD
  if (ACE_OS::ioctl (handle,
                     FIONREAD,
                     &inlen) == -1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::handle_input %C failed to get available byte count: %m\n"), name_.c_str()));
    return 0;
  }
#else
  ACE_UNUSED_ARG(handle);
#endif

  if (inlen < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::handle_input %C available byte count is negative\n"), name_.c_str()));
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

    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::handle_input %C failed to recv: %m\n"), name_.c_str()));
    return 0;
  } else if (bytes == 0) {
    // Okay.  Empty datagram.
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: RelayHandler::handle_input %C received an empty datagram from %C\n"), name_.c_str(), addr_to_string(remote).c_str()));
    return 0;
  }

  buffer->length(bytes);

  handler_statistics_._bytes_in += bytes;
  ++handler_statistics_._messages_in;

  if (config_.publish_participant_statistics()) {
    auto& ps = participant_statistics_[remote];
    ps._bytes_in += bytes;
    ++ps._messages_in;
  }

  process_message(remote, OpenDDS::DCPS::MonotonicTimePoint::now(), buffer);
  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, outgoing_mutex_, 0);

  if (!outgoing_.empty()) {
    const auto& out = outgoing_.front();

    const int BUFFERS_SIZE = 2;
    iovec buffers[BUFFERS_SIZE];

    int idx = 0;
    for (ACE_Message_Block* block = out.second.get(); block && idx < BUFFERS_SIZE; block = block->cont(), ++idx) {
      buffers[idx].iov_base = block->rd_ptr();
#ifdef _MSC_VER
#pragma warning(push)
      // iov_len is 32-bit on 64-bit VC++, but we don't want a cast here
      // since on other platforms iov_len is 64-bit
#pragma warning(disable : 4267)
#endif
      buffers[idx].iov_len = block->length();
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    }

    const auto bytes = socket_.send(buffers, idx, out.first, 0);

    if (bytes < 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::handle_output %C failed to send to %C: %m\n"), name_.c_str(), addr_to_string(out.first).c_str()));
    } else {
      governor_.add_bytes(bytes);
      handler_statistics_._bytes_out += bytes;
      ++handler_statistics_._messages_out;

      if (config_.publish_participant_statistics()) {
        auto& ps = participant_statistics_[out.first];
        ps._bytes_out += bytes;
        ++ps._messages_out;
      }
    }

    outgoing_.pop();
  }

  if (outgoing_.empty()) {
    reactor()->remove_handler(this, WRITE_MASK);
  } else {
    // Check if rate limiting.
    const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();
    const auto send_time = governor_.get_next_send_time();
    const auto d = send_time - now;
    const auto dds_d = d.to_dds_duration();
    const Duration_t relay_duration = { dds_d.sec, dds_d.nanosec };

    if (send_time > now) {
      reactor()->remove_handler(this, WRITE_MASK);
      reactor()->schedule_timer(this, 0, d.value());

      handler_statistics_._max_queue_latency = std::max(handler_statistics_._max_queue_latency, relay_duration);
    }
  }

  return 0;
}

int RelayHandler::handle_timeout(const ACE_Time_Value& ace_now, const void* ptr)
{
  if (ptr == &this->handler_statistics_) {
    // Statistics.
    const OpenDDS::DCPS::MonotonicTimePoint now(ace_now);
    const auto duration = now - last_report_time_;
    const auto dds_duration = duration.to_dds_duration();

    if (config_.handler_statistics_writer()) {
      handler_statistics_._interval._sec = dds_duration.sec;
      handler_statistics_._interval._nanosec = dds_duration.nanosec;
      handler_statistics_._local_active_participants = local_active_participants();

      if (config_.publish_participant_statistics()) {
        for (auto& p : participant_statistics_) {
          p.second.address(addr_to_string(p.first));
          handler_statistics_.participant_statistics().push_back(p.second);
        }
      }

      const auto ret = config_.handler_statistics_writer()->write(handler_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: RelayHandler::handle_timeout %C failed to write handler statistics\n"), name_.c_str()));
      }
    }

    reset_statistics(now);
  } else {
    // Governor.
    reactor()->register_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::reset_statistics(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  last_report_time_ = now;
  handler_statistics_._bytes_in = 0;
  handler_statistics_._messages_in = 0;
  handler_statistics_._bytes_out = 0;
  handler_statistics_._messages_out = 0;
  handler_statistics_._max_fan_out = 0;
  handler_statistics_._max_queue_size = 0;
  handler_statistics_._max_queue_latency._sec = 0;
  handler_statistics_._max_queue_latency._nanosec = 0;
  handler_statistics_.participant_statistics().clear();
  participant_statistics_.clear();
}

void RelayHandler::enqueue_message(const ACE_INET_Addr& addr, const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outgoing_mutex_);

  const auto empty = outgoing_.empty();

  outgoing_.push(std::make_pair(addr, msg));
  handler_statistics_._max_queue_size = std::max(handler_statistics_._max_queue_size, static_cast<uint32_t>(outgoing_.size()));

  if (empty) {
    reactor()->register_handler(this, WRITE_MASK);
  }
}

VerticalHandler::VerticalHandler(const RelayHandlerConfig& config,
                                 const std::string& name,
                                 const ACE_INET_Addr& horizontal_address,
                                 ACE_Reactor* reactor,
                                 Governor& governor,
                                 const AssociationTable& association_table,
                                 GuidNameAddressDataWriter_ptr responsible_relay_writer,
                                 GuidNameAddressDataReader_ptr responsible_relay_reader,
                                 const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                                 const CRYPTO_TYPE& crypto)
  : RelayHandler(config, name, reactor, governor)
  , association_table_(association_table)
  , responsible_relay_writer_(responsible_relay_writer)
  , responsible_relay_reader_(responsible_relay_reader)
  , horizontal_handler_(nullptr)
  , horizontal_address_(horizontal_address)
  , horizontal_address_str_(addr_to_string(horizontal_address))
  , rtps_discovery_(rtps_discovery)
#ifdef OPENDDS_SECURITY
  , crypto_(crypto)
  , application_participant_crypto_handle_(rtps_discovery_->get_crypto_handle(config.application_domain(), config.application_participant_guid()))
#endif
{
  ACE_UNUSED_ARG(crypto);
}

void VerticalHandler::process_message(const ACE_INET_Addr& remote,
                                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                                      const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  OpenDDS::DCPS::RepoId src_guid;
  GuidSet to;
  bool is_beacon = true;

  OpenDDS::RTPS::MessageParser mp(*msg);
  if (!parse_message(mp, msg, src_guid, to, is_beacon, true)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::process_message %C failed to parse_message from %C\n"), name_.c_str(), addr_to_string(remote).c_str()));
    return;
  }

  guid_addr_map_[src_guid].insert(remote);
  const GuidAddr ga(src_guid, remote);

  // Compute the new expiration time for this SPDP client.
  const auto expiration = now + config_.lifespan();
  const auto res = guid_expiration_map_.insert(std::make_pair(ga, expiration));
  if (!res.second) {
    // The SPDP client already exists.  Remove the previous expiration.
    const auto previous_expiration = res.first->second;
    auto r = expiration_guid_map_.equal_range(previous_expiration);
    while (r.first != r.second && r.first->second != ga) {
      ++r.first;
    }
    expiration_guid_map_.erase(r.first);
    // Assign the new expiration time.
    res.first->second = expiration;
    // Assert ownership.
    const auto address = read_address(src_guid);
    if (address != horizontal_address_) {
      write_address(src_guid);
    }
  } else {
    // Assert ownership.
    write_address(src_guid);
  }
  // Assign the new expiration time.
  expiration_guid_map_.insert(std::make_pair(expiration, ga));

  // Process expirations.
  for (auto pos = expiration_guid_map_.begin(), limit = expiration_guid_map_.end(); pos != limit && pos->first < now;) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l VerticalHandler::process_message %C %C expired at %d.%d now=%d.%d\n"), name_.c_str(), guid_to_string(pos->second.guid).c_str(), pos->first.value().sec(), pos->first.value().usec(), now.value().sec(), now.value().usec()));
    purge(pos->second);
    guid_addr_map_.erase(pos->second.guid);
    guid_expiration_map_.erase(pos->second);
    unregister_address(pos->second.guid);
    expiration_guid_map_.erase(pos++);
  }

  // Readers send empty messages so we know where they are.
  if (is_beacon) {
    return;
  }

  if (do_normal_processing(remote, src_guid, to, msg)) {
    association_table_.lookup_destinations(to, src_guid);
    send(remote, to, msg);
  }
}

bool VerticalHandler::parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                    OpenDDS::DCPS::RepoId& src_guid,
                                    GuidSet& to,
                                    bool& is_beacon,
                                    bool check_submessages)
{
  ACE_UNUSED_ARG(msg);

  if (!message_parser.parseHeader()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C failed to deserialize RTPS header\n"), name_.c_str()));
    return false;
  }

  const auto& header = message_parser.header();
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  while (message_parser.parseSubmessageHeader()) {
    const auto submessage_header = message_parser.submessageHeader();

    if (submessage_header.submessageId != OpenDDS::RTPS::PAD) {
      is_beacon = false;
    }

    if (submessage_header.submessageId == OpenDDS::RTPS::INFO_DST) {
      OpenDDS::DCPS::RepoId dest;
      OpenDDS::DCPS::GuidPrefix_t_forany guidPrefix(dest.guidPrefix);
      if (!(message_parser >> guidPrefix)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C failed to deserialize INFO_DST from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
        return false;
      }
      dest.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      to.insert(dest);
    }

    if (check_submessages) {
#ifdef OPENDDS_SECURITY
      switch (submessage_header.submessageId) {
      case OpenDDS::RTPS::SRTPS_PREFIX:
        {
          if (application_participant_crypto_handle_ == DDS::HANDLE_NIL) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C no crypto handle for application participant\n"), name_.c_str()));
            return false;
          }

          DDS::Security::ParticipantCryptoHandle remote_crypto_handle = rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid(), src_guid);
          if (remote_crypto_handle == DDS::HANDLE_NIL) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C no crypto handle for message from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }

          DDS::OctetSeq encoded_buffer, plain_buffer;
          DDS::Security::SecurityException ex;

          if (msg->cont() != nullptr) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C does not support message block chaining\n"), name_.c_str()));
            return false;
          }

          encoded_buffer.length(static_cast<CORBA::ULong>(msg->length()));
          std::memcpy(encoded_buffer.get_buffer(), msg->rd_ptr(), msg->length());

          if (!crypto_->decode_rtps_message(plain_buffer, encoded_buffer, application_participant_crypto_handle_, remote_crypto_handle, ex)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C message from %C could not be verified [%d.%d]: \"%C\"\n"), name_.c_str(), guid_to_string(src_guid).c_str(), ex.code, ex.minor_code, ex.message.in()));
            return false;
          }

          OpenDDS::RTPS::MessageParser mp(plain_buffer);
          to.clear();
          is_beacon = true;
          return parse_message(mp, msg, src_guid, to, is_beacon, false);
        }
        break;
      case OpenDDS::RTPS::DATA:
      case OpenDDS::RTPS::DATA_FRAG:
        {
          unsigned short extraFlags;
          unsigned short octetsToInlineQos;
          if (!(message_parser >> extraFlags) ||
              !(message_parser >> octetsToInlineQos)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
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
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C could not parse submessage from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
            return false;
          }
          if (rtps_discovery_->get_crypto_handle(config_.application_domain(), config_.application_participant_guid()) != DDS::HANDLE_NIL &&
              !(OpenDDS::DCPS::RtpsUdpDataLink::separate_message(writerId) ||
                writerId == OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER)) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C submessage from %C with id %d could not be verified writerId=%02X%02X%02X%02X\n"), name_.c_str(), guid_to_string(src_guid).c_str(), submessage_header.submessageId, writerId.entityKey[0], writerId.entityKey[1], writerId.entityKey[2], writerId.entityKind));
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
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::parse_message %C trailing bytes from %C\n"), name_.c_str(), guid_to_string(src_guid).c_str()));
    return false;
  }

  return true;
}

void VerticalHandler::send(const ACE_INET_Addr& from,
                           const GuidSet& to,
                           const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  AddressMap address_map;
  populate_address_map(address_map, to);

  size_t fan_out = 0;
  for (const auto& p : address_map) {
    const auto& addr = p.first;
    const auto& guids = p.second;
    fan_out += guids.size();
    if (addr != horizontal_address_) {
      horizontal_handler_->enqueue_message(addr, guids, msg);
    } else {
      for (const auto& guid : guids) {
        const auto p = find(guid);
        if (p != end()) {
          for (const auto& addr : p->second) {
            enqueue_message(addr, msg);
          }
        } else {
          ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: VerticalHandler::send %C failed to get address for %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
        }
      }
    }
  }
  max_fan_out(from, fan_out);
}

void VerticalHandler::populate_address_map(AddressMap& address_map, const GuidSet& to)
{
  for (const auto& guid : to) {
    const auto address = read_address(guid);
    if (address == ACE_INET_Addr()) {
      continue;
    }
    address_map[address].insert(guid);
  }
}

ACE_INET_Addr VerticalHandler::read_address(const OpenDDS::DCPS::RepoId& guid) const
{
  GuidNameAddress key;
  assign(key.guid(), guid);
  key.name(name_);

  DDS::InstanceHandle_t handle = responsible_relay_reader_->lookup_instance(key);
  if (handle == DDS::HANDLE_NIL) {
    return ACE_INET_Addr();
  }
  GuidNameAddressSeq received_data;
  DDS::SampleInfoSeq info_seq;
  const auto ret = responsible_relay_reader_->read_instance(received_data, info_seq, 1, handle,
                                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: VerticalHandler::read_address %C failed to read address for %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
    return ACE_INET_Addr();
  }

  return ACE_INET_Addr(received_data[0].address().c_str());
}

void VerticalHandler::write_address(const OpenDDS::DCPS::RepoId& guid)
{
  GuidNameAddress gna;
  assign(gna.guid(), guid);
  gna.name(name_);
  gna.address(horizontal_address_str_);

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l VerticalHandler::write_address %C claiming %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
  const auto ret = responsible_relay_writer_->write(gna, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::write_address %C failed to write address for %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
  }
}

void VerticalHandler::unregister_address(const OpenDDS::DCPS::RepoId& guid)
{
  GuidNameAddress gna;
  assign(gna.guid(), guid);
  gna.name(name_);

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l %C VerticalHandler::unregister_address disclaiming %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
  const auto ret = responsible_relay_writer_->unregister_instance(gna, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: VerticalHandler::unregister_address %C failed to unregister_instance for %C\n"), name_.c_str(), guid_to_string(guid).c_str()));
  }
}

HorizontalHandler::HorizontalHandler(const RelayHandlerConfig& config,
                                     const std::string& name,
                                     ACE_Reactor* reactor,
                                     Governor& governor)
  : RelayHandler(config, name, reactor, governor)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::enqueue_message(const ACE_INET_Addr& addr,
                                        const GuidSet& guids,
                                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  using namespace OpenDDS::DCPS;

  const Encoding encoding(Encoding::KIND_XCDR1);

  // Determine how many guids we can pack into a single UDP message.
  const auto max_guids_per_message =
    (TransportSendStrategy::UDP_MAX_MESSAGE_SIZE - msg->length() - 4) / sizeof(RepoId);

  auto remaining = guids.size();
  auto pos = guids.begin();

  while (remaining) {
    auto guids_in_message = std::min(max_guids_per_message, remaining);
    remaining -= guids_in_message;

    RelayHeader relay_header;
    auto& to = relay_header.to();
    for (; guids_in_message; --guids_in_message, ++pos) {
      to.push_back(repoid_to_guid(*pos));
    }

    size_t size = 0;
    serialized_size(encoding, size, relay_header);
    Message_Block_Shared_Ptr header_block(new ACE_Message_Block(size));
    Serializer ser(header_block.get(), encoding);
    ser << relay_header;
    header_block->cont(msg.get()->duplicate());
    RelayHandler::enqueue_message(addr, header_block);
  }
}

void HorizontalHandler::process_message(const ACE_INET_Addr& from,
                                        const OpenDDS::DCPS::MonotonicTimePoint&,
                                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  OpenDDS::RTPS::MessageParser mp(*msg);

  const size_t size_before_header = mp.remaining();

  RelayHeader relay_header;
  if (!(mp >> relay_header)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: HorizontalHandler::process_message %C failed to deserialize Relay header\n"), name_.c_str()));
    return;
  }

  const size_t size_after_header = mp.remaining();

  msg->rd_ptr(size_before_header - size_after_header);

  for (const auto& guid : relay_header.to()) {
    const auto p = vertical_handler_->find(guid_to_repoid(guid));
    if (p != vertical_handler_->end()) {
      for (const auto& addr : p->second) {
        vertical_handler_->enqueue_message(addr, msg);
      }
    } else {
      ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: HorizontalHandler::process_message %C failed to get address for %C\n"), name_.c_str(), guid_to_string(guid_to_repoid(guid)).c_str()));
    }
  }
  max_fan_out(from, relay_header.to().size());
}

SpdpHandler::SpdpHandler(const RelayHandlerConfig& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         Governor& governor,
                         const AssociationTable& association_table,
                         GuidNameAddressDataWriter_ptr responsible_relay_writer,
                         GuidNameAddressDataReader_ptr responsible_relay_reader,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(config, name, address, reactor, governor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto)
, application_participant_addr_(application_participant_addr)
{}

bool SpdpHandler::do_normal_processing(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      return false;
    }

    // SPDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = guid_addr_map_.find(guid);
        if (pos != guid_addr_map_.end()) {
          for (const auto& addr : pos->second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(remote, to.size());
    } else {
      // Forward to all peers except the application participant.
      for (const auto& p : guid_addr_map_) {
        if (p.first != config_.application_participant_guid()) {
          for (const auto& addr : p.second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(remote, guid_addr_map_.size());
    }

    return false;
  }

  // SPDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    // Forward to the application participant.
    enqueue_message(application_participant_addr_, msg);
    max_fan_out(remote, 1);
  }

  // Cache it.
  if (to.empty()) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, spdp_messages_mutex_, false);
    spdp_messages_[src_guid] = msg;
  }

  return true;
}

void SpdpHandler::purge(const GuidAddr& ga)
{
  ACE_GUARD(ACE_Thread_Mutex, g, spdp_messages_mutex_);
  const auto pos = spdp_messages_.find(ga.guid);
  if (pos != spdp_messages_.end()) {
    spdp_messages_.erase(pos);
  }
}

void SpdpHandler::replay(const OpenDDS::DCPS::RepoId& from,
                         const GuidSet& to)
{
  if (to.empty()) {
    return;
  }

  const auto from_guid = to_participant_guid(from);

  ACE_GUARD(ACE_Thread_Mutex, g, replay_queue_mutex_);
  bool notify = replay_queue_.empty();
  replay_queue_.push(Replay{from_guid, to});
  if (notify) {
    reactor()->notify(this);
  }
}

int SpdpHandler::handle_exception(ACE_HANDLE /*fd*/)
{
  ReplayQueue q;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, replay_queue_mutex_, 0);
    std::swap(q, replay_queue_);
  }

  while (!q.empty()) {
    const Replay& r = q.front();
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, spdp_messages_mutex_, 0);

    const auto pos = spdp_messages_.find(r.from_guid);
    if (pos != spdp_messages_.end()) {
      send(ACE_INET_Addr(), r.to, pos->second);
    }

    q.pop();
  }

  return 0;
}

SedpHandler::SedpHandler(const RelayHandlerConfig& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         Governor& governor,
                         const AssociationTable& association_table,
                         GuidNameAddressDataWriter_ptr responsible_relay_writer,
                         GuidNameAddressDataReader_ptr responsible_relay_reader,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(config, name, address, reactor, governor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto)
  , application_participant_addr_(application_participant_addr)
{}

bool SedpHandler::do_normal_processing(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  if (src_guid == config_.application_participant_guid()) {
    if (remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      return false;
    }

    // SEDP message is from the application participant.
    if (!to.empty()) {
      // Forward to destinations.
      for (const auto& guid : to) {
        const auto pos = guid_addr_map_.find(guid);
        if (pos != guid_addr_map_.end()) {
          for (const auto& addr : pos->second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(remote, to.size());
    } else {
      // Forward to all peers except the application participant.
      for (const auto& p : guid_addr_map_) {
        if (p.first != config_.application_participant_guid()) {
          for (const auto& addr : p.second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(remote, guid_addr_map_.size());
    }

    return false;
  }

  // SEDP message is from a client.
  if (to.empty() || to.count(config_.application_participant_guid()) != 0) {
    // Forward to the application participant.
    enqueue_message(application_participant_addr_, msg);
    max_fan_out(remote, 1);
  }
  return true;
}

DataHandler::DataHandler(const RelayHandlerConfig& config,
                         const std::string& name,
                         const ACE_INET_Addr& address,
                         ACE_Reactor* reactor,
                         Governor& governor,
                         const AssociationTable& association_table,
                         GuidNameAddressDataWriter_ptr responsible_relay_writer,
                         GuidNameAddressDataReader_ptr responsible_relay_reader,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         const CRYPTO_TYPE& crypto)
: VerticalHandler(config, name, address, reactor, governor, association_table, responsible_relay_writer, responsible_relay_reader, rtps_discovery, crypto)
{}

#ifdef OPENDDS_SECURITY

StunHandler::StunHandler(const RelayHandlerConfig& config,
                         const std::string& name,
                         ACE_Reactor* reactor,
                         Governor& governor)
  : RelayHandler(config, name, reactor, governor)
{}

void StunHandler::process_message(const ACE_INET_Addr& remote_address,
                                  const OpenDDS::DCPS::MonotonicTimePoint&,
                                  const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  OpenDDS::DCPS::Serializer serializer(msg.get(), OpenDDS::STUN::encoding);
  OpenDDS::STUN::Message message;
  message.block = msg.get();
  if (!(serializer >> message)) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: StunHandler::process_message %C Could not deserialize STUN message from %C\n"), addr_to_string(remote_address).c_str()));
    return;
  }

  if (message.class_ == OpenDDS::STUN::INDICATION) {
    return;
  }

  if (message.class_ != OpenDDS::STUN::REQUEST) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: StunHandler::process_message Unknown STUN message class from %C\n"), addr_to_string(remote_address).c_str()));
    return;
  }

  std::vector<OpenDDS::STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: StunHandler::process_message Unknown comprehension requird attributes from %C\n"), addr_to_string(remote_address).c_str()));
    send(remote_address, make_unknown_attributes_error_response(message, unknown_attributes));
    return;
  }

  if (!message.has_fingerprint()) {
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: StunHandler::process_message No FINGERPRINT attribute from %C\n"), addr_to_string(remote_address).c_str()));
    send(remote_address, make_bad_request_error_response(message, "Bad Request: FINGERPRINT must be pesent"));
    return;
  }

  switch (message.method) {
  case OpenDDS::STUN::BINDING:
    {
      OpenDDS::STUN::Message response;
      response.class_ = OpenDDS::STUN::SUCCESS_RESPONSE;
      response.method = OpenDDS::STUN::BINDING;
      std::memcpy(response.transaction_id.data, message.transaction_id.data, sizeof(message.transaction_id.data));
      response.append_attribute(OpenDDS::STUN::make_mapped_address(remote_address));
      response.append_attribute(OpenDDS::STUN::make_xor_mapped_address(remote_address));
      response.append_attribute(OpenDDS::STUN::make_fingerprint());
      send(remote_address, response);
    }
    break;

  default:
    // Unknown method.  Stop processing.
    ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) %N:%l WARNING: StunHandler::process_message Unknown STUN method from %C\n"), addr_to_string(remote_address).c_str()));
    send(remote_address, make_bad_request_error_response(message, "Bad Request: Unknown method"));
    break;
  }
}

void StunHandler::send(const ACE_INET_Addr& addr, OpenDDS::STUN::Message message)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::STUN;
  Message_Block_Shared_Ptr block(new ACE_Message_Block(HEADER_SIZE + message.length()));
  Serializer serializer(block.get(), encoding);
  message.block = block.get();
  serializer << message;
  enqueue_message(addr, block);
}
#endif /* OPENDDS_SECURITY */

}
