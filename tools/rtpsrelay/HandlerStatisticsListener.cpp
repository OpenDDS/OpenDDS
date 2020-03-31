#include "HandlerStatisticsListener.h"

#include "utility.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/QosIndex.h"

#include <iostream>

namespace RtpsRelay {

void HandlerStatisticsListener::on_data_available(DDS::DataReader_ptr reader)
{
  HandlerStatisticsDataReader_var dr = HandlerStatisticsDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: HandlerStatisticsListener::on_data_available failed to narrow HandlerStatisticsDataReader\n")));
    return;
  }

  HandlerStatisticsSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ALIVE_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: HandlerStatisticsListener::on_data_available failed to read\n")));
    return;
  }

  ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
  ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      const HandlerStatistics& hs = data[idx];

      std::cout << timestamp << ' '
                << "source_timestamp=" << infos[idx].source_timestamp.sec << '.' << infos[idx].source_timestamp.nanosec << ' '
                << "application_participant_guid=" << guid_to_string(guid_to_repoid(hs.application_participant_guid())) << ' '
                << "name=\"" << hs.name() << "\" "
                << "interval=" << hs.interval().sec() << '.' << hs.interval().nanosec() << ' '
                << "messages_in=" << hs.messages_in() << ' '
                << "bytes_in=" << hs.bytes_in() << ' '
                << "messages_out=" << hs.messages_out() << ' '
                << "bytes_out=" << hs.bytes_out() << ' '
                << "max_man_out=" << hs.max_fan_out() << ' '
                << "max_queue_size=" << hs.max_queue_size() << ' '
                << "max_queue_latency=" << hs.max_queue_latency().sec() << '.' << hs.max_queue_latency().nanosec() << ' '
                << "local_active_participants=" << hs.local_active_participants()
                << std::endl;

      if (report_participant_statistics_) {
        for (const auto& ps : hs.participant_statistics()) {
          std::cout << timestamp << ' '
                    << "source_timestamp=" << infos[idx].source_timestamp.sec << '.' << infos[idx].source_timestamp.nanosec << ' '
                    << "application_participant_guid=" << guid_to_string(guid_to_repoid(hs.application_participant_guid())) << ' '
                    << "name=\"" << hs.name() << "\" "
                    << "interval=" << hs.interval().sec() << '.' << hs.interval().nanosec() << ' '
                    << "address=\"" << ps.address() << "\" "
                    << "messages_in=" << hs.messages_in() << ' '
                    << "bytes_in=" << ps.bytes_in() << ' '
                    << "messages_out=" << ps.messages_out() << ' '
                    << "bytes_out=" << ps.bytes_out() << ' '
                    << "max_man_out=" << ps.max_fan_out() << ' '
                    << std::endl;
        }
      }
    }
  }
}

}
