#ifndef RTPSRELAY_GUID_ADDR_SET_H_
#define RTPSRELAY_GUID_ADDR_SET_H_

#include "ParticipantStatisticsReporter.h"
#include "RelayStatisticsReporter.h"
#include "RelayThreadMonitor.h"

#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>

#include <ace/INET_Addr.h>

namespace RtpsRelay {

struct PortSet {
  using PortToExpirationMap = std::map<u_short, OpenDDS::DCPS::MonotonicTimePoint>;
  PortToExpirationMap spdp_ports, sedp_ports, data_ports;
  bool empty() const { return spdp_ports.empty() && sedp_ports.empty() && data_ports.empty(); }
  PortToExpirationMap* select(Port p);
  const PortToExpirationMap* select(Port p) const;
};

struct InetAddrHash {
  std::size_t operator()(const ACE_INET_Addr& addr) const noexcept { return addr.hash(); }
};

using IpToPorts = std::unordered_map<ACE_INET_Addr, PortSet, InetAddrHash>;

struct AddrSetStats {
  bool allow_rtps;
  bool seen_spdp_message;
  IpToPorts ip_to_ports;
  ParticipantStatisticsReporter spdp_stats_reporter;
  ParticipantStatisticsReporter sedp_stats_reporter;
  ParticipantStatisticsReporter data_stats_reporter;
  OpenDDS::DCPS::Lockable_Message_Block_Ptr spdp_message;
  OpenDDS::DCPS::MonotonicTimePoint session_start;
  OpenDDS::DCPS::MonotonicTimePoint deactivation;
  RelayStatisticsReporter& relay_stats_reporter;
  std::string common_name;
  size_t& total_ips;
  size_t& total_ports;

  AddrSetStats(const OpenDDS::DCPS::GUID_t& guid,
               const OpenDDS::DCPS::MonotonicTimePoint& a_session_start,
               RelayStatisticsReporter& a_relay_stats_reporter,
               size_t& a_total_ips,
               size_t& a_total_ports)
    : allow_rtps(false)
    , seen_spdp_message(false)
    , spdp_stats_reporter(rtps_guid_to_relay_guid(guid), "SPDP")
    , sedp_stats_reporter(rtps_guid_to_relay_guid(guid), "SEDP")
    , data_stats_reporter(rtps_guid_to_relay_guid(guid), "DATA")
    , session_start(a_session_start)
    , relay_stats_reporter(a_relay_stats_reporter)
    , total_ips(a_total_ips)
    , total_ports(a_total_ports)
  {}

  bool upsert_address(const AddrPort& remote_address,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      const OpenDDS::DCPS::MonotonicTimePoint& expiration,
                      size_t max_ip_addresses);

  template <typename Function>
  void foreach_addr(Port port, const Function& func) const
  {
    for (const auto& ip : ip_to_ports) {
      ACE_INET_Addr a = ip.first;
      const auto port_map = ip.second.select(port);
      if (port_map) {
        for (const auto& p : *port_map) {
          a.set_port_number(p.first);
          func(a);
        }
      }
    }
  }

  bool remove_if_expired(const AddrPort& remote_address, const OpenDDS::DCPS::MonotonicTimePoint& now,
                         bool& ip_now_unused, OpenDDS::DCPS::MonotonicTimePoint& updated_expiration);

  bool has_discovery_addrs() const
  {
    bool spdp = false;
    bool sedp = false;
    for (const auto& ip : ip_to_ports) {
      spdp = spdp || !ip.second.spdp_ports.empty();
      sedp = sedp || !ip.second.sedp_ports.empty();
      if (spdp && sedp) {
        return true;
      }
    }
    return false;
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

    return nullptr;
  }

  OpenDDS::DCPS::TimeDuration get_session_time(const OpenDDS::DCPS::MonotonicTimePoint& now) const
  {
    return now - session_start;
  }
};

struct Remote {
  static const size_t GUID_PREFIX_PREFIX_LENGTH = 6;

  ACE_INET_Addr addr_;
  unsigned char guid_prefix_prefix_[GUID_PREFIX_PREFIX_LENGTH];

  Remote(const ACE_INET_Addr& addr,
         const OpenDDS::DCPS::GUID_t& guid)
    : addr_(addr)
  {
    addr_.set_port_number(0);
    std::memcpy(&guid_prefix_prefix_[0], &guid.guidPrefix[0], sizeof(guid_prefix_prefix_));
  }

