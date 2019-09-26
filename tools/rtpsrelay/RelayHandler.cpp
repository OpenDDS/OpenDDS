#include "RelayHandler.h"

#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <ace/Reactor.h>

#include <array>
#include <cstring>
#include <sstream>
#include <string>

#ifdef OPENDDS_SECURITY
#include <dds/DCPS/security/framework/SecurityRegistry.h>
#endif

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
#if defined (ACE_DEFAULT_MAX_SOCKET_BUFSIZ)
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

  const auto data_block = new ACE_Data_Block(bytes, ACE_Message_Block::MB_DATA, static_cast<const char*>(iov.iov_base), 0, 0, 0, 0);
  Message_Block_Ptr buffer(new ACE_Message_Block(data_block));
  buffer->length(bytes);

  const auto rd_ptr = buffer->rd_ptr();

  OpenDDS::DCPS::Serializer serializer(buffer.get(), false, OpenDDS::DCPS::Serializer::ALIGN_CDR);
  OpenDDS::RTPS::Header header;
  if (!(serializer >> header)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to deserialize RTPS header\n"));
    return 0;
  }

  bool is_beacon_message = buffer->length() == OpenDDS::RTPS::BEACON_MESSAGE_LENGTH &&
    static_cast<CORBA::Octet>(buffer->rd_ptr()[0]) == OpenDDS::RTPS::BEACON_MSG_ID;

  OpenDDS::DCPS::RepoId src_guid;
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;

  buffer->rd_ptr(rd_ptr);
  process_message(remote, ACE_Time_Value().now(), src_guid, buffer.get(), is_beacon_message);
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
                                 const ACE_Time_Value& purge_period,
                                 const OpenDDS::DCPS::RepoId& application_participant_guid)
  : RelayHandler(a_reactor, a_association_table)
  , horizontal_handler_(nullptr)
  , lifespan_(lifespan)
  , application_participant_guid_(application_participant_guid)
{
  reactor()->schedule_timer(this, 0, purge_period, purge_period);
}

void VerticalHandler::process_message(const ACE_INET_Addr& a_remote,
                                      const ACE_Time_Value& a_now,
                                      const OpenDDS::DCPS::RepoId& a_src_guid,
                                      ACE_Message_Block* a_msg,
                                      bool is_beacon_message)
{
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

  // Readers send empty messages so we know where they are.
  if (is_beacon_message) {
    return;
  }

  std::set<std::string> addrs;
  std::set<std::string> horizontal_relay_addrs;

  GuidSet guids;
  association_table_.get_guids_from_local(a_src_guid, guids);

  for (const auto& guid : guids) {
    auto p = find(guid);
    if (p != end()) {
      // Local.
      addrs.insert(p->second);
      continue;
    }
    auto addresses = association_table_.get_relay_addresses(guid);
    horizontal_relay_addrs.insert(extract_relay_address(addresses));
  }

  add_addresses(a_remote, a_src_guid, addrs);

  for (const auto& addr : addrs) {
    enqueue_message(addr, a_msg);
  }

  for (const auto& addr : horizontal_relay_addrs) {
    horizontal_handler_->enqueue_message(addr, a_msg);
  }
}

int VerticalHandler::handle_timeout(const ACE_Time_Value& a_now, const void*) {
  for (ExpirationGuidMap::iterator pos = expiration_guid_map_.begin(), limit = expiration_guid_map_.end(); pos != limit && pos->first < a_now;) {
    guid_addr_map_.erase(pos->second);
    guid_expiration_map_.erase(pos->second);
    expiration_guid_map_.erase(pos++);
  }
  return 0;
}

