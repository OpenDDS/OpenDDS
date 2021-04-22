#ifndef RTPSRELAY_GUID_PARTITION_TABLE_H_
#define RTPSRELAY_GUID_PARTITION_TABLE_H_

#include "lib/RelayTypeSupportImpl.h"
#include "utility.h"

#include "dds/DCPS/GuidConverter.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

const size_t MAX_INCREMENT_SIZE = 20;

class GuidPartitionTable {
public:
  GuidPartitionTable(const Config& config,
                     RelayPartitionsDataWriter_var relay_partitions_writer,
                     RelayPartitionsIncrementDataWriter_var relay_partitions_increment_writer,
                     SpdpReplayDataWriter_var spdp_replay_writer)
    : config_(config)
    , relay_partitions_writer_(relay_partitions_writer)
    , relay_partitions_increment_writer_(relay_partitions_increment_writer)
    , spdp_replay_writer_(spdp_replay_writer)
  {
    relay_partitions_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
    relay_partitions_increment_.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
  }

  // Insert a reader/writer guid and its partitions.
  void insert(const OpenDDS::DCPS::GUID_t& guid, const DDS::StringSeq& partitions)
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
        const auto q = partition_to_guid_.insert(std::make_pair(part, OGuidSet()));
        q.first->second.insert(guid);
        if (q.second) {
          globally_new.insert(part);
        }
      }
      for (const auto& part : to_remove) {
        r.first->second.erase(part);
        partition_to_guid_[part].erase(guid);
        if (partition_to_guid_[part].empty()) {
          partition_to_guid_.erase(part);
        }
      }
      if (r.first->second.empty()) {
        guid_to_partitions_.erase(r.first);
      }
    }

    add_globally_new(globally_new);

    if (spdp_replay_writer_->write(spdp_replay, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to write Relay Partitions\n")));
    }
  }

  void remove(const OpenDDS::DCPS::GUID_t& guid)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const auto pos = guid_to_partitions_.find(guid);
    if (pos != guid_to_partitions_.end()) {
      for (const auto& partition : pos->second) {
        partition_to_guid_[partition].erase(guid);
        if (partition_to_guid_[partition].empty()) {
          partition_to_guid_.erase(partition);
        }
      }
    }
    guid_to_partitions_.erase(guid);
  }

  // Look up the partitions for the participant from.
  void lookup(StringSet& partitions, const OpenDDS::DCPS::GUID_t& from) const
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    // Match on the prefix.
    OpenDDS::DCPS::RepoId prefix(from);
    prefix.entityId = OpenDDS::DCPS::ENTITYID_UNKNOWN;

    for (auto pos = guid_to_partitions_.lower_bound(prefix), limit = guid_to_partitions_.end();
         pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
      partitions.insert(pos->second.begin(), pos->second.end());
    }
  }

  void lookup(GuidSet& guids, const StringSet& partitions) const
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (const auto& part : partitions) {
      const auto pos = partition_to_guid_.find(part);
      if (pos != partition_to_guid_.end()) {
        for (const auto& guid : pos->second) {
          guids.insert(make_part_guid(guid));
        }
      }
    }
  }

  void lookup(GuidSet& guids, const StringSequence& partitions) const
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (const auto& part : partitions) {
      const auto pos = partition_to_guid_.find(part);
      if (pos != partition_to_guid_.end()) {
        for (const auto& guid : pos->second) {
          guids.insert(make_part_guid(guid));
        }
      }
    }
  }

private:
  void populate_replay(SpdpReplay& spdp_replay,
                       const OpenDDS::DCPS::GUID_t& guid,
                       const std::vector<std::string>& to_add) const
  {
    // The partitions are new for this reader/writer.
    // Check if they are new for the participant.

    const OpenDDS::DCPS::GUID_t prefix = make_id(guid, OpenDDS::DCPS::ENTITYID_UNKNOWN);

    for (const auto& part : to_add) {
      const auto pos1 = partition_to_guid_.find(part);
      if (pos1 == partition_to_guid_.end()) {
        spdp_replay.partitions().push_back(part);
        continue;
      }

      const auto pos2 = pos1->second.lower_bound(prefix);

      if (pos2 == pos1->second.end() ||
          std::memcmp(pos2->guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) != 0) {
        spdp_replay.partitions().push_back(part);
      }
    }
  }

  void add_globally_new(const StringSet& partitions)
  {
    std::copy(partitions.begin(), partitions.end(), std::back_inserter(relay_partitions_increment_.partitions()));
    if (relay_partitions_increment_writer_->write(relay_partitions_increment_, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to write Relay Partitions Increment\n")));
    }

    if (relay_partitions_increment_.partitions().size() > MAX_INCREMENT_SIZE) {
      relay_partitions_increment_.partitions().clear();
      for (const auto& p : partition_to_guid_) {
        relay_partitions_.partitions().push_back(p.first);
      }
      if (relay_partitions_writer_->write(relay_partitions_, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to write Relay Partitions\n")));
      }
    }
  }

  const Config& config_;
  RelayPartitionsDataWriter_var relay_partitions_writer_;
  RelayPartitions relay_partitions_;
  RelayPartitionsIncrementDataWriter_var relay_partitions_increment_writer_;
  RelayPartitionsIncrement relay_partitions_increment_;
  SpdpReplayDataWriter_var spdp_replay_writer_;

  typedef std::map<OpenDDS::DCPS::RepoId, StringSet, OpenDDS::DCPS::GUID_tKeyLessThan> GuidToPartitions;
  GuidToPartitions guid_to_partitions_;

  typedef std::set<OpenDDS::DCPS::GUID_t, OpenDDS::DCPS::GUID_tKeyLessThan> OGuidSet;
  typedef std::map<std::string, OGuidSet> PartitionToGuid;
  PartitionToGuid partition_to_guid_;

  mutable ACE_Thread_Mutex mutex_;
};

}

#endif // RTPSRELAY_GUID_PARTITION_TABLE_H_