  bool operator==(const Remote& other) const
  {
    return addr_ == other.addr_ &&
      std::memcmp(&guid_prefix_prefix_[0], &other.guid_prefix_prefix_[0], sizeof(guid_prefix_prefix_)) == 0;
  }
};

struct RemoteHash {
  size_t operator()(const Remote& remote) const
  {
    return remote.addr_.hash() ^
      ((static_cast<size_t>(remote.guid_prefix_prefix_[0]) << 0) |
       (static_cast<size_t>(remote.guid_prefix_prefix_[1]) << 8) |
       (static_cast<size_t>(remote.guid_prefix_prefix_[2]) << 16) |
       (static_cast<size_t>(remote.guid_prefix_prefix_[3]) << 24)) ^
      ((static_cast<size_t>(remote.guid_prefix_prefix_[4]) << 0) |
       (static_cast<size_t>(remote.guid_prefix_prefix_[5]) << 8));
  }
};

class RelayHandler;
class RelayParticipantStatusReporter;

class GuidAddrSet;
using GuidAddrSet_rch = OpenDDS::DCPS::RcHandle<GuidAddrSet>;

class GuidAddrSet : public OpenDDS::DCPS::RcObject {
public:
  using GuidAddrSetMap = std::unordered_map<OpenDDS::DCPS::GUID_t, AddrSetStats, GuidHash>;

  GuidAddrSet(const Config& config,
              const OpenDDS::DCPS::ReactorTask_rch& reactor_task,
              OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery,
              RelayParticipantStatusReporter& relay_participant_status_reporter,
              RelayStatisticsReporter& relay_stats_reporter,
              RelayThreadMonitor& relay_thread_monitor)
    : config_(config)
    , reactor_task_(reactor_task)
    , rtps_discovery_(rtps_discovery)
    , relay_participant_status_reporter_(relay_participant_status_reporter)
    , relay_stats_reporter_(relay_stats_reporter)
    , relay_thread_monitor_(relay_thread_monitor)
    , total_ips_(0)
    , total_ports_(0)
    , participant_admission_limit_reached_(false)
    , last_admit_(true)
  {}

  ~GuidAddrSet();

  using CreatedAddrSetStats = std::pair<bool, AddrSetStats&>;

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

    CreatedAddrSetStats find_or_create(const OpenDDS::DCPS::GUID_t& guid,
                                       const OpenDDS::DCPS::MonotonicTimePoint& now)
    {
      return gas_.find_or_create(guid, now);
    }

    ParticipantStatisticsReporter&
    record_activity(const AddrPort& remote_address,
                    const OpenDDS::DCPS::MonotonicTimePoint& now,
                    const OpenDDS::DCPS::GUID_t& src_guid,
                    MessageType msg_type,
                    const size_t& msg_len,
                    const RelayHandler& handler)
    {
      return gas_.record_activity(remote_address, now, src_guid, msg_type, msg_len, handler);
    }

    ParticipantStatisticsReporter&
    participant_statistics_reporter(const OpenDDS::DCPS::GUID_t& guid,
                                    const OpenDDS::DCPS::MonotonicTimePoint& now,
                                    Port port)
    {
      return *find_or_create(guid, now).second.select_stats_reporter(port);
    }

    bool ignore_rtps(bool from_application_participant,
                     const OpenDDS::DCPS::GUID_t& guid,
                     const OpenDDS::DCPS::MonotonicTimePoint& now,
                     bool& admitted)
    {
      return gas_.ignore_rtps(from_application_participant, guid, now, admitted);
    }

    OpenDDS::DCPS::TimeDuration get_session_time(const OpenDDS::DCPS::GUID_t& guid,
                                                 const OpenDDS::DCPS::MonotonicTimePoint& now)
    {
      return gas_.get_session_time(guid, now);
    }

    void remove(const OpenDDS::DCPS::GUID_t& guid,
                const OpenDDS::DCPS::MonotonicTimePoint& now,
                RelayParticipantStatusReporter* reporter)
    {
      const auto it = find(guid);
      if (it == end()) {
        return;
      }

      gas_.remove(guid, it, now, reporter);
    }

    void reject_address(const ACE_INET_Addr& addr,
                        const OpenDDS::DCPS::MonotonicTimePoint& now)
    {
      gas_.reject_address(addr, now);
    }

    bool check_address(const ACE_INET_Addr& addr)
    {
      return gas_.check_address(addr);
    }

    void maintain_admission_queue(const OpenDDS::DCPS::MonotonicTimePoint& now)
    {
      gas_.maintain_admission_queue(now);
    }

    bool admitting() const
    {
      return gas_.admitting();
    }

  private:
    GuidAddrSet& gas_;

    Proxy(const Proxy&) = delete;
    Proxy(Proxy&&) = delete;
    Proxy& operator=(const Proxy&) = delete;
    Proxy& operator=(Proxy&&) = delete;
  };

private:
  CreatedAddrSetStats find_or_create(const OpenDDS::DCPS::GUID_t& guid,
                                     const OpenDDS::DCPS::MonotonicTimePoint& now);

