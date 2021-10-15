#include "GuidAddrSet.h"

#include "RelayHandler.h"

#include <dds/DCPS/LogAddr.h>

namespace RtpsRelay {

ParticipantStatisticsReporter&
GuidAddrSet::record_activity(const AddrPort& remote_address,
                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                             const OpenDDS::DCPS::GUID_t& src_guid,
                             MessageType msg_type,
                             const size_t& msg_len,
                             RelayHandler& handler)
{
  const auto expiration = now + config_.lifespan();
  const auto cass = find_or_create(src_guid, now);
  const bool created = cass.first;
  AddrSetStats& addr_set_stats = cass.second;

  {
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
        remove_i(result.first->second, pos, now);
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

  {
    const auto res = addr_set_stats.select_addr_set(remote_address.port)->insert(std::make_pair(remote_address, expiration));
    if (res.second) {
      if (config_.log_activity()) {
        const auto session_time = now - addr_set_stats.session_start;
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity %C %C is at %C %C into session total=%B pending=%B/%B\n"), handler.name().c_str(), guid_to_string(src_guid).c_str(), OpenDDS::DCPS::LogAddr(remote_address.addr).c_str(), session_time.sec_str().c_str(), guid_addr_set_map_.size(), pending_.size(), config_.max_pending()));
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

void GuidAddrSet::process_expirations(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  GuidAddrSet::Proxy proxy(*this);

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
        "%C %C expired %C ago %C into session total=%B pending=%B/%B\n",
        guid_to_string(ga.guid).c_str(), OpenDDS::DCPS::LogAddr(ga.address.addr).c_str(),
        ago.str().c_str(), proxy.get_session_time(ga.guid, now).sec_str().c_str(),
        guid_addr_set_map_.size(), pending_.size(), config_.max_pending()));
    }
    relay_stats_reporter_.expired_address(now);

    if (addr_stats.empty()) {
      remove_i(ga.guid, pos, now);
    }
  }

  while (!pending_expiration_queue_.empty() && pending_expiration_queue_.front().first <= now) {
    const auto& expiration = pending_expiration_queue_.front().first;
    const auto& guid = pending_expiration_queue_.front().second;
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::process_expirations %C pending expired at %d.%d now=%d.%d %C into session total=%B pending=%B/%B\n"), guid_to_string(guid).c_str(), expiration.value().sec(), expiration.value().usec(), now.value().sec(), now.value().usec(), proxy.get_session_time(guid, now).sec_str().c_str(), guid_addr_set_map_.size(), pending_.size(), config_.max_pending()));
    }
    relay_stats_reporter_.expired_pending(now);
    pending_.erase(guid);
    pending_expiration_queue_.pop_front();
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
      const auto session_time = now - pos->second.session_start;
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity ")
                 ACE_TEXT("%C was admitted %C into session\n"),
                 guid_to_string(guid).c_str(),
                 session_time.sec_str().c_str()));
    }

    return false;
  }

  if (pos->second.spdp_addr_set.empty() || pos->second.sedp_addr_set.empty() || !pos->second.spdp_message) {
    // Don't have the necessary addresses or message to complete discovery.
    return true;
  }

  if (config_.max_pending() == 0) {
    pos->second.allow_rtps = true;
    admitted = true;
    return false;
  }

  if (!admitting_i()) {
    // Too many new clients to admit another.
    return true;
  }

  pending_.insert(guid);
  pending_expiration_queue_.push_back(std::make_pair(now + config_.pending_timeout(), guid));
  pos->second.allow_rtps = true;
  admitted = true;

  if (config_.log_activity()) {
    const auto session_time = now - pos->second.session_start;
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::record_activity ")
      ACE_TEXT("%C was admitted %C into session\n"),
      guid_to_string(guid).c_str(),
      session_time.sec_str().c_str()));
  }

  return false;
}

OpenDDS::DCPS::MonotonicTimePoint GuidAddrSet::get_first_spdp(const OpenDDS::DCPS::GUID_t& guid)
{
  GuidAddrSet::Proxy proxy(*this);
  const auto it = guid_addr_set_map_.find(guid);
  if (it == guid_addr_set_map_.end()) {
    return OpenDDS::DCPS::MonotonicTimePoint::zero_value;
  }

  return it->second.first_spdp;
}

void GuidAddrSet::remove(const OpenDDS::DCPS::GUID_t& guid,
                         const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  GuidAddrSet::Proxy proxy(*this);

  const auto it = guid_addr_set_map_.find(guid);
  if (it == guid_addr_set_map_.end()) {
    return;
  }

  remove_i(guid, it, now);
}

void GuidAddrSet::remove_i(const OpenDDS::DCPS::GUID_t& guid,
                           GuidAddrSetMap::iterator it,
                           const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  AddrSetStats& addr_stats = it->second;
  addr_stats.spdp_stats_reporter.report(addr_stats.session_start, now);
  addr_stats.sedp_stats_reporter.report(addr_stats.session_start, now);
  addr_stats.data_stats_reporter.report(addr_stats.session_start, now);

  pending_.erase(guid);
  guid_addr_set_map_.erase(it);
  relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidAddrSet::remove_i %C removed %C into session total=%B pending=%B/%B\n"), guid_to_string(guid).c_str(), get_session_time(guid, now).sec_str().c_str(), guid_addr_set_map_.size(), pending_.size(), config_.max_pending()));
  }
}

}
