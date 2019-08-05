#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "GroupTable.h"
#include "RoutingTable.h"

#include <ace/Message_Block.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Time_Value.h>

#include <queue>
#include <string>
#include <utility>

class RelayHandler : public ACE_Event_Handler {
public:
  RelayHandler(ACE_Reactor* a_reactor, const GroupTable& a_group_table);
  int open(const ACE_INET_Addr& a_local);
  int handle_input(ACE_HANDLE a_handle) override;
  int handle_output(ACE_HANDLE a_handle) override;
  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }
  const std::string& relay_address() const { return relay_address_; }
  size_t bytes_received() const { return bytes_received_; }
  size_t bytes_sent() const { return bytes_sent_; }
  void reset_byte_counts() { bytes_received_ = 0; bytes_sent_ = 0; }
  void enqueue_message(const std::string& a_addr, ACE_Message_Block* a_msg);

protected:
  const GroupTable& group_table_;
  virtual void process_message(const ACE_INET_Addr& a_remote,
                               const ACE_Time_Value& a_now,
                               const std::string& a_src_guid,
                               ACE_Message_Block* a_msg,
                               bool is_beacon_message) = 0;

private:
  std::string relay_address_;
  ACE_SOCK_Dgram socket_;
  typedef std::queue<std::pair<std::string, ACE_Message_Block*>> OutgoingType;
  OutgoingType outgoing_;
  size_t bytes_received_;
  size_t bytes_sent_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  VerticalHandler(ACE_Reactor* a_reactor, const GroupTable& a_group_table, RoutingTable& a_routing_table);
  void horizontal_handler(HorizontalHandler* a_horizontal_handler) { horizontal_handler_ = a_horizontal_handler; }

protected:
  RoutingTable& routing_table_;
  HorizontalHandler* horizontal_handler_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const ACE_Time_Value& a_now,
                       const std::string& a_src_guid,
                       ACE_Message_Block* a_msg,
                       bool is_beacon_message) override;
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  HorizontalHandler(ACE_Reactor* a_reactor, const GroupTable& a_group_table, const RoutingTable& a_routing_table);
  void vertical_handler(VerticalHandler* a_vertical_handler) { vertical_handler_ = a_vertical_handler; }

private:
  const RoutingTable& routing_table_;
  VerticalHandler* vertical_handler_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const ACE_Time_Value& a_now,
                       const std::string& a_src_guid,
                       ACE_Message_Block* a_msg,
                       bool is_beacon_message) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(ACE_Reactor* a_reactor, GroupTable& a_group_table, RoutingTable& a_routing_table);

private:
  GroupTable& mutable_group_table_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const ACE_Time_Value& a_now,
                       const std::string& a_src_guid,
                       ACE_Message_Block* a_msg,
                       bool is_beacon_message) override;
};

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
