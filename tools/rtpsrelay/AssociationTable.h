#ifndef RTPSRELAY_ASSOCIATION_TABLE_H_
#define RTPSRELAY_ASSOCIATION_TABLE_H_

#include "lib/RelayTypeSupportImpl.h"
#include "utility.h"
#include "lib/QosIndex.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

class AssociationTable {
public:
  explicit AssociationTable(const RelayAddresses& relay_addresses) :
    relay_addresses_(relay_addresses)
  {}
  void insert(const WriterEntry& entry,
              GuidSet& local_guids,
              RelayAddressesSet& relay_addresses);
  void remove(const WriterEntry& entry);
  void insert(const ReaderEntry& entry,
              GuidSet& local_guids,
              RelayAddressesSet& relay_addresses);
  void remove(const ReaderEntry& entry);

  void get_guids(const OpenDDS::DCPS::RepoId& guid, GuidSet& local_guids, RelayAddressesSet& relay_addresses) const;

private:
  const RelayAddresses& relay_addresses_;

  typedef std::map<OpenDDS::DCPS::RepoId, WriterPtr, OpenDDS::DCPS::GUID_tKeyLessThan> WritersMap;
  WritersMap writers_;
  typedef std::map<OpenDDS::DCPS::RepoId, ReaderPtr, OpenDDS::DCPS::GUID_tKeyLessThan> ReadersMap;
  ReadersMap readers_;

  TopicIndex<PartitionIndex<NoIndex> > index_;

  ACE_Thread_Mutex mutex_;
};

}

#endif // RTPSRELAY_ASSOCIATION_TABLE_H_
