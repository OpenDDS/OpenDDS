#include "GuidAddrSet.h"

#include "RelayHandler.h"
#include "RelayParticipantStatusReporter.h"

#include <dds/DCPS/LogAddr.h>

namespace RtpsRelay {

PortSet::PortToExpirationMap* PortSet::select(Port p)
{
  switch (p) {
  case SPDP:
    return &spdp_ports;
  case SEDP:
    return &sedp_ports;
  case DATA:
    return &data_ports;
  }
  return nullptr;
}

const PortSet::PortToExpirationMap* PortSet::select(Port p) const
{
  switch (p) {
  case SPDP:
    return &spdp_ports;
  case SEDP:
    return &sedp_ports;
  case DATA:
    return &data_ports;
  }
  return nullptr;
}

bool AddrSetStats::upsert_address(const AddrPort& remote_address,
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
    ++total_ips;
    relay_stats_reporter.total_client_ips(total_ips, now);
  }

  relay_stats_reporter.max_ips_per_client(static_cast<uint32_t>(ip_to_ports.size()), now);

  const auto port_map = iter->second.select(remote_address.port);
  if (!port_map) {
    return false;
  }

  const auto pair = port_map->insert(std::make_pair(remote_address.addr.get_port_number(), expiration));
  if (pair.second) {
    ++total_ports;
    relay_stats_reporter.total_client_ports(total_ports, now);
    return true;
  }
  pair.first->second = expiration;
  return false;
}

bool AddrSetStats::remove_if_expired(const AddrPort& remote_address, const OpenDDS::DCPS::MonotonicTimePoint& now,
                                     bool& ip_now_unused, OpenDDS::DCPS::MonotonicTimePoint& updated_expiration)
{
  ACE_INET_Addr addr_only(remote_address.addr);
  addr_only.set_port_number(0);
  const auto iter = ip_to_ports.find(addr_only);
  if (iter == ip_to_ports.end()) {
    return false;
  }

  const auto port_map = iter->second.select(remote_address.port);
  if (!port_map) {
    return false;
  }

  const auto port_iter = port_map->find(remote_address.addr.get_port_number());
  if (port_iter == port_map->end()) {
    return false;
  }

  if (port_iter->second <= now) {
    port_map->erase(port_iter);
    --total_ports;
    relay_stats_reporter.total_client_ports(total_ports, now);
    if (iter->second.empty()) {
      ip_to_ports.erase(addr_only);
      ip_now_unused = true;
      --total_ips;
      relay_stats_reporter.total_client_ips(total_ips, now);
    }
    return true;
  }
  updated_expiration = port_iter->second;
  return false;
}

GuidAddrSet::~GuidAddrSet()
{
  if (rejected_address_expiration_task_) {
    rejected_address_expiration_task_->cancel();
  }
  if (deactivation_task_) {
    deactivation_task_->cancel();
  }
  if (expiration_task_) {
    expiration_task_->cancel();
  }
  if (drain_task_) {
    drain_task_->cancel();
  }

  TheServiceParticipant->config_topic()->disconnect(config_reader_);
}

GuidAddrSet::CreatedAddrSetStats GuidAddrSet::find_or_create(const OpenDDS::DCPS::GUID_t& guid,
                                                             const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  auto it = guid_addr_set_map_.find(guid);
  const bool create = it == guid_addr_set_map_.end();
  if (create) {
    const auto it_bool_pair =
      guid_addr_set_map_.insert(std::make_pair(guid, AddrSetStats(guid, now, relay_stats_reporter_, total_ips_, total_ports_)));
    it = it_bool_pair.first;
    relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);
  }
  return {create, it->second};
}

