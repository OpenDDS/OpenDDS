#include "RelayHandler.h"

#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DCPS/Message_Block_Ptr.h>

#include <ace/Reactor.h>

#include <array>
#include <cstring>
#include <sstream>
#include <string>

namespace {
  const CORBA::UShort encap_LE = 0x0300; // {PL_CDR_LE} in LE
  const CORBA::UShort encap_BE = 0x0200; // {PL_CDR_BE} in LE

  std::string guid_to_string(const OpenDDS::DCPS::GUID_t& a_guid)
  {
    std::stringstream ss;
    ss << a_guid;
    return ss.str();
  }

  void parse_property(GroupTable::GroupSet& a_groups, const DDS::Property_t& a_property)
  {
    static const std::string GROUPS_PROPERTY_NAME = "OpenDDS.RtpsRelay.Groups";
    if (a_property.name.in() == GROUPS_PROPERTY_NAME) {
      std::string value(a_property.value);
      for (size_t idx = 0, limit = value.length(); idx != limit;) {
        size_t const n = value.find(',', idx);
        a_groups.insert(value.substr(idx, (n == std::string::npos) ? n : n - idx));
        idx = (n == std::string::npos) ? limit : n + 1;
      }
    }
  }

  void parse_property_seq(GroupTable::GroupSet& a_groups, const DDS::PropertySeq& a_property_seq)
  {
    for (auto idx = 0u, count = a_property_seq.length(); idx != count; ++idx) {
      parse_property(a_groups, a_property_seq[idx]);
    }
  }

  void parse_rtps_parameter(GroupTable::GroupSet& a_groups, const OpenDDS::RTPS::Parameter& a_parameter)
  {
    if (a_parameter._d() == OpenDDS::RTPS::PID_PROPERTY_LIST) {
      parse_property_seq(a_groups, a_parameter.property().value);
    }
  }

  std::string addr_to_string(const ACE_INET_Addr& a_addr)
  {
    std::array<ACE_TCHAR, 256> as_string{};
    if (a_addr.addr_to_string(as_string.data(), as_string.size()) != 0) {
      ACE_ERROR((LM_ERROR, "(%P:%t) %N:%l ERROR: addr_to_string failed to convert address to string"));
      return "";
    }
    return ACE_TEXT_ALWAYS_CHAR(as_string.data());
  }
}

