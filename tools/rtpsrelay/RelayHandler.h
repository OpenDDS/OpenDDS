#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "AssociationTable.h"
#include "Config.h"
#include "Governor.h"
#include "HandlerStatisticsReporter.h"
#include "ParticipantStatisticsReporter.h"

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
  int open(const ACE_INET_Addr& address);

protected:
  explicit RelayHandler(const Config& config,
                        const std::string& name,
                        ACE_Reactor* reactor,
                        Governor& governor,
                        HandlerStatisticsReporter& stats_reporter);

  int handle_input(ACE_HANDLE handle) override;
  int handle_output(ACE_HANDLE handle) override;
  int handle_timeout(const ACE_Time_Value&, const void*) override;

  void enqueue_message(const ACE_INET_Addr& addr,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                       const OpenDDS::DCPS::MonotonicTimePoint& now);

  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }

  virtual void process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) = 0;

private:
  Governor& governor_;
  ACE_SOCK_Dgram socket_;
  struct Element {
    ACE_INET_Addr address;
    OpenDDS::DCPS::Message_Block_Shared_Ptr message_block;
    OpenDDS::DCPS::MonotonicTimePoint timestamp;

    Element(const ACE_INET_Addr& a_address,
            OpenDDS::DCPS::Message_Block_Shared_Ptr a_message_block,
            const OpenDDS::DCPS::MonotonicTimePoint& a_timestamp)
      : address(a_address)
      , message_block(a_message_block)
      , timestamp(a_timestamp)
    {}
  };
  typedef std::queue<Element> OutgoingType;
  OutgoingType outgoing_;
  ACE_Thread_Mutex outgoing_mutex_;

protected:
  const Config& config_;
  const std::string name_;
  HandlerStatisticsReporter& stats_reporter_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  struct AddrSetStats {
    std::set<ACE_INET_Addr> addr_set;
    ParticipantStatisticsReporter stats_reporter;
  };
  typedef std::map<OpenDDS::DCPS::RepoId, AddrSetStats, OpenDDS::DCPS::GUID_tKeyLessThan> GuidAddrSetMap;

  VerticalHandler(const Config& config,
                  const std::string& name,
                  const ACE_INET_Addr& horizontal_address,
                  ACE_Reactor* reactor,
                  Governor& governor,
                  const AssociationTable& association_table,
                  GuidNameAddressDataWriter_ptr responsible_relay_writer,
                  GuidNameAddressDataReader_ptr responsible_relay_reader,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                  const CRYPTO_TYPE& crypto,
                  HandlerStatisticsReporter& stats_reporter);

  void horizontal_handler(HorizontalHandler* horizontal_handler) { horizontal_handler_ = horizontal_handler; }

  GuidAddrSetMap::iterator find(const OpenDDS::DCPS::RepoId& guid)
  {
    return guid_addr_set_map_.find(guid);
  }

  GuidAddrSetMap::const_iterator end()
  {
    return guid_addr_set_map_.end();
  }

  void venqueue_message(const ACE_INET_Addr& addr,
                        ParticipantStatisticsReporter& stats_reporter,
                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                        const OpenDDS::DCPS::MonotonicTimePoint& now);

protected:
  typedef std::map<ACE_INET_Addr, GuidSet> AddressMap;

  virtual bool do_normal_processing(const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::RepoId& /*src_guid*/,
                                    ParticipantStatisticsReporter& /*stats_reporter*/,
                                    const GuidSet& /*to*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/,
                                    const OpenDDS::DCPS::MonotonicTimePoint& /*now*/) { return true; }
  virtual void purge(const OpenDDS::DCPS::RepoId& /*guid*/) {}

  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  ParticipantStatisticsReporter& record_activity(const ACE_INET_Addr& remote_address,
                                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                 const OpenDDS::DCPS::RepoId& src_guid,
                                                 const size_t& msg_len);
  void send(const OpenDDS::DCPS::RepoId& src_guid,
            ParticipantStatisticsReporter& stats_reporter,
            const GuidSet& to,
            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
            const OpenDDS::DCPS::MonotonicTimePoint& now);
  void send(const ACE_INET_Addr& addr,
            OpenDDS::STUN::Message message,
            const OpenDDS::DCPS::MonotonicTimePoint& now);

  void populate_address_map(AddressMap& address_map, const GuidSet& to);

  const AssociationTable& association_table_;
  GuidNameAddressDataWriter_ptr responsible_relay_writer_;
  GuidNameAddressDataReader_ptr responsible_relay_reader_;
  HorizontalHandler* horizontal_handler_;
  GuidAddrSetMap guid_addr_set_map_;
  typedef std::map<GuidAddr, OpenDDS::DCPS::MonotonicTimePoint> GuidAddrExpirationMap;
  GuidAddrExpirationMap guid_addr_expiration_map_;
  typedef std::multimap<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> ExpirationGuidAddrMap;
  ExpirationGuidAddrMap expiration_guid_addr_map_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::RepoId& src_guid,
                     GuidSet& to,
                     bool check_submessages,
                     const OpenDDS::DCPS::MonotonicTimePoint& now);

  ACE_INET_Addr read_address(const OpenDDS::DCPS::RepoId& guid) const;
  void write_address(const OpenDDS::DCPS::RepoId& guid, const OpenDDS::DCPS::MonotonicTimePoint& now);
  void unregister_address(const OpenDDS::DCPS::RepoId& guid, const OpenDDS::DCPS::MonotonicTimePoint& now);

  const ACE_INET_Addr horizontal_address_;
  const std::string horizontal_address_str_;

  OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery_;
#ifdef OPENDDS_SECURITY
  const DDS::Security::CryptoTransform_var crypto_;
  const DDS::Security::ParticipantCryptoHandle application_participant_crypto_handle_;
#endif
};

// Sends to and receives from other relays.
class HorizontalHandler : public RelayHandler {
public:
  explicit HorizontalHandler(const Config& config,
                             const std::string& name,
                             ACE_Reactor* reactor,
                             Governor& governor,
                             HandlerStatisticsReporter& stats_reporter);

  void vertical_handler(VerticalHandler* vertical_handler) { vertical_handler_ = vertical_handler; }
  void enqueue_message(const ACE_INET_Addr& addr,
                       const GuidSet& to,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                       const OpenDDS::DCPS::MonotonicTimePoint& now);

private:
  VerticalHandler* vertical_handler_;
  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

  void replay(const OpenDDS::DCPS::RepoId& from,
              const GuidSet& to);

private:
  const ACE_INET_Addr application_participant_addr_;
  typedef std::map<OpenDDS::DCPS::RepoId, OpenDDS::DCPS::Message_Block_Shared_Ptr, OpenDDS::DCPS::GUID_tKeyLessThan> SpdpMessages;
  SpdpMessages spdp_messages_;
  ACE_Thread_Mutex spdp_messages_mutex_;

  struct Replay {
    OpenDDS::DCPS::RepoId from_guid;
    GuidSet to;
  };
  typedef std::queue<Replay> ReplayQueue;
  ReplayQueue replay_queue_;
  ACE_Thread_Mutex replay_queue_mutex_;

  bool do_normal_processing(const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            ParticipantStatisticsReporter& stats_reporter,
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now) override;

  void purge(const OpenDDS::DCPS::RepoId& guid) override;
  int handle_exception(ACE_HANDLE fd) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

private:
  const ACE_INET_Addr application_participant_addr_;

  bool do_normal_processing(const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            ParticipantStatisticsReporter& stats_reporter,
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              HandlerStatisticsReporter& stats_reporter);
};

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
