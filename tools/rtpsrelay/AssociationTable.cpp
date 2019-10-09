#include "AssociationTable.h"

#include "dds/DCPS/DCPS_Utils.h"

namespace RtpsRelay {

void AssociationTable::insert(const WriterEntry& writer_entry)
{
  const auto writer_guid = guid_to_guid(writer_entry.guid());
  const auto p = writers_.find(writer_guid);
  if (p == writers_.end()) {
    WriterPtr writer(new Writer(writer_entry, writer_entry.relay_addresses() == relay_addresses_));
    writers_[writer_guid] = writer;
    index_.insert(writer);
  } else {
    index_.reinsert(p->second, writer_entry);
  }
}

void AssociationTable::remove(const WriterEntry& writer)
{
  const auto writer_guid = guid_to_guid(writer.guid());
  const auto pos = writers_.find(writer_guid);
  index_.erase(pos->second);
  writers_.erase(pos);
}

void AssociationTable::insert(const ReaderEntry& reader_entry)
{
  const auto reader_guid = guid_to_guid(reader_entry.guid());
  const auto p = readers_.find(reader_guid);
  if (p == readers_.end()) {
    ReaderPtr reader(new Reader(reader_entry, reader_entry.relay_addresses() == relay_addresses_));
    readers_[reader_guid] = reader;
    index_.insert(reader);
  } else {
    index_.reinsert(p->second, reader_entry);
  }
}

void AssociationTable::remove(const ReaderEntry& reader)
{
  const auto reader_guid = guid_to_guid(reader.guid());
  const auto pos = readers_.find(reader_guid);
  index_.erase(pos->second);
  readers_.erase(pos);
}

RelayAddresses AssociationTable::get_relay_addresses_for_participant(const OpenDDS::DCPS::RepoId& guid) const
{
  OpenDDS::DCPS::RepoId prefix(guid);
  prefix.entityId = OpenDDS::DCPS::ENTITYID_UNKNOWN;

  {
    const auto pos = writers_.lower_bound(prefix);
    if (pos != writers_.end() && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0) {
      return pos->second->writer_entry.relay_addresses();
    }
  }

  {
    const auto pos = readers_.lower_bound(prefix);
    if (pos != readers_.end() && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0) {
      return pos->second->reader_entry.relay_addresses();
    }
  }
  return {};
}

void AssociationTable::get_guids(const OpenDDS::DCPS::RepoId& guid,
                                 GuidSet& local_guids,
                                 GuidSet& remote_guids) const
{
  // Match on the prefix.
  OpenDDS::DCPS::RepoId prefix(guid);
  prefix.entityId = OpenDDS::DCPS::ENTITYID_UNKNOWN;

  for (auto pos = writers_.lower_bound(prefix), limit = writers_.end();
       pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    ReaderSet readers;
    for (const auto index : pos->second->indexes) {
      index->get_readers(pos->second, readers);
    }
    for (const auto reader : readers) {
      OpenDDS::DCPS::RepoId x = guid_to_guid(reader->reader_entry.guid());
      x.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      if (reader->local()) {
        local_guids.insert(x);
      } else {
        remote_guids.insert(x);
      }
    }
  }

  for (auto pos = readers_.lower_bound(prefix), limit = readers_.end();
       pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    WriterSet writers;
    for (const auto index : pos->second->indexes) {
      index->get_writers(pos->second, writers);
    }

    for (const auto writer : writers) {
      OpenDDS::DCPS::RepoId x = guid_to_guid(writer->writer_entry.guid());
      x.entityId = OpenDDS::DCPS::ENTITYID_PARTICIPANT;
      if (writer->local()) {
        local_guids.insert(x);
      } else {
        remote_guids.insert(x);
      }
    }
  }
}

}
