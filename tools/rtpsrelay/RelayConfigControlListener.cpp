#include "RelayConfigControlListener.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/RelayC.h>

#include <ace/Log_Msg.h>

namespace RtpsRelay {

void RelayConfigControlListener::on_data_available(DDS::DataReader_ptr reader)
{
  RelayConfigDataReader_var control_reader = RelayConfigDataReader::_narrow(reader);
  if (!control_reader) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RelayConfigControlListener::on_data_available: "
                         "Failed to narrow RelayConfigDataReader\n"));
    return;
  }

  RelayConfig control;
  DDS::SampleInfo info;
  while (control_reader->take_next_sample(control, info) == DDS::RETCODE_OK) {
    if (info.valid_data && info.instance_state == DDS::ALIVE_INSTANCE_STATE) {
      for (const auto& p : control.config()) {
        TheServiceParticipant->config_store()->set(p.first, p.second);
      }
    }
  }
}

}
