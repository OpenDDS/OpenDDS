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
      // No change.
      return NO_CHANGE;
    }

    remove_from_cache(guid);

    populate_replay(spdp_replay, guid, to_add);

    StringSet globally_new;
    {
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
    }

    add_new(relay_partitions, globally_new);
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
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: GuidPartitionTable::insert %C add partitions %C\n"), guid_to_string(part_guid).c_str(), OpenDDS::DCPS::to_json(spdp_replay).c_str()));
  }

  return result;
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
      denied_partitions_cleanup_task_ = OpenDDS::DCPS::make_rch<DeniedPartitionsCleanupSporadicTask>(TheServiceParticipant->time_source(),
                                                                 reactor_task_,
                                                                 rchandle_from(this),
                                                                 &GuidPartitionTable::cleanup_denied_partitions);
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
