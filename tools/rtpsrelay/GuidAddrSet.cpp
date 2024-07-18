#include "GuidAddrSet.h"

#include "RelayHandler.h"
#include "RelayParticipantStatusReporter.h"

#include <dds/DCPS/LogAddr.h>

namespace RtpsRelay {

ParticipantStatisticsReporter&
GuidAddrSet::record_activity(const AddrPort& remote_address,
                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                             const OpenDDS::DCPS::GUID_t& src_guid,
                             MessageType msg_type,
                             const size_t& msg_len,
                             const RelayHandler& handler)
{
  const auto expiration = now + config_.lifespan();
  const auto deactivation = now + config_.inactive_period();
  const auto cass = find_or_create(src_guid, now);
  const bool created = cass.first;
  AddrSetStats& addr_set_stats = cass.second;

  if (config_.restart_detection()) {
    Remote remote(remote_address.addr, src_guid);
    auto result = remote_map_.insert(std::make_pair(remote, src_guid));
    if (result.second) {
      relay_stats_reporter_.remote_map_size(static_cast<uint32_t>(remote_map_.size()), now);
    } else if (result.first->second != src_guid) {
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity change detected %C -> %C\n"),
                   guid_to_string(result.first->second).c_str(),
                   guid_to_string(src_guid).c_str()));
      }
      rtps_discovery_->remove_domain_participant(config_.application_domain(), config_.application_participant_guid(), result.first->second);
      const auto pos = guid_addr_set_map_.find(result.first->second);
      const auto prev_guid = result.first->second;
      if (pos != guid_addr_set_map_.end()) {
        remove(prev_guid, pos, now, &relay_participant_status_reporter_);
        result = remote_map_.insert(std::make_pair(remote, src_guid));
        if (!result.second) {
          result.first->second = src_guid;
        }
      }
      if (config_.admission_control_queue_size()) {
        for (auto it = admission_control_queue_.begin(); it != admission_control_queue_.end(); ++it) {
          if (OpenDDS::DCPS::equal_guid_prefixes(it->prefix_, prev_guid.guidPrefix)) {
            admission_control_queue_.erase(it);
            break;
          }
        }
      }
    }
  }

  if (created) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity ")
                 ACE_TEXT("%C added 0.000 s into session\n"),
                 guid_to_string(src_guid).c_str()));
    }
    relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);
  }

  if (addr_set_stats.deactivation == OpenDDS::DCPS::MonotonicTimePoint::zero_value) {
    deactivation_guid_queue_.push_back(std::make_pair(deactivation, src_guid));
  }
  addr_set_stats.deactivation = deactivation;
  relay_participant_status_reporter_.set_alive_active(src_guid, true, true);

  if (addr_set_stats.upsert_address(remote_address, now, expiration, config_.max_ips_per_client())) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::record_activity "
                 "%C %C is at %C %C into session ips=%B total=%B remote=%B deactivation=%B expire=%B admit=%B\n",
                 handler.name().c_str(),
                 guid_to_string(src_guid).c_str(),
                 OpenDDS::DCPS::LogAddr(remote_address.addr).c_str(),
                 addr_set_stats.get_session_time(now).sec_str().c_str(),
                 addr_set_stats.ip_to_ports.size(),
                 guid_addr_set_map_.size(),
                 remote_map_.size(),
                 deactivation_guid_queue_.size(),
                 expiration_guid_addr_queue_.size(),
                 admission_control_queue_.size()));
    }
    relay_stats_reporter_.new_address(now);
    const GuidAddr ga(src_guid, remote_address);
    expiration_guid_addr_queue_.push_back(std::make_pair(expiration, ga));
  }

  ParticipantStatisticsReporter& stats_reporter =
    *addr_set_stats.select_stats_reporter(remote_address.port);
  stats_reporter.input_message(msg_len, msg_type);

  return stats_reporter;
}

