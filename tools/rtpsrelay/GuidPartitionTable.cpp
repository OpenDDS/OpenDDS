#include "GuidPartitionTable.h"

#include "RelayHandler.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

GuidPartitionTable::~GuidPartitionTable()
{
  if (denied_partitions_cleanup_task_) {
    denied_partitions_cleanup_task_->cancel();
  }
  if (local_async_disc_cache_cleanup_task_) {
    local_async_disc_cache_cleanup_task_->cancel();
  }
  if (remote_async_disc_cache_cleanup_task_) {
    remote_async_disc_cache_cleanup_task_->cancel();
  }
}

GuidPartitionTable::Result GuidPartitionTable::insert(const OpenDDS::DCPS::GUID_t& guid,
                                                      const DDS::StringSeq& partitions)
{
  Result result;
  std::vector<RelayPartitions> relay_partitions;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, NO_CHANGE);

    StringSet parts;
    for (CORBA::ULong idx = 0; idx != partitions.length(); ++idx) {
      parts.insert(partitions[idx].in());
    }
    if (parts.empty()) {
      // Special case for empty list of partitions.
      parts.insert("");
    }

    const auto r = guid_to_partitions_.insert(std::make_pair(guid, StringSet()));
    result = r.second ? ADDED : UPDATED;
    auto& x = r.first->second;

    std::vector<std::string> to_add;
    std::set_difference(parts.begin(), parts.end(), x.begin(), x.end(), std::back_inserter(to_add));

    std::vector<std::string> to_remove;
    std::set_difference(x.begin(), x.end(), parts.begin(), parts.end(), std::back_inserter(to_remove));

    if (to_add.empty() && to_remove.empty()) {
      if (result == ADDED) {
        relay_stats_reporter_.partition_guids(guid_to_partitions_.size(), guid_to_partitions_cache_.size());
      }
      return NO_CHANGE;
    }

    remove_from_cache(guid);

    StringSet globally_new;
    x.insert(to_add.begin(), to_add.end());
    for (const auto& part : to_add) {
      const auto q = partition_to_guid_.insert(std::make_pair(part, OrderedGuidSet()));
      q.first->second.insert(guid);
      partition_index_.insert(part, guid);
      if (q.second) {
        globally_new.insert(part);
      }
    }
    for (const auto& part : to_remove) {
      x.erase(part);
      partition_to_guid_[part].erase(guid);
      partition_index_.remove(part, guid);
      if (partition_to_guid_[part].empty()) {
        partition_to_guid_.erase(part);
      }
    }
    if (x.empty()) {
      guid_to_partitions_.erase(r.first);
    }

    add_new(relay_partitions, globally_new);
    relay_stats_reporter_.partition_index_nodes(partition_index_.size());
    relay_stats_reporter_.partition_index_cache(partition_index_.cache_size());
    relay_stats_reporter_.partition_guids(guid_to_partitions_.size(), guid_to_partitions_cache_.size());
  }

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, write_mutex_, NO_CHANGE);
    write_relay_partitions(relay_partitions);
  }

  return result;
}

void GuidPartitionTable::remove(const OpenDDS::DCPS::GUID_t& guid)
{
  std::vector<RelayPartitions> relay_partitions;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    StringSet defunct;

    const auto pos = guid_to_partitions_.find(guid);
    if (pos != guid_to_partitions_.end()) {
      for (const auto& partition : pos->second) {
        partition_index_.remove(partition, guid);
        const auto pos2 = partition_to_guid_.find(partition);
        if (pos2 != partition_to_guid_.end()) {
          pos2->second.erase(guid);
          if (pos2->second.empty()) {
            defunct.insert(pos2->first);
            partition_to_guid_.erase(pos2);
          }
        }
      }
      guid_to_partitions_.erase(pos);
      remove_from_cache(guid);
    }

    relay_stats_reporter_.partition_index_nodes(partition_index_.size());
    relay_stats_reporter_.partition_index_cache(partition_index_.cache_size());
    relay_stats_reporter_.partition_guids(guid_to_partitions_.size(), guid_to_partitions_cache_.size());
    remove_defunct(relay_partitions, defunct);
  }

  {
    ACE_GUARD(ACE_Thread_Mutex, g, write_mutex_);
    write_relay_partitions(relay_partitions);
  }
}

