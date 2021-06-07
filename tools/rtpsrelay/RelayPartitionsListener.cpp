#include "RelayPartitionsListener.h"

#include "lib/RelayTypeSupportImpl.h"

#include <dds/DCPS/DCPS_Utils.h>

namespace RtpsRelay {

RelayPartitionsListener::RelayPartitionsListener(RelayPartitionTable& relay_partition_table)
  : relay_partition_table_(relay_partition_table)
{}

void RelayPartitionsListener::on_data_available(DDS::DataReader_ptr reader)
{
  RelayPartitionsDataReader_var dr = RelayPartitionsDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayPartitionsListener::on_data_available failed to narrow RtpsRelay::RelayPartitionsDataReader\n")));
    return;
  }

  RelayPartitionsSeq datas;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(datas,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayPartitionsListener::on_data_available failed to take %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    const auto& data = datas[idx];
    const auto& info = infos[idx];

    switch (info.instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (info.valid_data) {
        relay_partition_table_.complete_insert(std::make_pair(guid_to_repoid(data.application_participant_guid()), data.slot()),
                                               data.partitions());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      relay_partition_table_.complete_insert(std::make_pair(guid_to_repoid(data.application_participant_guid()), data.slot()),
                                             RtpsRelay::StringSequence());
      break;
    }
  }
}

}
