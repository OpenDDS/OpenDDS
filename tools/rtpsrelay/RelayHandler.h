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
  int open(const ACE_INET_Addr& local);
  void enqueue_message(const ACE_INET_Addr& addr, const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg);
  const ACE_INET_Addr& relay_address() const { return relay_address_; }
  size_t bytes_received() const { return bytes_received_; }
  size_t messages_received() const { return messages_received_; }
  size_t bytes_sent() const { return bytes_sent_; }
  size_t messages_sent() const { return messages_sent_; }
  size_t max_fan_out() const { return max_fan_out_; }
  void reset_counters()
  {
    bytes_received_ = 0;
    messages_received_ = 0;
    bytes_sent_ = 0;
    messages_sent_ = 0;
    max_fan_out_ = 0;
  }

  void max_fan_out(size_t fan_out) { max_fan_out_ = std::max(max_fan_out_, fan_out); }

protected:
  explicit RelayHandler(ACE_Reactor* reactor);

  int handle_input(ACE_HANDLE handle) override;
  int handle_output(ACE_HANDLE handle) override;
  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }

  virtual void process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) = 0;
private:
  ACE_INET_Addr relay_address_;
  ACE_SOCK_Dgram socket_;
  typedef std::queue<std::pair<ACE_INET_Addr, OpenDDS::DCPS::Message_Block_Shared_Ptr>> OutgoingType;
  OutgoingType outgoing_;
  ACE_Thread_Mutex outgoing_mutex_;
  size_t bytes_received_;
  size_t messages_received_;
  size_t bytes_sent_;
  size_t messages_sent_;
  size_t max_fan_out_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  typedef std::map<OpenDDS::DCPS::RepoId, std::set<ACE_INET_Addr>, OpenDDS::DCPS::GUID_tKeyLessThan> GuidAddrMap;

  VerticalHandler(ACE_Reactor* reactor,
                  const RelayAddresses& relay_addresses,
                  const AssociationTable& association_table,
                  GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
                  GuidRelayAddressesDataReader_ptr responsible_relay_reader,
                  const OpenDDS::DCPS::TimeDuration& lifespan,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                  DDS::DomainId_t application_domain,
                  const OpenDDS::DCPS::RepoId& application_participant_guid,
                  const CRYPTO_TYPE& crypto);
  void horizontal_handler(HorizontalHandler* horizontal_handler) { horizontal_handler_ = horizontal_handler; }

  GuidAddrMap::const_iterator find(const OpenDDS::DCPS::RepoId& guid) const
  {
    return guid_addr_map_.find(guid);
  }

  GuidAddrMap::const_iterator end() const
  {
    return guid_addr_map_.end();
  }

protected:
  virtual ACE_INET_Addr extract_relay_address(const RelayAddresses& relay_addresses) const = 0;
  virtual bool do_normal_processing(const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::RepoId& /*src_guid*/,
                                    const GuidSet& /*to*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/) { return true; }
  virtual void purge(const GuidAddr& /*ga*/) {}

  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  void send(const GuidSet& to,
            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg);
  RelayAddressesMap populate_relay_addresses_map(const GuidSet& to);

  const AssociationTable& association_table_;
  GuidRelayAddressesDataWriter_ptr responsible_relay_writer_;
  GuidRelayAddressesDataReader_ptr responsible_relay_reader_;
  const RelayAddresses& relay_addresses_;
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
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::RepoId& src_guid,
                     GuidSet& to,
                     bool& is_pad_only,
                     bool check_submessages);
  RelayAddresses read_relay_addresses(const OpenDDS::DCPS::RepoId& guid) const;
  void write_relay_addresses(const OpenDDS::DCPS::RepoId& guid,
                             const RelayAddresses& relay_addresses);
  void unregister_relay_addresses(const OpenDDS::DCPS::RepoId& guid);

  OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery_;
#ifdef OPENDDS_SECURITY
  const DDS::DomainId_t application_domain_;
  const DDS::Security::CryptoTransform_var crypto_;
  const DDS::Security::ParticipantCryptoHandle application_participant_crypto_handle_;
#endif
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  explicit HorizontalHandler(ACE_Reactor* reactor);
  void vertical_handler(VerticalHandler* vertical_handler) { vertical_handler_ = vertical_handler; }
  void enqueue_message(const ACE_INET_Addr& addr,
                       const GuidSet& to,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg);

private:
  VerticalHandler* vertical_handler_;
  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(ACE_Reactor* reactor,
              const RelayAddresses& relay_addresses,
              const AssociationTable& association_table,
              GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
              GuidRelayAddressesDataReader_ptr responsible_relay_reader,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

  void replay(const OpenDDS::DCPS::RepoId& from,
              const GuidSet& to);

private:
  const ACE_INET_Addr application_participant_addr_;
  typedef std::map<OpenDDS::DCPS::RepoId, OpenDDS::DCPS::Message_Block_Shared_Ptr, OpenDDS::DCPS::GUID_tKeyLessThan> SpdpMessages;
  SpdpMessages spdp_messages_;
  ACE_Thread_Mutex spdp_messages_mutex_;

  ACE_INET_Addr extract_relay_address(const RelayAddresses& relay_addresses) const override;

  bool do_normal_processing(const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;

  void purge(const GuidAddr& ga) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(ACE_Reactor* reactor,
              const RelayAddresses& relay_addresses,
              const AssociationTable& association_table,
              GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
              GuidRelayAddressesDataReader_ptr responsible_relay_reader,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

private:
  const ACE_INET_Addr application_participant_addr_;

  ACE_INET_Addr extract_relay_address(const RelayAddresses& relay_addresses) const override;

  bool do_normal_processing(const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(ACE_Reactor* reactor,
              const RelayAddresses& relay_addresses,
              const AssociationTable& association_table,
              GuidRelayAddressesDataWriter_ptr responsible_relay_writer,
              GuidRelayAddressesDataReader_ptr responsible_relay_reader,
              const OpenDDS::DCPS::TimeDuration& lifespan,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              DDS::DomainId_t application_domain,
              const OpenDDS::DCPS::RepoId& application_participant_guid,
              const CRYPTO_TYPE& crypto
              );

private:
  ACE_INET_Addr extract_relay_address(const RelayAddresses& relay_addresses) const override;
};

#ifdef OPENDDS_SECURITY

class StunHandler : public RelayHandler {
public:
  explicit StunHandler(ACE_Reactor* reactor);

private:
  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  void send(const ACE_INET_Addr& addr, OpenDDS::STUN::Message message);

};
#endif /* OPENDDS_SECURITY */

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
