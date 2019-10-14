#include "RelayHandler.h"

#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsGuidTypeSupportImpl.h>

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

RelayHandler::RelayHandler(ACE_Reactor* a_reactor,
                           const AssociationTable& a_association_table)
  : ACE_Event_Handler(a_reactor)
  , association_table_(a_association_table)
  , bytes_received_(0)
  , bytes_sent_(0)
{}

int RelayHandler::open(const ACE_INET_Addr& a_local)
{
  relay_address_ = addr_to_string(a_local);

  if (socket_.open(a_local) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to open socket on '%C'\n", relay_address_.c_str()));
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

int RelayHandler::handle_input(ACE_HANDLE)
{
  using OpenDDS::DCPS::Message_Block_Ptr;
  ACE_INET_Addr remote;
  iovec iov;
  const auto bytes = socket_.recv(&iov, remote);

  if (bytes <= 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to recv\n"));
    return 0;
  }

  bytes_received_ += bytes;

  const auto data_block =
    new (ACE_Allocator::instance()->malloc(sizeof(ACE_Data_Block))) ACE_Data_Block(bytes, ACE_Message_Block::MB_DATA,
                                                                                   static_cast<const char*>(iov.iov_base), 0, 0, 0, 0);
  Message_Block_Ptr buffer(new ACE_Message_Block(data_block));
  buffer->length(bytes);

  const auto rd_ptr = buffer->rd_ptr();

  OpenDDS::DCPS::RepoId src_guid;
  OpenDDS::RTPS::MessageParser mp(*buffer);
  if (!mp.parseHeader()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message failed to deserialize RTPS header\n"));
    return 0;
  }

  const auto& header = mp.header();
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  buffer->rd_ptr(rd_ptr);
  process_message(remote, ACE_Time_Value().now(), src_guid, buffer.get());
  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE)
{
  if (!outgoing_.empty()) {
    const auto& out = outgoing_.front();

    ACE_INET_Addr addr(out.first.c_str());
    const auto bytes = socket_.send(out.second->rd_ptr(), out.second->length(), addr, 0, 0);

    if (bytes < 0) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_output failed to send to %C: %m\n", out.first.c_str()));
    } else {
      bytes_sent_ += bytes;
    }

    out.second->release();
    outgoing_.pop();
  }

  if (outgoing_.empty()) {
    reactor()->remove_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::enqueue_message(const std::string& a_addr, ACE_Message_Block* a_msg)
{
  const auto empty = outgoing_.empty();

  outgoing_.push(std::make_pair(a_addr, a_msg->duplicate()));

  if (empty) {
    reactor()->register_handler(this, WRITE_MASK);
  }
}

VerticalHandler::VerticalHandler(ACE_Reactor* a_reactor,
                                 const AssociationTable& a_association_table,
                                 const ACE_Time_Value& lifespan,
                                 const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                                 DDS::DomainId_t application_domain,
                                 const OpenDDS::DCPS::RepoId& application_participant_guid
#ifdef OPENDDS_SECURITY
                                 ,const DDS::Security::CryptoTransform_var& crypto
#endif
                                 )
  : RelayHandler(a_reactor, a_association_table)
  , lifespan_(lifespan)
  , application_participant_guid_(application_participant_guid)
  , horizontal_handler_(nullptr)
  , rtps_discovery_(rtps_discovery)
  , application_domain_(application_domain)
#ifdef OPENDDS_SECURITY
  , crypto_(crypto)
  , application_participant_crypto_handle_(rtps_discovery_->get_crypto_handle(application_domain_, application_participant_guid_))
#endif
{}

void VerticalHandler::process_message(const ACE_INET_Addr& a_remote,
                                      const ACE_Time_Value& a_now,
                                      const OpenDDS::DCPS::RepoId& a_src_guid,
                                      ACE_Message_Block* a_msg)
{
  bool is_beacon = true;

  const auto rd_ptr = a_msg->rd_ptr();
  OpenDDS::RTPS::MessageParser mp(*a_msg);
  if (!parse_message(mp, a_msg, is_beacon, true)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: VerticalHandler::process_message failed to parse_message\n"));
    return;
  }
  a_msg->rd_ptr(rd_ptr);

  const std::string addr_str = addr_to_string(a_remote);
  guid_addr_map_[a_src_guid] = addr_str;
  // Compute the new expiration time for this SPDP client.
  const ACE_Time_Value expiration = a_now + lifespan_;
  std::pair<GuidExpirationMap::iterator, bool> res = guid_expiration_map_.insert(std::make_pair(a_src_guid, expiration));
  if (!res.second) {
    // The SPDP client already exists.  Remove the previous expiration.
    const ACE_Time_Value previous_expiration = res.first->second;
    std::pair<ExpirationGuidMap::iterator, ExpirationGuidMap::iterator> r = expiration_guid_map_.equal_range(previous_expiration);
    while (r.first != r.second && r.first->second != a_src_guid) {
      ++r.first;
    }
    expiration_guid_map_.erase(r.first);
    // Assign the new expiration time.
    res.first->second = expiration;
  }
  // Assign the new expiration time.
  expiration_guid_map_.insert(std::make_pair(expiration, a_src_guid));

  // Process expirations.
  for (ExpirationGuidMap::iterator pos = expiration_guid_map_.begin(), limit = expiration_guid_map_.end(); pos != limit && pos->first < a_now;) {
    purge(pos->second);
    guid_addr_map_.erase(pos->second);
    guid_expiration_map_.erase(pos->second);
    expiration_guid_map_.erase(pos++);
  }

  // Readers send empty messages so we know where they are.
  if (is_beacon) {
    return;
  }

  if (do_normal_processing(a_remote, a_src_guid, a_msg)) {
    GuidSet local_guids;
    RelayAddressesSet relay_addresses;
    association_table_.get_guids(a_src_guid, local_guids, relay_addresses);

    for (const auto& guid : local_guids) {
      auto p = find(guid);
      if (p != end()) {
        enqueue_message(p->second, a_msg);
      } else {
        ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get address\n"));
      }
    }

    for (const auto& addrs : relay_addresses) {
      horizontal_handler_->enqueue_message(extract_relay_address(addrs), a_msg);
    }
  }
}

HorizontalHandler::HorizontalHandler(ACE_Reactor* a_reactor,
                                     const AssociationTable& a_association_table)
  : RelayHandler(a_reactor, a_association_table)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::process_message(const ACE_INET_Addr&,
                                        const ACE_Time_Value&,
                                        const OpenDDS::DCPS::RepoId& a_src_guid,
                                        ACE_Message_Block* a_msg)
{
  GuidSet local_guids;
  RelayAddressesSet relay_addresses;
  association_table_.get_guids(a_src_guid, local_guids, relay_addresses);

  for (auto guid : local_guids) {
    auto p = vertical_handler_->find(guid);
    if (p != vertical_handler_->end()) {
      vertical_handler_->enqueue_message(p->second, a_msg);
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: HorizontalHandler::process_message failed to get address\n"));
    }
  }
}

SpdpHandler::SpdpHandler(ACE_Reactor* a_reactor
                         ,const AssociationTable& a_association_table
                         ,const ACE_Time_Value& lifespan
                         ,const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery
                         ,DDS::DomainId_t application_domain
                         ,const OpenDDS::DCPS::RepoId& application_participant_guid
#ifdef OPENDDS_SECURITY
                         ,const DDS::Security::CryptoTransform_var& crypto
#endif
                         ,const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(a_reactor, a_association_table, lifespan, rtps_discovery, application_domain, application_participant_guid
#ifdef OPENDDS_SECURITY
                  , crypto
#endif
                  )
  , application_participant_addr_(application_participant_addr)
  , application_participant_addr_str_(addr_to_string(application_participant_addr))
  , spdp_message_(nullptr)
{}

std::string SpdpHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return relay_addresses.spdp_relay_address();
}

bool SpdpHandler::do_normal_processing(const ACE_INET_Addr& a_remote,
                                       const OpenDDS::DCPS::RepoId& a_src_guid,
                                       ACE_Message_Block* a_msg)
{
  if (a_src_guid == application_participant_guid_) {
    if (a_remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      return false;
    }

    // SPDP message is from the application participant.
    // Cache the SPDP announcement from the application participant.
    if (spdp_message_) {
      spdp_message_->release();
    }
    spdp_message_ = a_msg->duplicate();

    return false;
  }

  // SPDP message is from a client.
  // Cache it.
  auto p = spdp_messages_.insert(std::make_pair(a_src_guid, nullptr));
  if (p.first->second) {
    p.first->second->release();
  }
  p.first->second = a_msg->duplicate();

  // Forward to the application participant.
  enqueue_message(application_participant_addr_str_, a_msg);

  // Send the client the SPDP from the application participant.
  if (spdp_message_) {
    enqueue_message(addr_to_string(a_remote), spdp_message_);
  }

  return true;
}

bool VerticalHandler::parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                                    ACE_Message_Block* msg,
                                    bool& is_beacon,
                                    bool check_submessages)
{
  if (!message_parser.parseHeader()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::parse_message failed to deserialize RTPS header\n"));
    return false;
  }

  const auto& header = message_parser.header();
  OpenDDS::DCPS::RepoId src_guid;
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  while (message_parser.parseSubmessage()) {
    const auto submessage_header = message_parser.submessageHeader();

    if (submessage_header.submessageId != OpenDDS::RTPS::PAD) {
      is_beacon = false;
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

          encoded_buffer.length(msg->length());
          std::memcpy(encoded_buffer.get_buffer(), msg->rd_ptr(), msg->length());

          if (!crypto_->decode_rtps_message(plain_buffer, encoded_buffer, application_participant_crypto_handle_, remote_crypto_handle, ex)) {
            ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: Message from %C could not be verified [%d.%d]: \"%C\"\n", guid_to_string(src_guid).c_str(), ex.code, ex.minor_code, ex.message.in()));
            return false;
          }

          OpenDDS::RTPS::MessageParser mp(plain_buffer);
          is_beacon = true;
          return parse_message(mp, msg, is_beacon, false);
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

void SpdpHandler::purge(const OpenDDS::DCPS::RepoId& guid)
{
  SpdpMessages::iterator pos = spdp_messages_.find(guid);
  if (pos != spdp_messages_.end()) {
    pos->second->release();
    spdp_messages_.erase(pos);
  }
}

void SpdpHandler::replay(const OpenDDS::DCPS::RepoId& x,
                         const GuidSet& local_guids,
                         const RelayAddressesSet& relay_addresses)
{
  if (local_guids.empty() && relay_addresses.empty()) {
    return;
  }

  const OpenDDS::DCPS::RepoId a_src_guid = to_participant_guid(x);

  SpdpMessages::const_iterator pos = spdp_messages_.find(a_src_guid);
  if (pos == spdp_messages_.end()) {
    return;
  }

  for (const auto& guid : local_guids) {
    auto p = find(guid);
    if (p != end()) {
      enqueue_message(p->second, pos->second);
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get address\n"));
    }
  }

  for (const auto& addrs : relay_addresses) {
    horizontal_handler_->enqueue_message(extract_relay_address(addrs), pos->second);
  }
}

SedpHandler::SedpHandler(ACE_Reactor* a_reactor
                         ,const AssociationTable& a_association_table
                         ,const ACE_Time_Value& lifespan
                         ,const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery
                         ,DDS::DomainId_t application_domain
                         ,const OpenDDS::DCPS::RepoId& application_participant_guid
#ifdef OPENDDS_SECURITY
                         ,const DDS::Security::CryptoTransform_var& crypto
#endif
                         ,const ACE_INET_Addr& application_participant_addr)
: VerticalHandler(a_reactor, a_association_table, lifespan, rtps_discovery, application_domain, application_participant_guid
#ifdef OPENDDS_SECURITY
                  , crypto
#endif
                  )
  , application_participant_addr_(application_participant_addr)
  , application_participant_addr_str_(addr_to_string(application_participant_addr))
{}

std::string SedpHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return relay_addresses.sedp_relay_address();
}

bool SedpHandler::do_normal_processing(const ACE_INET_Addr& a_remote,
                                       const OpenDDS::DCPS::RepoId& a_src_guid,
                                       ACE_Message_Block* a_msg)
{
  if (a_src_guid == application_participant_guid_) {
    if (a_remote != application_participant_addr_) {
      // Something is impersonating our application participant.
      return false;
    }

    // SEDP message is from the application participant.
    // Forward to all peers except the application participant.
    for (const auto p : guid_addr_map_) {
      if (p.first != application_participant_guid_) {
        enqueue_message(p.second, a_msg);
      }
    }

    return false;
  }

  // SEDP message is from a client.
  // Forward to the application participant.
  enqueue_message(application_participant_addr_str_, a_msg);
  return true;
}

DataHandler::DataHandler(ACE_Reactor* a_reactor,
                         const AssociationTable& a_association_table,
                         const ACE_Time_Value& lifespan,
                         const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                         DDS::DomainId_t application_domain,
                         const OpenDDS::DCPS::RepoId& application_participant_guid
#ifdef OPENDDS_SECURITY
                         ,const DDS::Security::CryptoTransform_var& crypto
#endif
                         )
: VerticalHandler(a_reactor, a_association_table, lifespan, rtps_discovery, application_domain, application_participant_guid
#ifdef OPENDDS_SECURITY
                  , crypto
#endif
                  )
{}

std::string DataHandler::extract_relay_address(const RelayAddresses& relay_addresses) const
{
  return relay_addresses.data_relay_address();
}

}
