#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "Config.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class ParticipantStatisticsReporter {
public:
  static const Config* config;
  static ParticipantStatisticsDataWriter_var writer;
  static CORBA::String_var topic_name;

  ParticipantStatisticsReporter() {}

  ParticipantStatisticsReporter(const GUID_t& guid,
                                const std::string& name)
    : log_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , publish_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
  {
    log_participant_statistics_.guid(guid);
    log_participant_statistics_.name(name);
    publish_participant_statistics_.guid(guid);
    publish_participant_statistics_.name(name);
  }

  void message_from(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_participant_statistics_.bytes_from() += byte_count;
    ++log_participant_statistics_.messages_from();
    publish_participant_statistics_.bytes_from() += byte_count;
    ++publish_participant_statistics_.messages_from();
    report(now);
  }

  void message_to(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_participant_statistics_.bytes_to() += byte_count;
    ++log_participant_statistics_.messages_to();
    publish_participant_statistics_.bytes_to() += byte_count;
    ++publish_participant_statistics_.messages_to();
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
    if (config->log_participant_statistics().is_zero()) {
      return;
    }

    const auto d = now - log_last_report_;
    if (!force && d < config->log_participant_statistics()) {
      return;
    }

    log_participant_statistics_.interval(time_diff_to_duration(d));

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name.in(), OpenDDS::DCPS::to_json(log_participant_statistics_).c_str()));

    log_last_report_ = now;

    log_participant_statistics_.messages_from(0);
    log_participant_statistics_.bytes_from(0);
    log_participant_statistics_.messages_to(0);
    log_participant_statistics_.bytes_to(0);
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (config->publish_participant_statistics().is_zero()) {
      return;
    }

    const auto d = now - publish_last_report_;
    if (!force && d < config->publish_participant_statistics()) {
      return;
    }

    publish_participant_statistics_.interval(time_diff_to_duration(d));

    const auto ret = writer->write(publish_participant_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: writing participant %C statistics\n"), guid_to_string(guid_to_repoid(publish_participant_statistics_.guid())).c_str()));
    }

    publish_last_report_ = now;

    publish_participant_statistics_.messages_from(0);
    publish_participant_statistics_.bytes_from(0);
    publish_participant_statistics_.messages_to(0);
    publish_participant_statistics_.bytes_to(0);
  }

private:
  OpenDDS::DCPS::MonotonicTimePoint log_last_report_;
  ParticipantStatistics log_participant_statistics_;

  OpenDDS::DCPS::MonotonicTimePoint publish_last_report_;
  ParticipantStatistics publish_participant_statistics_;
};

}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
