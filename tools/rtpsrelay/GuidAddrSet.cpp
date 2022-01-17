#include "GuidAddrSet.h"

#include "RelayHandler.h"
#include "RelayParticipantStatusReporter.h"

#include <dds/DCPS/LogAddr.h>

namespace RtpsRelay {

ParticipantStatisticsReporter&
GuidAddrSet::record_activity(const Proxy& proxy,
                             const AddrPort& remote_address,
                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                             const OpenDDS::DCPS::GUID_t& src_guid,
                             MessageType msg_type,
                             const size_t& msg_len,
                             RelayHandler& handler)
{
  const auto expiration = now + config_.lifespan();
  const auto deactivation = now + config_.inactive_period();
  const auto cass = find_or_create(src_guid, now);
  const bool created = cass.first;
  AddrSetStats& addr_set_stats = cass.second;

  if (config_.restart_detection()) {
    Remote remote(remote_address.addr, src_guid);
    const auto result = remote_map_.insert(std::make_pair(remote, src_guid));
    if (!result.second && result.first->second != src_guid) {
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity change detected %C -> %C\n"),
                   guid_to_string(result.first->second).c_str(),
                   guid_to_string(src_guid).c_str()));
      }
      rtps_discovery_->remove_domain_participant(config_.application_domain(), config_.application_participant_guid(), result.first->second);
      const auto pos = guid_addr_set_map_.find(result.first->second);
      if (pos != guid_addr_set_map_.end()) {
        remove(proxy, result.first->second, pos, now, &relay_participant_status_reporter_);
      }
      result.first->second = src_guid;
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
  relay_participant_status_reporter_.set_alive_active(proxy, src_guid, true, true);

  {
    const auto res = addr_set_stats.select_addr_set(remote_address.port)->insert(std::make_pair(remote_address, expiration));
    if (res.second) {
      if (config_.log_activity()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity %C %C is at %C %C into session total=%B\n"), handler.name().c_str(), guid_to_string(src_guid).c_str(), OpenDDS::DCPS::LogAddr(remote_address.addr).c_str(), addr_set_stats.get_session_time(now).sec_str().c_str(), guid_addr_set_map_.size()));
      }
      relay_stats_reporter_.new_address(now);

      const GuidAddr ga(src_guid, remote_address);
      expiration_guid_addr_queue_.push_back(std::make_pair(expiration, ga));
    }
    res.first->second = expiration;
  }

  ParticipantStatisticsReporter& stats_reporter =
    *addr_set_stats.select_stats_reporter(remote_address.port);
  stats_reporter.input_message(msg_len, msg_type);

  return stats_reporter;
}

void GuidAddrSet::process_expirations(const Proxy& proxy,
                                      const OpenDDS::DCPS::MonotonicTimePoint& now)
{
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
      relay_participant_status_reporter_.set_active(proxy, guid, false);
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
    AddrSet& addr_set = *addr_stats.select_addr_set(ga.address.port);
    const auto p = addr_set.find(ga.address);
    if (p == addr_set.end()) {
      continue;
    }

    if (p->second <= now) {
      addr_set.erase(p);
    } else {
      expiration_guid_addr_queue_.push_back(std::make_pair(p->second, ga));
      continue;
    }

    // Address actually expired.
    if (config_.log_activity()) {
      const auto ago = now - expiration;
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::process_expirations "
        "%C %C expired %C ago %C into session total=%B\n",
        guid_to_string(ga.guid).c_str(), OpenDDS::DCPS::LogAddr(ga.address.addr).c_str(),
        ago.str().c_str(), get_session_time(ga.guid, now).sec_str().c_str(),
        guid_addr_set_map_.size()));
    }
    relay_stats_reporter_.expired_address(now);

    if (addr_stats.empty()) {
      remove(proxy, ga.guid, pos, now, &relay_participant_status_reporter_);
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
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity ")
                 ACE_TEXT("%C was admitted %C into session\n"),
                 guid_to_string(guid).c_str(),
                 pos->second.get_session_time(now).sec_str().c_str()));
    }

    return false;
  }

  if (pos->second.spdp_addr_set.empty() || pos->second.sedp_addr_set.empty() || !pos->second.spdp_message) {
    // Don't have the necessary addresses or message to complete discovery.
    return true;
  }

  if (!admitting()) {
    // Too many new clients to admit another.
    return true;
  }

  pos->second.allow_rtps = true;
  admitted = true;

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity ")
               ACE_TEXT("%C was admitted %C into session\n"),
               guid_to_string(guid).c_str(),
               pos->second.get_session_time(now).sec_str().c_str()));
  }

  return false;
}

void GuidAddrSet::remove(const Proxy& proxy,
                         const OpenDDS::DCPS::GUID_t& guid,
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

  guid_addr_set_map_.erase(it);
  relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::remove_i %C removed %C into session total=%B\n"), guid_to_string(guid).c_str(), session_time.sec_str().c_str(), guid_addr_set_map_.size()));
  }

  if (reporter) {
    reporter->set_alive(proxy, guid, false);
  }
}

}
