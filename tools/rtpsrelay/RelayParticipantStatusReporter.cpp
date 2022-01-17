#include "RelayParticipantStatusReporter.h"

namespace RtpsRelay {

void RelayParticipantStatusReporter::add_participant(GuidAddrSet::Proxy& proxy,
                                                     const OpenDDS::DCPS::GUID_t& repoid,
                                                     const DDS::ParticipantBuiltinTopicData& data)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  RelayParticipantStatus status;
  status.relay_id(config_.relay_id());
  status.guid(rtps_guid_to_relay_guid(repoid));
  status.active(true);
  status.alive(true);
  status.user_data(data.user_data);

  if (writer_->write(status, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::add_participant failed to write participant status\n"));
  }

  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const auto p = guids_.insert(std::make_pair(repoid, status));

    if (p.second) {
      if (config_.log_discovery()) {
        ACE_DEBUG((LM_INFO, "(%P|%t) INFO: RelayParticipantStatusReporter::add_participant "
                   "add local participant %C %C %C into session\n",
                   guid_to_string(repoid).c_str(), OpenDDS::DCPS::to_json(data).c_str(),
                   proxy.get_session_time(repoid, now).sec_str().c_str()));
      }
    }

    stats_reporter_.local_participants(guids_.size(), now);
  }
}

void RelayParticipantStatusReporter::remove_participant(GuidAddrSet::Proxy& proxy,
                                                        const OpenDDS::DCPS::GUID_t& repoid)
{
  const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();

  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  const auto pos = guids_.find(repoid);
  if (pos == guids_.end()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::remove_participant participant %C not found\n",
               guid_to_string(repoid).c_str()));
    return;
  }

  if (writer_->unregister_instance(pos->second, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::remove_participant failed to unregister participant\n"));
  }

  if (config_.log_discovery()) {
    ACE_DEBUG((LM_INFO, "(%P|%t) INFO: RelayParticipantStatusReporter::remove_participant "
               "remove local participant %C %C into session\n",
               guid_to_string(repoid).c_str(),
               proxy.get_session_time(repoid, now).sec_str().c_str()));
  }

  proxy.remove(repoid, now, nullptr);

  guids_.erase(pos);

  stats_reporter_.local_participants(guids_.size(), now);
}

void RelayParticipantStatusReporter::set_alive(const GuidAddrSet::Proxy& /*proxy*/,
                                               const OpenDDS::DCPS::GUID_t& repoid,
                                               bool alive)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  const auto pos = guids_.find(repoid);
  if (pos == guids_.end()) {
    return;
  }

  if (pos->second.alive() == alive) {
    return;
  }

  pos->second.alive(alive);

  if (writer_->write(pos->second, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::set_alive failed to write participant status\n"));
  }
}

void RelayParticipantStatusReporter::set_active(const GuidAddrSet::Proxy& /*proxy*/,
                                                const OpenDDS::DCPS::GUID_t& repoid,
                                                bool active)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  const auto pos = guids_.find(repoid);
  if (pos == guids_.end()) {
    return;
  }

  if (pos->second.active() == active) {
    return;
  }

  pos->second.active(active);

  if (writer_->write(pos->second, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::set_active failed to write participant status\n"));
  }
}

void RelayParticipantStatusReporter::set_alive_active(const GuidAddrSet::Proxy& /*proxy*/,
                                                      const OpenDDS::DCPS::GUID_t& repoid,
                                                      bool alive,
                                                      bool active)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  const auto pos = guids_.find(repoid);
  if (pos == guids_.end()) {
    return;
  }

  if (pos->second.alive() == alive && pos->second.active() == active) {
    return;
  }

  pos->second.alive(alive);
  pos->second.active(active);

  if (writer_->write(pos->second, DDS::HANDLE_NIL) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
               "RelayParticipantStatusReporter::set_alive_active failed to write participant status\n"));
  }
}

}
