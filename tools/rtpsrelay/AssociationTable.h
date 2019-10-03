#ifndef RTPSRELAY_ASSOCIATION_TABLE_H_
#define RTPSRELAY_ASSOCIATION_TABLE_H_

#include "RelayTypeSupportImpl.h"
#include "utility.h"

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
  void get_guids_from_local(const OpenDDS::DCPS::RepoId& guid, GuidSet& guids) const;
  void get_guids_to_local(const OpenDDS::DCPS::RepoId& guid, GuidSet& guids) const;
  RelayAddresses get_relay_addresses_for_participant(const OpenDDS::DCPS::RepoId& guid) const;

private:
  void attempt_match(const WriterEntry& writer,
                     bool writer_local,
                     const ReaderEntry& reader,
                     bool reader_local);
  void record_next_hop(const OpenDDS::DCPS::RepoId& local_guid,
                       const OpenDDS::DCPS::RepoId& other_guid);
  void erase_next_hop(const OpenDDS::DCPS::RepoId& local_guid,
                      const OpenDDS::DCPS::RepoId& other_guid);
  void remove_local(const OpenDDS::DCPS::RepoId& guid);
  void remove_remote(const OpenDDS::DCPS::RepoId& guid);

  RelayAddresses relay_addresses_;

  typedef std::map<OpenDDS::DCPS::RepoId, WriterEntry, OpenDDS::DCPS::GUID_tKeyLessThan> WritersMap;
  WritersMap local_writers_;
  WritersMap remote_writers_;
  typedef std::map<OpenDDS::DCPS::RepoId, ReaderEntry, OpenDDS::DCPS::GUID_tKeyLessThan> ReadersMap;
  ReadersMap local_readers_;
  ReadersMap remote_readers_;

  /*
    The following two maps record the associations between local
    readers/writers and their matches.  A local reader or writer is
    one that is connected to this relay.  A remote reader or writer is
    one that is not connected to this relay.

    The key for the forward map is the guid of a local reader/writer
    and the value is the set of matched (local and remote)
    writers/readers.  The forward map is used in the logic that
    forwards RTPS datagrams received on the vertical ports.

    The key for the reverse map is the guid of a remote reader/writer
    and the value is the set of matched local writers/readers.  The
    reverse map is used in the logic that forwards RTPS datagrams
    received on the horizontal ports.  It is used to clean up the
    forward map when a remote reader/writer goes away.

    Excluding the relay addresses, the forward and reverse maps are
    symmetric.  This is a class invariant.
   */
  // Key is guid of local reader or writer.
  typedef std::map<OpenDDS::DCPS::RepoId, GuidSet, OpenDDS::DCPS::GUID_tKeyLessThan> ForwardMap;
  ForwardMap forward_map_;

  // Key is guid of reader or writer.
  // Value is set of the local guids.
  typedef std::map<OpenDDS::DCPS::RepoId, GuidSet, OpenDDS::DCPS::GUID_tKeyLessThan> ReverseMap;
  ReverseMap reverse_map_;
};

}

#endif // RTPSRELAY_ASSOCIATION_TABLE_H_
