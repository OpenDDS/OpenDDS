#include "ParticipantEntryListener.h"

namespace RtpsRelay {

ParticipantEntryListener::ParticipantEntryListener(const Config& config,
                                                   DomainStatisticsReporter& stats_reporter)
  : config_(config)
  , participant_count_(0)
  , stats_reporter_(stats_reporter)
{}

void ParticipantEntryListener::on_data_available(DDS::DataReader_ptr reader)
{
  ParticipantEntryDataReader_var dr = ParticipantEntryDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantEntryListener::on_data_available failed to narrow RtpsRelay::ParticipantEntryDataReader\n")));
    return;
  }

  ParticipantEntrySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantEntryListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (config_.log_entries()) {
        const auto from = guid_to_repoid(data[idx].guid());
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantEntryListener::on_data_available add global writer %C\n"), guid_to_string(from).c_str()));
      }
      stats_reporter_.total_participants(++participant_count_, OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      if (config_.log_entries()) {
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantEntryListener::on_data_available remove global writer %C\n"), guid_to_string(guid_to_repoid(data[idx].guid())).c_str()));
      }
      stats_reporter_.total_participants(--participant_count_, OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    }
  }
}

}
