#include "ParticipantListener.h"

#include "lib/Utility.h"

#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

namespace RtpsRelay {

ParticipantListener::ParticipantListener(const Config& config,
                                         GuidAddrSet& guid_addr_set,
                                         OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         DomainStatisticsReporter& stats_reporter)
  : config_(config)
  , guid_addr_set_(guid_addr_set)
  , participant_(participant)
  , stats_reporter_(stats_reporter)
{}

void ParticipantListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::ParticipantBuiltinTopicDataDataReader_var dr = DDS::ParticipantBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
    return;
  }

  DDS::ParticipantBuiltinTopicDataSeq datas;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(datas,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::on_data_available failed to take %C\n"), OpenDDS::DCPS::retcode_to_string(ret)));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    const auto& data = datas[idx];
    const auto& info = infos[idx];

    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (info.valid_data) {
        const auto repoid = participant_->get_repoid(info.instance_handle);

        guid_addr_set_.remove_pending(repoid);

        if (config_.log_discovery()) {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantListener::on_data_available add local participant %C %C\n"), guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str()));
        }

        stats_reporter_.add_local_participant(OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        const auto repoid = participant_->get_repoid(info.instance_handle);

        guid_addr_set_.remove(repoid);

        if (config_.log_discovery()) {
          ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantListener::on_data_available remove local participant %C\n"), guid_to_string(repoid).c_str()));
        }

        stats_reporter_.remove_local_participant(OpenDDS::DCPS::MonotonicTimePoint::now());
      }
      break;
    }
  }
}

}
