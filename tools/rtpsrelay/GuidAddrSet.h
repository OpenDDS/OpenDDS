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
  std::map<u_short, OpenDDS::DCPS::MonotonicTimePoint> spdp_ports, sedp_ports, data_ports;
  bool empty() const { return spdp_ports.empty() && sedp_ports.empty() && data_ports.empty(); }
};

struct InetAddrHash {
  std::size_t operator()(const ACE_INET_Addr& addr) const noexcept
  {
    return addr.hash();
  }
};

typedef std::unordered_map<ACE_INET_Addr, PortSet, InetAddrHash> IpToPorts;

struct AddrSetStats {
  bool allow_rtps;
  IpToPorts ip_to_ports;
  ParticipantStatisticsReporter spdp_stats_reporter;
  ParticipantStatisticsReporter sedp_stats_reporter;
  ParticipantStatisticsReporter data_stats_reporter;
  OpenDDS::DCPS::Lockable_Message_Block_Ptr spdp_message;
  OpenDDS::DCPS::MonotonicTimePoint session_start;
  OpenDDS::DCPS::MonotonicTimePoint deactivation;
  RelayStatisticsReporter& relay_stats_reporter_;
  std::string common_name;

  AddrSetStats(const OpenDDS::DCPS::GUID_t& guid,
               const OpenDDS::DCPS::MonotonicTimePoint& a_session_start,
               RelayStatisticsReporter& relay_stats_reporter)
    : allow_rtps(false)
    , spdp_stats_reporter(rtps_guid_to_relay_guid(guid), "SPDP")
    , sedp_stats_reporter(rtps_guid_to_relay_guid(guid), "SEDP")
    , data_stats_reporter(rtps_guid_to_relay_guid(guid), "DATA")
    , session_start(a_session_start)
    , relay_stats_reporter_(relay_stats_reporter)
  {}

  bool upsert_address(const AddrPort& remote_address,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      const OpenDDS::DCPS::MonotonicTimePoint& expiration,
                      size_t max_ip_addresses)
  {
    ACE_INET_Addr addr_only(remote_address.addr);
    addr_only.set_port_number(0);
    auto iter = ip_to_ports.find(addr_only);
    if (iter == ip_to_ports.end()) {
      if (max_ip_addresses > 0 && ip_to_ports.size() == max_ip_addresses) {
        return false;
      }
      iter = ip_to_ports.insert(std::make_pair(addr_only, PortSet())).first;
    }

    relay_stats_reporter_.max_ips_per_client(static_cast<uint32_t>(ip_to_ports.size()), now);

    std::map<u_short, OpenDDS::DCPS::MonotonicTimePoint>* port_map = nullptr;
    switch (remote_address.port) {
    case SPDP:
      port_map = &iter->second.spdp_ports;
      break;
    case SEDP:
      port_map = &iter->second.sedp_ports;
      break;
    case DATA:
      port_map = &iter->second.data_ports;
      break;
    }
    if (!port_map) {
      return false;
    }
    const auto pair = port_map->insert(std::make_pair(remote_address.addr.get_port_number(), expiration));
    if (pair.second) {
      return true;
    }
    pair.first->second = expiration;
    return false;
  }

  template <typename Function>
  void foreach_addr(Port port, const Function& func) const
  {
    for (const auto& ip : ip_to_ports) {
      ACE_INET_Addr a = ip.first;
      const std::map<u_short, OpenDDS::DCPS::MonotonicTimePoint>* port_map = nullptr;
      switch (port) {
      case SPDP:
        port_map = &ip.second.spdp_ports;
        break;
      case SEDP:
        port_map = &ip.second.sedp_ports;
        break;
      case DATA:
        port_map = &ip.second.data_ports;
        break;
      }
      if (port_map) {
        for (const auto& p : *port_map) {
          a.set_port_number(p.first);
          func(a);
        }
      }
    }
  }

  bool remove_if_expired(const AddrPort& remote_address, const OpenDDS::DCPS::MonotonicTimePoint& now,
                         bool& ip_now_unused, OpenDDS::DCPS::MonotonicTimePoint& updated_expiration)
  {
    ACE_INET_Addr addr_only(remote_address.addr);
    addr_only.set_port_number(0);
    const auto iter = ip_to_ports.find(addr_only);
    if (iter == ip_to_ports.end()) {
      return false;
    }

    std::map<u_short, OpenDDS::DCPS::MonotonicTimePoint>* port_map = nullptr;
    switch (remote_address.port) {
    case SPDP:
      port_map = &iter->second.spdp_ports;
      break;
    case SEDP:
      port_map = &iter->second.sedp_ports;
      break;
    case DATA:
      port_map = &iter->second.data_ports;
      break;
    }

    if (!port_map) {
      return false;
    }

    const auto port_iter = port_map->find(remote_address.addr.get_port_number());
    if (port_iter == port_map->end()) {
      return false;
    }

    if (port_iter->second <= now) {
      port_map->erase(port_iter);
      if (iter->second.empty()) {
        ip_to_ports.erase(addr_only);
        ip_now_unused = true;
      }
      return true;
    }
    updated_expiration = port_iter->second;
    return false;
  }

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

