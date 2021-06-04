#include "SpdpReplayListener.h"

#include <dds/DCPS/DCPS_Utils.h>

namespace RtpsRelay {

SpdpReplayListener::SpdpReplayListener(SpdpHandler& spdp_handler)
  : spdp_handler_(spdp_handler)
{}

void SpdpReplayListener::on_data_available(DDS::DataReader_ptr reader)
{
  SpdpReplayDataReader_var dr = SpdpReplayDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpReplayListener::on_data_available failed to narrow RtpsRelay::SpdpReplayDataReader\n")));
    return;
  }

  SpdpReplaySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: SpdpReplayListener::on_data_available failed to take %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      const auto& d = data[idx];
      spdp_handler_.replay(d.partitions());
    }
  }
}

}
