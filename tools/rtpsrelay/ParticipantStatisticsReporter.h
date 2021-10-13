#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "Config.h"
#include "CommonIoStatsReportHelper.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class ParticipantStatisticsReporter {
public:
  using Helper = ProtocolStatisticsReportHelper<ParticipantStatistics>;

  static const Config* config;
  static ParticipantStatisticsDataWriter_var writer;
  static CORBA::String_var topic_name;

  ParticipantStatisticsReporter(const OpenDDS::DCPS::MonotonicTimePoint& session_start)
    : session_start_(session_start)
    , log_helper_(log_participant_statistics_)
    , publish_helper_(publish_participant_statistics_)
  {
  }

  ParticipantStatisticsReporter(const GUID_t& guid,
                                const std::string& name,
                                const OpenDDS::DCPS::MonotonicTimePoint& session_start)
    : session_start_(session_start)
    , log_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , log_helper_(log_participant_statistics_)
    , publish_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , publish_helper_(publish_participant_statistics_)
  {
    log_participant_statistics_.guid(guid);
    log_participant_statistics_.name(name);
    publish_participant_statistics_.guid(guid);
    publish_participant_statistics_.name(name);
  }

  ParticipantStatisticsReporter& operator=(const ParticipantStatisticsReporter& other)
  {
    session_start_ = other.session_start_;
    log_last_report_ = other.log_last_report_;
    log_participant_statistics_ = other.log_participant_statistics_;
    publish_last_report_ = other.publish_last_report_;
    publish_participant_statistics_ = other.publish_participant_statistics_;
    return *this;
  }

  void input_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now,
    MessageType type)
  {
    const auto& time = OpenDDS::DCPS::TimeDuration::zero_value;
    log_helper_.input_message(byte_count, time, type);
    publish_helper_.input_message(byte_count, time, type);
    report(now);
  }

  void output_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now,
    MessageType type)
  {
    const auto& time = OpenDDS::DCPS::TimeDuration::zero_value;
    log_helper_.output_message(byte_count, time, type);
    publish_helper_.output_message(byte_count, time, type);
    report(now);
  }

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now,
              bool force = false)
  {
    log_report(now, force);
    publish_report(now, force);
  }

  void log_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                  bool force)
  {
    if (!(config->log_discovery() || force)) {
      return;
    }

    log_helper_.prepare_report();
    log_participant_statistics_.interval(time_diff_to_duration(now - log_last_report_));
    log_participant_statistics_.session_time(time_diff_to_duration(now - session_start_));

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name.in(), OpenDDS::DCPS::to_json(log_participant_statistics_).c_str()));

    log_last_report_ = now;
    log_helper_.reset();
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool end_session = false, bool force = false)
  {
    if (!((config->publish_participant_statistics() && end_session) || force)) {
      return;
    }

    publish_helper_.prepare_report();
    publish_participant_statistics_.interval(time_diff_to_duration(now - publish_last_report_));
    publish_participant_statistics_.session_time(time_diff_to_duration(now - session_start_));

    const auto ret = writer->write(publish_participant_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: writing participant %C statistics\n"), guid_to_string(relay_guid_to_rtps_guid(publish_participant_statistics_.guid())).c_str()));
    }

    publish_last_report_ = now;
    publish_helper_.reset();
  }

private:
  OpenDDS::DCPS::MonotonicTimePoint session_start_;

  OpenDDS::DCPS::MonotonicTimePoint log_last_report_;
  ParticipantStatistics log_participant_statistics_;
  Helper log_helper_;

  OpenDDS::DCPS::MonotonicTimePoint publish_last_report_;
  ParticipantStatistics publish_participant_statistics_;
  Helper publish_helper_;
};

}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
