#ifndef RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
#define RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <iostream>

namespace RtpsRelay {

class HandlerStatisticsReporter {
public:
  HandlerStatisticsReporter(const Config& config,
                            const std::string& name,
                            HandlerStatisticsDataWriter_ptr writer)
    : config_(config)
    , last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    handler_statistics_.name(name);
  }

  void input_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._bytes_in += byte_count;
    ++handler_statistics_._messages_in;
    report(now);
  }

  void output_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._bytes_out += byte_count;
    ++handler_statistics_._messages_out;
    report(now);
  }

  void max_fan_out(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._max_fan_out = std::max(handler_statistics_._max_fan_out, static_cast<uint32_t>(value));
    report(now);
  }

  void max_queue_size(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._max_queue_size = std::max(handler_statistics_._max_queue_size, static_cast<uint32_t>(value));
    report(now);
  }

  void max_queue_latency(const OpenDDS::DCPS::TimeDuration& latency, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._max_queue_latency = std::max(handler_statistics_._max_queue_latency, time_diff_to_duration(latency));
    report(now);
  }


  void local_active_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_._local_active_participants = static_cast<uint32_t>(count);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++handler_statistics_._error_count;
    report(now);
  }

  void governor(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++handler_statistics_._governor_count;
    report(now);
  }

private:

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    const auto d = now - last_report_;
    if (d < config_.statistics_interval()) {
      return;
    }

    handler_statistics_.interval(time_diff_to_duration(d));

    if (config_.log_relay_statistics()) {
      ACE_TCHAR timestamp[OpenDDS::DCPS::AceTimestampSize];
      ACE::timestamp(timestamp, sizeof(timestamp) / sizeof(ACE_TCHAR));

      std::cout << timestamp << ' '
                << "application_participant_guid=" << guid_to_string(guid_to_repoid(handler_statistics_.application_participant_guid())) << ' '
                << "name=\"" << handler_statistics_.name() << "\" "
                << "interval=" << handler_statistics_.interval().sec() << '.' << handler_statistics_.interval().nanosec() << ' '
                << "messages_in=" << handler_statistics_.messages_in() << ' '
                << "bytes_in=" << handler_statistics_.bytes_in() << ' '
                << "messages_out=" << handler_statistics_.messages_out() << ' '
                << "bytes_out=" << handler_statistics_.bytes_out() << ' '
                << "max_fan_out=" << handler_statistics_.max_fan_out() << ' '
                << "max_queue_size=" << handler_statistics_.max_queue_size() << ' '
                << "max_queue_latency=" << handler_statistics_.max_queue_latency().sec() << '.' << handler_statistics_.max_queue_latency().nanosec() << ' '
                << "local_active_participants=" << handler_statistics_.local_active_participants() << ' '
                << "error_count=" << handler_statistics_.error_count() << ' '
                << "governor_count=" << handler_statistics_.governor_count()
                << std::endl;
    }

    if (config_.publish_relay_statistics()) {
      const auto ret = writer_->write(handler_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: HandlerStatisticsReporter::report %C failed to write handler statistics\n"), handler_statistics_.name().c_str()));
      }
    }

    last_report_ = now;

    handler_statistics_._messages_in = 0;
    handler_statistics_._bytes_in = 0;
    handler_statistics_._messages_out = 0;
    handler_statistics_._bytes_out = 0;
    handler_statistics_._max_fan_out = 0;
    handler_statistics_._max_queue_size = 0;
    handler_statistics_._max_queue_latency._sec = 0;
    handler_statistics_._max_queue_latency._nanosec = 0;
    // Don't reset local_active_participant_count.
    handler_statistics_._error_count = 0;
    handler_statistics_._governor_count = 0;
  }

  const Config& config_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  HandlerStatistics handler_statistics_;
  HandlerStatisticsDataWriter_ptr writer_;
};

}

#endif // RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
