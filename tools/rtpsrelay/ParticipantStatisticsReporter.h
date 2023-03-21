#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_

#include "Config.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

// after RelayTypeSupportImpl.h so that set_default is available
#include "CommonIoStatsReportHelper.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class ParticipantStatisticsReporter {
public:
  using Helper = ProtocolStatisticsReportHelper<ParticipantStatistics>;

  static const Config* config;
  static ParticipantStatisticsDataWriter_var writer;
  static CORBA::String_var topic_name;

  ParticipantStatisticsReporter(const GUID_t& guid,
                                const std::string& name)
  {
    participant_statistics_.guid(guid);
    participant_statistics_.name(name);
  }

  void input_message(size_t byte_count, MessageType type)
  {
    const auto& time = OpenDDS::DCPS::TimeDuration::zero_value;
    helper_.input_message(participant_statistics_, byte_count, time, type);
  }

  void output_message(size_t byte_count, MessageType type)
  {
    const auto& time = OpenDDS::DCPS::TimeDuration::zero_value;
    helper_.output_message(participant_statistics_, byte_count, time, type);
  }

  void report(const OpenDDS::DCPS::MonotonicTimePoint& session_start,
              const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    helper_.prepare_report(participant_statistics_);
    participant_statistics_.session_time(time_diff_to_duration(now - session_start));

    if (config->log_participant_statistics()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name.in(), OpenDDS::DCPS::to_json(participant_statistics_).c_str()));
    }

    if (config->publish_participant_statistics()) {
      const auto ret = writer->write(participant_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: writing participant %C statistics\n"), guid_to_string(relay_guid_to_rtps_guid(participant_statistics_.guid())).c_str()));
      }
    }
  }

  void unregister()
  {
    if (config->publish_participant_statistics()) {
      const auto ret = writer->unregister_instance(participant_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: unregistering participant %C statistics\n"), guid_to_string(relay_guid_to_rtps_guid(participant_statistics_.guid())).c_str()));
      }
    }
  }

private:
  ParticipantStatistics participant_statistics_;
  Helper helper_;
};

}

#endif // RTPSRELAY_PARTICIPANT_STATISTICS_REPORTER_H_
