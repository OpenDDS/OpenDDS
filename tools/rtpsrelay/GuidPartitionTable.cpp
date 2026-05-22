#include "GuidPartitionTable.h"

#include "RelayHandler.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

GuidPartitionTable::~GuidPartitionTable()
{
  if (denied_partitions_cleanup_task_) {
    denied_partitions_cleanup_task_->cancel();
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

}
