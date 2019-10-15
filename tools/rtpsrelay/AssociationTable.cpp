#include "AssociationTable.h"

#include <dds/DCPS/DCPS_Utils.h>

#include <ace/Global_Macros.h>

namespace RtpsRelay {

void AssociationTable::insert(const WriterEntry& writer_entry,
                              GuidSet& local_guids,
                              RelayAddressesSet& relay_addresses)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto local = writer_entry.relay_addresses() == relay_addresses_;
  const auto writer_guid = guid_to_guid(writer_entry.guid());
  const auto p = writers_.find(writer_guid);
  if (p == writers_.end()) {
    WriterPtr writer(new Writer(writer_entry, local));
    writers_[writer_guid] = writer;
    index_.insert(writer, local_guids, relay_addresses);
  } else {
    index_.reinsert(p->second, writer_entry, local, local_guids, relay_addresses);
  }
}

void AssociationTable::remove(const WriterEntry& writer)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto writer_guid = guid_to_guid(writer.guid());
  const auto pos = writers_.find(writer_guid);
  index_.erase(pos->second);
  writers_.erase(pos);
}

void AssociationTable::insert(const ReaderEntry& reader_entry,
                              GuidSet& local_guids,
                              RelayAddressesSet& relay_addresses)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto local = reader_entry.relay_addresses() == relay_addresses_;
  const auto reader_guid = guid_to_guid(reader_entry.guid());
  const auto p = readers_.find(reader_guid);
  if (p == readers_.end()) {
    ReaderPtr reader(new Reader(reader_entry, reader_entry.relay_addresses() == relay_addresses_));
    readers_[reader_guid] = reader;
    index_.insert(reader, local_guids, relay_addresses);
  } else {
    index_.reinsert(p->second, reader_entry, local, local_guids, relay_addresses);
  }
}

void AssociationTable::remove(const ReaderEntry& reader)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto reader_guid = guid_to_guid(reader.guid());
  const auto pos = readers_.find(reader_guid);
  index_.erase(pos->second);
  readers_.erase(pos);
}

void AssociationTable::get_guids(const OpenDDS::DCPS::RepoId& guid,
                                 GuidSet& local_guids,
                                 RelayAddressesSet& relay_addresses) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, const_cast<ACE_Thread_Mutex&>(mutex_));
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
      add_reader(reader, local_guids, relay_addresses);
    }
  }

  for (auto pos = readers_.lower_bound(prefix), limit = readers_.end();
       pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    WriterSet writers;
    for (const auto index : pos->second->indexes) {
      index->get_writers(pos->second, writers);
    }

    for (const auto writer : writers) {
      add_writer(writer, local_guids, relay_addresses);
    }
  }
}

}
