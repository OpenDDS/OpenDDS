#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

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
    : last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
  {
    participant_statistics_.guid(guid);
    participant_statistics_.name(name);
  }

  void message_from(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.bytes_from() += byte_count;
    ++participant_statistics_.messages_from();
    report(now);
  }

  void message_to(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.bytes_to() += byte_count;
    ++participant_statistics_.messages_to();
    report(now);
  }

  void max_directed_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.max_directed_gain() = std::max(participant_statistics_.max_directed_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void max_undirected_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.max_undirected_gain() = std::max(participant_statistics_.max_undirected_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now,
              bool force = false)
  {
    const auto d = now - last_report_;
    if (!force && d < config->statistics_interval()) {
      return;
    }

    participant_statistics_.interval(time_diff_to_duration(d));

    if (config->log_relay_statistics()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name.in(), OpenDDS::DCPS::to_json(participant_statistics_).c_str()));
    }

    if (config->publish_relay_statistics()) {
      const auto ret = writer->write(participant_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: writing participant %C statistics\n"), guid_to_string(guid_to_repoid(participant_statistics_.guid())).c_str()));
      }
    }

    last_report_ = now;

    participant_statistics_.messages_from(0);
    participant_statistics_.bytes_from(0);
    participant_statistics_.messages_to(0);
    participant_statistics_.bytes_to(0);
    participant_statistics_.max_directed_gain(0);
    participant_statistics_.max_undirected_gain(0);
  }

private:
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  ParticipantStatistics participant_statistics_;
};

}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
