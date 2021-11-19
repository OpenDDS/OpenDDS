#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "Config.h"
#include "GuidAddrSet.h"
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

class RelayHandler : public ACE_Event_Handler {
public:
  int open(const ACE_INET_Addr& address);

  const std::string& name() const { return name_; }

  Port port() const { return port_; }

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
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type);

  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }

  virtual CORBA::ULong process_message(const ACE_INET_Addr& remote,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                                       MessageType& type) = 0;

private:
  ACE_SOCK_Dgram socket_;

  struct Element {
    ACE_INET_Addr address;
    OpenDDS::DCPS::Message_Block_Shared_Ptr message_block;
    OpenDDS::DCPS::MonotonicTimePoint timestamp;
    MessageType type;

    Element(const ACE_INET_Addr& a_address,
            OpenDDS::DCPS::Message_Block_Shared_Ptr a_message_block,
            const OpenDDS::DCPS::MonotonicTimePoint& a_timestamp,
            MessageType type)
      : address(a_address)
      , message_block(a_message_block)
      , timestamp(a_timestamp)
      , type(type)
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
class SpdpHandler;

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

  void spdp_handler(SpdpHandler* spdp_handler) { spdp_handler_ = spdp_handler; }

  GuidAddrSet& guid_addr_set()
  {
    return guid_addr_set_;
  }

  void venqueue_message(const ACE_INET_Addr& addr,
                        ParticipantStatisticsReporter& stats_reporter,
                        const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                        const OpenDDS::DCPS::MonotonicTimePoint& now,
                        MessageType type);

protected:
  virtual void cache_message(GuidAddrSet::Proxy& /*proxy*/,
                             const OpenDDS::DCPS::GUID_t& /*src_guid*/,
                             const GuidSet& /*to*/,
                             const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/,
                             const OpenDDS::DCPS::MonotonicTimePoint& /*now*/) {}

  virtual bool do_normal_processing(GuidAddrSet::Proxy& /*proxy*/,
                                    const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::GUID_t& /*src_guid*/,
                                    const GuidSet& /*to*/,
                                    bool /*admitted*/,
                                    bool& /*send_to_application_participant*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/,
                                    const OpenDDS::DCPS::MonotonicTimePoint& /*now*/,
                                    CORBA::ULong& /*sent*/) { return true; }

  CORBA::ULong process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                               MessageType& type) override;
  ParticipantStatisticsReporter& record_activity(GuidAddrSet::Proxy& proxy,
                                                 const AddrPort& remote_address,
                                                 const OpenDDS::DCPS::MonotonicTimePoint& now,
                                                 const OpenDDS::DCPS::GUID_t& src_guid,
                                                 MessageType msg_type,
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
  SpdpHandler* spdp_handler_;
  const ACE_INET_Addr application_participant_addr_;
  const ACE_INET_Addr horizontal_address_;
  const std::string horizontal_address_str_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::GUID_t& src_guid,
                     GuidSet& to,
                     bool check_submessages,
                     const OpenDDS::DCPS::MonotonicTimePoint& now);

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
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                               MessageType& type) override;
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

  void replay(const SpdpReplay& spdp_replay);

  CORBA::ULong send_to_application_participant(GuidAddrSet::Proxy& proxy,
                                               const OpenDDS::DCPS::GUID_t& guid,
                                               const OpenDDS::DCPS::MonotonicTimePoint& now);

private:
  typedef std::vector<SpdpReplay> ReplayQueue;
  ReplayQueue replay_queue_;
  ACE_Thread_Mutex replay_queue_mutex_;

  void cache_message(GuidAddrSet::Proxy& proxy,
                     const OpenDDS::DCPS::GUID_t& src_guid,
                     const GuidSet& to,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     const OpenDDS::DCPS::MonotonicTimePoint& now) override;

  bool do_normal_processing(GuidAddrSet::Proxy& proxy,
                            const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::GUID_t& src_guid,
                            const GuidSet& to,
                            bool admitted,
                            bool& send_to_application_participant,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                            const OpenDDS::DCPS::MonotonicTimePoint& now,
                            CORBA::ULong& sent) override;

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
                            bool admitted,
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
