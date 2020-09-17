#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_BASE_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_BASE_H_

#include "StatisticsReporter.h"
#include "utility.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/TimeTypes.h"


namespace RtpsRelay {

class ParticipantStatisticsReporterBase : public StatisticsReporter {
public:

  explicit ParticipantStatisticsReporterBase();

  virtual ~ParticipantStatisticsReporterBase();

  virtual void update_input_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count);

  virtual void update_output_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count);

  virtual void update_fan_out(const OpenDDS::DCPS::RepoId& participant, size_t count);

  virtual void report(const OpenDDS::DCPS::MonotonicTimePoint& time_now);
  virtual void reset_stats();

  virtual void remove_participant(const OpenDDS::DCPS::RepoId& guid);
};

typedef OpenDDS::DCPS::RcHandle<ParticipantStatisticsReporterBase> ParticipantStatisticsReporterBase_rch;
}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_BASE_H_
