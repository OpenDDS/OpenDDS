#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <iostream>

namespace RtpsRelay {

class ParticipantStatisticsReporter {
public:
  static const Config* config;
  static ParticipantStatisticsDataWriter_var writer;

  ParticipantStatisticsReporter() {}

  ParticipantStatisticsReporter(const GUID_t& guid,
                                const std::string& name)
    : last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
  {
    participant_statistics_.guid(guid);
    participant_statistics_.name(name);
  }

  void input_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.bytes_in() += byte_count;
    ++participant_statistics_.messages_in();
    report(now);
  }

  void output_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.bytes_out() += byte_count;
    ++participant_statistics_.messages_out();
    report(now);
  }

  void max_fan_out(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    participant_statistics_.max_fan_out() = std::max(participant_statistics_.max_fan_out(), static_cast<uint32_t>(value));
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
      ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
      ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

      std::cout << timestamp << ' '
                << "participant_guid=" << guid_to_string(guid_to_repoid(participant_statistics_.guid())) << ' '
                << "interval=" << participant_statistics_.interval().sec() << '.' << participant_statistics_.interval().nanosec() << ' '
                << "messages_in=" << participant_statistics_.messages_in() << ' '
                << "bytes_in=" << participant_statistics_.bytes_in() << ' '
                << "messages_out=" << participant_statistics_.messages_out() << ' '
                << "bytes_out=" << participant_statistics_.bytes_out() << ' '
                << "max_fan_out=" << participant_statistics_.max_fan_out() << ' '
                << std::endl;
    }

    if (config->publish_relay_statistics()) {
      const auto ret = writer->write(participant_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: writing participant %C statistics\n"), guid_to_string(guid_to_repoid(participant_statistics_.guid())).c_str()));
      }
    }

    last_report_ = now;

    participant_statistics_.messages_in(0);
    participant_statistics_.bytes_in(0);
    participant_statistics_.messages_out(0);
    participant_statistics_.bytes_out(0);
    participant_statistics_.max_fan_out(0);
  }

private:
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  ParticipantStatistics participant_statistics_;
};

}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
