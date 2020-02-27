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

RelayHandler::RelayHandler(ACE_Reactor* reactor)
  : ACE_Event_Handler(reactor)
  , bytes_received_(0)
  , messages_received_(0)
  , bytes_sent_(0)
  , messages_sent_(0)
  , max_fan_out_(0)
{}

int RelayHandler::open(const ACE_INET_Addr& local)
{
  relay_address_ = local;

  if (socket_.open(local) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to open socket on '%C'\n", addr_to_string(relay_address_).c_str()));
    return -1;
  }
  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to enable ACE_NONBLOCK\n"));
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open: failed to set the send buffer size to %d errno %m\n", send_buffer_size));
    return -1;
  }

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to register READ_MASK handler\n"));
    return -1;
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to get available byte count: %m\n"));
    return 0;
  }
#else
  ACE_UNUSED_ARG(handle);
#endif

  if (inlen < 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input available byte count is negative\n"));
    return 0;
  }

  // Allocate at least one byte so that recv cannot return early.
  OpenDDS::DCPS::Message_Block_Shared_Ptr buffer(new ACE_Message_Block(std::max(inlen, 1)));

  const auto bytes = socket_.recv(buffer->wr_ptr(), buffer->space(), remote);

  if (bytes < 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to recv: %m\n"));
    return 0;
  } else if (bytes == 0) {
    // Okay.  Empty datagram.
    ACE_DEBUG((LM_WARNING, "(%P|%t) %N:%l WARNING: RelayHandler::handle_input received an empty datagram\n"));
    return 0;
  }

  buffer->length(bytes);

  bytes_received_ += bytes;
  ++messages_received_;

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
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_output failed to send to %C: %m\n", addr_to_string(out.first).c_str()));
    } else {
      bytes_sent_ += bytes;
      ++messages_sent_;
    }

    outgoing_.pop();
  }

  if (outgoing_.empty()) {
    reactor()->remove_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::enqueue_message(const ACE_INET_Addr& addr, const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outgoing_mutex_);

  const auto empty = outgoing_.empty();

  outgoing_.push(std::make_pair(addr, msg));

  if (empty) {
    reactor()->register_handler(this, WRITE_MASK);
  }
}

VerticalHandler::VerticalHandler(ACE_Reactor* reactor,
                                 const RelayAddresses& relay_addresses,
                                 const AssociationTable& association_table,
                                 GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
                                 GuidRelayAddressesDataReader_ptr responsible_relay_reader,
                                 const OpenDDS::DCPS::TimeDuration& lifespan,
                                 const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                                 DDS::DomainId_t application_domain,
                                 const OpenDDS::DCPS::RepoId& application_participant_guid,
                                 const CRYPTO_TYPE& crypto)
  : RelayHandler(reactor)
  , association_table_(association_table)
  , responsible_relay_writer_(responsible_relay_writer)
  , responsible_relay_reader_(responsible_relay_reader)
  , relay_addresses_(relay_addresses)
  , horizontal_handler_(nullptr)
  , lifespan_(lifespan)
  , application_participant_guid_(application_participant_guid)
  , rtps_discovery_(rtps_discovery)
#ifdef OPENDDS_SECURITY
  , application_domain_(application_domain)
  , crypto_(crypto)
  , application_participant_crypto_handle_(rtps_discovery_->get_crypto_handle(application_domain_, application_participant_guid_))
