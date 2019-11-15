#ifndef RTPSRELAY_ASSOCIATION_TABLE_H_
#define RTPSRELAY_ASSOCIATION_TABLE_H_

#include "lib/RelayTypeSupportImpl.h"
#include "utility.h"
#include "lib/QosIndex.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

class AssociationTable {
public:
  void insert(const WriterEntry& entry,
              GuidSet& guids);
  void remove(const WriterEntry& entry);
  void insert(const ReaderEntry& entry,
              GuidSet& guids);
  void remove(const ReaderEntry& entry);

  void lookup_destinations(GuidSet& to,
                           const OpenDDS::DCPS::RepoId& from) const;

private:
  typedef std::map<OpenDDS::DCPS::RepoId, WriterPtr, OpenDDS::DCPS::GUID_tKeyLessThan> WritersMap;
  WritersMap writers_;
  typedef std::map<OpenDDS::DCPS::RepoId, ReaderPtr, OpenDDS::DCPS::GUID_tKeyLessThan> ReadersMap;
  ReadersMap readers_;

  TopicIndex<PartitionIndex<NoIndex> > index_;

  mutable ACE_Thread_Mutex mutex_;
};

}

#endif // RTPSRELAY_ASSOCIATION_TABLE_H_