ParticipantStatisticsReporter&
GuidAddrSet::record_activity(const AddrPort& remote_address,
                             const OpenDDS::DCPS::MonotonicTimePoint& now,
                             const OpenDDS::DCPS::GUID_t& src_guid,
                             MessageType msg_type,
                             const size_t& msg_len,
                             bool from_application_participant,
                             bool* allow_stun_responses,
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
        ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::record_activity change detected %C -> %C\n",
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
        relay_stats_reporter_.admission_queue_size(admission_control_queue_.size(), now);
      }
    }
  }

  if (created) {
    if (config_.log_activity()) {
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::record_activity "
                 "%C added 0.000 s into session from %C\n",
                 guid_to_string(src_guid).c_str(),
                 OpenDDS::DCPS::LogAddr(remote_address.addr).c_str()));
    }
    check_participants_limit();
  }

  if (addr_set_stats.deactivation == OpenDDS::DCPS::MonotonicTimePoint::zero_value) {
    deactivation_guid_queue_.push_back(std::make_pair(deactivation, src_guid));
    relay_stats_reporter_.deactivation_queue_size(deactivation_guid_queue_.size(), now);
    schedule_deactivation();
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
    relay_stats_reporter_.expiration_queue_size(expiration_guid_addr_queue_.size(), now);
    schedule_expiration();
  }

  ParticipantStatisticsReporter& stats_reporter =
    *addr_set_stats.select_stats_reporter(remote_address.port);
  stats_reporter.input_message(msg_len, msg_type);

  switch (drain_state_) {
  case DrainState::DS_NORMAL:
    if (!addr_set_stats.allow_stun_responses) {
      addr_set_stats.allow_stun_responses = true;
      --mark_count_;
    }
    break;
  case DrainState::DS_DRAINING:
    if (!from_application_participant && addr_set_stats.allow_stun_responses && mark_budget_) {
      addr_set_stats.allow_stun_responses = false;
      --mark_budget_;
      ++mark_count_;
    }
    break;
  }

  if (allow_stun_responses) {
    *allow_stun_responses = addr_set_stats.allow_stun_responses;
  }

  return stats_reporter;
}

void GuidAddrSet::schedule_rejected_address_expiration()
{
  if (rejected_address_expiration_queue_.empty()) {
    if (rejected_address_expiration_task_) {
      rejected_address_expiration_task_->cancel();
    }
  } else {
    if (!rejected_address_expiration_task_) {
      rejected_address_expiration_task_ =
        OpenDDS::DCPS::make_rch<GuidAddrSetSporadicTask>(TheServiceParticipant->time_source(), reactor_task_,
                                                         rchandle_from(this), &GuidAddrSet::process_rejected_address_expiration);
    }
    rejected_address_expiration_task_->schedule(rejected_address_expiration_queue_.front()->second - OpenDDS::DCPS::MonotonicTimePoint::now());
  }
}

void GuidAddrSet::process_rejected_address_expiration(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (!rejected_address_expiration_queue_.empty() && rejected_address_expiration_queue_.front()->second < now) {
    const auto& reject = rejected_address_expiration_queue_.front();
    if (config_.log_activity()) {
      const auto ago = now - reject->second;
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::process_rejected_address_expiration "
                 "Rejected address %C expired %C ago, removing from rejected address map.\n",
                 OpenDDS::DCPS::LogAddr(reject->first).c_str(),
                 ago.str().c_str()));
    }
    rejected_address_map_.erase(reject);
    rejected_address_expiration_queue_.pop_front();
    relay_stats_reporter_.rejected_address_map_size(static_cast<uint32_t>(rejected_address_map_.size()), now);
  }
  schedule_rejected_address_expiration();
}

void GuidAddrSet::schedule_deactivation()
{
  if (deactivation_guid_queue_.empty()) {
    if (deactivation_task_) {
      deactivation_task_->cancel();
    }
  } else {
    if (!deactivation_task_) {
      deactivation_task_ =
        OpenDDS::DCPS::make_rch<GuidAddrSetSporadicTask>(TheServiceParticipant->time_source(), reactor_task_,
                                                         rchandle_from(this), &GuidAddrSet::process_deactivation);
    }
    deactivation_task_->schedule(deactivation_guid_queue_.front().first - OpenDDS::DCPS::MonotonicTimePoint::now());
  }
}