void GuidAddrSet::process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  bool update_reject_stat = false;
  auto it = rejected_address_expiration_queue_.begin();
  while (it != rejected_address_expiration_queue_.end() && (*it)->second < now) {
    if (config_.log_activity()) {
      const auto ago = now - (*it)->second;
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::process_expirations "
                 "Rejected address %C expired %C ago, removing from rejected address map.\n",
                 OpenDDS::DCPS::LogAddr((*it)->first).c_str(),
                 ago.str().c_str()));
    }
    rejected_address_map_.erase(*it);
    rejected_address_expiration_queue_.erase(it++);
    update_reject_stat = true;
  }
  if (update_reject_stat) {
    relay_stats_reporter_.rejected_address_map_size(static_cast<uint32_t>(rejected_address_map_.size()), now);
  }

  while (!deactivation_guid_queue_.empty() && deactivation_guid_queue_.front().first <= now) {
    const OpenDDS::DCPS::MonotonicTimePoint deactivation = deactivation_guid_queue_.front().first;
    const OpenDDS::DCPS::GUID_t guid = deactivation_guid_queue_.front().second;

    deactivation_guid_queue_.pop_front();

    const auto pos = guid_addr_set_map_.find(guid);
    if (pos == guid_addr_set_map_.end()) {
      continue;
    }

    AddrSetStats& addr_stats = pos->second;

    if (addr_stats.deactivation <= now) {
      relay_participant_status_reporter_.set_active(guid, false);
      addr_stats.deactivation = OpenDDS::DCPS::MonotonicTimePoint::zero_value;
    } else {
      deactivation_guid_queue_.push_back(std::make_pair(addr_stats.deactivation, guid));
      continue;
    }
  }

  while (!expiration_guid_addr_queue_.empty() && expiration_guid_addr_queue_.front().first <= now) {
    const OpenDDS::DCPS::MonotonicTimePoint expiration = expiration_guid_addr_queue_.front().first;
    const GuidAddr ga = expiration_guid_addr_queue_.front().second;

    expiration_guid_addr_queue_.pop_front();

    const auto pos = guid_addr_set_map_.find(ga.guid);
    if (pos == guid_addr_set_map_.end()) {
      continue;
    }

    AddrSetStats& addr_stats = pos->second;
    bool ip_now_unused = false;
    OpenDDS::DCPS::MonotonicTimePoint updated_expiration;
    if (addr_stats.remove_if_expired(ga.address, now, ip_now_unused, updated_expiration)) {
      if (ip_now_unused) {
        const auto remote_iter = remote_map_.find(Remote(ga.address.addr, ga.guid));
        if (remote_iter != remote_map_.end() && OpenDDS::DCPS::equal_guid_prefixes(remote_iter->second, ga.guid)) {
          remote_map_.erase(remote_iter);
          relay_stats_reporter_.remote_map_size(static_cast<uint32_t>(remote_map_.size()), now);
        }
      }
    } else {
      expiration_guid_addr_queue_.push_back(std::make_pair(updated_expiration, ga));
      continue;
    }

    // Address actually expired.
    if (config_.log_activity()) {
      const auto ago = now - expiration;
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::process_expirations "
                 "%C %C expired %C ago %C into session ips=%B total=%B remote=%B deactivation=%B expire=%B admit=%B\n",
                 guid_to_string(ga.guid).c_str(),
                 OpenDDS::DCPS::LogAddr(ga.address.addr).c_str(),
                 ago.str().c_str(),
                 get_session_time(ga.guid, now).sec_str().c_str(),
                 addr_stats.ip_to_ports.size(),
                 guid_addr_set_map_.size(),
                 remote_map_.size(),
                 deactivation_guid_queue_.size(),
                 expiration_guid_addr_queue_.size(),
                 admission_control_queue_.size()));
    }
    relay_stats_reporter_.expired_address(now);

    if (addr_stats.ip_to_ports.empty()) {
      remove(ga.guid, pos, now, &relay_participant_status_reporter_);
    }
  }
}

