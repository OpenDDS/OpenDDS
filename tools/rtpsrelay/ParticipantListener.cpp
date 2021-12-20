#include "ParticipantListener.h"

namespace RtpsRelay {

ParticipantListener::ParticipantListener(OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         GuidAddrSet& guid_addr_set,
                                         RelayParticipantStatusReporter& participant_status_reporter)
  : participant_(participant)
  , guid_addr_set_(guid_addr_set)
  , participant_status_reporter_(participant_status_reporter)
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
        GuidAddrSet::Proxy proxy(guid_addr_set_);
        participant_status_reporter_.add_participant(proxy, participant_->get_repoid(info.instance_handle), data);
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      {
        GuidAddrSet::Proxy proxy(guid_addr_set_);
        participant_status_reporter_.remove_participant(proxy, participant_->get_repoid(info.instance_handle));
      }
      break;
    }
  }
}

}
