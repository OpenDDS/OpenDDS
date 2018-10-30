#include "RelayHandler.h"

#include <array>
#include <sstream>

#include <ace/Reactor.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

namespace {
  const CORBA::UShort encap_LE = 0x0300; // {PL_CDR_LE} in LE
  const CORBA::UShort encap_BE = 0x0200; // {PL_CDR_BE} in LE

  std::string guid_to_string(OpenDDS::DCPS::GUID_t const & a_guid) {
    std::stringstream ss;
    ss << a_guid;
    return ss.str();
  }

  void parse_property(GroupTable::GroupSet & a_groups, DDS::Property_t const & a_property) {
    if (strncmp(a_property.name.in(), "OpenDDS.RtpsRelay.Groups", strlen(a_property.name.in())) == 0) {
      std::string value(a_property.value.in());
      for (size_t idx = 0, limit = value.length(); idx != limit;) {
        size_t const n = value.find(',', idx);
        a_groups.insert(value.substr(idx, (n == std::string::npos) ? n : n - idx));
        idx = (n == std::string::npos) ? limit : n + 1;
      }
    }
  }

  void parse_property_seq(GroupTable::GroupSet & a_groups, DDS::PropertySeq const & a_property_seq) {
    for (size_t idx = 0, count = a_property_seq.length(); idx != count; ++idx) {
      parse_property(a_groups, a_property_seq[idx]);
    }
  }

  void parse_rtps_parameter(GroupTable::GroupSet & a_groups, OpenDDS::RTPS::Parameter const & a_parameter) {
    if (a_parameter._d() == OpenDDS::RTPS::PID_PROPERTY_LIST) {
      parse_property_seq(a_groups, a_parameter.property().value);
    }
  }

  std::string addr_to_string(ACE_INET_Addr const & a_addr) {
    std::array<char, 256> as_string{};
    if (a_addr.addr_to_string(as_string.data(), as_string.size()) != 0) {
      ACE_ERROR((LM_ERROR, "(%P:%t) %N:%l ERROR: addr_to_string failed to convert address to string"));
      return "";
    }
    return as_string.data();
  }
}

RelayHandler::RelayHandler(ACE_Reactor * a_reactor,
                           GroupTable const & a_group_table)
  : ACE_Event_Handler(a_reactor),
    m_group_table(a_group_table),
    m_bytes_received(0),
    m_bytes_sent(0)
{ }

int RelayHandler::open(ACE_INET_Addr const & a_local) {
  m_relay_address = addr_to_string(a_local);

  if (m_socket.open(a_local) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to open socket on '%s'\n", m_relay_address.c_str()));
    return -1;
  }
  if (m_socket.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to enable ACE_NONBLOCK\n"));
    return -1;
  }
  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::open failed to register READ_MASK handler\n"));
    return -1;
  }

  return 0;
}

int RelayHandler::handle_input(ACE_HANDLE /*a_handle*/) {

  ACE_Message_Block* buffer = nullptr;
  ACE_INET_Addr remote;

  iovec iov;
  ssize_t bytes = m_socket.recv(&iov, remote);

  if (bytes <= 0) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to recv\n"));
    return 0;
  }

  m_bytes_received += bytes;

  ACE_Data_Block* data_block = new ACE_Data_Block(bytes, ACE_Message_Block::MB_DATA, static_cast<const char*>(iov.iov_base), 0, 0, 0, 0);
  buffer = new ACE_Message_Block(data_block);
  buffer->length(bytes);

  char* rd_ptr = buffer->rd_ptr();

  OpenDDS::DCPS::Serializer serializer(buffer, false, OpenDDS::DCPS::Serializer::ALIGN_CDR);
  OpenDDS::RTPS::Header header;
  if (!(serializer >> header)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_input failed to deserialize RTPS header\n"));
    buffer->release();
    return 0;
  }
  bool empty_message = buffer->length() == 0;

  OpenDDS::DCPS::RepoId src_guid;
  std::memcpy(src_guid.guidPrefix, header.guidPrefix, sizeof(OpenDDS::DCPS::GuidPrefix_t));
  src_guid.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
  const std::string guid = guid_to_string(src_guid);

  buffer->rd_ptr(rd_ptr);
  process_message(remote, ACE_Time_Value().now(), guid, buffer, empty_message);
  buffer->release();
  return 0;
}

int RelayHandler::handle_output(ACE_HANDLE /*a_handle*/) {
  if (!m_outgoing.empty()) {
    const OutgoingType::value_type& out = m_outgoing.front();

    ACE_INET_Addr addr(out.first.c_str());
    ssize_t bytes = m_socket.send(out.second->rd_ptr(), out.second->length(), addr, 0, 0);

    if (bytes < 0) {
      ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: RelayHandler::handle_output failed to send to %s\n", out.first.c_str()));
    } else {
      m_bytes_sent += bytes;
    }

    out.second->release();
    m_outgoing.pop();
  }

  if (m_outgoing.empty()) {
    this->reactor()->remove_handler(this, WRITE_MASK);
  }

  return 0;
}

