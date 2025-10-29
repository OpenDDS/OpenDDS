#include "RelayDeniedPartitionsListener.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/RelayC.h>

#include <ace/Log_Msg.h>

namespace RtpsRelay {

RelayDeniedPartitionsListener::RelayDeniedPartitionsListener(GuidPartitionTable& guid_partition_table)
  : guid_partition_table_(guid_partition_table) {}

void RelayDeniedPartitionsListener::on_data_available(DDS::DataReader_ptr reader)
{
  RelayDeniedPartitionsDataReader_var denied_partitions_reader = RelayDeniedPartitionsDataReader::_narrow(reader);
  if (!denied_partitions_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayDeniedPartitionsListener::on_data_available: "
                         "Failed to narrow RelayDeniedPartitionsDataReader\n"));
    return;
  }

  RelayDeniedPartitions sample;
  DDS::SampleInfo info;
  
  GuidPartitionTable::DeniedPartitions requested_denied_partitions;
  while (denied_partitions_reader->take_next_sample(sample, info) == DDS::RETCODE_OK) {
    if (info.valid_data) {
      // TODO: Handle ACTION_REMOVE
      if (sample.add_or_remove() == Action::ACTION_ADD) {
        requested_denied_partitions.insert(sample.partitions().begin(), sample.partitions().end());
      }
    }
  }
  guid_partition_table_.deny_partitions(requested_denied_partitions);
}

}
