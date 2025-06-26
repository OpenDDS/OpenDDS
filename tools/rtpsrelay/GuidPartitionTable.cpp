#include "GuidPartitionTable.h"

#include "RelayHandler.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

GuidPartitionTable::Result GuidPartitionTable::insert(const OpenDDS::DCPS::GUID_t& guid,
                                                      const DDS::StringSeq& partitions)
{
  Result result;
  std::vector<RelayPartitions> relay_partitions;
  SpdpReplay spdp_replay;

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

    populate_replay(spdp_replay, guid, to_add);

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

    if (!spdp_replay.partitions().empty()) {
      spdp_replay.address(address_);
      spdp_replay.guid(rtps_guid_to_relay_guid(OpenDDS::DCPS::make_id(guid, OpenDDS::DCPS::ENTITYID_PARTICIPANT)));
      if (spdp_replay_writer_->write(spdp_replay, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: GuidPartitionTable::insert failed to write SPDP Replay\n"));
      }
    }
  }

  if (!spdp_replay.partitions().empty() && config_.log_activity()) {
    const auto part_guid = make_id(guid, OpenDDS::DCPS::ENTITYID_PARTICIPANT);
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: GuidPartitionTable::insert %C add partitions %C\n", guid_to_string(part_guid).c_str(), OpenDDS::DCPS::to_json(spdp_replay).c_str()));
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

void GuidPartitionTable::populate_replay(SpdpReplay& spdp_replay,
                                         const OpenDDS::DCPS::GUID_t& guid,
                                         const std::vector<std::string>& to_add) const
{
  // The partitions are new for this reader/writer.
  // Check if they are new for the participant.

  const auto prefix = make_unknown_guid(guid);

  for (const auto& part : to_add) {
    const auto pos1 = partition_to_guid_.find(part);
    if (pos1 == partition_to_guid_.end()) {
      if (config_.allow_empty_partition() || !part.empty()) {
        spdp_replay.partitions().push_back(part);
      }
      continue;
    }

    const auto pos2 = pos1->second.lower_bound(prefix);

    if (pos2 == pos1->second.end() || equal_guid_prefixes(*pos2, prefix)) {
      if (config_.allow_empty_partition() || !part.empty()) {
        spdp_replay.partitions().push_back(part);
      }
    }
  }
}

}
