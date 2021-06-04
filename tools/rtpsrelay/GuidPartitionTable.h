#ifndef RTPSRELAY_GUID_PARTITION_TABLE_H_
#define RTPSRELAY_GUID_PARTITION_TABLE_H_

#include "Config.h"

#include "lib/PartitionIndex.h"
#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

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
  void insert(const OpenDDS::DCPS::GUID_t& guid, const DDS::StringSeq& partitions);

  void remove(const OpenDDS::DCPS::GUID_t& guid)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const auto pos = guid_to_partitions_.find(guid);
    if (pos != guid_to_partitions_.end()) {
      for (const auto& partition : pos->second) {
        partition_index_.remove(partition, guid);
        const auto pos2 = partition_to_guid_.find(partition);
        if (pos2 != partition_to_guid_.end()) {
          pos2->second.erase(guid);
          if (pos2->second.empty()) {
            partition_to_guid_.erase(pos2);
          }
        }
      }
      guid_to_partitions_.erase(pos);
    }
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

  template <typename T>
  void lookup(GuidSet& guids, const T& partitions) const
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (const auto& part : partitions) {
      partition_index_.lookup(part, guids);
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

  typedef std::set<OpenDDS::DCPS::GUID_t, OpenDDS::DCPS::GUID_tKeyLessThan> OrderedGuidSet;
  typedef std::map<std::string, OrderedGuidSet> PartitionToGuid;
  PartitionToGuid partition_to_guid_;
  PartitionIndex partition_index_;

  mutable ACE_Thread_Mutex mutex_;
};

}

#endif // RTPSRELAY_GUID_PARTITION_TABLE_H_
