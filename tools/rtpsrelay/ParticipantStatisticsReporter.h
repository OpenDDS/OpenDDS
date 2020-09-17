#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "ParticipantStatisticsReporterBase.h"

#include "lib/RelayTypeSupportImpl.h"

#include <ace/Thread_Mutex.h>

#include <map>
#include <deque>

namespace RtpsRelay {

class ParticipantStatisticsReporter : public ParticipantStatisticsReporterBase {
public:

  explicit ParticipantStatisticsReporter(ParticipantStatisticsDataWriter_ptr writer,
                                         unsigned int min_interval_sec,
                                         unsigned int report_limit,
                                         bool log_stats);

  virtual ~ParticipantStatisticsReporter();

  void update_input_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count) override;

  void update_output_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count) override;

  void update_fan_out(const OpenDDS::DCPS::RepoId& participant, size_t count) override;

  void report(const OpenDDS::DCPS::MonotonicTimePoint& time_now) override;
  void reset_stats() override;

  void remove_participant(const OpenDDS::DCPS::RepoId& guid);

private:

  struct ParticipantStatsNode {
    ParticipantStatsNode()
      : stats()
      , last_time()
    {}

    ParticipantStatistics stats;
    OpenDDS::DCPS::MonotonicTimePoint last_time;
  };

  ParticipantStatistics& get_stats(const OpenDDS::DCPS::RepoId& participant);

  void report_stats(
    const std::string& partition_guid,
    ParticipantStatsNode& stats_node,
    const OpenDDS::DCPS::MonotonicTimePoint& time_now,
    ACE_TCHAR* timestamp);

  void reset_participant_stats(ParticipantStatistics& stats);

  // Queue used to throttle reporting of participant stats
  typedef std::deque<std::string> ParticpantReportQueue;
  ParticpantReportQueue report_queue_;
  unsigned int min_interval_sec_;
  unsigned int report_limit_;
  std::map<std::string, ParticipantStatsNode> participant_stats_;

  ParticipantStatisticsDataWriter_ptr writer_;

  ACE_Thread_Mutex stats_mutex_;

  bool log_stats_;
};

typedef OpenDDS::DCPS::RcHandle<ParticipantStatisticsReporter> ParticipantStatisticsReporter_rch;
}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