void GuidPartitionTable::lookup(StringSet& partitions, const OpenDDS::DCPS::GUID_t& from) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  // Match on the prefix.
  const auto prefix = make_unknown_guid(from);

  const auto p = guid_to_partitions_cache_.find(prefix);
  if (p != guid_to_partitions_cache_.end()) {
    partitions.insert(p->second.begin(), p->second.end());
    return;
  }

  StringSet c;

  for (auto pos = guid_to_partitions_.lower_bound(prefix), limit = guid_to_partitions_.end();
        pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    partitions.insert(pos->second.begin(), pos->second.end());
    c.insert(pos->second.begin(), pos->second.end());
  }

  if (!config_.allow_empty_partition()) {
    partitions.erase("");
    c.erase("");
  }

  // Only create a cache entry if there is something to cache.
  if (!c.empty()) {
    guid_to_partitions_cache_[prefix] = c;
    relay_stats_reporter_.partition_guids(guid_to_partitions_.size(), guid_to_partitions_cache_.size());
  }
}

  void GuidPartitionTable::deny_partitions(const DeniedPartitions& partitions)
  {
    std::unordered_set<std::string> partitions_to_drain;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, denied_partitions_mutex_);

      // Collect partitions that have not already been denied.
      for (const auto& partition : partitions) {
        const auto res = denied_partitions_.insert(partition);
        if (res.second) {
          partitions_to_drain.insert(partition.first);
        }
      }

      if (!denied_partitions_.empty() && !denied_partitions_cleanup_task_) {
        const auto base = OpenDDS::DCPS::make_rch<GuidPartitionTableEvent>(rchandle_from(this), &GuidPartitionTable::cleanup_denied_partitions);
        denied_partitions_cleanup_task_ = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(TheServiceParticipant->event_dispatcher(), base);
      }

      if (!denied_partitions_.empty() && !pending_denied_partitions_cleanup_) {
        denied_partitions_cleanup_task_->schedule(config_.denied_partitions_timeout());
        pending_denied_partitions_cleanup_ = true;
      }
    }

    GuidSet guids_to_drain;
    lookup(guids_to_drain, partitions_to_drain);
    GuidAddrSet::Proxy proxy(guid_addr_set_);
    for (const auto& guid : guids_to_drain) {
      proxy.deny(guid);
    }
  }

  void GuidPartitionTable::cleanup_denied_partitions(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, denied_partitions_mutex_);

    const auto cutoff_time = now - config_.denied_partitions_timeout();
    for (auto it = denied_partitions_.begin(); it != denied_partitions_.end();) {
      if (it->second <= cutoff_time) {
        it = denied_partitions_.erase(it);
      } else {
        ++it;
      }
    }

    if (!denied_partitions_.empty()) {
      denied_partitions_cleanup_task_->schedule(config_.denied_partitions_timeout());
    } else {
      pending_denied_partitions_cleanup_ = false;
    }
  }

  bool GuidPartitionTable::is_denied(const StringSet& partitions) const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, denied_partitions_mutex_, false);

    for (const auto& partition : partitions) {
      if (denied_partitions_.find(partition) != denied_partitions_.end()) {
        return true;
      }
    }
    return false;
  }

