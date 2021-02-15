#ifndef RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
#define RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_

#include "Config.h"
#include "RelayStatisticsReporter.h"
#include "utility.h"

#include "lib/QosIndex.h"
#include "lib/RelayTypeSupportImpl.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class HandlerStatisticsReporter {
public:
  HandlerStatisticsReporter(const Config& config,
                            const std::string& name,
                            HandlerStatisticsDataWriter_var writer,
                            RelayStatisticsReporter& relay_statistics_reporter)
    : config_(config)
    , last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
    , relay_statistics_reporter_(relay_statistics_reporter)
  {
    DDS::Topic_var topic = writer->get_topic();
    topic_name_ = topic->get_name();
    handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    handler_statistics_.name(name);
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.input_message(byte_count, time, now);
    handler_statistics_.bytes_in() += byte_count;
    ++handler_statistics_.messages_in();
    input_processing_time_ += time;
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.output_message(byte_count, time, queue_latency, now);
    handler_statistics_.bytes_out() += byte_count;
    ++handler_statistics_.messages_out();
    output_processing_time_ += time;
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
    report(now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.dropped_message(byte_count, time, queue_latency, now);
    handler_statistics_.bytes_dropped() += byte_count;
    ++handler_statistics_.messages_dropped();
    output_processing_time_ += time;
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
    report(now);
  }

  void max_directed_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_directed_gain(value, now);
    handler_statistics_.max_directed_gain() = std::max(handler_statistics_.max_directed_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void max_undirected_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_undirected_gain(value, now);
    handler_statistics_.max_undirected_gain() = std::max(handler_statistics_.max_undirected_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void local_active_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    handler_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.error(now);
    ++handler_statistics_.error_count();
    report(now);
  }

  void new_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.new_address(now);
    ++handler_statistics_.new_address_count();
    report(now);
  }

  void expired_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.expired_address(now);
    ++handler_statistics_.expired_address_count();
    report(now);
  }

  void claim(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.claim(now);
    ++handler_statistics_.claim_count();
    report(now);
  }

  void disclaim(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.disclaim(now);
    ++handler_statistics_.disclaim_count();
    report(now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_queue_size(size, now);
    handler_statistics_.max_queue_size() = std::max(handler_statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
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
    handler_statistics_.input_processing_time(time_diff_to_duration(input_processing_time_));
    handler_statistics_.output_processing_time(time_diff_to_duration(output_processing_time_));
    handler_statistics_.max_queue_latency(time_diff_to_duration(max_queue_latency_));

    if (config_.log_relay_statistics()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(handler_statistics_).c_str()));
    }

    if (config_.publish_relay_statistics()) {
      const auto ret = writer_->write(handler_statistics_, DDS::HANDLE_NIL);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: HandlerStatisticsReporter::report %C failed to write handler statistics\n"), handler_statistics_.name().c_str()));
      }
    }

    last_report_ = now;

    handler_statistics_.messages_in(0);
    handler_statistics_.bytes_in(0);
    handler_statistics_.messages_out(0);
    handler_statistics_.bytes_out(0);
    handler_statistics_.messages_dropped(0);
    handler_statistics_.bytes_dropped(0);
    handler_statistics_.max_directed_gain(0);
    handler_statistics_.max_undirected_gain(0);
    // Don't reset local_active_participant_count.
    handler_statistics_.error_count(0);
    handler_statistics_.new_address_count(0);
    handler_statistics_.expired_address_count(0);
    handler_statistics_.claim_count(0);
    handler_statistics_.disclaim_count(0);
    handler_statistics_.max_queue_size(0);
    input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

  const Config& config_;
  OpenDDS::DCPS::MonotonicTimePoint last_report_;
  HandlerStatistics handler_statistics_;
  OpenDDS::DCPS::TimeDuration input_processing_time_;
  OpenDDS::DCPS::TimeDuration output_processing_time_;
  OpenDDS::DCPS::TimeDuration max_queue_latency_;
  HandlerStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
  RelayStatisticsReporter& relay_statistics_reporter_;
};

}

#endif // RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