#endif
{
  ACE_UNUSED_ARG(crypto);
  ACE_UNUSED_ARG(application_domain);
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: VerticalHandler::process_message failed to parse_message\n"));
    return;
  }

  guid_addr_map_[src_guid].insert(remote);
  const GuidAddr ga(src_guid, remote);

  // Compute the new expiration time for this SPDP client.
  const auto expiration = now + lifespan_;
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
    const auto relay_addresses = read_relay_addresses(src_guid);
    if (relay_addresses != relay_addresses_) {
      write_relay_addresses(src_guid, relay_addresses_);
    }
  } else {
    // Assert ownership.
    write_relay_addresses(src_guid, relay_addresses_);
  }
  // Assign the new expiration time.
  expiration_guid_map_.insert(std::make_pair(expiration, ga));

  // Process expirations.
  for (auto pos = expiration_guid_map_.begin(), limit = expiration_guid_map_.end(); pos != limit && pos->first < now;) {
    purge(pos->second);
    guid_addr_map_.erase(pos->second.guid);
    guid_expiration_map_.erase(pos->second);
    expiration_guid_map_.erase(pos++);
    unregister_relay_addresses(pos->second.guid);
  }

  // Readers send empty messages so we know where they are.
  if (is_beacon) {
    return;
  }

  if (do_normal_processing(remote, src_guid, to, msg)) {
    association_table_.lookup_destinations(to, src_guid);
    send(to, msg);
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message failed to deserialize RTPS header\n"));
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
        ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message failed to deserialize INFO_DST\n"));
        return false;
      }
      dest.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      to.insert(dest);
    }

    if (check_submessages && src_guid != application_participant_guid_) {
#ifdef OPENDDS_SECURITY
      switch (submessage_header.submessageId) {
      case OpenDDS::RTPS::SRTPS_PREFIX:
        {
          if (application_participant_crypto_handle_ == DDS::HANDLE_NIL) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message no crypto handle for application participant\n"));
            return false;
          }

          DDS::Security::ParticipantCryptoHandle remote_crypto_handle = rtps_discovery_->get_crypto_handle(application_domain_, application_participant_guid_, src_guid);
          if (remote_crypto_handle == DDS::HANDLE_NIL) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message no crypto handle for client %C\n", guid_to_string(src_guid).c_str()));
            return false;
          }

          DDS::OctetSeq encoded_buffer, plain_buffer;
          DDS::Security::SecurityException ex;

          if (msg->cont() != nullptr) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message does not support message block chaining\n"));
            return false;
          }

          encoded_buffer.length(static_cast<CORBA::ULong>(msg->length()));
          std::memcpy(encoded_buffer.get_buffer(), msg->rd_ptr(), msg->length());

          if (!crypto_->decode_rtps_message(plain_buffer, encoded_buffer, application_participant_crypto_handle_, remote_crypto_handle, ex)) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Message from %C could not be verified [%d.%d]: \"%C\"\n", guid_to_string(src_guid).c_str(), ex.code, ex.minor_code, ex.message.in()));
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
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: could not parse submessage\n"));
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
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: could not parse submessage\n"));
            return false;
          }
          if (rtps_discovery_->get_crypto_handle(application_domain_, application_participant_guid_) != DDS::HANDLE_NIL &&
              !(OpenDDS::DCPS::RtpsUdpDataLink::separate_message(writerId) ||
                writerId == OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER)) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: submessage %d could not be verified writerId=%02X%02X%02X%02X\n", submessage_header.submessageId, writerId.entityKey[0], writerId.entityKey[1], writerId.entityKey[2], writerId.entityKind));
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
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message trailing bytes\n"));
    return false;
  }

  return true;
}

