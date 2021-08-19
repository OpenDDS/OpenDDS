#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "Config.h"
#include "GuidPartitionTable.h"
#include "HandlerStatisticsReporter.h"
#include "ParticipantStatisticsReporter.h"
#include "RelayPartitionTable.h"
#include "RelayStatisticsReporter.h"

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

typedef std::map<AddrPort, OpenDDS::DCPS::MonotonicTimePoint> AddrSet;

struct AddrSetStats {
  AddrSet spdp_addr_set;
  AddrSet sedp_addr_set;
  AddrSet data_addr_set;
  ParticipantStatisticsReporter spdp_stats_reporter;
  ParticipantStatisticsReporter sedp_stats_reporter;
  ParticipantStatisticsReporter data_stats_reporter;
  const OpenDDS::DCPS::MonotonicTimePoint first_activity;

  AddrSetStats()
    : first_activity(OpenDDS::DCPS::MonotonicTimePoint::now())
  {}

  bool empty() const
  {
    return spdp_addr_set.empty() && sedp_addr_set.empty() && data_addr_set.empty();
  }

  AddrSet* select_addr_set(Port port)
  {
    switch (port) {
    case SPDP:
      return &spdp_addr_set;
    case SEDP:
      return &sedp_addr_set;
    case DATA:
      return &data_addr_set;
    }

    return 0;
  }

  ParticipantStatisticsReporter* select_stats_reporter(Port port)
  {
    switch (port) {
    case SPDP:
      return &spdp_stats_reporter;
    case SEDP:
      return &sedp_stats_reporter;
    case DATA:
      return &data_stats_reporter;
    }

    return 0;
  }
};

class RelayHandler;

class GuidAddrSet {
public:
  typedef std::unordered_map<OpenDDS::DCPS::GUID_t, AddrSetStats, GuidHash> GuidAddrSetMap;

  GuidAddrSet(const Config& config,
              RelayStatisticsReporter& relay_stats_reporter)
    : config_(config)
    , relay_stats_reporter_(relay_stats_reporter)
    , expected_discovery_time_(config.lifespan() / 10)
  {}

  void spdp_vertical_handler(RelayHandler* spdp_vertical_handler)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    spdp_vertical_handler_ = spdp_vertical_handler;
  }

  void sedp_vertical_handler(RelayHandler* sedp_vertical_handler)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    sedp_vertical_handler_ = sedp_vertical_handler;
  }

  void data_vertical_handler(RelayHandler* data_vertical_handler)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    data_vertical_handler_ = data_vertical_handler;
  }

  void process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now);

  bool ignore(const OpenDDS::DCPS::GUID_t& guid,
              const OpenDDS::DCPS::MonotonicTimePoint& now);

  void remove(const OpenDDS::DCPS::GUID_t& guid);

  void remove_pending(const OpenDDS::DCPS::GUID_t& guid,
                      const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    pending_.erase(guid);

    const auto pos = guid_addr_set_map_.find(guid);
    if (pos != guid_addr_set_map_.end()) {
      const auto time_to_discovery = now - pos->second.first_activity;
      expected_discovery_time_ = .99 * expected_discovery_time_ + .01 * time_to_discovery;
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::remove_pending %C expected discovery time %d.%d total=%B/%B pending=%B/%B\n"), guid_to_string(guid).c_str(), expected_discovery_time_.value().sec(), expected_discovery_time_.value().usec(), guid_addr_set_map_.size(), config_.static_limit(), pending_.size(), config_.max_pending()));
      }
    }
  }

  class Proxy {
  public:
    Proxy(GuidAddrSet& gas)
      : gas_(gas)
    {
      gas_.mutex_.acquire();
    }

    ~Proxy()
    {
      gas_.mutex_.release();
    }

    GuidAddrSetMap::iterator find(const OpenDDS::DCPS::GUID_t& guid)
    {
      return gas_.guid_addr_set_map_.find(guid);
    }

    GuidAddrSetMap::const_iterator end()
    {
      return gas_.guid_addr_set_map_.end();
    }

    ParticipantStatisticsReporter&
    record_activity(const AddrPort& remote_address,
                    const OpenDDS::DCPS::MonotonicTimePoint& now,
                    const OpenDDS::DCPS::GUID_t& src_guid,
                    const size_t& msg_len,
                    RelayHandler& handler);

    ParticipantStatisticsReporter&
    participant_statistics_reporter(const OpenDDS::DCPS::GUID_t& guid,
                                    Port port)
    {
      return *gas_.guid_addr_set_map_[guid].select_stats_reporter(port);
    }

  private:
    GuidAddrSet& gas_;
    OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(Proxy)
  };

