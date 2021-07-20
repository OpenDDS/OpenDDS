#ifndef RTPSRELAY_RELAY_PARTITION_TABLE_H_
#define RTPSRELAY_RELAY_PARTITION_TABLE_H_

#include "lib/PartitionIndex.h"
#include "lib/Utility.h"

#include "dds/DCPS/GuidConverter.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

typedef std::set<ACE_INET_Addr> AddressSet;
typedef std::pair<OpenDDS::DCPS::GUID_t, size_t> SlotKey;

class RelayPartitionTable {
public:
  RelayPartitionTable()
    : complete_(relay_to_address_)
  {}

  void insert(const OpenDDS::DCPS::GUID_t& application_participant_guid,
              const std::string& name,
              const ACE_INET_Addr& address)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    relay_to_address_[application_participant_guid][name] = address;
  }

  void remove(const OpenDDS::DCPS::GUID_t& application_participant_guid,
              const std::string& name)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const auto pos1 = relay_to_address_.find(application_participant_guid);
    if (pos1 != relay_to_address_.end()) {
      pos1->second.erase(name);
      if (pos1->second.empty()) {
        relay_to_address_.erase(pos1);
      }
    }
  }

  void complete_insert(const SlotKey& slot_key,
                       const StringSequence& partitions)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    complete_.insert(slot_key, partitions);
  }

  void lookup(AddressSet& address_set, const StringSet& partitions, const std::string& name) const
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    complete_.lookup(address_set, partitions, name);
  }

private:
  typedef std::map<std::string, ACE_INET_Addr> NameToAddress;
  typedef std::map<OpenDDS::DCPS::GUID_t, NameToAddress> RelayToAddress;
  RelayToAddress relay_to_address_;

  struct Map {
    Map(RelayToAddress& relay_to_address)
      : relay_to_address_(relay_to_address)
    {}

    void insert(const SlotKey& slot_key,
                const StringSequence& partitions)
    {
      StringSet parts(partitions.begin(), partitions.end());

      const auto& x = relay_to_partitions_[slot_key];

      std::vector<std::string> to_add;
      std::set_difference(parts.begin(), parts.end(), x.begin(), x.end(), std::back_inserter(to_add));

      std::vector<std::string> to_remove;
      std::set_difference(x.begin(), x.end(), parts.begin(), parts.end(), std::back_inserter(to_remove));

      if (to_add.empty() && to_remove.empty()) {
        // No change.
        return;
      }

      {
        const auto r = relay_to_partitions_.insert(std::make_pair(slot_key, StringSet()));
        r.first->second.insert(to_add.begin(), to_add.end());
        for (const auto& part : to_add) {
          partition_index_.insert(part, slot_key.first);
        }
        for (const auto& part : to_remove) {
          r.first->second.erase(part);
          partition_index_.remove(part, slot_key.first);
        }
        if (r.first->second.empty()) {
          relay_to_partitions_.erase(r.first);
        }
      }
    }

    void lookup(AddressSet& address_set, const StringSet& partitions, const std::string& name) const
    {
      for (const auto& partition : partitions) {
        GuidSet guids;
        partition_index_.lookup(partition, guids);
        for (const auto& guid : guids) {
          const auto pos2 = relay_to_address_.find(guid);
          if (pos2 != relay_to_address_.end()) {
            const auto pos3 = pos2->second.find(name);
            if (pos3 != pos2->second.end()) {
              address_set.insert(pos3->second);
            }
          }
        }
      }
    }

    RelayToAddress& relay_to_address_;

    PartitionIndex partition_index_;

    typedef std::map<SlotKey, StringSet> RelayToPartitions;
    RelayToPartitions relay_to_partitions_;
  };

  Map complete_;

  mutable ACE_Thread_Mutex mutex_;
};

}

#endif // RTPSRELAY_RELAY_PARTITION_TABLE_H_
