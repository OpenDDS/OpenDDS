#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "Config.h"
#include "GuidPartitionTable.h"
#include "HandlerStatisticsReporter.h"
#include "ParticipantStatisticsReporter.h"
#include "RelayPartitionTable.h"
#include "RelayStatisticsReporter.h"

#include <dds/DCPS/Claimable.h>
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

typedef std::set<ACE_INET_Addr> AddrSet;

struct AddrSetStats {
  AddrSet spdp_addr_set;
  AddrSet sedp_addr_set;
  AddrSet data_addr_set;
  ParticipantStatisticsReporter spdp_stats_reporter;
  ParticipantStatisticsReporter sedp_stats_reporter;
  ParticipantStatisticsReporter data_stats_reporter;

  bool empty() const
  {
    return spdp_addr_set.empty() && sedp_addr_set.empty() && data_addr_set.empty();
  }
};

class RelayHandler;

class GuidAddrSet {
public:
  typedef std::map<OpenDDS::DCPS::RepoId, AddrSetStats, OpenDDS::DCPS::GUID_tKeyLessThan> GuidAddrSetMap;

  GuidAddrSet(const Config& config,
              RelayStatisticsReporter& relay_stats_reporter)
    : config_(config)
    , relay_stats_reporter_(relay_stats_reporter)
  {}

  void spdp_vertical_handler(RelayHandler* spdp_vertical_handler)
  {
    spdp_vertical_handler_ = spdp_vertical_handler;
  }

  void sedp_vertical_handler(RelayHandler* sedp_vertical_handler)
  {
    sedp_vertical_handler_ = sedp_vertical_handler;
  }

  void data_vertical_handler(RelayHandler* data_vertical_handler)
  {
    data_vertical_handler_ = data_vertical_handler;
  }

  void process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now);

  bool ignore(const OpenDDS::DCPS::GUID_t& guid);

  void remove(const OpenDDS::DCPS::RepoId& guid);

  void remove_pending(const OpenDDS::DCPS::RepoId& guid)
  {
    pending_.erase(guid);
  }

  GuidAddrSetMap::iterator find(const OpenDDS::DCPS::RepoId& guid)
  {
    return guid_addr_set_map_.find(guid);
  }

  GuidAddrSetMap::const_iterator end()
  {
    return guid_addr_set_map_.end();
  }

  ParticipantStatisticsReporter&
  record_activity(const ACE_INET_Addr& remote_address,
                  const OpenDDS::DCPS::MonotonicTimePoint& now,
                  const OpenDDS::DCPS::RepoId& src_guid,
                  const size_t& msg_len,
                  RelayHandler& handler);

  ParticipantStatisticsReporter&
  participant_statistics_reporter(const OpenDDS::DCPS::RepoId& guid,
                                  const RelayHandler& handler);

private:
  void remove_helper(const OpenDDS::DCPS::RepoId& guid, const AddrSet& addr_set);

  const Config& config_;
  RelayStatisticsReporter& relay_stats_reporter_;
  RelayHandler* spdp_vertical_handler_;
  RelayHandler* sedp_vertical_handler_;
  RelayHandler* data_vertical_handler_;
  GuidAddrSetMap guid_addr_set_map_;
  typedef std::map<GuidAddr, OpenDDS::DCPS::MonotonicTimePoint> GuidAddrExpirationMap;
  GuidAddrExpirationMap guid_addr_expiration_map_;
  typedef std::multimap<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> ExpirationGuidAddrMap;
  ExpirationGuidAddrMap expiration_guid_addr_map_;
  GuidSet pending_;
};

typedef OpenDDS::DCPS::Claimable<GuidAddrSet> ClaimableGuidAddrSet;

class RelayHandler : public ACE_Event_Handler {
public:
  int open(const ACE_INET_Addr& address);

  const std::string& name() const { return name_; }

  virtual AddrSet* select_addr_set(AddrSetStats&) const
  {
    return 0;
  }

  virtual ParticipantStatisticsReporter* select_stats_reporter(AddrSetStats&) const
  {
    return 0;
  }

  virtual void purge(const OpenDDS::DCPS::RepoId& /*guid*/) {}

protected:
  typedef ClaimableGuidAddrSet::Claim Claim;

  RelayHandler(const Config& config,
               const std::string& name,
               ACE_Reactor* reactor,
               const GuidPartitionTable& guid_partition_table,
               ClaimableGuidAddrSet& guid_addr_set,
               HandlerStatisticsReporter& stats_reporter);

  int handle_input(ACE_HANDLE handle) override;
  int handle_output(ACE_HANDLE handle) override;

  void enqueue_message(const ACE_INET_Addr& addr,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                       const OpenDDS::DCPS::MonotonicTimePoint& now);

  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }

  virtual CORBA::ULong process_message(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) = 0;

private:
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
  mutable ACE_Thread_Mutex outgoing_mutex_;

