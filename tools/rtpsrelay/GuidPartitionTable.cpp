#include "GuidPartitionTable.h"
namespace RtpsRelay {

void GuidPartitionTable::insert(const OpenDDS::DCPS::GUID_t& guid, const DDS::StringSeq& partitions)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  StringSet parts;
  for (CORBA::ULong idx = 0; idx != partitions.length(); ++idx) {
    parts.insert(partitions[idx].in());
  }
  if (parts.empty()) {
    // Special case for empty list of partitions.
    parts.insert("");
  }

  const auto& x = guid_to_partitions_[guid];

  std::vector<std::string> to_add;
  std::set_difference(parts.begin(), parts.end(), x.begin(), x.end(), std::back_inserter(to_add));

  std::vector<std::string> to_remove;
  std::set_difference(x.begin(), x.end(), parts.begin(), parts.end(), std::back_inserter(to_remove));

  if (to_add.empty() && to_remove.empty()) {
    // No change.
    return;
  }

  SpdpReplay spdp_replay;
  populate_replay(spdp_replay, guid, to_add);

  StringSet globally_new;
  {
    const auto r = guid_to_partitions_.insert(std::make_pair(guid, StringSet()));
    r.first->second.insert(to_add.begin(), to_add.end());
    for (const auto& part : to_add) {
      const auto q = partition_to_guid_.insert(std::make_pair(part, OrderedGuidSet()));
      q.first->second.insert(guid);
      partition_index_.insert(part, guid);
      if (q.second) {
        globally_new.insert(part);
      }
    }
    for (const auto& part : to_remove) {
      r.first->second.erase(part);
      partition_to_guid_[part].erase(guid);
      partition_index_.remove(part, guid);
      if (partition_to_guid_[part].empty()) {
        partition_to_guid_.erase(part);
      }
    }
    if (r.first->second.empty()) {
      guid_to_partitions_.erase(r.first);
    }
  }

  add_new(globally_new);

  if (spdp_replay_writer_->write(spdp_replay, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to write Relay Partitions\n")));
  }
}

}
