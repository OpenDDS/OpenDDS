#include "ParticipantStatisticsReporterBase.h"


namespace RtpsRelay {

ParticipantStatisticsReporterBase::ParticipantStatisticsReporterBase()
{
}

ParticipantStatisticsReporterBase::~ParticipantStatisticsReporterBase()
{
}

void ParticipantStatisticsReporterBase::update_input_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count)
{
  ACE_UNUSED_ARG(participant);
  ACE_UNUSED_ARG(byte_count);
}

void ParticipantStatisticsReporterBase::update_output_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count)
{
  ACE_UNUSED_ARG(participant);
  ACE_UNUSED_ARG(byte_count);
}

void ParticipantStatisticsReporterBase::update_fan_out(const OpenDDS::DCPS::RepoId& participant, size_t count)
{
  ACE_UNUSED_ARG(participant);
  ACE_UNUSED_ARG(count);
}

void ParticipantStatisticsReporterBase::report(const OpenDDS::DCPS::MonotonicTimePoint& time_now)
{
  ACE_UNUSED_ARG(time_now);
}

void ParticipantStatisticsReporterBase::reset_stats()
{
}

void ParticipantStatisticsReporterBase::remove_participant(const OpenDDS::DCPS::RepoId& guid)
{
  ACE_UNUSED_ARG(guid);
}

} // namespace RtpsRelay


