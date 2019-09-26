#ifndef RTPSRELAY_ASSOCIATION_TABLE_H_
#define RTPSRELAY_ASSOCIATION_TABLE_H_

#include "lib/RelayTypeSupportImpl.h"
#include "utility.h"
#include "lib/QosIndex.h"

namespace RtpsRelay {

class AssociationTable {
public:
  explicit AssociationTable(const RelayAddresses& relay_addresses) :
    relay_addresses_(relay_addresses)
  {}
  void insert(const WriterEntry& entry);
  void remove(const WriterEntry& entry);
  void insert(const ReaderEntry& entry);
  void remove(const ReaderEntry& entry);

  const RelayAddresses& local_relay_addresses() const { return relay_addresses_; }
  void get_guids(const OpenDDS::DCPS::RepoId& guid, GuidSet& local_guids, GuidSet& remote_guids) const;
  RelayAddresses get_relay_addresses_for_participant(const OpenDDS::DCPS::RepoId& guid) const;

private:
  RelayAddresses relay_addresses_;

  typedef std::map<OpenDDS::DCPS::RepoId, Writer*, OpenDDS::DCPS::GUID_tKeyLessThan> WritersMap;
  WritersMap writers_;
  typedef std::map<OpenDDS::DCPS::RepoId, Reader*, OpenDDS::DCPS::GUID_tKeyLessThan> ReadersMap;
  ReadersMap readers_;

  TopicIndex<PartitionIndex<NoIndex> > index_;
};

}

#endif // RTPSRELAY_ASSOCIATION_TABLE_H_
