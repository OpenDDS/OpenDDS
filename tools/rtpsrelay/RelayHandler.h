#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "AssociationTable.h"

#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include <ace/Message_Block.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Thread_Mutex.h>
#include <ace/Time_Value.h>

#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>

#ifdef OPENDDS_SECURITY
#define CRYPTO_TYPE DDS::Security::CryptoTransform_var
#else
#define CRYPTO_TYPE int
#endif

namespace RtpsRelay {

class RelayHandler : public ACE_Event_Handler {
public:
  int open(const ACE_INET_Addr& a_local);
  void enqueue_message(const std::string& a_addr, ACE_Message_Block* a_msg);
  const std::string& relay_address() const { return relay_address_; }
  size_t bytes_received() const { return bytes_received_; }
  size_t bytes_sent() const { return bytes_sent_; }
  void reset_byte_counts() { bytes_received_ = 0; bytes_sent_ = 0; }

protected:
  RelayHandler(ACE_Reactor* a_reactor, const AssociationTable& a_association_table);
  int handle_input(ACE_HANDLE a_handle) override;
  int handle_output(ACE_HANDLE a_handle) override;
  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }
  virtual void process_message(const ACE_INET_Addr& a_remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& a_now,
                               const OpenDDS::DCPS::RepoId& a_src_guid,
                               ACE_Message_Block* a_msg) = 0;

  const AssociationTable& association_table_;

private:
  std::string relay_address_;
  ACE_SOCK_Dgram socket_;
  typedef std::queue<std::pair<std::string, ACE_Message_Block*>> OutgoingType;
  OutgoingType outgoing_;
  ACE_Thread_Mutex outgoing_mutex_;
  size_t bytes_received_;
  size_t bytes_sent_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  typedef std::map<OpenDDS::DCPS::RepoId, std::set<std::string>, OpenDDS::DCPS::GUID_tKeyLessThan> GuidAddrMap;

  VerticalHandler(ACE_Reactor* a_reactor,
                  const AssociationTable& a_association_table,
                  const OpenDDS::DCPS::TimeDuration& lifespan,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                  DDS::DomainId_t application_domain,
                  const OpenDDS::DCPS::RepoId& application_participant_guid,
                  const CRYPTO_TYPE& crypto);
  void horizontal_handler(HorizontalHandler* a_horizontal_handler) { horizontal_handler_ = a_horizontal_handler; }

  GuidAddrMap::const_iterator find(const OpenDDS::DCPS::RepoId& guid) const
  {
    return guid_addr_map_.find(guid);
  }

  GuidAddrMap::const_iterator end() const
  {
    return guid_addr_map_.end();
  }

protected:
  virtual std::string extract_relay_address(const RelayAddresses& relay_addresses) const = 0;
  virtual bool do_normal_processing(const ACE_INET_Addr& /*a_remote*/,
                                    const OpenDDS::DCPS::RepoId& /*a_src_guid*/,
                                    ACE_Message_Block* /*a_msg*/) { return true; }
  virtual void purge(const GuidAddr& /*ga*/) {}
  void process_message(const ACE_INET_Addr& a_remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& a_now,
                       const OpenDDS::DCPS::RepoId& a_src_guid,
                       ACE_Message_Block* a_msg) override;

  HorizontalHandler* horizontal_handler_;
  GuidAddrMap guid_addr_map_;
  typedef std::map<GuidAddr, OpenDDS::DCPS::MonotonicTimePoint> GuidExpirationMap;
  GuidExpirationMap guid_expiration_map_;
  typedef std::multimap<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> ExpirationGuidMap;
  ExpirationGuidMap expiration_guid_map_;
  const OpenDDS::DCPS::TimeDuration lifespan_;
  const OpenDDS::DCPS::RepoId application_participant_guid_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     ACE_Message_Block* msg,
                     bool& is_pad_only,
                     bool check_submessages);
  OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery_;
  const DDS::DomainId_t application_domain_;
#ifdef OPENDDS_SECURITY
  const DDS::Security::CryptoTransform_var crypto_;
  const DDS::Security::ParticipantCryptoHandle application_participant_crypto_handle_;
#endif
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  HorizontalHandler(ACE_Reactor* a_reactor,
                    const AssociationTable& a_association_table);
  void vertical_handler(VerticalHandler* a_vertical_handler) { vertical_handler_ = a_vertical_handler; }

private:
  VerticalHandler* vertical_handler_;
  void process_message(const ACE_INET_Addr& a_remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& a_now,
                       const OpenDDS::DCPS::RepoId& a_src_guid,
                       ACE_Message_Block* a_msg) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

  void replay(const OpenDDS::DCPS::RepoId& guid,
              const GuidSet& local_guids,
              const RelayAddressesSet& relay_addresses);

private:
  const ACE_INET_Addr application_participant_addr_;
  const std::string application_participant_addr_str_;
  ACE_Message_Block* spdp_message_;
  typedef std::map<OpenDDS::DCPS::RepoId, ACE_Message_Block*, OpenDDS::DCPS::GUID_tKeyLessThan> SpdpMessages;
  SpdpMessages spdp_messages_;
  ACE_Thread_Mutex spdp_messages_mutex_;

  std::string extract_relay_address(const RelayAddresses& relay_addresses) const override;

  bool do_normal_processing(const ACE_INET_Addr& a_remote,
                            const OpenDDS::DCPS::RepoId& a_src_guid,
                            ACE_Message_Block* a_msg) override;

  void purge(const GuidAddr& ga) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

private:
  const ACE_INET_Addr application_participant_addr_;
  const std::string application_participant_addr_str_;

  std::string extract_relay_address(const RelayAddresses& relay_addresses) const override;

  bool do_normal_processing(const ACE_INET_Addr& a_remote,
                            const OpenDDS::DCPS::RepoId& a_src_guid,
                            ACE_Message_Block* a_msg) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(ACE_Reactor* a_reactor,
              const AssociationTable& a_association_table,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto
              );

private:
  std::string extract_relay_address(const RelayAddresses& relay_addresses) const override;
};

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