void GuidAddrSet::process_deactivation(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (!deactivation_guid_queue_.empty() && deactivation_guid_queue_.front().first <= now) {
    const OpenDDS::DCPS::MonotonicTimePoint deactivation = deactivation_guid_queue_.front().first;
    const OpenDDS::DCPS::GUID_t guid = deactivation_guid_queue_.front().second;

    deactivation_guid_queue_.pop_front();

    const auto pos = guid_addr_set_map_.find(guid);
    if (pos != guid_addr_set_map_.end()) {

      AddrSetStats& addr_stats = pos->second;

      if (addr_stats.deactivation <= now) {
        relay_participant_status_reporter_.set_active(guid, false);
        addr_stats.deactivation = OpenDDS::DCPS::MonotonicTimePoint::zero_value;
      } else {
        deactivation_guid_queue_.push_back(std::make_pair(addr_stats.deactivation, guid));
      }
    }
  }
  relay_stats_reporter_.deactivation_queue_size(deactivation_guid_queue_.size(), now);
  schedule_deactivation();
}

void GuidAddrSet::schedule_expiration()
{
  if (expiration_guid_addr_queue_.empty()) {
    if (expiration_task_) {
      expiration_task_->cancel();
    }
  } else {
    if (!expiration_task_) {
      expiration_task_ =
        OpenDDS::DCPS::make_rch<GuidAddrSetSporadicTask>(TheServiceParticipant->time_source(), reactor_task_,
                                                         rchandle_from(this), &GuidAddrSet::process_expiration);
    }
    expiration_task_->schedule(expiration_guid_addr_queue_.front().first - OpenDDS::DCPS::MonotonicTimePoint::now());
  }
}

void GuidAddrSet::process_expiration(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (!expiration_guid_addr_queue_.empty() && expiration_guid_addr_queue_.front().first <= now) {
    const OpenDDS::DCPS::MonotonicTimePoint expiration = expiration_guid_addr_queue_.front().first;
    const GuidAddr ga = expiration_guid_addr_queue_.front().second;

    expiration_guid_addr_queue_.pop_front();

    const auto pos = guid_addr_set_map_.find(ga.guid);
    if (pos != guid_addr_set_map_.end()) {

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

        // Address actually expired.
        if (config_.log_activity()) {
          const auto ago = now - expiration;
          ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::process_expiration "
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

      } else if (updated_expiration != OpenDDS::DCPS::MonotonicTimePoint::zero_value) {
        expiration_guid_addr_queue_.push_back(std::make_pair(updated_expiration, ga));
      }
    }
  }
  relay_stats_reporter_.expiration_queue_size(expiration_guid_addr_queue_.size(), now);
  schedule_expiration();
}

void GuidAddrSet::maintain_admission_queue(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  if (config_.admission_control_queue_size()) {
    const OpenDDS::DCPS::MonotonicTimePoint earliest = now - config_.admission_control_queue_duration();
    auto limit = admission_control_queue_.begin();
    while (limit != admission_control_queue_.end() && limit->admitted_ < earliest) {
      ++limit;
    }
    admission_control_queue_.erase(admission_control_queue_.begin(), limit);
  }
  relay_stats_reporter_.admission_queue_size(admission_control_queue_.size(), now);
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
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::ignore_rtps %C was admitted %C into session\n",
                 guid_to_string(guid).c_str(),
                 pos->second.get_session_time(now).sec_str().c_str()));
    }

    return false;
  }

  if (!pos->second.has_discovery_addrs() || !pos->second.seen_spdp_message) {
    // Don't have the necessary addresses or message to complete discovery.
    return true;
  }

  if (!admitting()) {
    // Too many new clients to admit another.
    relay_stats_reporter_.admission_deferral_count(now);
    return true;
  }

  if (config_.admission_control_queue_size()) {
    admission_control_queue_.emplace_back(guid.guidPrefix, now);
    relay_stats_reporter_.admission_queue_size(admission_control_queue_.size(), now);
  }

  pos->second.allow_rtps = true;
  admitted = true;

  if (config_.log_activity()) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::ignore_rtps %C was admitted %C into session\n",
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

  if (!addr_stats.allow_stun_responses) {
    --mark_count_;
  }

  guid_addr_set_map_.erase(it);
  relay_stats_reporter_.local_active_participants(guid_addr_set_map_.size(), now);
  check_participants_limit();

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
      ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidAddrSet::reject_address Adding %C to list of rejected addresses.\n",
                 OpenDDS::DCPS::LogAddr(addr).c_str()));
    }
    rejected_address_expiration_queue_.push_back(result.first);
    if (rejected_address_expiration_queue_.size() == 1) {
      schedule_rejected_address_expiration();
    }
    relay_stats_reporter_.rejected_address_map_size(static_cast<uint32_t>(rejected_address_map_.size()), now);
  }
}

