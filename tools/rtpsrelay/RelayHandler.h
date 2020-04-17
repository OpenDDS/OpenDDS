#ifndef RTPSRELAY_RELAY_HANDLER_H_
#define RTPSRELAY_RELAY_HANDLER_H_

#include "AssociationTable.h"
#include "Governor.h"

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

class RelayHandlerConfig {
public:
  RelayHandlerConfig()
    : statistics_interval_(60) // 1 minute
    , handler_statistics_writer_(nullptr)
    , domain_statistics_writer_(nullptr)
    , lifespan_(60) // 1 minute
    , application_domain_(1)
    , publish_participant_statistics_(true)
  {}

  void statistics_interval(const OpenDDS::DCPS::TimeDuration& flag)
  {
    statistics_interval_ = flag;
  }

  const OpenDDS::DCPS::TimeDuration& statistics_interval() const
  {
    return statistics_interval_;
  }

  bool handler_statistics_writer(DDS::DataWriter_var writer_var)
  {
    if (!writer_var) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: failed to create Handler Statistics data writer\n")));
      return false;
    }

    handler_statistics_writer_ = HandlerStatisticsDataWriter::_narrow(writer_var);

    if (!handler_statistics_writer_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: failed to narrow Handler Statistics data writer\n")));
      return false;
    }

    handler_statistics_writer_var_ = writer_var;
    return true;
  }

  HandlerStatisticsDataWriter_ptr handler_statistics_writer() const
  {
    return handler_statistics_writer_;
  }

  bool domain_statistics_writer(DDS::DataWriter_var writer_var)
  {
    if (!writer_var) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: failed to create Domain Statistics data writer\n")));
      return EXIT_FAILURE;
    }

    domain_statistics_writer_ = DomainStatisticsDataWriter::_narrow(writer_var);

    if (!domain_statistics_writer_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: failed to narrow Domain Statistics data writer\n")));
      return EXIT_FAILURE;
    }

    domain_statistics_writer_var_ = writer_var;
    return true;
  }

  DomainStatisticsDataWriter_ptr domain_statistics_writer() const
  {
    return domain_statistics_writer_;
  }

  void application_participant_guid(const OpenDDS::DCPS::RepoId& flag)
  {
    application_participant_guid_ = flag;
  }

  const OpenDDS::DCPS::RepoId& application_participant_guid() const
  {
    return application_participant_guid_;
  }

  void lifespan(const OpenDDS::DCPS::TimeDuration& flag)
  {
    lifespan_ = flag;
  }

  const OpenDDS::DCPS::TimeDuration& lifespan() const
  {
    return lifespan_;
  }

  void application_domain(DDS::DomainId_t flag)
  {
    application_domain_ = flag;
  }

  DDS::DomainId_t application_domain() const
  {
    return application_domain_;
  }

  void publish_participant_statistics(bool flag)
  {
    publish_participant_statistics_ = flag;
  }

  bool publish_participant_statistics() const
  {
    return publish_participant_statistics_;
  }

private:
  OpenDDS::DCPS::TimeDuration statistics_interval_;
  DDS::DataWriter_var handler_statistics_writer_var_;
  HandlerStatisticsDataWriter_ptr handler_statistics_writer_;
  DDS::DataWriter_var participant_statistics_writer_var_;
  DDS::DataWriter_var domain_statistics_writer_var_;
  DomainStatisticsDataWriter_ptr domain_statistics_writer_;
  OpenDDS::DCPS::RepoId application_participant_guid_;
  OpenDDS::DCPS::TimeDuration lifespan_;
  DDS::DomainId_t application_domain_;
  bool publish_participant_statistics_;
};

class RelayHandler : public ACE_Event_Handler {
public:
  int open(const ACE_INET_Addr& address);
  void enqueue_message(const ACE_INET_Addr& addr, const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg);

protected:
  explicit RelayHandler(const RelayHandlerConfig& config,
                        const std::string& name,
                        ACE_Reactor* reactor,
                        Governor& governor);

  int handle_input(ACE_HANDLE handle) override;
  int handle_output(ACE_HANDLE handle) override;
  int handle_timeout(const ACE_Time_Value&, const void*) override;

  ACE_HANDLE get_handle() const override { return socket_.get_handle(); }

  virtual void process_message(const ACE_INET_Addr& remote,
                               const OpenDDS::DCPS::MonotonicTimePoint& now,
                               const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) = 0;

  virtual uint32_t local_active_participants() const { return 0; }

  void max_fan_out(const ACE_INET_Addr& from, size_t fan_out)
  {
    handler_statistics_._max_fan_out = std::max(handler_statistics_._max_fan_out, static_cast<uint32_t>(fan_out));

    if (config_.publish_participant_statistics()) {
      auto& ps = participant_statistics_[from];
      ps._max_fan_out = std::max(ps._max_fan_out, static_cast<uint32_t>(fan_out));
    }
  }

private:
  void reset_statistics(const OpenDDS::DCPS::MonotonicTimePoint& now);