void
GuidPartitionTable::update_cert_partitions_cache(const std::string& key, const StringSet& partitions, const OpenDDS::DCPS::GUID_t& guid)
{
  if (key.empty() || !config_.async_discovery_enabled()) {
    return;
  }

  // The corresponding participant becomes local, remove it from the remote cache if it exists.
  remote_async_disc_cache_.remove(key);

  StringSet prev_partitions;
  StringSet* out_arg = nullptr;
  if (config_.synchronize_async_discovery_cache()) {
    out_arg = &prev_partitions;
  }
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();
  const auto sizes = local_async_disc_cache_.update(key, partitions, now, out_arg);

  relay_stats_reporter_.async_discovery_local_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_local_expiration_map_size(sizes.second, now);

  if (!local_async_disc_cache_cleanup_task_) {
    const auto base = OpenDDS::DCPS::make_rch<GuidPartitionTableEvent>(rchandle_from(this), &GuidPartitionTable::cleanup_local_async_disc_cache);
    local_async_disc_cache_cleanup_task_ = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(TheServiceParticipant->event_dispatcher(), base);
  }
  if (sizes.first) {
    local_async_disc_cache_cleanup_task_->schedule(config_.async_discovery_cache_timeout());
  }

  if (config_.log_async_discovery()) {
    const std::string parts_str = concat_strings(partitions);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::update_cert_partitions_cache: "
               "For %C key='%C' partitions=[%C] count=%B cache size=%B expiration map size=%B\n",
               guid_to_string(guid).c_str(), key.c_str(), parts_str.c_str(), partitions.size(),
               sizes.first, sizes.second));
  }

  if (config_.synchronize_async_discovery_cache()) {
    if (prev_partitions != partitions) {
      AsyncDiscoveryCacheUpdate update;
      update.relay_id(config_.relay_id());
      update.entries().emplace_back(key, StringSequence(partitions.begin(), partitions.end()));
      const auto rc = async_disc_cache_update_writer_->write(update, DDS::HANDLE_NIL);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
          "(%P|%t) ERROR: GuidPartitionTable::update_cert_partitions_cache: failed to write Async Discovery Cache Update: %C\n",
          OpenDDS::DCPS::retcode_to_string(rc)));
      }
    }
  }
}

void
GuidPartitionTable::lookup_cert_partitions_cache(StringSet& partitions, const std::string& key, const OpenDDS::DCPS::GUID_t& guid)
{
  if (key.empty() || !config_.async_discovery_enabled()) {
    return;
  }

  auto found = local_async_disc_cache_.lookup(partitions, key);
  if (!found) {
    found = remote_async_disc_cache_.lookup(partitions, key);
  }

  if (config_.log_async_discovery()) {
    if (found) {
      const std::string parts_str = concat_strings(partitions);
      ACE_DEBUG((LM_INFO,
                 "(%P|%t) INFO: GuidPartitionTable::lookup_cert_partitions_cache: For %C key='%C' Found partitions=[%C] count=%B\n",
                 guid_to_string(guid).c_str(), key.c_str(), parts_str.c_str(), partitions.size()));
    } else {
      ACE_DEBUG((LM_INFO,
                 "(%P|%t) INFO: GuidPartitionTable::lookup_cert_partitions_cache: For %C key='%C' Not found\n",
                 guid_to_string(guid).c_str(), key.c_str()));
    }
  }
}

void GuidPartitionTable::cleanup_local_async_disc_cache(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  StringSet expired_keys;
  const auto sizes = local_async_disc_cache_.remove_expired(now, config_.async_discovery_cache_timeout(),
    expired_keys, config_.log_async_discovery() || config_.synchronize_async_discovery_cache());

  relay_stats_reporter_.async_discovery_local_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_local_expiration_map_size(sizes.second, now);

  if (config_.log_async_discovery()) {
    const auto keys_str = concat_strings(expired_keys);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::cleanup_local_async_disc_cache: "
               "Removed keys=[%C] -- cache size=%B expiration map size=%B\n",
               keys_str.c_str(), sizes.first, sizes.second));
  }

  const auto earliest_last_access = local_async_disc_cache_.earliest_last_access();
  if (earliest_last_access != OpenDDS::DCPS::MonotonicTimePoint::max_value) {
    const auto next_fire_in = earliest_last_access + config_.async_discovery_cache_timeout() - now;
    local_async_disc_cache_cleanup_task_->schedule(next_fire_in);
  }

  if (config_.synchronize_async_discovery_cache()) {
    if (!expired_keys.empty()) {
      AsyncDiscoveryCachePrune prune;
      prune.relay_id(config_.relay_id());
      prune.keys().insert(prune.keys().end(), expired_keys.begin(), expired_keys.end());
      const auto rc = async_disc_cache_prune_writer_->write(prune, DDS::HANDLE_NIL);
      if (rc != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: GuidPartitionTable::cleanup_local_async_disc_cache: failed to write Async Discovery Cache Prune: %C\n",
                   OpenDDS::DCPS::retcode_to_string(rc)));
      }
    }
  }
}

