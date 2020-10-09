#ifndef RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
#define RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_

#include "Config.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class HandlerStatisticsReporter {
public:
  HandlerStatisticsReporter(const Config& config,
                            const std::string& name,
                            HandlerStatisticsDataWriter_var writer)
    : config_(config)
    , last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    handler_statistics_.name(name);
  }

  void input_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.bytes_in() += byte_count;
    ++handler_statistics_.messages_in();
    report(now);
  }

  void output_message(size_t byte_count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.bytes_out() += byte_count;
    ++handler_statistics_.messages_out();
    report(now);
  }

  void max_fan_out(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.max_fan_out() = std::max(handler_statistics_.max_fan_out(), static_cast<uint32_t>(value));
    report(now);
  }

  void max_queue_size(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.max_queue_size() = std::max(handler_statistics_.max_queue_size(), static_cast<uint32_t>(value));
    report(now);
  }

  void max_queue_latency(const OpenDDS::DCPS::TimeDuration& latency, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.max_queue_latency() = std::max(handler_statistics_.max_queue_latency(), time_diff_to_duration(latency));
    report(now);
  }


  void local_active_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++handler_statistics_.error_count();
    report(now);
  }

  void governor(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++handler_statistics_.governor_count();
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
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) %N:%l INFO: HandlerStatisticsReporter::report %C\n"), OpenDDS::DCPS::to_json(handler_statistics_).c_str()));
    }

    if (config_.publish_relay_statistics()) {
      const auto ret = writer_->write(handler_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %N:%l ERROR: HandlerStatisticsReporter::report %C failed to write handler statistics\n"), handler_statistics_.name().c_str()));
      }
    }

    last_report_ = now;

    handler_statistics_.messages_in(0);
    handler_statistics_.bytes_in(0);
    handler_statistics_.messages_out(0);
    handler_statistics_.bytes_out(0);
    handler_statistics_.max_fan_out(0);
    handler_statistics_.max_queue_size(0);
    handler_statistics_.max_queue_latency().sec(0);
    handler_statistics_.max_queue_latency().nanosec(0);
    // Don't reset local_active_participant_count.
    handler_statistics_.error_count(0);
    handler_statistics_.governor_count(0);
  }

  const Config& config_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  HandlerStatistics handler_statistics_;
  HandlerStatisticsDataWriter_var writer_;
};

}

#endif // RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
