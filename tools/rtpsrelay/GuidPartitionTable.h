#ifndef RTPSRELAY_GUID_PARTITION_TABLE_H_
#define RTPSRELAY_GUID_PARTITION_TABLE_H_

#include "Config.h"

#include "lib/PartitionIndex.h"
#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

#include "dds/DCPS/GuidConverter.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

// FUTURE: Make this configurable, adaptive, etc.
const size_t MAX_SLOT_SIZE = 64;

class GuidPartitionTable {
public:
  GuidPartitionTable(const Config& config,
                     RelayPartitionsDataWriter_var relay_partitions_writer,
                     SpdpReplayDataWriter_var spdp_replay_writer)
    : config_(config)
    , relay_partitions_writer_(relay_partitions_writer)
    , spdp_replay_writer_(spdp_replay_writer)
  {}

  // Insert a reader/writer guid and its partitions.
  void insert(const OpenDDS::DCPS::GUID_t& guid, const DDS::StringSeq& partitions);

  void remove(const OpenDDS::DCPS::GUID_t& guid)
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
    }

    remove_defunct(defunct);
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

  void add_new(const StringSet& partitions)
  {
    std::set<size_t> slots_to_write;
    for (const auto& partition : partitions) {
      const size_t slot = get_free_slot();
      add_to_slot(slot, partition);
      slots_to_write.insert(slot);
    }

    write_slots(slots_to_write);
  }

  void remove_defunct(const StringSet& partitions)
  {
    std::set<size_t> slots_to_write;
    for (const auto& partition : partitions) {
      const size_t slot = partition_to_slot_[partition];
      remove_from_slot(slot, partition);
      slots_to_write.insert(slot);
    }

    write_slots(slots_to_write);
  }

  void write_slots(const std::set<size_t>& slots_to_write)
  {
    RelayPartitions relay_partitions;
    relay_partitions.application_participant_guid(repoid_to_guid(config_.application_participant_guid()));
    for (const auto slot : slots_to_write) {
      relay_partitions.slot(static_cast<CORBA::ULong>(slot));
      relay_partitions.partitions().assign(slots_[slot].begin(), slots_[slot].end());
      if (relay_partitions_writer_->write(relay_partitions, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to write Relay Partitions\n")));
      }
    }
  }

  size_t get_free_slot()
  {
    if (!free_slot_list_.empty()) {
      return free_slot_list_.front();
    } else {
      const size_t slot = slots_.size();
      slots_.resize(slot + 1);
      free_slot_list_.push_back(slot);
      return slot;
    }
  }

  void add_to_slot(size_t slot, const std::string& partition_name)
  {
    OPENDDS_ASSERT(slot < slots_.size());
    OPENDDS_ASSERT(!free_slot_list_.empty());
    OPENDDS_ASSERT(slot == free_slot_list_.front());
    OPENDDS_ASSERT(slots_[slot].size() < MAX_SLOT_SIZE);

    slots_[slot].insert(partition_name);
    partition_to_slot_[partition_name] = slot;
    if (slots_[slot].size() == MAX_SLOT_SIZE) {
      free_slot_list_.pop_front();
    }
  }

  void remove_from_slot(size_t slot, const std::string& partition_name)
  {
    OPENDDS_ASSERT(slot < slots_.size());
    OPENDDS_ASSERT(slots_[slot].count(partition_name) != 0);
    OPENDDS_ASSERT(partition_to_slot_[partition_name] == slot);

    const bool full = slots_.size() == MAX_SLOT_SIZE;
    slots_[slot].erase(partition_name);
    partition_to_slot_.erase(partition_name);
    if (full) {
      free_slot_list_.push_back(slot);
    }
  }

  const Config& config_;
  RelayPartitionsDataWriter_var relay_partitions_writer_;

  typedef std::vector<StringSet> Slots;
  Slots slots_;
  typedef std::list<size_t> FreeSlotList;
  FreeSlotList free_slot_list_;
  typedef std::unordered_map<std::string, size_t> PartitionToSlot;
  PartitionToSlot partition_to_slot_;

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
