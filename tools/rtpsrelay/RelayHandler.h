#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include <ace/SOCK_Dgram.h>

#include "GroupTable.h"
#include "RoutingTable.h"

class RelayHandler : public ACE_Event_Handler {
public:
  RelayHandler(ACE_Reactor * a_reactor,
               GroupTable const & a_group_table);
  int open(ACE_INET_Addr const & a_local);
  int handle_input(ACE_HANDLE /*a_handle*/) override;
  int handle_output(ACE_HANDLE /*a_handle*/) override;
  ACE_HANDLE get_handle() const override { return m_socket.get_handle(); }
  std::string const & relay_address() const { return m_relay_address; }
  size_t bytes_received() const { return m_bytes_received; }
  size_t bytes_sent() const { return m_bytes_sent; }
  void reset_byte_counts() { m_bytes_received = 0; m_bytes_sent = 0; }
  void enqueue_message(std::string const & a_addr, ACE_Message_Block * a_msg);

protected:
  GroupTable const & m_group_table;

  virtual void process_message(ACE_INET_Addr const & a_remote,
                               ACE_Time_Value const & a_now,
                               std::string const & a_src_guid,
                               ACE_Message_Block * a_msg,
                               bool a_empty_message) = 0;

private:
  std::string m_relay_address;
  ACE_SOCK_Dgram m_socket;
  typedef std::queue<std::pair<std::string, ACE_Message_Block *> > OutgoingType;
  OutgoingType m_outgoing;
  size_t m_bytes_received;
  size_t m_bytes_sent;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  VerticalHandler(ACE_Reactor * a_reactor,
                  GroupTable const & a_group_table,
                  RoutingTable & a_routing_table);
  void horizontal_handler(HorizontalHandler * a_horizontal_handler) { m_horizontal_handler = a_horizontal_handler; }

protected:
  RoutingTable & m_routing_table;
  HorizontalHandler * m_horizontal_handler;
  void process_message(ACE_INET_Addr const & a_remote,
                       ACE_Time_Value const & a_now,
                       std::string const & a_src_guid,
                       ACE_Message_Block * a_msg,
                       bool a_empty_message) override;
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  HorizontalHandler(ACE_Reactor * a_reactor,
                    GroupTable const & a_group_table,
                    RoutingTable const & a_routing_table);
  void vertical_handler(VerticalHandler * a_vertical_handler) { m_vertical_handler = a_vertical_handler; }

private:
  RoutingTable const & m_routing_table; // NS rw, EW readonly
  VerticalHandler * m_vertical_handler;
  void process_message(ACE_INET_Addr const & a_remote,
                       ACE_Time_Value const & a_now,
                       std::string const & a_src_guid,
                       ACE_Message_Block * a_msg,
                       bool a_empty_message) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(ACE_Reactor * a_reactor,
              GroupTable & a_group_table,
              RoutingTable & a_routing_table);

private:
  GroupTable& m_mutable_group_table;
  void process_message(ACE_INET_Addr const & a_remote,
                       ACE_Time_Value const & a_now,
                       std::string const & a_src_guid,
                       ACE_Message_Block * a_msg,
                       bool a_empty_message) override;
};

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
