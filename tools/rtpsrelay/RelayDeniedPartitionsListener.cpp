#include "RelayDeniedPartitionsListener.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/RelayC.h>

#include <ace/Log_Msg.h>

namespace RtpsRelay {

void RelayDeniedPartitionsListener::on_data_available(DDS::DataReader_ptr reader)
{
  RelayDeniedPartitionsDataReader_var denied_partitions_reader = RelayDeniedPartitionsDataReader::_narrow(reader);
  if (!denied_partitions_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayDeniedPartitionsListener::on_data_available: "
                         "Failed to narrow RelayDeniedPartitionsDataReader\n"));
    return;
  }

  RelayDeniedPartitions denied_partitions;
  DDS::SampleInfo info;
  while (denied_partitions_reader->take_next_sample(denied_partitions, info) == DDS::RETCODE_OK) {
    if (info.valid_data) {
      // TODO(sonndinh): Drain the denied partitions and set time out.
    }
  }
}

}