  ParticipantStatisticsReporter&
  record_activity(const AddrPort& remote_address,
                  const OpenDDS::DCPS::MonotonicTimePoint& now,
                  const OpenDDS::DCPS::GUID_t& src_guid,
                  MessageType msg_type,
                  const size_t& msg_len,
                  const RelayHandler& handler);

  void schedule_rejected_address_expiration();
  void process_rejected_address_expiration(const OpenDDS::DCPS::MonotonicTimePoint& now);
  void schedule_deactivation();
  void process_deactivation(const OpenDDS::DCPS::MonotonicTimePoint& now);
  void schedule_expiration();
  void process_expiration(const OpenDDS::DCPS::MonotonicTimePoint& now);

  void maintain_admission_queue(const OpenDDS::DCPS::MonotonicTimePoint& now);

  bool admitting() const
  {
    const size_t limit = config_.admission_control_queue_size();
    const bool limit_okay = !limit || admission_control_queue_.size() < limit;
    const bool admit = !participant_admission_limit_reached_ && limit_okay && relay_thread_monitor_.threads_okay();
    if (admit != last_admit_) {
      last_admit_ = admit;
      relay_stats_reporter_.admission_state_changed(admit);
    }
    return admit;
  }

  bool ignore_rtps(bool from_application_participant,
                   const OpenDDS::DCPS::GUID_t& guid,
                   const OpenDDS::DCPS::MonotonicTimePoint& now,
                   bool& admitted);

  void remove(const OpenDDS::DCPS::GUID_t& guid,
              GuidAddrSetMap::iterator it,
              const OpenDDS::DCPS::MonotonicTimePoint& now,
              RelayParticipantStatusReporter* reporter);

  void reject_address(const ACE_INET_Addr& addr,
                      const OpenDDS::DCPS::MonotonicTimePoint& now);

  bool check_address(const ACE_INET_Addr& addr);

  OpenDDS::DCPS::TimeDuration get_session_time(const OpenDDS::DCPS::GUID_t& guid,
                                               const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    const auto it = guid_addr_set_map_.find(guid);
    return it == guid_addr_set_map_.end() ? OpenDDS::DCPS::TimeDuration::zero_value :
      it->second.get_session_time(now);
  }

  void check_participants_limit();

  struct AdmissionControlInfo {
    AdmissionControlInfo(const OpenDDS::DCPS::GuidPrefix_t& prefix, const OpenDDS::DCPS::MonotonicTimePoint& admitted)
     : admitted_(admitted)
    {
      std::memcpy(prefix_, prefix, sizeof (OpenDDS::DCPS::GuidPrefix_t));
    }
    OpenDDS::DCPS::GuidPrefix_t prefix_;
    OpenDDS::DCPS::MonotonicTimePoint admitted_;
  };

  const Config& config_;
  OpenDDS::DCPS::ReactorTask_rch reactor_task_;
  OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery_;
  RelayParticipantStatusReporter& relay_participant_status_reporter_;
  RelayStatisticsReporter& relay_stats_reporter_;
  RelayThreadMonitor& relay_thread_monitor_;
  GuidAddrSetMap guid_addr_set_map_;
  size_t total_ips_;
  size_t total_ports_;

  using RemoteMap = std::unordered_map<Remote, OpenDDS::DCPS::GUID_t, RemoteHash>;
  RemoteMap remote_map_;

  using DeactivationGuidQueue = std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, OpenDDS::DCPS::GUID_t>>;
  DeactivationGuidQueue deactivation_guid_queue_;

  using ExpirationGuidAddrQueue = std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr>>;
  ExpirationGuidAddrQueue expiration_guid_addr_queue_;

  using AdmissionControlQueue = std::deque<AdmissionControlInfo>;
  AdmissionControlQueue admission_control_queue_;

  using RejectedAddressMapType = std::unordered_map<OpenDDS::DCPS::NetworkAddress, OpenDDS::DCPS::MonotonicTimePoint>;
  RejectedAddressMapType rejected_address_map_;

  using RejectedAddressExpirationQueue = std::list<RejectedAddressMapType::iterator>;
  RejectedAddressExpirationQueue rejected_address_expiration_queue_;

  mutable ACE_Thread_Mutex mutex_;
  bool participant_admission_limit_reached_;
  mutable bool last_admit_;

  using GuidAddrSetSporadicTask = OpenDDS::DCPS::PmfSporadicTask<GuidAddrSet>;
  using GuidAddrSetSporadicTask_rch = OpenDDS::DCPS::RcHandle<GuidAddrSetSporadicTask>;
  GuidAddrSetSporadicTask_rch rejected_address_expiration_task_;
  GuidAddrSetSporadicTask_rch deactivation_task_;
  GuidAddrSetSporadicTask_rch expiration_task_;
};

}

#endif /* RTPSRELAY_GUID_ADDR_SET_H_ */