void VerticalHandler::send(const GuidSet& to,
                           const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  const auto relay_addresses_map = populate_relay_addresses_map(to);

  size_t fan_out = 0;
  for (const auto& p : relay_addresses_map) {
    const auto& addrs = p.first;
    const auto& guids = p.second;
    fan_out += guids.size();
    if (addrs != relay_addresses_) {
      horizontal_handler_->enqueue_message(extract_relay_address(addrs), guids, msg);
    } else {
      for (const auto& guid : guids) {
        const auto p = find(guid);
        if (p != end()) {
          for (const auto& addr : p->second) {
            enqueue_message(addr, msg);
          }
        } else {
          ACE_DEBUG((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::send failed to get address\n"));
        }
      }
    }
  }
  max_fan_out(fan_out);
}

RelayAddressesMap VerticalHandler::populate_relay_addresses_map(const GuidSet& to)
{
  RelayAddressesMap relay_addresses_map;

  for (const auto& guid : to) {
    const auto relay_addresses = read_relay_addresses(guid);
    if (relay_addresses == RelayAddresses()) {
      continue;
    }
    relay_addresses_map[relay_addresses].insert(guid);
  }

  return relay_addresses_map;
}

RelayAddresses VerticalHandler::read_relay_addresses(const OpenDDS::DCPS::RepoId& guid) const
{
  RelayAddresses relay_addresses;

  GuidRelayAddresses key;
  key.guid(repoid_to_guid(guid));
  DDS::InstanceHandle_t handle = responsible_relay_reader_->lookup_instance(key);
  if (handle == DDS::HANDLE_NIL) {
    return relay_addresses;
  }
  GuidRelayAddressesSeq received_data;
  DDS::SampleInfoSeq info_seq;
  const auto ret = responsible_relay_reader_->read_instance(received_data, info_seq, 1, handle,
                                                            DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: VerticalHandler::read_relay_addresses failed to read\n"));
    return relay_addresses;
  }

  return received_data[0].relay_addresses();
}

void VerticalHandler::write_relay_addresses(const OpenDDS::DCPS::RepoId& guid,
                                            const RelayAddresses& relay_addresses)
{
  GuidRelayAddresses gra = {
    repoid_to_guid(guid),
    relay_addresses
  };

  const auto ret = responsible_relay_writer_->write(gra, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: VerticalHandler::write_relay_addresses failed to write\n"));
  }
}

void VerticalHandler::unregister_relay_addresses(const OpenDDS::DCPS::RepoId& guid)
{
  GuidRelayAddresses gra = {
    repoid_to_guid(guid),
    RelayAddresses()
  };

  const auto ret = responsible_relay_writer_->unregister_instance(gra, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: VerticalHandler::unregister_relay_addresses failed to unregister_instance\n"));
  }
}

HorizontalHandler::HorizontalHandler(ACE_Reactor* reactor)
  : RelayHandler(reactor)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::enqueue_message(const ACE_INET_Addr& addr,
                                        const GuidSet& guids,
                                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  // Determine how many guids we can pack into a single UDP message.
  const auto max_guids_per_message = (OpenDDS::DCPS::TransportSendStrategy::UDP_MAX_MESSAGE_SIZE - msg->length() - 4) / sizeof(OpenDDS::DCPS::RepoId);

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

    size_t size = 0, padding = 0;
    OpenDDS::DCPS::gen_find_size(relay_header, size, padding);
    OpenDDS::DCPS::Message_Block_Shared_Ptr header_block(new ACE_Message_Block(size + padding));
    OpenDDS::DCPS::Serializer ser(header_block.get());
    ser << relay_header;
    header_block->cont(msg.get()->duplicate());
    RelayHandler::enqueue_message(addr, header_block);
  }
}

void HorizontalHandler::process_message(const ACE_INET_Addr&,
                                        const OpenDDS::DCPS::MonotonicTimePoint&,
                                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  OpenDDS::RTPS::MessageParser mp(*msg);

  const size_t size_before_header = mp.remaining();

  RelayHeader relay_header;
  if (!(mp >> relay_header)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to deserialize Relay header\n"));
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
      ACE_DEBUG((LM_WARNING, "(%P|%t) %N:%l WARNING: HorizontalHandler::process_message failed to get address\n"));
    }
  }
  max_fan_out(relay_header.to().size());
}

SpdpHandler::SpdpHandler(ACE_Reactor* reactor,
                         const RelayAddresses& relay_addresses,
                         const AssociationTable& association_table,
                         GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
                         GuidRelayAddressesDataReader_ptr responsible_relay_reader,
                         const OpenDDS::DCPS::TimeDuration& lifespan,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         DDS::DomainId_t application_domain,
                         const OpenDDS::DCPS::RepoId& application_participant_guid,
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_guid , crypto)
, application_participant_addr_(application_participant_addr)
{}

ACE_INET_Addr SpdpHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return ACE_INET_Addr(relay_addresses.spdp_relay_address().c_str());
}