bool GuidAddrSet::check_address(const ACE_INET_Addr& addr)
{
  return rejected_address_map_.find(OpenDDS::DCPS::NetworkAddress(addr)) == rejected_address_map_.end();
}

void GuidAddrSet::check_participants_limit()
{
  const auto low = config_.admission_max_participants_low_water();
  if (low > 0) {
    participant_admission_limit_reached_ = guid_addr_set_map_.size() >=
      (participant_admission_limit_reached_ ? low : config_.admission_max_participants_high_water());
  }
}

void GuidAddrSet::admit_state(AdmitState as, const DDS::Time_t& now)
{
  if (admit_state_ != as) {
    admit_state_ = as;
    admit_state_change_ = now;
  }
}

void GuidAddrSet::drain_state(DrainState ds, const DDS::Time_t& now)
{
  if (!drain_task_) {
    drain_task_ = OpenDDS::DCPS::make_rch<GuidAddrSetSporadicTask>(TheServiceParticipant->time_source(),
                                                                   reactor_task_,
                                                                   rchandle_from(this),
                                                                   &GuidAddrSet::process_drain_state);
  }

  if (drain_state_ != ds) {
    switch (ds) {
    case DrainState::DS_NORMAL:
      mark_budget_ = 0;
      drain_task_->cancel();
      break;
    case DrainState::DS_DRAINING:
      drain_task_->schedule(drain_interval_);
      break;
    }

    drain_state_ = ds;
    drain_state_change_ = now;
  }
}

void GuidAddrSet::process_drain_state(const OpenDDS::DCPS::MonotonicTimePoint&)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  ++mark_budget_;
  drain_task_->schedule(drain_interval_);
}

void GuidAddrSet::populate_relay_status(RelayStatus& relay_status)
{
  relay_status.admitting(admitting());
  relay_status.admit_state(admit_state_);
  relay_status.admit_state_change(admit_state_change_);
  relay_status.drain_state(drain_state_);
  relay_status.drain_state_change(drain_state_change_);
  relay_status.local_active_participants(static_cast<uint32_t>(guid_addr_set_map_.size()));
  relay_status.marked_participants(static_cast<uint32_t>(mark_count_));
}

void GuidAddrSet::ConfigReaderListener::on_data_available(InternalDataReader_rch reader)
{
  using OpenDDS::DCPS::ConfigStoreImpl;
  OpenDDS::DCPS::ConfigReader::SampleSequence samples;
  OpenDDS::DCPS::InternalSampleInfoSequence infos;
  reader->read(samples, infos, DDS::LENGTH_UNLIMITED,
               DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ALIVE_INSTANCE_STATE);

  GuidAddrSet::Proxy proxy(guid_addr_set_);

  for (size_t idx = 0; idx != samples.size(); ++idx) {
    const auto& info = infos[idx];
    if (info.valid_data) {
      const auto& pair = samples[idx];
      if (pair.key() == RTPS_RELAY_ADMIT_STATE) {
        AdmitState admit = AdmitState::AS_NORMAL;
        if (ConfigStoreImpl::convert_value(pair.value(), admit_state_encoding, admit)) {
          proxy.admit_state(admit, info.source_timestamp);
        }
      } else if (pair.key() == RTPS_RELAY_DRAIN_STATE) {
        DrainState drain = DrainState::DS_NORMAL;
        if (ConfigStoreImpl::convert_value(pair.value(), drain_state_encoding, drain)) {
          proxy.drain_state(drain, info.source_timestamp);
        }
      } else if (pair.key() == RTPS_RELAY_DRAIN_INTERVAL) {
        OpenDDS::DCPS::TimeDuration interval;
        if (ConfigStoreImpl::convert_value(pair, ConfigStoreImpl::Format_IntegerMilliseconds, interval)) {
          proxy.drain_interval(interval);
        }
      }
    }
  }
}

}