private:
  ParticipantStatisticsReporter&
  record_activity(const AddrPort& remote_address,
                  const OpenDDS::DCPS::MonotonicTimePoint& now,
                  const OpenDDS::DCPS::GUID_t& src_guid,
                  const size_t& msg_len,
                  RelayHandler& handler);

  const Config& config_;
  RelayStatisticsReporter& relay_stats_reporter_;
  RelayHandler* spdp_vertical_handler_;
  RelayHandler* sedp_vertical_handler_;
  RelayHandler* data_vertical_handler_;
  GuidAddrSetMap guid_addr_set_map_;
  typedef std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> > ExpirationGuidAddrQueue;
  ExpirationGuidAddrQueue expiration_guid_addr_queue_;
  GuidSet pending_;
  typedef std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, OpenDDS::DCPS::GUID_t> > PendingExpirationQueue;
  PendingExpirationQueue pending_expiration_queue_;
  OpenDDS::DCPS::TimeDuration expected_discovery_time_;
  mutable ACE_Thread_Mutex mutex_;
};

class RelayHandler : public ACE_Event_Handler {
public:
  int open(const ACE_INET_Addr& address);

  const std::string& name() const { return name_; }

  Port port() const { return port_; }

  virtual void purge(GuidAddrSet::Proxy& /*proxy*/,
                     const OpenDDS::DCPS::GUID_t& /*guid*/) {}

protected:
  RelayHandler(const Config& config,
               const std::string& name,
               Port port,
               ACE_Reactor* reactor,
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
  const Port port_;
  HandlerStatisticsReporter& stats_reporter_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  VerticalHandler(const Config& config,
                  const std::string& name,
                  Port port,
                  const ACE_INET_Addr& horizontal_address,
                  ACE_Reactor* reactor,
                  const GuidPartitionTable& guid_partition_table,
                  const RelayPartitionTable& relay_partition_table,
                  GuidAddrSet& guid_addr_set,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
                  const CRYPTO_TYPE& crypto,
                  const ACE_INET_Addr& application_participant_addr,
                  HandlerStatisticsReporter& stats_reporter);
  void stop();

  void horizontal_handler(HorizontalHandler* horizontal_handler) { horizontal_handler_ = horizontal_handler; }

  GuidAddrSet& guid_addr_set()
  {
    return guid_addr_set_;
  }

  void venqueue_message(const ACE_INET_Addr& addr,
                        ParticipantStatisticsReporter& stats_reporter,
                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                        const OpenDDS::DCPS::MonotonicTimePoint& now);

protected:
  virtual bool do_normal_processing(GuidAddrSet::Proxy& /*proxy*/,
                                    const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::GUID_t& /*src_guid*/,
                                    const GuidSet& /*to*/,
                                    bool& /*send_to_application_participant*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/,
                                    const OpenDDS::DCPS::MonotonicTimePoint& /*now*/,
                                    CORBA::ULong& /*sent*/) { return true; }

  CORBA::ULong process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  ParticipantStatisticsReporter& record_activity(GuidAddrSet::Proxy& proxy,
                                                 const AddrPort& remote_address,
                                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                 const OpenDDS::DCPS::GUID_t& src_guid,
                                                 const size_t& msg_len);
  CORBA::ULong send(GuidAddrSet::Proxy& proxy,
                    const OpenDDS::DCPS::GUID_t& src_guid,
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

  const GuidPartitionTable& guid_partition_table_;
  const RelayPartitionTable& relay_partition_table_;
  GuidAddrSet& guid_addr_set_;
  HorizontalHandler* horizontal_handler_;
  const ACE_INET_Addr application_participant_addr_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::GUID_t& src_guid,
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
  HorizontalHandler(const Config& config,
                    const std::string& name,
                    Port port,
                    ACE_Reactor* reactor,
                    const GuidPartitionTable& guid_partition_table,
                    HandlerStatisticsReporter& stats_reporter);

  void vertical_handler(VerticalHandler* vertical_handler) { vertical_handler_ = vertical_handler; }
  void enqueue_message(const ACE_INET_Addr& addr,
                       const StringSet& to_partitions,
                       const GuidSet& to_guids,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                       const OpenDDS::DCPS::MonotonicTimePoint& now);

private:
  const GuidPartitionTable& guid_partition_table_;
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
              GuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

  void replay(const StringSequence& partitions);

private:
  typedef std::unordered_map<OpenDDS::DCPS::GUID_t, OpenDDS::DCPS::Message_Block_Shared_Ptr, GuidHash> SpdpMessages;
  SpdpMessages spdp_messages_;
  ACE_Thread_Mutex spdp_messages_mutex_;

  StringSet replay_queue_;
  ACE_Thread_Mutex replay_queue_mutex_;

  bool do_normal_processing(GuidAddrSet::Proxy& proxy,
                            const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::GUID_t& src_guid,
                            const GuidSet& to,
                            bool& send_to_application_participant,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now,
                            CORBA::ULong& sent) override;

  void purge(GuidAddrSet::Proxy& proxy,
             const OpenDDS::DCPS::GUID_t& guid) override;
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
              GuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr,
              HandlerStatisticsReporter& stats_reporter);

private:
  bool do_normal_processing(GuidAddrSet::Proxy& proxy,
                            const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::GUID_t& src_guid,
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
              GuidAddrSet& guid_addr_set,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              HandlerStatisticsReporter& stats_reporter);
};

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
