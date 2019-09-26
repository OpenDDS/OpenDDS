#include "QosIndex.h"

#include "dds/DCPS/DCPS_Utils.h"

void NoIndex::match(Writers::iterator pos)
{
  Writer* writer = pos->first;
  ReaderSet& matched_readers = pos->second;

  for (Readers::iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
    Reader* reader = pos->first;
    std::set<Writer*>& matched_writers = pos->second;
    if (writer->remote() && reader->remote()) {
      continue;
    }
    if (writer->writer_entry.topic_name() == reader->reader_entry.topic_name() &&
        writer->writer_entry.type_name() == reader->reader_entry.type_name() &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry._data_writer_qos, &reader->reader_entry._data_reader_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry._publisher_qos, &reader->reader_entry._subscriber_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::matching_partitions(writer->writer_entry._publisher_qos.partition, reader->reader_entry._subscriber_qos.partition)) {

      if (matched_readers.count(reader) == 0) {
        matched_readers.insert(reader);
        matched_writers.insert(writer);
      }
    } else {
      if (matched_readers.count(reader) == 1) {
        matched_readers.erase(reader);
        matched_writers.erase(writer);
      }
    }
  }
}

void NoIndex::match(Readers::iterator pos)
{
  Reader* reader = pos->first;
  std::set<Writer*>& matched_writers = pos->second;

  for (Writers::iterator pos = writers_.begin(), limit = writers_.end(); pos != limit; ++pos) {
    Writer* writer = pos->first;
    std::set<Reader*>& matched_readers = pos->second;
    if (reader->remote() && writer->remote()) {
      continue;
    }
    if (writer->writer_entry.topic_name() == reader->reader_entry.topic_name() &&
        writer->writer_entry.type_name() == reader->reader_entry.type_name() &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry._data_writer_qos, &reader->reader_entry._data_reader_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry._publisher_qos, &reader->reader_entry._subscriber_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::matching_partitions(writer->writer_entry._publisher_qos.partition, reader->reader_entry._subscriber_qos.partition)) {
      if (matched_writers.count(writer) == 0) {
        matched_writers.insert(writer);
        matched_readers.insert(reader);
      }
    } else {
      if (matched_writers.count(writer) == 1) {
        matched_writers.erase(writer);
        matched_readers.erase(reader);
      }
    }
  }
}