  Governor& governor_;
  ACE_SOCK_Dgram socket_;
  typedef std::queue<std::pair<ACE_INET_Addr, OpenDDS::DCPS::Message_Block_Shared_Ptr>> OutgoingType;
  OutgoingType outgoing_;
  ACE_Thread_Mutex outgoing_mutex_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_time_;
protected:
  const RelayHandlerConfig& config_;
  const std::string name_;
  HandlerStatistics handler_statistics_;
  std::map<ACE_INET_Addr, ParticipantStatistics> participant_statistics_;
};

class HorizontalHandler;

// Sends to and receives from peers.
class VerticalHandler : public RelayHandler {
public:
  typedef std::map<OpenDDS::DCPS::RepoId, std::set<ACE_INET_Addr>, OpenDDS::DCPS::GUID_tKeyLessThan> GuidAddrMap;

  VerticalHandler(const RelayHandlerConfig& config,
                  const std::string& name,
                  const ACE_INET_Addr& horizontal_address,
                  ACE_Reactor* reactor,
                  Governor& governor,
                  const AssociationTable& association_table,
                  GuidNameAddressDataWriter_ptr responsible_relay_writer,
                  GuidNameAddressDataReader_ptr responsible_relay_reader,
                  const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
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
  typedef std::map<ACE_INET_Addr, GuidSet> AddressMap;

  virtual bool do_normal_processing(const ACE_INET_Addr& /*remote*/,
                                    const OpenDDS::DCPS::RepoId& /*src_guid*/,
                                    const GuidSet& /*to*/,
                                    const OpenDDS::DCPS::Message_Block_Shared_Ptr& /*msg*/) { return true; }
  virtual void purge(const GuidAddr& /*ga*/) {}

  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  void send(const ACE_INET_Addr& from,
            const GuidSet& to,
            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg);
  void populate_address_map(AddressMap& address_map, const GuidSet& to);

  virtual uint32_t local_active_participants() const override { return guid_addr_map_.size(); }

  const AssociationTable& association_table_;
  GuidNameAddressDataWriter_ptr responsible_relay_writer_;
  GuidNameAddressDataReader_ptr responsible_relay_reader_;
  HorizontalHandler* horizontal_handler_;
  GuidAddrMap guid_addr_map_;
  typedef std::map<GuidAddr, OpenDDS::DCPS::MonotonicTimePoint> GuidExpirationMap;
  GuidExpirationMap guid_expiration_map_;
  typedef std::multimap<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> ExpirationGuidMap;
  ExpirationGuidMap expiration_guid_map_;

private:
  bool parse_message(OpenDDS::RTPS::MessageParser& message_parser,
                     const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg,
                     OpenDDS::DCPS::RepoId& src_guid,
                     GuidSet& to,
                     bool& is_pad_only,
                     bool check_submessages);
  ACE_INET_Addr read_address(const OpenDDS::DCPS::RepoId& guid) const;
  void write_address(const OpenDDS::DCPS::RepoId& guid);
  void unregister_address(const OpenDDS::DCPS::RepoId& guid);

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
  explicit HorizontalHandler(const RelayHandlerConfig& config,
                             const std::string& name,
                             ACE_Reactor* reactor,
                             Governor& governor);
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
  SpdpHandler(const RelayHandlerConfig& config,
              const std::string& name,
              const ACE_INET_Addr& address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

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
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;

  void purge(const GuidAddr& ga) override;
  int handle_exception(ACE_HANDLE fd) override;
};

class SedpHandler : public VerticalHandler {
public:
  SedpHandler(const RelayHandlerConfig& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto,
              const ACE_INET_Addr& application_participant_addr);

private:
  const ACE_INET_Addr application_participant_addr_;

  bool do_normal_processing(const ACE_INET_Addr& remote,
                            const OpenDDS::DCPS::RepoId& src_guid,
                            const GuidSet& to,
                            const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
};

class DataHandler : public VerticalHandler {
public:
  DataHandler(const RelayHandlerConfig& config,
              const std::string& name,
              const ACE_INET_Addr& horizontal_address,
              ACE_Reactor* reactor,
              Governor& governor,
              const AssociationTable& association_table,
              GuidNameAddressDataWriter_ptr responsible_relay_writer,
              GuidNameAddressDataReader_ptr responsible_relay_reader,
              const OpenDDS::RTPS::RtpsDiscovery_rch& rtps_discovery,
              const CRYPTO_TYPE& crypto);
};

#ifdef OPENDDS_SECURITY

class StunHandler : public RelayHandler {
public:
  explicit StunHandler(const RelayHandlerConfig& config,
                       const std::string& name,
                       ACE_Reactor* reactor,
                       Governor& governor);

private:
  void process_message(const ACE_INET_Addr& remote,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       const OpenDDS::DCPS::Message_Block_Shared_Ptr& msg) override;
  void send(const ACE_INET_Addr& addr, OpenDDS::STUN::Message message);

};
#endif /* OPENDDS_SECURITY */

}

#endif /* RTPSRELAY_RELAY_HANDLER_H_ */