    return 0;
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
  size_t operator() (const Remote& remote) const
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

class GuidAddrSet {
public:
  typedef std::unordered_map<OpenDDS::DCPS::GUID_t, AddrSetStats, GuidHash> GuidAddrSetMap;

  GuidAddrSet(const Config& config,
              OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery,
              RelayParticipantStatusReporter& relay_participant_status_reporter,
              RelayStatisticsReporter& relay_stats_reporter,
              RelayThreadMonitor& relay_thread_monitor)
    : config_(config)
    , rtps_discovery_(rtps_discovery)
    , relay_participant_status_reporter_(relay_participant_status_reporter)
    , relay_stats_reporter_(relay_stats_reporter)
    , relay_thread_monitor_(relay_thread_monitor)
    , spdp_vertical_handler_(0)
    , sedp_vertical_handler_(0)
    , data_vertical_handler_(0)
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

    void process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now)
    {
      gas_.process_expirations(now);
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
                                     const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    auto it = guid_addr_set_map_.find(guid);
    const bool create = it == guid_addr_set_map_.end();
    if (create) {
      const auto it_bool_pair =
        guid_addr_set_map_.insert(std::make_pair(guid, AddrSetStats(guid, now, relay_stats_reporter_)));
      it = it_bool_pair.first;
    }
    return CreatedAddrSetStats(create, it->second);
  }

  ParticipantStatisticsReporter&
  record_activity(const AddrPort& remote_address,
                  const OpenDDS::DCPS::MonotonicTimePoint& now,
                  const OpenDDS::DCPS::GUID_t& src_guid,
                  MessageType msg_type,
                  const size_t& msg_len,
                  const RelayHandler& handler);

  void process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now);

  bool admitting() const
  {
    const size_t limit = config_.admission_control_queue_size();
    const bool limit_okay = !limit || admission_control_queue_.size() < limit;
    return limit_okay && relay_thread_monitor_.threads_okay();
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

  struct AdmissionControlInfo
  {
    AdmissionControlInfo(const OpenDDS::DCPS::GuidPrefix_t& prefix, const OpenDDS::DCPS::MonotonicTimePoint& admitted)
     : admitted_(admitted)
    {
      std::memcpy(prefix_, prefix, sizeof (OpenDDS::DCPS::GuidPrefix_t));
    }
    OpenDDS::DCPS::GuidPrefix_t prefix_;
    OpenDDS::DCPS::MonotonicTimePoint admitted_;
  };
  typedef std::deque<AdmissionControlInfo> AdmissionControlQueue;

  const Config& config_;
  OpenDDS::RTPS::RtpsDiscovery_rch rtps_discovery_;
  RelayParticipantStatusReporter& relay_participant_status_reporter_;
  RelayStatisticsReporter& relay_stats_reporter_;
  RelayThreadMonitor& relay_thread_monitor_;
  RelayHandler* spdp_vertical_handler_;
  RelayHandler* sedp_vertical_handler_;
  RelayHandler* data_vertical_handler_;
  GuidAddrSetMap guid_addr_set_map_;
  typedef std::unordered_map<Remote, OpenDDS::DCPS::GUID_t, RemoteHash> RemoteMap;
  RemoteMap remote_map_;
  typedef std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, OpenDDS::DCPS::GUID_t> > DeactivationGuidQueue;
  DeactivationGuidQueue deactivation_guid_queue_;
  typedef std::list<std::pair<OpenDDS::DCPS::MonotonicTimePoint, GuidAddr> > ExpirationGuidAddrQueue;
  ExpirationGuidAddrQueue expiration_guid_addr_queue_;
  AdmissionControlQueue admission_control_queue_;
  typedef std::unordered_map<OpenDDS::DCPS::NetworkAddress, OpenDDS::DCPS::MonotonicTimePoint> RejectedAddressMapType;
  RejectedAddressMapType rejected_address_map_;
  typedef std::list<RejectedAddressMapType::iterator> RejectedAddressExpirationQueue;
  RejectedAddressExpirationQueue rejected_address_expiration_queue_;
  mutable ACE_Thread_Mutex mutex_;
};

}

#endif /* RTPSRELAY_GUID_ADDR_SET_H_ */
