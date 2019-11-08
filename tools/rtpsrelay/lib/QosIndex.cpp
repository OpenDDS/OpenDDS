#include "QosIndex.h"

#include <dds/DCPS/DCPS_Utils.h>

namespace RtpsRelay {

void NoIndex::match(Writers::iterator pos,
                    RelayAddressesMap& relay_addresses_map)
{
  const auto writer = pos->first;
  auto& matched_readers = pos->second;

  for (auto pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
    const auto reader = pos->first;
    auto& matched_writers = pos->second;
    if (!(reader->remote() && writer->remote()) &&
        writer->writer_entry().topic_name() == reader->reader_entry().topic_name() &&
        writer->writer_entry().type_name() == reader->reader_entry().type_name() &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry()._data_writer_qos, &reader->reader_entry()._data_reader_qos) &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry()._publisher_qos, &reader->reader_entry()._subscriber_qos) &&
        OpenDDS::DCPS::matching_partitions(writer->writer_entry()._publisher_qos.partition, reader->reader_entry()._subscriber_qos.partition)) {
      if (matched_readers.count(reader) == 0) {
        matched_readers.insert(reader);
        matched_writers.insert(writer);
        add_reader(reader, relay_addresses_map);
      }
    } else {
      if (matched_readers.count(reader) == 1) {
        matched_readers.erase(reader);
        matched_writers.erase(writer);
      }
    }
  }
}

void NoIndex::match(Readers::iterator pos,
                    RelayAddressesMap& relay_addresses_map)
{
  const auto reader = pos->first;
  auto& matched_writers = pos->second;

  for (Writers::iterator pos = writers_.begin(), limit = writers_.end(); pos != limit; ++pos) {
    const auto writer = pos->first;
    auto& matched_readers = pos->second;
    if (!(reader->remote() && writer->remote()) &&
        writer->writer_entry().topic_name() == reader->reader_entry().topic_name() &&
        writer->writer_entry().type_name() == reader->reader_entry().type_name() &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry()._data_writer_qos, &reader->reader_entry()._data_reader_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::compatibleQOS(&writer->writer_entry()._publisher_qos, &reader->reader_entry()._subscriber_qos, nullptr, nullptr) &&
        OpenDDS::DCPS::matching_partitions(writer->writer_entry()._publisher_qos.partition, reader->reader_entry()._subscriber_qos.partition)) {
      if (matched_writers.count(writer) == 0) {
        matched_writers.insert(writer);
        matched_readers.insert(reader);
        add_writer(writer, relay_addresses_map);
      }
    } else {
      if (matched_writers.count(writer) == 1) {
        matched_writers.erase(writer);
        matched_readers.erase(reader);
      }
    }
  }
}

}
