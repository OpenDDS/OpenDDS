#include "ParticipantStatisticsReporter.h"

#include "lib/QosIndex.h"

#include <ace/Global_Macros.h>

#include <iostream>

namespace RtpsRelay {

ParticipantStatisticsReporter::ParticipantStatisticsReporter(ParticipantStatisticsDataWriter_ptr writer,
                                                             unsigned int min_interval_sec,
                                                             unsigned int report_limit,
                                                             bool log_stats)
  : min_interval_sec_(min_interval_sec)
  , report_limit_(report_limit)
  , writer_(writer)
  , log_stats_(log_stats)
{
}

ParticipantStatisticsReporter::~ParticipantStatisticsReporter()
{
}

void ParticipantStatisticsReporter::update_input_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count)
{
  ACE_GUARD(ACE_Thread_Mutex, g, stats_mutex_);

  ParticipantStatistics& stats = get_stats(participant);
  stats._bytes_in += byte_count;
  ++stats._messages_in;
}

void ParticipantStatisticsReporter::update_output_msgs(const OpenDDS::DCPS::RepoId& participant, size_t byte_count)
{
  ACE_GUARD(ACE_Thread_Mutex, g, stats_mutex_);

  ParticipantStatistics& stats = get_stats(participant);
  stats._bytes_out += byte_count;
  ++stats._messages_out;
}

void ParticipantStatisticsReporter::update_fan_out(const OpenDDS::DCPS::RepoId& participant, size_t count)
{
  ACE_GUARD(ACE_Thread_Mutex, g, stats_mutex_);

  ParticipantStatistics& stats = get_stats(participant);
  stats._max_fan_out = std::max(stats._max_fan_out, static_cast<uint32_t>(count));
}

void ParticipantStatisticsReporter::report(const OpenDDS::DCPS::MonotonicTimePoint& time_now)
{
  // This is temporary - just to get the concept working
  ACE_GUARD(ACE_Thread_Mutex, g, stats_mutex_);

  ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
  ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

  // Loop through the queue until it is empty or until the next participant
  // is not ready to be reported yet.
  unsigned int num_reports = 0;
  while (num_reports < report_limit_ && !report_queue_.empty()) {
    // If the participant no longer exists, then just skip this one
    // and continue on without counting it as a report sent
    const auto& next = report_queue_.front();
    auto iPart = participant_stats_.find(next);
    if (iPart == participant_stats_.end()) {
      // This participant doesn't exist anymore
      report_queue_.pop_front();
    } else {
      const auto report_interval = time_now - iPart->second.last_time;
      if (report_interval.value().sec() < min_interval_sec_) {
        break;
      } else {
        // Send stats and move to the back of the queue
        report_stats(iPart->first, iPart->second, time_now, timestamp);
        report_queue_.pop_front();
        report_queue_.push_back(next);
        ++num_reports;
      }
    }
  }
}

void ParticipantStatisticsReporter::report_stats(
  const std::string& partition_guid,
  ParticipantStatsNode& stats_node,
  const OpenDDS::DCPS::MonotonicTimePoint& time_now,
  ACE_TCHAR* timestamp)
{
  auto& stats = stats_node.stats;

  // No interval on first report
  if (!stats_node.last_time.is_zero()) {
    const auto time_diff = time_now - stats_node.last_time;
    const auto new_duration = time_diff.to_dds_duration();
    stats._interval._sec = new_duration.sec;
    stats._interval._nanosec = new_duration.nanosec;
  }
  stats_node.last_time = time_now;

  if (writer_) {
    const auto ret = writer_->write(stats, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: writing participant %C statistics\n"), partition_guid.c_str()));
    }
  }

  // Update the timing data for this participant
  if (log_stats_) {
    std::cout << timestamp << ' '
              << "participant_guid=" << partition_guid << ' '
              << "interval=" << stats.interval().sec() << '.' << stats.interval().nanosec() << ' '
              << "messages_in=" << stats.messages_in() << ' '
              << "bytes_in=" << stats.bytes_in() << ' '
              << "messages_out=" << stats.messages_out() << ' '
              << "bytes_out=" << stats.bytes_out() << ' '
              << "max_fan_out=" << stats.max_fan_out() << ' '
              << std::endl;
  }

  // Reset the stats
  reset_participant_stats(stats);
}

void ParticipantStatisticsReporter::reset_stats()
{
  // Done in report
}

ParticipantStatistics& ParticipantStatisticsReporter::get_stats(const OpenDDS::DCPS::RepoId& participant)
{
  // This function should always be called with the mutex already held
  std::string id_str = guid_to_string(participant);
  auto iPart = participant_stats_.find(id_str);
  if (iPart != participant_stats_.end()) {
    return iPart->second.stats;
  }

  // Must be a new participant
  ParticipantStatsNode new_stats;
  reset_participant_stats(new_stats.stats);
  participant_stats_[id_str] = new_stats;
  report_queue_.push_back(id_str);
  return participant_stats_[id_str].stats;
}

void ParticipantStatisticsReporter::remove_participant(const OpenDDS::DCPS::RepoId& guid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, stats_mutex_);

  std::string id_str = guid_to_string(guid);
  participant_stats_.find(id_str);
}

void ParticipantStatisticsReporter::reset_participant_stats(ParticipantStatistics& stats)
{
  stats._bytes_in = 0;
  stats._messages_in = 0;
  stats._bytes_out = 0;
  stats._messages_out = 0;
  stats._max_fan_out = 0;
  stats._interval.sec(0);
  stats._interval.nanosec(0);
}


} // namespace RtpsRelay