HorizontalHandler::HorizontalHandler(ACE_Reactor* a_reactor,
                                     const AssociationTable& a_association_table)
  : RelayHandler(a_reactor, a_association_table)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::process_message(const ACE_INET_Addr&,
                                        const ACE_Time_Value&,
                                        const OpenDDS::DCPS::RepoId& a_src_guid,
                                        ACE_Message_Block* a_msg,
                                        bool /*is_beacon_message*/)
{
  std::set<std::string> addrs;

  GuidSet guids;
  association_table_.get_guids_to_local(a_src_guid, guids);

  for (auto guid : guids) {
    auto p = vertical_handler_->find(guid);

    if (p != vertical_handler_->end()) {
      // Local.
      addrs.insert(p->second);
    } else {
      ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: HorizontalHandler::process_message failed to get address\n"));
    }
  }

  for (auto addr : addrs) {
    vertical_handler_->enqueue_message(addr, a_msg);
  }
}

SpdpHandler::SpdpHandler(ACE_Reactor* a_reactor,
                         const AssociationTable& a_association_table,
                         const ACE_Time_Value& lifespan,
                         const ACE_Time_Value& purge_period,
                         const OpenDDS::DCPS::RepoId& application_participant_guid)
  : VerticalHandler(a_reactor, a_association_table, lifespan, purge_period, application_participant_guid)
{}

std::string SpdpHandler::extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const
{
  return relay_addresses.spdp_relay_address();
}

void SpdpHandler::add_addresses(const ACE_INET_Addr& a_remote,
                                const OpenDDS::DCPS::RepoId& a_src_guid,
                                std::set<std::string>& a_addrs)
{
  if (a_src_guid == application_participant_guid_) {
    // SPDP message is from the application participant.
    // Forward to all peers except the application participant.
    if (application_participant_addr_.empty()) {
      application_participant_addr_ = addr_to_string(a_remote);
      ACE_DEBUG((LM_INFO, "(%P|%t) %N:%l SpdpHandler::add_addresses application participant has address %C\n", application_participant_addr_.c_str()));
    }
    for (const auto p : guid_addr_map_) {
      if (p.first != application_participant_guid_) {
        a_addrs.insert(p.second);
      }
    }

    return;
  }

  // SPDP message is from a client.
  // Forward to the application participant.
  if (!application_participant_addr_.empty()) {
    a_addrs.insert(application_participant_addr_);
  }
}

SedpHandler::SedpHandler(ACE_Reactor* a_reactor,
                         const AssociationTable& a_association_table,
                         const ACE_Time_Value& lifespan,
                         const ACE_Time_Value& purge_period,
                         const OpenDDS::DCPS::RepoId& application_participant_guid)
  : VerticalHandler(a_reactor, a_association_table, lifespan, purge_period, application_participant_guid)
{}

std::string SedpHandler::extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const
{
  return relay_addresses.sedp_relay_address();
}

void SedpHandler::add_addresses(const ACE_INET_Addr& a_remote,
                                const OpenDDS::DCPS::RepoId& a_src_guid,
                                std::set<std::string>& a_addrs)
{
  if (a_src_guid == application_participant_guid_) {
    // SEDP message is from the application participant.
    // Forward to all peers except the application participant.
    if (application_participant_addr_.empty()) {
      application_participant_addr_ = addr_to_string(a_remote);
      ACE_DEBUG((LM_INFO, "(%P|%t) %N:%l SedpHandler::add_addresses application participant has address %C\n", application_participant_addr_.c_str()));
    }
    for (const auto p : guid_addr_map_) {
      if (p.first != application_participant_guid_) {
        a_addrs.insert(p.second);
      }
    }

    return;
  }

  // SEDP message is from a client.
  // Forward to the application participant.
  if (!application_participant_addr_.empty()) {
    a_addrs.insert(application_participant_addr_);
  }
}

DataHandler::DataHandler(ACE_Reactor* a_reactor,
                         const AssociationTable& a_association_table,
                         const ACE_Time_Value& lifespan,
                         const ACE_Time_Value& purge_period,
                         const OpenDDS::DCPS::RepoId& application_participant_guid)
  : VerticalHandler(a_reactor, a_association_table, lifespan, purge_period, application_participant_guid)
{}

std::string DataHandler::extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const
{
  return relay_addresses.data_relay_address();
}
