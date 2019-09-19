#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "AssociationTable.h"

#include <ace/Message_Block.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Time_Value.h>

#include <queue>
#include <string>
#include <utility>

class RelayHandler : public ACE_Event_Handler {
public:
  RelayHandler(ACE_Reactor* a_reactor, const AssociationTable& a_association_table);
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
  const AssociationTable& association_table_;
  virtual void process_message(const ACE_INET_Addr& a_remote,
                               const ACE_Time_Value& a_now,
                               const RtpsRelay::GUID_t& a_src_guid,
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
  typedef std::map<RtpsRelay::GUID_t, std::string, GUID_tKeyLessThan> GuidAddrMap;

  VerticalHandler(ACE_Reactor* a_reactor,
                  const AssociationTable& a_association_table,
                  const ACE_Time_Value& lifespan,
                  const ACE_Time_Value& purge_period);
  void horizontal_handler(HorizontalHandler* a_horizontal_handler) { horizontal_handler_ = a_horizontal_handler; }

  GuidAddrMap::const_iterator find(const RtpsRelay::GUID_t& guid) const
  {
    return guid_addr_map_.find(guid);
  }

  GuidAddrMap::const_iterator end() const
  {
    return guid_addr_map_.end();
  }

protected:
  virtual std::string extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const = 0;
  virtual void add_addresses(const ACE_INET_Addr& /*a_remote*/,
                             const RtpsRelay::GUID_t& /*a_src_guid*/,
                             std::set<std::string>& /* a_addrs*/) {}

  HorizontalHandler* horizontal_handler_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const ACE_Time_Value& a_now,
                       const RtpsRelay::GUID_t& a_src_guid,
                       ACE_Message_Block* a_msg,
                       bool is_beacon_message) override;
  GuidAddrMap guid_addr_map_;
  typedef std::map<RtpsRelay::GUID_t, ACE_Time_Value, GUID_tKeyLessThan> GuidExpirationMap;
  GuidExpirationMap guid_expiration_map_;
  typedef std::multimap<ACE_Time_Value, RtpsRelay::GUID_t> ExpirationGuidMap;
  ExpirationGuidMap expiration_guid_map_;
  ACE_Time_Value const lifespan_;

  int handle_timeout(const ACE_Time_Value& a_now, const void*) override;
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  HorizontalHandler(ACE_Reactor* a_reactor, const AssociationTable& a_association_table);
  void vertical_handler(VerticalHandler* a_vertical_handler) { vertical_handler_ = a_vertical_handler; }

private:
  VerticalHandler* vertical_handler_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const ACE_Time_Value& a_now,
                       const RtpsRelay::GUID_t& a_src_guid,
                       ACE_Message_Block* a_msg,
                       bool is_beacon_message) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const ACE_Time_Value& lifespan,
              const ACE_Time_Value& purge_period,
              const RtpsRelay::GUID_t& application_participant_guid);

private:
  std::string application_participant_addr_;
  const RtpsRelay::GUID_t application_participant_guid_;

  std::string extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const override;

  void add_addresses(const ACE_INET_Addr& a_remote,
                     const RtpsRelay::GUID_t& a_src_guid,
                     std::set<std::string>& a_addrs) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const ACE_Time_Value& lifespan,
              const ACE_Time_Value& purge_period,
              const RtpsRelay::GUID_t& application_participant_guid);

private:
  std::string application_participant_addr_;
  const RtpsRelay::GUID_t application_participant_guid_;

  std::string extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const override;

  void add_addresses(const ACE_INET_Addr& a_remote,
                     const RtpsRelay::GUID_t& a_src_guid,
                     std::set<std::string>& a_addrs) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const ACE_Time_Value& lifespan,
              const ACE_Time_Value& purge_period);

private:
  std::string extract_relay_address(const RtpsRelay::RelayAddresses& relay_addresses) const override;
};

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