void RelayHandler::enqueue_message(std::string const & a_addr, ACE_Message_Block * a_msg) {
  bool empty = m_outgoing.empty();

  m_outgoing.push(std::make_pair(a_addr, a_msg->duplicate()));

  if (empty) {
    reactor()->register_handler(this, WRITE_MASK);
  }
}

VerticalHandler::VerticalHandler(ACE_Reactor * a_reactor,
                                     GroupTable const & a_group_table,
                                     RoutingTable & a_routing_table)
  : RelayHandler(a_reactor, a_group_table),
    m_routing_table(a_routing_table),
    m_horizontal_handler(nullptr)
{}

void VerticalHandler::process_message(ACE_INET_Addr const & a_remote,
                                      ACE_Time_Value const & a_now,
                                      std::string const & a_src_guid,
                                      ACE_Message_Block * a_msg,
                                      bool a_empty_message) {
  // Readers send empty messages so we know where they are.
  m_routing_table.update(a_src_guid, m_horizontal_handler->relay_address(), addr_to_string(a_remote), a_now);

  if (a_empty_message) {
    return;
  }

  std::set<std::string> addrs;
  std::set<std::string> horizontal_relay_addrs;

  for (auto g : m_group_table.groups(a_src_guid)) {
    for (auto p : m_group_table.guids(g)) {
      // Don't reflect.
      if (p == a_src_guid) {
        continue;
      }

      std::string addr = m_routing_table.horizontal_relay_address(p);
      if (addr.empty()) {
        ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get horizontal relay address for %s\n", p.c_str()));
        continue;
      }

      if (addr == m_horizontal_handler->relay_address()) {
        // Replace with local.
        addr = m_routing_table.address(p);
        if (addr.empty()) {
          ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: VerticalHandler::process_message failed to get address for %s\n", addr.c_str()));
          continue;
        }
        addrs.insert(addr);
      } else {
        horizontal_relay_addrs.insert(addr);
      }
    }
  }

  for (auto p : addrs) {
    enqueue_message(p, a_msg);
  }

  for (auto p : horizontal_relay_addrs) {
    m_horizontal_handler->enqueue_message(p, a_msg);
  }
}

HorizontalHandler::HorizontalHandler(ACE_Reactor * a_reactor,
                                     GroupTable const & a_group_table,
                                     RoutingTable const & a_routing_table)
  : RelayHandler(a_reactor, a_group_table),
    m_routing_table(a_routing_table),
    m_vertical_handler(nullptr)
{}

void HorizontalHandler::process_message(ACE_INET_Addr const &,
                                        ACE_Time_Value const & /*a_now*/,
                                        std::string const & a_src_guid,
                                        ACE_Message_Block * a_msg,
                                        bool a_empty_message) {
  if (a_empty_message) {
    return;
  }

  std::set<std::string> addrs;

  for (auto g : m_group_table.groups(a_src_guid)) {
    for (auto p : m_group_table.guids(g)) {
      // Don't reflect.
      if (p == a_src_guid) {
        continue;
      }

      std::string addr = m_routing_table.horizontal_relay_address(p);
      if (addr != relay_address()) {
        continue;
      }

      // Replace with local.
      addr = m_routing_table.address(p);
      if (addr.empty()) {
        ACE_ERROR((LM_WARNING, "(%P|%t) %N:%l WARNING: HorizontalHandler::process_message failed to get address for %s\n", p.c_str()));
        continue;
      }

      addrs.insert(addr);
    }
  }

  for (auto p : addrs) {
    m_vertical_handler->enqueue_message(p, a_msg);
  }
}

SpdpHandler::SpdpHandler(ACE_Reactor * a_reactor,
                         GroupTable & a_group_table,
                         RoutingTable & a_routing_table)
  : VerticalHandler(a_reactor, a_group_table, a_routing_table),
    m_mutable_group_table(a_group_table)
{}

void SpdpHandler::process_message(ACE_INET_Addr const & a_remote,
                                  ACE_Time_Value const & a_now,
                                  std::string const & a_src_guid,
                                  ACE_Message_Block * a_buffer,
                                  bool a_empty_message) {
  if (a_empty_message) {
    return;
  }

  char* rd_ptr = a_buffer->rd_ptr();

  OpenDDS::DCPS::Serializer serializer(a_buffer, false, OpenDDS::DCPS::Serializer::ALIGN_CDR);
  OpenDDS::RTPS::Header header;
  if (!(serializer >> header)) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: SpdpHandler::handle_input failed to deserialize RTPS header\n"));
    a_buffer->release();
    return;
  }

  while (a_buffer->length() > 3) {
    const char subm = a_buffer->rd_ptr()[0], flags = a_buffer->rd_ptr()[1];
    serializer.swap_bytes((flags & OpenDDS::RTPS::FLAG_E) != ACE_CDR_BYTE_ORDER);
    const size_t start = a_buffer->length();
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

      for (size_t idx = 0, count = plist.length(); idx != count; ++idx) {
        parse_rtps_parameter(groups, plist[idx]);
      }

      m_mutable_group_table.update(a_src_guid, groups, a_now);

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
      const size_t read = start - a_buffer->length();
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
  VerticalHandler::process_message(a_remote, a_now, a_src_guid, a_buffer, a_empty_message);
}