void
GuidPartitionTable::update_remote_async_disc_cache(const AsyncDiscoveryCacheEntrySeq& entries,
                                                       const std::string& from_relay,
                                                       const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  const auto sizes = remote_async_disc_cache_.update(entries, now);

  relay_stats_reporter_.async_discovery_remote_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_remote_expiration_map_size(sizes.second, now);

  if (!remote_async_disc_cache_cleanup_task_) {
    const auto base = OpenDDS::DCPS::make_rch<GuidPartitionTableEvent>(rchandle_from(this), &GuidPartitionTable::cleanup_remote_async_disc_cache);
    remote_async_disc_cache_cleanup_task_ = OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SporadicEvent>(TheServiceParticipant->event_dispatcher(), base);
  }
  if (sizes.first) {
    remote_async_disc_cache_cleanup_task_->schedule(config_.async_discovery_remote_cache_timeout());
  }

  if (config_.log_async_discovery()) {
    StringSet keys;
    for (const auto& entry : entries) {
      keys.insert(entry.key());
    }
    const std::string keys_str = concat_strings(keys);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::update_remote_async_disc_cache: "
               "Update remote cache for keys=[%C] from relay '%C' -- remote cache size=%B remote expiration map size=%B\n",
               keys_str.c_str(), from_relay.c_str(), sizes.first, sizes.second));
  }
}

void GuidPartitionTable::remove_from_local_async_disc_cache(const AsyncDiscoveryCacheEntrySeq& entries,
                                                            const std::string& from_relay,
                                                            const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  StringSet removed_keys;
  const auto sizes = local_async_disc_cache_.remove(entries, removed_keys);

  relay_stats_reporter_.async_discovery_local_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_local_expiration_map_size(sizes.second, now);

  if (config_.log_async_discovery() && !removed_keys.empty()) {
    const std::string keys_str = concat_strings(removed_keys);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::remove_from_local_async_disc_cache: "
               "Removed local cache keys=[%C], now local on relay '%C' -- cache size=%B expiration map size=%B\n",
               keys_str.c_str(), from_relay.c_str(), sizes.first, sizes.second));
  }
}

void GuidPartitionTable::handle_async_disc_cache_update(const AsyncDiscoveryCacheEntrySeq& entries,
                                                        const std::string& from_relay,
                                                        const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  if (!config_.synchronize_async_discovery_cache()) {
    if (config_.log_async_discovery()) {
      StringSequence keys;
      keys.reserve(entries.size());
      for (const auto& entry : entries) {
        keys.push_back(entry.key());
      }
      const std::string keys_str = concat_strings(keys);
      ACE_DEBUG((LM_NOTICE, "(%P|%t) NOTICE: GuidPartitionTable::handle_async_disc_cache_update: "
                 "Received remote cache update for keys=[%C] from relay '%C' while synchronization is disabled!\n",
                 keys_str.c_str(), from_relay.c_str()));
    }
    return;
  }

  remove_from_local_async_disc_cache(entries, from_relay, now);
  update_remote_async_disc_cache(entries, from_relay, now);
}

