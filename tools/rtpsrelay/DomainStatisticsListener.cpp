#include "DomainStatisticsListener.h"

#include "utility.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/QosIndex.h"

#include <iostream>

namespace RtpsRelay {

void DomainStatisticsListener::on_data_available(DDS::DataReader_ptr reader)
{
  DomainStatisticsDataReader_var dr = DomainStatisticsDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: DomainStatisticsListener::on_data_available failed to narrow DomainStatisticsDataReader\n")));
    return;
  }

  DomainStatisticsSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ALIVE_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: DomainStatisticsListener::on_data_available failed to read\n")));
    return;
  }

  ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
  ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    if (infos[idx].valid_data) {
      const DomainStatistics& hs = data[idx];

      std::cout << timestamp << ' '
                << "source_timestamp=" << infos[idx].source_timestamp.sec << '.' << infos[idx].source_timestamp.nanosec << ' '
                << "application_participant_guid=" << guid_to_string(guid_to_repoid(hs.application_participant_guid())) << ' '
                << "local_participants=" << hs.local_participants() << ' '
                << "local_writers=" << hs.local_writers() << ' '
                << "local_readers=" << hs.local_readers() << ' '
                << "total_writers=" << hs.total_writers() << ' '
                << "total_readers=" << hs.total_readers() << ' '
                << std::endl;
    }
  }
}

}