bool SpdpHandler::do_normal_processing(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  if (src_guid == application_participant_guid_) {
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
      max_fan_out(to.size());
    } else {
      // Forward to all peers except the application participant.
      for (const auto& p : guid_addr_map_) {
        if (p.first != application_participant_guid_) {
          for (const auto& addr : p.second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(guid_addr_map_.size());
    }

    return false;
  }

  // SPDP message is from a client.
  if (to.empty() || to.count(application_participant_guid_) != 0) {
    // Forward to the application participant.
    enqueue_message(application_participant_addr_, msg);
    max_fan_out(1);
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

  ACE_GUARD(ACE_Thread_Mutex, g, spdp_messages_mutex_);

  const auto pos = spdp_messages_.find(from_guid);
  if (pos == spdp_messages_.end()) {
    return;
  }

  send(to, pos->second);
}

SedpHandler::SedpHandler(ACE_Reactor* reactor,
                         const RelayAddresses& relay_addresses,
                         const AssociationTable& association_table,
                         GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
                         GuidRelayAddressesDataReader_ptr responsible_relay_reader,
                         const OpenDDS::DCPS::TimeDuration& lifespan,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         DDS::DomainId_t application_domain,
                         const OpenDDS::DCPS::RepoId& application_participant_guid,
                         const CRYPTO_TYPE& crypto,
                         const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_guid, crypto)
  , application_participant_addr_(application_participant_addr)
{}

ACE_INET_Addr SedpHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return ACE_INET_Addr(relay_addresses.sedp_relay_address().c_str());
}

bool SedpHandler::do_normal_processing(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::RepoId& src_guid,
                                       const GuidSet& to,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  if (src_guid == application_participant_guid_) {
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
      max_fan_out(to.size());
    } else {
      // Forward to all peers except the application participant.
      for (const auto& p : guid_addr_map_) {
        if (p.first != application_participant_guid_) {
          for (const auto& addr : p.second) {
            enqueue_message(addr, msg);
          }
        }
      }
      max_fan_out(guid_addr_map_.size());
    }

    return false;
  }

  // SEDP message is from a client.
  if (to.empty() || to.count(application_participant_guid_) != 0) {
    // Forward to the application participant.
    enqueue_message(application_participant_addr_, msg);
    max_fan_out(1);
  }
  return true;
}

DataHandler::DataHandler(ACE_Reactor* reactor,
                         const RelayAddresses& relay_addresses,
                         const AssociationTable& association_table,
                         GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
                         GuidRelayAddressesDataReader_ptr responsible_relay_reader,
                         const OpenDDS::DCPS::TimeDuration& lifespan,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         DDS::DomainId_t application_domain,
                         const OpenDDS::DCPS::RepoId& application_participant_guid,
                         const CRYPTO_TYPE& crypto
                         )
: VerticalHandler(reactor, relay_addresses, association_table, responsible_relay_writer, responsible_relay_reader, lifespan, rtps_discovery, application_domain, application_participant_guid, crypto)
{}

ACE_INET_Addr DataHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return ACE_INET_Addr(relay_addresses.data_relay_address().c_str());
}

#ifdef OPENDDS_SECURITY

StunHandler::StunHandler(ACE_Reactor* reactor)
  : RelayHandler(reactor)
{}

void StunHandler::process_message(const ACE_INET_Addr& remote_address,
                                  const OpenDDS::DCPS::MonotonicTimePoint&,
                                  const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg)
{
  OpenDDS::DCPS::Serializer serializer(msg.get(), OpenDDS::DCPS::Serializer::SWAP_BE);
  OpenDDS::STUN::Message message;
  message.block = msg.get();
  if (!(serializer >> message)) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) VerticalHandler::process_message: WARNING Could not deserialize STUN mssage\n")));
    return;
  }

  if (message.class_ != OpenDDS::STUN::REQUEST) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) VerticalHandler::process_message: WARNING Unknown STUN message class\n")));
    return;
  }

  std::vector<OpenDDS::STUN::AttributeType> unknown_attributes = message.unknown_comprehension_required_attributes();

  if (!unknown_attributes.empty()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) VerticalHandler::process_message: WARNING Unknown comprehension requird attributes\n")));
    send(remote_address, make_unknown_attributes_error_response(message, unknown_attributes));
    return;
  }

  if (!message.has_fingerprint()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) VerticalHandler::process_message: WARNING No FINGERPRINT attribute\n")));
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
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) VerticalHandler::process_message: WARNING Unknown STUN method\n")));
    send(remote_address, make_bad_request_error_response(message, "Bad Request: Unknown method"));
    break;
  }
}

void StunHandler::send(const ACE_INET_Addr& addr, OpenDDS::STUN::Message message)
{
  using namespace OpenDDS::DCPS;
  using namespace OpenDDS::STUN;
  Message_Block_Shared_Ptr block(new ACE_Message_Block(HEADER_SIZE + message.length()));
  Serializer serializer(block.get(), Serializer::SWAP_BE);
  message.block = block.get();
  serializer << message;
  enqueue_message(addr, block);
}
#endif /* OPENDDS_SECURITY */

}
