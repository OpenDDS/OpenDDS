#include "AssociationTable.h"

#include <dds/DCPS/DCPS_Utils.h>

#include <ace/Global_Macros.h>

namespace RtpsRelay {

void AssociationTable::insert(const WriterEntry& writer_entry,
                              GuidSet& guids)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto writer_guid = guid_to_repoid(writer_entry.guid());
  const auto p = writers_.find(writer_guid);
  if (p == writers_.end()) {
    WriterPtr writer(new Writer(writer_entry));
    writers_[writer_guid] = writer;
    index_.insert(writer, guids);
  } else {
    index_.reinsert(p->second, writer_entry, guids);
  }
}

void AssociationTable::remove(const WriterEntry& writer)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto writer_guid = guid_to_repoid(writer.guid());
  const auto pos = writers_.find(writer_guid);
  index_.erase(pos->second);
  writers_.erase(pos);
}

void AssociationTable::insert(const ReaderEntry& reader_entry,
                              GuidSet& guids)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto reader_guid = guid_to_repoid(reader_entry.guid());
  const auto p = readers_.find(reader_guid);
  if (p == readers_.end()) {
    ReaderPtr reader(new Reader(reader_entry));
    readers_[reader_guid] = reader;
    index_.insert(reader, guids);
  } else {
    index_.reinsert(p->second, reader_entry, guids);
  }
}

void AssociationTable::remove(const ReaderEntry& reader)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  const auto reader_guid = guid_to_repoid(reader.guid());
  const auto pos = readers_.find(reader_guid);
  index_.erase(pos->second);
  readers_.erase(pos);
}

void AssociationTable::lookup_destinations(GuidSet& to,
                                           const OpenDDS::DCPS::RepoId& from) const
{
  if (!to.empty()) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  // Match on the prefix.
  OpenDDS::DCPS::RepoId prefix(from);
  prefix.entityId = OpenDDS::DCPS::ENTITYID_UNKNOWN;

  for (auto pos = writers_.lower_bound(prefix), limit = writers_.end();
       pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    ReaderSet readers;
    for (const auto index : pos->second->indexes) {
      index->get_readers(pos->second, readers);
    }
    for (const auto reader : readers) {
      to.insert(reader->participant_guid());
    }
  }

  for (auto pos = readers_.lower_bound(prefix), limit = readers_.end();
       pos != limit && std::memcmp(pos->first.guidPrefix, prefix.guidPrefix, sizeof(prefix.guidPrefix)) == 0; ++pos) {
    WriterSet writers;
    for (const auto index : pos->second->indexes) {
      index->get_writers(pos->second, writers);
    }

    for (const auto writer : writers) {
      to.insert(writer->participant_guid());
    }
  }
}

}