void GuidPartitionTable::cleanup_remote_async_disc_cache(const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  StringSet expired_keys;
  const auto sizes = remote_async_disc_cache_.remove_expired(now, config_.async_discovery_remote_cache_timeout(),
    expired_keys, config_.log_async_discovery());

  relay_stats_reporter_.async_discovery_remote_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_remote_expiration_map_size(sizes.second, now);

  if (config_.log_async_discovery()) {
    const auto keys_str = concat_strings(expired_keys);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::cleanup_remote_async_disc_cache: "
               "Removed remote keys=[%C] -- remote cache size=%B remote expiration map size=%B\n",
               keys_str.c_str(), sizes.first, sizes.second));
  }

  const auto earliest_last_access = remote_async_disc_cache_.earliest_last_access();
  if (earliest_last_access != OpenDDS::DCPS::MonotonicTimePoint::max_value) {
    const auto next_fire_in = earliest_last_access + config_.async_discovery_remote_cache_timeout() - now;
    remote_async_disc_cache_cleanup_task_->schedule(next_fire_in);
  }
}

void
GuidPartitionTable::handle_async_disc_cache_prune(const StringSequence& keys, const std::string& from_relay)
{
  if (!config_.synchronize_async_discovery_cache()) {
    if (config_.log_async_discovery()) {
      const std::string keys_str = concat_strings(keys);
      ACE_DEBUG((LM_NOTICE, "(%P|%t) NOTICE: GuidPartitionTable::handle_async_disc_cache_prune: "
                 "Received remote cache prune for keys=[%C] from relay '%C' while synchronization is disabled!\n",
                 keys_str.c_str(), from_relay.c_str()));
    }
    return;
  }

  // Suppose there are two remote relays R1 and R2, and a user A expires on R1 and connects to R2 in a short
  // period of time. Depending on the receive order of the prune message from R1 and the update message from R2,
  // this relay may end up with no cache entry for A -- when the prune message arrives after the update message.
  // To address this, we can keep the relay Id together with the cache entry and only remove the entry when the
  // prune message comes from the same relay. For simplicity, we currently do not track the relay Id and accept
  // the possibility of remote cache inconsistency.
  StringSet removed_keys;
  const auto sizes = remote_async_disc_cache_.remove(keys, removed_keys);

  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();
  relay_stats_reporter_.async_discovery_remote_cache_size(sizes.first, now);
  relay_stats_reporter_.async_discovery_remote_expiration_map_size(sizes.second, now);

  if (config_.log_async_discovery()) {
    const std::string keys_str = concat_strings(removed_keys);
    ACE_DEBUG((LM_INFO,
               "(%P|%t) INFO: GuidPartitionTable::handle_async_disc_cache_prune: "
               "Removed remote cache keys=[%C] from relay '%C' -- remote cache size=%B remote expiration map size=%B\n",
               keys_str.c_str(), from_relay.c_str(), sizes.first, sizes.second));
  }
}

bool GuidPartitionTable::AsyncDiscoveryCache::lookup(StringSet& partitions, const std::string& key)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  const auto it = cert_to_partitions_.find(key);
  if (it != cert_to_partitions_.end()) {
    partitions = it->second.first;

    // Update last access time
    const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();
    remove_from_expiration_map(it->second.second, key);
    it->second.second = now;
    expiration_map_[now].insert(key);
    return true;
  }
  return false;
}

std::pair<size_t, size_t>
GuidPartitionTable::AsyncDiscoveryCache::update(const std::string& key, const StringSet& partitions,
                                                const OpenDDS::DCPS::MonotonicTimePoint& now, StringSet* prev_partitions)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, std::make_pair(0, 0));
  update_i(key, partitions, now, prev_partitions);
  return {cert_to_partitions_.size(), expiration_map_.size()};
}

std::pair<size_t, size_t>
GuidPartitionTable::AsyncDiscoveryCache::update(const AsyncDiscoveryCacheEntrySeq& entries, const OpenDDS::DCPS::MonotonicTimePoint& now)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, std::make_pair(0, 0));
  for (const auto& entry : entries) {
    const auto& key = entry.key();
    const StringSet partitions(entry.partitions().begin(), entry.partitions().end());
    update_i(key, partitions, now, nullptr);
  }
  return {cert_to_partitions_.size(), expiration_map_.size()};
}

