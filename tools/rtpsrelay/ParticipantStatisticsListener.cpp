#include "ParticipantStatisticsListener.h"

#include "utility.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/QosIndex.h"

#include <iostream>

namespace RtpsRelay {

void ParticipantStatisticsListener::on_data_available(DDS::DataReader_ptr reader)
{
  ParticipantStatisticsDataReader_var dr = ParticipantStatisticsDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ParticipantStatisticsListener::on_data_available failed to narrow ParticipantStatisticsDataReader\n")));
    return;
  }

  ParticipantStatisticsSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ALIVE_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ParticipantStatisticsListener::on_data_available failed to read\n")));
    return;
  }

  ACE_TCHAR timestamp[AceTimestampSize];
  ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      const ParticipantStatistics& hs = data[idx];

      std::cout << timestamp << ' '
                << "source_timestamp=" << infos[idx].source_timestamp.sec << '.' << infos[idx].source_timestamp.nanosec << ' '
                << "application_participant_guid=" << guid_to_string(guid_to_repoid(hs.application_participant_guid())) << ' '
                << "name=\"" << hs.name() << "\" "
                << "address=\"" << hs.address() << "\" "
                << "interval=" << hs.interval().sec() << '.' << hs.interval().nanosec() << ' '
                << "messages_in=" << hs.messages_in() << ' '
                << "bytes_in=" << hs.bytes_in() << ' '
                << "messages_out=" << hs.messages_out() << ' '
                << "bytes_out=" << hs.bytes_out() << ' '
                << "max_man_out=" << hs.max_fan_out() << ' '
                << std::endl;
    }
  }
}

}
