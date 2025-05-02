#ifndef RTPSRELAY_GUID_PARTITION_TABLE_H_
#define RTPSRELAY_GUID_PARTITION_TABLE_H_

#include "Config.h"
#include "RelayStatisticsReporter.h"

#include <dds/rtpsrelaylib/PartitionIndex.h>
#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/LogAddr.h>

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

// FUTURE: Make this configurable, adaptive, etc.
const size_t MAX_SLOT_SIZE = 64;

class GuidPartitionTable {
public:
  enum Result {
    ADDED,
    UPDATED,
    NO_CHANGE
  };

  GuidPartitionTable(const Config& config,
                     const ACE_INET_Addr& address,
                     RelayPartitionsDataWriter_var relay_partitions_writer,
                     SpdpReplayDataWriter_var spdp_replay_writer,
                     RelayStatisticsReporter& relay_stats_reporter)
    : config_(config)
    , address_(OpenDDS::DCPS::LogAddr(address).c_str())
    , relay_stats_reporter_(relay_stats_reporter)
    , relay_partitions_writer_(relay_partitions_writer)
    , spdp_replay_writer_(spdp_replay_writer)
  {}

  // Insert a reader/writer guid and its partitions.
  Result insert(const OpenDDS::DCPS::GUID_t& guid,
                const DDS::StringSeq& partitions);

  void remove(const OpenDDS::DCPS::GUID_t& guid);

  // Look up the partitions for the participant "from".
  void lookup(StringSet& partitions, const OpenDDS::DCPS::GUID_t& from) const;

  /// Add to 'guids' the GUIDs of participants that should receive messages based on 'partitions'.
  /// If 'allowed' is empty, it has no effect. Otherwise all entires added to 'guids' must be in 'allowed'.
  template <typename T>
  void lookup(GuidSet& guids, const T& partitions, const GuidSet& allowed) const
  {
    const auto limits = allowed.empty() ? nullptr : &allowed;
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (const auto& part : partitions) {
      if (config_.allow_empty_partition() || !part.empty()) {
        partition_index_.lookup(part, guids, limits);
      }
    }
    relay_stats_reporter_.partition_index_cache(partition_index_.cache_size());
  }

private:
  void remove_from_cache(const OpenDDS::DCPS::GUID_t& guid)
  {
    // Invalidate the cache.
    guid_to_partitions_cache_.erase(make_id(guid, OpenDDS::DCPS::ENTITYID_UNKNOWN));
  }

  void populate_replay(SpdpReplay& spdp_replay,
                       const OpenDDS::DCPS::GUID_t& guid,
                       const std::vector<std::string>& to_add) const;

  void add_new(std::vector<RelayPartitions>& relay_partitions, const StringSet& partitions)
  {
    std::unordered_set<size_t> slots_to_write;
    for (const auto& partition : partitions) {
      const size_t slot = get_free_slot();
      add_to_slot(slot, partition);
      slots_to_write.insert(slot);
    }

    relay_stats_reporter_.partition_slots(slots_.size(), free_slot_list_.size());
    relay_stats_reporter_.partitions(partition_to_slot_.size());

    prepare_relay_partitions(relay_partitions, slots_to_write);
  }

  void remove_defunct(std::vector<RelayPartitions>& relay_partitions, const StringSet& partitions)
  {
    std::unordered_set<size_t> slots_to_write;
    for (const auto& partition : partitions) {
      const size_t slot = partition_to_slot_[partition];
      remove_from_slot(slot, partition);
      slots_to_write.insert(slot);
    }

    relay_stats_reporter_.partition_slots(slots_.size(), free_slot_list_.size());
    relay_stats_reporter_.partitions(partition_to_slot_.size());

    prepare_relay_partitions(relay_partitions, slots_to_write);
  }

  void prepare_relay_partitions(std::vector<RelayPartitions>& relay_partitions,
                                const std::unordered_set<size_t>& slots_to_write)
  {
    size_t idx = 0;
    relay_partitions.resize(slots_to_write.size());
    for (const auto slot : slots_to_write) {
      relay_partitions[idx].relay_id(config_.relay_id());
      relay_partitions[idx].slot(static_cast<CORBA::ULong>(slot));
      relay_partitions[idx].partitions().assign(slots_[slot].begin(), slots_[slot].end());
      ++idx;
    }
  }

  void write_relay_partitions(const std::vector<RelayPartitions>& relay_partitions)
  {
    for (const auto& relay_partition : relay_partitions) {
      if (relay_partitions_writer_->write(relay_partition, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: failed to write Relay Partitions\n"));
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

    const bool full = slots_[slot].size() == MAX_SLOT_SIZE;
    slots_[slot].erase(partition_name);
    partition_to_slot_.erase(partition_name);
    if (full) {
      free_slot_list_.push_back(slot);
    }
  }

  const Config& config_;
  const std::string address_;
  RelayStatisticsReporter& relay_stats_reporter_;
  RelayPartitionsDataWriter_var relay_partitions_writer_;

  using Slots = std::vector<StringSet>;
  Slots slots_;

  using FreeSlotList = std::list<size_t>;
  FreeSlotList free_slot_list_;

  using PartitionToSlot = std::unordered_map<std::string, size_t>;
  PartitionToSlot partition_to_slot_;

  SpdpReplayDataWriter_var spdp_replay_writer_;

  using GuidToPartitions = std::map<OpenDDS::DCPS::GUID_t, StringSet, OpenDDS::DCPS::GUID_tKeyLessThan>;
  GuidToPartitions guid_to_partitions_;

  using GuidToPartitionsCache = std::unordered_map<OpenDDS::DCPS::GUID_t, StringSet, GuidHash>;
  mutable GuidToPartitionsCache guid_to_partitions_cache_;

  using OrderedGuidSet = std::set<OpenDDS::DCPS::GUID_t, OpenDDS::DCPS::GUID_tKeyLessThan>;
  using PartitionToGuid = std::unordered_map<std::string, OrderedGuidSet>;
  PartitionToGuid partition_to_guid_;
  PartitionIndex<GuidSet, GuidToParticipantGuid> partition_index_;

  mutable ACE_Thread_Mutex mutex_;
  mutable ACE_Thread_Mutex write_mutex_;
};

}

#endif // RTPSRELAY_GUID_PARTITION_TABLE_H_
