#include "ParticipantListener.h"

#include "utility.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

ParticipantListener::ParticipantListener(const Config& config,
                                         OpenDDS::DCPS::DomainParticipantImpl* participant,
                                         DomainStatisticsReporter& stats_reporter,
                                         ParticipantEntryDataWriter_var participant_writer)
  : config_(config)
  , participant_(participant)
  , stats_reporter_(stats_reporter)
  , writer_(participant_writer)
  , unregister_(OpenDDS::DCPS::make_rch<Unregister>(OpenDDS::DCPS::ref(*this)))
{}

void ParticipantListener::enable()
{
  unregister_->enable();
}

void ParticipantListener::disable()
{
  unregister_->disable();
}

void ParticipantListener::on_data_available(DDS::DataReader_ptr reader)
{
  DDS::ParticipantBuiltinTopicDataDataReader_var dr = DDS::ParticipantBuiltinTopicDataDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::on_data_available failed to narrow PublicationBuiltinTopicDataDataReader\n")));
    return;
  }

  DDS::ParticipantBuiltinTopicDataSeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->take(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::on_data_available failed to read\n")));
    return;
  }

  for (CORBA::ULong idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      write_sample(data[idx], infos[idx]);
      stats_reporter_.add_local_participant(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      unregister_instance(infos[idx]);
      stats_reporter_.remove_local_participant(OpenDDS::DCPS::MonotonicTimePoint::now());
      break;
    }
  }
}

void ParticipantListener::write_sample(const DDS::ParticipantBuiltinTopicData& data,
                                       const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  GUID_t guid;
  assign(guid, repoid);

  const ParticipantEntry entry(guid, data.user_data);

  if (config_.log_discovery()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantListener::write_sample add local participant %C %C\n"), guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str()));
  }
  DDS::ReturnCode_t ret = writer_->write(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::write_sample failed to write\n")));
  }
}

void ParticipantListener::unregister_instance(const DDS::SampleInfo& info)
{
  const auto repoid = participant_->get_repoid(info.instance_handle);
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  unregister_queue_.push_back(repoid);
}

void ParticipantListener::unregister()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (unregister_queue_.empty()) {
    return;
  }

  const auto repoid = unregister_queue_.front();
  unregister_queue_.pop_front();
  GUID_t guid;
  assign(guid, repoid);

  ParticipantEntry entry;
  entry.guid(guid);

  if (config_.log_discovery()) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) INFO: ParticipantListener::unregister remove local participant %C\n"), guid_to_string(repoid).c_str()));
  }
  DDS::ReturnCode_t ret = writer_->unregister_instance(entry, DDS::HANDLE_NIL);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ParticipantListener::unregister failed to unregister_instance\n")));
  }
}

ParticipantListener::Unregister::Unregister(ParticipantListener& listener)
  : listener_(listener)
  , unregister_task_(TheServiceParticipant->interceptor(), *this, &ParticipantListener::Unregister::execute)
{}

void ParticipantListener::Unregister::enable()
{
  unregister_task_.enable(false, OpenDDS::DCPS::TimeDuration(1));
}

void ParticipantListener::Unregister::disable()
{
  unregister_task_.disable_and_wait();
}

void ParticipantListener::Unregister::execute(const OpenDDS::DCPS::MonotonicTimePoint&)
{
  listener_.unregister();
}

}