bool GuidAddrSet::ignore_rtps(bool from_application_participant,
                              const OpenDDS::DCPS::GUID_t& guid,
                              const OpenDDS::DCPS::MonotonicTimePoint& now,
                              bool& admitted)
{
  const auto pos = guid_addr_set_map_.find(guid);
  if (pos == guid_addr_set_map_.end()) {
    return true;
  }

  if (pos->second.allow_rtps) {
    // Client has already been admitted.
    return false;
  }

  if (from_application_participant) {
    pos->second.allow_rtps = true;

    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::ignore_rtps ")
                 ACE_TEXT("%C was admitted %C into session\n"),
                 guid_to_string(guid).c_str(),
                 pos->second.get_session_time(now).sec_str().c_str()));
    }

    return false;
  }

  if (!pos->second.has_discovery_addrs() || !pos->second.spdp_message) {
    // Don't have the necessary addresses or message to complete discovery.
    return true;
  }

  // Clean old entries from admission queue
  if (config_.admission_control_queue_size()) {
    const OpenDDS::DCPS::MonotonicTimePoint earliest = now - config_.admission_control_queue_duration();
    auto limit = admission_control_queue_.begin();
    while (limit != admission_control_queue_.end() && limit->admitted_ < earliest) {
      ++limit;
    }
    admission_control_queue_.erase(admission_control_queue_.begin(), limit);
  }

  if (!admitting()) {
    // Too many new clients to admit another.
    relay_stats_reporter_.admission_deferral_count(now);
    return true;
  }

  if (config_.admission_control_queue_size()) {
    admission_control_queue_.emplace_back(guid.guidPrefix, now);
  }

  pos->second.allow_rtps = true;
  admitted = true;

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::ignore_rtps ")
               ACE_TEXT("%C was admitted %C into session\n"),
               guid_to_string(guid).c_str(),
               pos->second.get_session_time(now).sec_str().c_str()));
  }

  return false;
}

void GuidAddrSet::remove(const OpenDDS::DCPS::GUID_t& guid,
                         GuidAddrSetMap::iterator it,
                         const OpenDDS::DCPS::MonotonicTimePoint& now,
                         RelayParticipantStatusReporter* reporter)
{
  AddrSetStats& addr_stats = it->second;
  const auto session_time = addr_stats.get_session_time(now);
  addr_stats.spdp_stats_reporter.report(addr_stats.session_start, now);
  addr_stats.spdp_stats_reporter.unregister();
  addr_stats.sedp_stats_reporter.report(addr_stats.session_start, now);
  addr_stats.sedp_stats_reporter.unregister();
  addr_stats.data_stats_reporter.report(addr_stats.session_start, now);
  addr_stats.data_stats_reporter.unregister();

  for (const auto& by_ip : addr_stats.ip_to_ports) {
    const auto remote_iter = remote_map_.find(Remote(by_ip.first, guid));
    if (remote_iter != remote_map_.end() && OpenDDS::DCPS::equal_guid_prefixes(remote_iter->second, guid)) {
      remote_map_.erase(remote_iter);
      relay_stats_reporter_.remote_map_size(static_cast<uint32_t>(remote_map_.size()), now);
    }
  }

  guid_addr_set_map_.erase(it);
  relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::remove "
               "%C removed %C into session total=%B remote=%B deactivation=%B expire=%B admit=%B\n",
               guid_to_string(guid).c_str(),
               session_time.sec_str().c_str(),
               guid_addr_set_map_.size(),
               remote_map_.size(),
               deactivation_guid_queue_.size(),
               expiration_guid_addr_queue_.size(),
               admission_control_queue_.size()));
  }

  if (reporter) {
    reporter->set_alive(guid, false);
  }
}

void GuidAddrSet::reject_address(const ACE_INET_Addr& addr,
                                 const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  OpenDDS::DCPS::MonotonicTimePoint expiration = now + config_.rejected_address_duration();
  auto result = rejected_address_map_.insert(std::make_pair(OpenDDS::DCPS::NetworkAddress(addr), expiration));
  if (result.second) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::reject_address ")
                 "Adding %C to list of rejected addresses.\n",
                 OpenDDS::DCPS::LogAddr(addr).c_str()));
    }
    rejected_address_expiration_queue_.push_back(result.first);
    relay_stats_reporter_.rejected_address_map_size(static_cast<uint32_t>(rejected_address_map_.size()), now);
  }
}

bool GuidAddrSet::check_address(const ACE_INET_Addr& addr)
{
  return rejected_address_map_.find(OpenDDS::DCPS::NetworkAddress(addr)) == rejected_address_map_.end();
}

}