RelayHandler::RelayHandler(ACE_Reactor* a_reactor,
                           const GroupTable& a_group_table)
  : ACE_Event_Handler(a_reactor)
  , group_table_(a_group_table)
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

  //
  bool is_beacon_message = false;
  if ((buffer->length() == OpenDDS::RTPS::BEACON_MESSAGE_LENGTH)
    && (static_cast<CORBA::Octet>(buffer->rd_ptr()[0]) == OpenDDS::RTPS::BEACON_MSG_ID)) {
    is_beacon_message = true;
  }

  OpenDDS::DCPS::RepoId src_guid;
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
  const auto guid = guid_to_string(src_guid);

  buffer->rd_ptr(rd_ptr);
  process_message(remote, ACE_Time_Value().now(), guid, buffer.get(), is_beacon_message);
  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE)
{
  if (!outgoing_.empty()) {
    const auto& out = outgoing_.front();

    ACE_INET_Addr addr(out.first.c_str());
    const auto bytes = socket_.send(out.second->rd_ptr(), out.second->length(), addr, 0, 0);

    if (bytes < 0) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_output failed to send to %C\n", out.first.c_str()));
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
                                 const GroupTable& a_group_table,
                                 RoutingTable& a_routing_table)
  : RelayHandler(a_reactor, a_group_table)
  , routing_table_(a_routing_table)
  , horizontal_handler_(nullptr)
{}

void VerticalHandler::process_message(const ACE_INET_Addr& a_remote,
                                      const ACE_Time_Value& a_now,
                                      const std::string& a_src_guid,
                                      ACE_Message_Block* a_msg,
                                      bool is_beacon_message)
{
  // Readers send empty messages so we know where they are.
  routing_table_.update(a_src_guid, horizontal_handler_->relay_address(), addr_to_string(a_remote), a_now);

  if (is_beacon_message) {
    return;
  }

  std::set<std::string> addrs;
  std::set<std::string> horizontal_relay_addrs;

  for (const auto& group : group_table_.groups(a_src_guid)) {
    for (const auto& guid : group_table_.guids(group)) {
      // Don't reflect.
      if (guid == a_src_guid) {
        continue;
      }

      auto addr = routing_table_.horizontal_relay_address(guid);
      if (addr.empty()) {
        ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get horizontal relay address for %C\n", guid.c_str()));
        continue;
      }

      if (addr == horizontal_handler_->relay_address()) {
        // Replace with local.
        addr = routing_table_.address(guid);
        if (addr.empty()) {
          ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get address for %C\n", addr.c_str()));
          continue;
        }
        addrs.insert(addr);
      } else {
        horizontal_relay_addrs.insert(addr);
      }
    }
  }

  for (const auto& addr : addrs) {
    enqueue_message(addr, a_msg);
  }

  for (const auto& addr : horizontal_relay_addrs) {
    horizontal_handler_->enqueue_message(addr, a_msg);
  }
}

HorizontalHandler::HorizontalHandler(ACE_Reactor* a_reactor,
                                     const GroupTable& a_group_table,
                                     const RoutingTable& a_routing_table)
  : RelayHandler(a_reactor, a_group_table)
  , routing_table_(a_routing_table)
  , vertical_handler_(nullptr)
{}

void HorizontalHandler::process_message(const ACE_INET_Addr&,
                                        const ACE_Time_Value&,
                                        const std::string& a_src_guid,
                                        ACE_Message_Block* a_msg,
                                        bool is_beacon_message)
{
  if (is_beacon_message) {
    return;
  }

  std::set<std::string> addrs;

  for (auto group : group_table_.groups(a_src_guid)) {
    for (auto guid : group_table_.guids(group)) {
      // Don't reflect.
      if (guid == a_src_guid) {
        continue;
      }

      auto addr = routing_table_.horizontal_relay_address(guid);
      if (addr != relay_address()) {
        continue;
      }

      // Replace with local.
      addr = routing_table_.address(guid);
      if (addr.empty()) {
        ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: HorizontalHandler::process_message failed to get address for %C\n", guid.c_str()));
        continue;
      }

      addrs.insert(addr);
    }
  }

  for (auto addr : addrs) {
    vertical_handler_->enqueue_message(addr, a_msg);
  }
}

SpdpHandler::SpdpHandler(ACE_Reactor* a_reactor,
                         GroupTable& a_group_table,
                         RoutingTable& a_routing_table)
  : VerticalHandler(a_reactor, a_group_table, a_routing_table)
  , mutable_group_table_(a_group_table)
{}

void SpdpHandler::process_message(const ACE_INET_Addr& a_remote,
                                  const ACE_Time_Value& a_now,
                                  const std::string& a_src_guid,
                                  ACE_Message_Block* a_buffer,
                                  bool is_beacon_message)
{
  if (is_beacon_message) {
    return;
  }

  const auto rd_ptr = a_buffer->rd_ptr();

  OpenDDS::DCPS::Serializer serializer(a_buffer, false, OpenDDS::DCPS::Serializer::ALIGN_CDR);
  OpenDDS::RTPS::Header header;
  if (!(serializer >> header)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS header\n"));
    return;
  }

  while (a_buffer->length() > 3) {
    const auto subm = a_buffer->rd_ptr()[0], flags = a_buffer->rd_ptr()[1];
    serializer.swap_bytes((flags & OpenDDS::RTPS::FLAG_E) != ACE_CDR_BYTE_ORDER);
    const auto start = a_buffer->length();
    CORBA::UShort submessageLength = 0;
    switch (subm) {
    case OpenDDS::RTPS::DATA: {
      OpenDDS::RTPS::DataSubmessage data;
      if (!(serializer >> data)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS data header\n"));
        return;
      }
      submessageLength = data.smHeader.submessageLength;

      if (data.writerId != OpenDDS::DCPS::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER) {
        // Not our message: this could be the same multicast group used
        // for SEDP and other traffic.
        break;
      }

      OpenDDS::RTPS::ParameterList plist;
      if (data.smHeader.flags & (OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_K_IN_DATA)) {
        serializer.swap_bytes(!ACE_CDR_BYTE_ORDER); // read "encap" itself in LE
        CORBA::UShort encap, options;
        if (!(serializer >> encap) || (encap != encap_LE && encap != encap_BE)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS encapsulation header\n"));
          return;
        }
        serializer >> options;
        // bit 8 in encap is on if it's PL_CDR_LE
        serializer.swap_bytes(((encap & 0x100) >> 8) != ACE_CDR_BYTE_ORDER);
        if (!(serializer >> plist)) {
          ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS data payload\n"));
          return;
        }
      } else {
        plist.length(1);
        OpenDDS::DCPS::RepoId guid;
        std::memcpy(guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
        guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
        plist[0].guid(guid);
        plist[0]._d(OpenDDS::RTPS::PID_PARTICIPANT_GUID);
      }

      GroupTable::GroupSet groups;

      for (auto idx = 0u, count = plist.length(); idx != count; ++idx) {
        parse_rtps_parameter(groups, plist[idx]);
      }

      mutable_group_table_.update(a_src_guid, groups, a_now);

      break;
    }
    default:
      OpenDDS::RTPS::SubmessageHeader smHeader;
      if (!(serializer >> smHeader)) {
        ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS submessage header\n"));
        return;
      }
      submessageLength = smHeader.submessageLength;
      break;
    }

    if (submessageLength && a_buffer->length()) {
      const auto read = start - a_buffer->length();
      if (read < static_cast<size_t>(submessageLength + OpenDDS::RTPS::SMHDR_SZ)) {
        if (!serializer.skip(static_cast<CORBA::UShort>(submessageLength + OpenDDS::RTPS::SMHDR_SZ
                                                        - read))) {
          ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to skip submessage length\n"));
          return;
        }
      }
    } else if (!submessageLength) {
      break; // submessageLength of 0 indicates the last submessage
    }
  }

  a_buffer->rd_ptr(rd_ptr);
  VerticalHandler::process_message(a_remote, a_now, a_src_guid, a_buffer, is_beacon_message);
}