protected:
  const Config& config_;
  const std::string name_;
  const GuidPartitionTable& guid_partition_table_;
  ClaimableGuidAddrSet& guid_addr_set_;
  HandlerStatisticsReporter& stats_reporter_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  VerticalHandler(const Config& config,
                  const std::string& name,
                  const ACE_INET_Addr& horizontal_address,
                  ACE_Reactor* reactor,
                  const GuidPartitionTable& guid_partition_table,
                  const RelayPartitionTable& relay_partition_table,
                  ClaimableGuidAddrSet& guid_addr_set,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                  const CRYPTO_TYPE& crypto,
                  const ACE_INET_Addr& application_participant_addr,
                  HandlerStatisticsReporter& stats_reporter);
  void stop();

  void horizontal_handler(HorizontalHandler* horizontal_handler) { horizontal_handler_ = horizontal_handler; }

  void venqueue_message(const ACE_INET_Addr& addr,
                        ParticipantStatisticsReporter& stats_reporter,
                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                        const OpenDDS::DCPS::MonotonicTimePoint& now);

protected:
  typedef std::map<ACE_INET_Addr, GuidSet> AddressMap;

  virtual bool do_normal_processing(Claim& /*claim*/,
                                    const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::RepoId& /*src_guid*/,
                                    const GuidSet& /*to*/,
                                    bool& /*send_to_application_participant*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/,
                                    const OpenDDS::DCPS::MonotonicTimePoint& /*now*/,
                                    CORBA::ULong& /*sent*/) { return true; }

  CORBA::ULong process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  ParticipantStatisticsReporter& record_activity(Claim& claim,
                                                 const ACE_INET_Addr& remote_address,
                                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                 const OpenDDS::DCPS::RepoId& src_guid,
                                                 const size_t& msg_len);
  CORBA::ULong send(Claim& claim,
                    const OpenDDS::DCPS::RepoId& src_guid,
                    const StringSet& to_partitions,
                    const GuidSet& to_guids,
                    bool send_to_application_participant,
                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                    const OpenDDS::DCPS::MonotonicTimePoint& now);
  size_t send(const ACE_INET_Addr& addr,
              OpenDDS::STUN::Message message,
              const OpenDDS::DCPS::MonotonicTimePoint& now);

  void populate_address_set(AddressSet& address_set,
                            const StringSet& to_partitions);

  const RelayPartitionTable& relay_partition_table_;
  HorizontalHandler* horizontal_handler_;
  const ACE_INET_Addr application_participant_addr_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::RepoId& src_guid,
                     GuidSet& to,
                     bool check_submessages,
                     const OpenDDS::DCPS::MonotonicTimePoint& now);

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
                             const GuidPartitionTable& guid_partition_table,
                             ClaimableGuidAddrSet& guid_addr_set,
                             HandlerStatisticsReporter& stats_reporter);

  void vertical_handler(VerticalHandler* vertical_handler) { vertical_handler_ = vertical_handler; }
  void enqueue_message(const ACE_INET_Addr& addr,
                       const StringSet& to_partitions,
                       const GuidSet& to_guids,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                       const OpenDDS::DCPS::MonotonicTimePoint& now);

private:
  VerticalHandler* vertical_handler_;
  CORBA::ULong process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
};

class SpdpHandler : public VerticalHandler {
public:
  SpdpHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& address,
              ACE_Reactor* reactor,
              const GuidPartitionTable& guid_partition_table,
              const RelayPartitionTable& relay_partition_table,
              ClaimableGuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

  void replay(const StringSequence& partitions);

  AddrSet* select_addr_set(AddrSetStats& stats) const override
  {
    return &stats.spdp_addr_set;
  }

  ParticipantStatisticsReporter* select_stats_reporter(AddrSetStats& stats) const override
  {
    return &stats.spdp_stats_reporter;
  }

private:
  typedef std::map<OpenDDS::DCPS::RepoId, OpenDDS::DCPS::Message_Block_Shared_Ptr, OpenDDS::DCPS::GUID_tKeyLessThan> SpdpMessages;
  SpdpMessages spdp_messages_;
  ACE_Thread_Mutex spdp_messages_mutex_;

  StringSet replay_queue_;
  ACE_Thread_Mutex replay_queue_mutex_;

  bool do_normal_processing(Claim& claim,
                            const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            const GuidSet& to,
                            bool& send_to_application_participant,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now,
                            CORBA::ULong& sent) override;

  void purge(const OpenDDS::DCPS::RepoId& guid) override;
  int handle_exception(ACE_HANDLE fd) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              const GuidPartitionTable& guid_partition_table,
              const RelayPartitionTable& relay_partition_table,
              ClaimableGuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

  AddrSet* select_addr_set(AddrSetStats& stats) const override
  {
    return &stats.sedp_addr_set;
  }

  ParticipantStatisticsReporter* select_stats_reporter(AddrSetStats& stats) const override
  {
    return &stats.sedp_stats_reporter;
  }

private:
  bool do_normal_processing(Claim& claim,
                            const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            const GuidSet& to,
                            bool& send_to_application_participant,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now,
                            CORBA::ULong& sent) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(const Config& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              const GuidPartitionTable& guid_partition_table,
              const RelayPartitionTable& relay_partition_table,
              ClaimableGuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              HandlerStatisticsReporter& stats_reporter);

  AddrSet* select_addr_set(AddrSetStats& stats) const override
  {
    return &stats.data_addr_set;
  }

  ParticipantStatisticsReporter* select_stats_reporter(AddrSetStats& stats) const override
  {
    return &stats.data_stats_reporter;
  }

};

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
