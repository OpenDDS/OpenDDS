#include "ParticipantListener.h"

#include "utility.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>

namespace RtpsRelay {

ParticipantListener::ParticipantListener(DomainStatisticsWriter& stats_writer)
  : stats_writer_(stats_writer)
{}

void ParticipantListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::ParticipantBuiltinTopicDataDataReader_var dr = DDS::ParticipantBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ParticipantListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
    return;
  }

  DDS::ParticipantBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: ParticipantListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      stats_writer_.add_local_participant();
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      stats_writer_.remove_local_participant();
      break;
    }
  }
}

}