bool GuidPartitionTable::AsyncDiscoveryCache::remove(const std::string& key)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remove_i(key);
}

std::pair<size_t, size_t>
GuidPartitionTable::AsyncDiscoveryCache::remove(const AsyncDiscoveryCacheEntrySeq& entries, StringSet& removed_keys)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, std::make_pair(0, 0));
  for (const auto& entry : entries) {
    if (remove_i(entry.key())) {
      removed_keys.insert(entry.key());
    }
  }
  return {cert_to_partitions_.size(), expiration_map_.size()};
}

std::pair<size_t, size_t>
GuidPartitionTable::AsyncDiscoveryCache::remove(const StringSequence& keys, StringSet& removed_keys)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, std::make_pair(0, 0));
  for (const auto& key : keys) {
    if (remove_i(key)) {
      removed_keys.insert(key);
    }
  }
  return {cert_to_partitions_.size(), expiration_map_.size()};
}

std::pair<size_t, size_t>
GuidPartitionTable::AsyncDiscoveryCache::remove_expired(const OpenDDS::DCPS::MonotonicTimePoint& now, const OpenDDS::DCPS::TimeDuration& timeout,
                                                        StringSet& expired_keys, bool record_expired_keys)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, std::make_pair(0, 0));
  const auto cutoff_time = now - timeout;
  const auto upper_bound = expiration_map_.upper_bound(cutoff_time);
  for (auto it = expiration_map_.begin(); it != upper_bound;) {
    const auto& keys = it->second;
    if (record_expired_keys) {
      expired_keys.insert(keys.begin(), keys.end());
    }
    for (const auto& key : keys) {
      cert_to_partitions_.erase(key);
    }
    it = expiration_map_.erase(it);
  }
  return {cert_to_partitions_.size(), expiration_map_.size()};
}

const OpenDDS::DCPS::MonotonicTimePoint GuidPartitionTable::AsyncDiscoveryCache::earliest_last_access() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, OpenDDS::DCPS::MonotonicTimePoint::max_value);
  if (!expiration_map_.empty()) {
    return expiration_map_.begin()->first;
  }
  return OpenDDS::DCPS::MonotonicTimePoint::max_value;
}

void GuidPartitionTable::AsyncDiscoveryCache::update_i(const std::string& key, const StringSet& partitions,
                                                       const OpenDDS::DCPS::MonotonicTimePoint& now, StringSet* prev_partitions)
{
  // Caller should hold lock already.
  // Remove old entry
  const auto it = cert_to_partitions_.find(key);
  if (it != cert_to_partitions_.end()) {
    if (prev_partitions) {
      *prev_partitions = it->second.first;
    }
    remove_from_expiration_map(it->second.second, key);
    cert_to_partitions_.erase(it);
  }

  // Add new entry
  if (!partitions.empty()) {
    cert_to_partitions_[key] = std::make_pair(partitions, now);
    expiration_map_[now].insert(key);
  }
}

bool GuidPartitionTable::AsyncDiscoveryCache::remove_i(const std::string& key)
{
  // Caller should hold lock already.
  const auto it = cert_to_partitions_.find(key);
  if (it != cert_to_partitions_.end()) {
    remove_from_expiration_map(it->second.second, key);
    cert_to_partitions_.erase(it);
    return true;
  }
  return false;
}

void GuidPartitionTable::AsyncDiscoveryCache::remove_from_expiration_map(const OpenDDS::DCPS::MonotonicTimePoint& last_access, const std::string& key)
{
  // Helper function, the caller should hold lock already.
  const auto it = expiration_map_.find(last_access);
  if (it != expiration_map_.end()) {
    it->second.erase(key);
    if (it->second.empty()) {
      expiration_map_.erase(it);
    }
  }
}

}
