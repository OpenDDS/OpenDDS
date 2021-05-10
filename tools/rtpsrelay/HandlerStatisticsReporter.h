#ifndef RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
#define RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_

#include "Config.h"
#include "RelayStatisticsReporter.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class HandlerStatisticsReporter {
public:
  HandlerStatisticsReporter(const Config& config,
                            const std::string& name,
                            HandlerStatisticsDataWriter_var writer,
                            RelayStatisticsReporter& relay_statistics_reporter)
    : config_(config)
    , log_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , publish_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
    , relay_statistics_reporter_(relay_statistics_reporter)
  {
    DDS::Topic_var topic = writer->get_topic();
    topic_name_ = topic->get_name();
    log_handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    log_handler_statistics_.name(name);
    publish_handler_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    publish_handler_statistics_.name(name);
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.input_message(byte_count, time, now);
    log_handler_statistics_.bytes_in() += byte_count;
    ++log_handler_statistics_.messages_in();
    log_input_processing_time_ += time;
    publish_handler_statistics_.bytes_in() += byte_count;
    ++publish_handler_statistics_.messages_in();
    publish_input_processing_time_ += time;
    report(now);
  }

  void ignored_message(size_t byte_count,
                       const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.ignored_message(byte_count, now);
    log_handler_statistics_.bytes_ignored() += byte_count;
    ++log_handler_statistics_.messages_ignored();
    publish_handler_statistics_.bytes_ignored() += byte_count;
    ++publish_handler_statistics_.messages_ignored();
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.output_message(byte_count, time, queue_latency, now);
    log_handler_statistics_.bytes_out() += byte_count;
    ++log_handler_statistics_.messages_out();
    log_output_processing_time_ += time;
    log_max_queue_latency_ = std::max(log_max_queue_latency_, queue_latency);
    publish_handler_statistics_.bytes_out() += byte_count;
    ++publish_handler_statistics_.messages_out();
    publish_output_processing_time_ += time;
    publish_max_queue_latency_ = std::max(publish_max_queue_latency_, queue_latency);
    report(now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.dropped_message(byte_count, time, queue_latency, now);
    log_handler_statistics_.bytes_dropped() += byte_count;
    ++log_handler_statistics_.messages_dropped();
    log_output_processing_time_ += time;
    log_max_queue_latency_ = std::max(log_max_queue_latency_, queue_latency);
    publish_handler_statistics_.bytes_dropped() += byte_count;
    ++publish_handler_statistics_.messages_dropped();
    publish_output_processing_time_ += time;
    publish_max_queue_latency_ = std::max(publish_max_queue_latency_, queue_latency);
    report(now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_gain(value, now);
    log_handler_statistics_.max_gain() = std::max(log_handler_statistics_.max_gain(), static_cast<uint32_t>(value));
    publish_handler_statistics_.max_gain() = std::max(publish_handler_statistics_.max_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.error(now);
    ++log_handler_statistics_.error_count();
    ++publish_handler_statistics_.error_count();
    report(now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_queue_size(size, now);
    log_handler_statistics_.max_queue_size() = std::max(log_handler_statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
    publish_handler_statistics_.max_queue_size() = std::max(publish_handler_statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
    report(now);
  }

  void report()
  {
    report(OpenDDS::DCPS::MonotonicTimePoint::now(), true);
  }

private:

  void report(const OpenDDS::DCPS::MonotonicTimePoint& now,
              bool force = false)
  {
    log_report(now, force);
    publish_report(now, force);
  }

  void log_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                  bool force)
  {
    if (config_.log_handler_statistics().is_zero()) {
      return;
    }

    const auto d = now - log_last_report_;
    if (!force && d < config_.log_handler_statistics()) {
      return;
    }

    log_handler_statistics_.interval(time_diff_to_duration(d));
    log_handler_statistics_.input_processing_time(time_diff_to_duration(log_input_processing_time_));
    log_handler_statistics_.output_processing_time(time_diff_to_duration(log_output_processing_time_));
    log_handler_statistics_.max_queue_latency(time_diff_to_duration(log_max_queue_latency_));

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_handler_statistics_).c_str()));

    log_last_report_ = now;

    log_handler_statistics_.messages_in(0);
    log_handler_statistics_.bytes_in(0);
    log_handler_statistics_.messages_ignored(0);
    log_handler_statistics_.bytes_ignored(0);
    log_handler_statistics_.messages_out(0);
    log_handler_statistics_.bytes_out(0);
    log_handler_statistics_.messages_dropped(0);
    log_handler_statistics_.bytes_dropped(0);
    log_handler_statistics_.max_gain(0);
    log_handler_statistics_.error_count(0);
    log_handler_statistics_.max_queue_size(0);
    log_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    log_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    log_max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (config_.publish_handler_statistics().is_zero()) {
      return;
    }

    const auto d = now - publish_last_report_;
    if (!force && d < config_.publish_handler_statistics()) {
      return;
    }

    publish_handler_statistics_.interval(time_diff_to_duration(d));
    publish_handler_statistics_.input_processing_time(time_diff_to_duration(publish_input_processing_time_));
    publish_handler_statistics_.output_processing_time(time_diff_to_duration(publish_output_processing_time_));
    publish_handler_statistics_.max_queue_latency(time_diff_to_duration(publish_max_queue_latency_));

    const auto ret = writer_->write(publish_handler_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: HandlerStatisticsReporter::report %C failed to write handler statistics\n"), publish_handler_statistics_.name().c_str()));
    }

    publish_last_report_ = now;

    publish_handler_statistics_.messages_in(0);
    publish_handler_statistics_.bytes_in(0);
    publish_handler_statistics_.messages_ignored(0);
    publish_handler_statistics_.bytes_ignored(0);
    publish_handler_statistics_.messages_out(0);
    publish_handler_statistics_.bytes_out(0);
    publish_handler_statistics_.messages_dropped(0);
    publish_handler_statistics_.bytes_dropped(0);
    publish_handler_statistics_.max_gain(0);
    publish_handler_statistics_.error_count(0);
    publish_handler_statistics_.max_queue_size(0);
    publish_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    publish_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    publish_max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

  const Config& config_;

  OpenDDS::DCPS::MonotonicTimePoint log_last_report_;
  HandlerStatistics log_handler_statistics_;
  OpenDDS::DCPS::TimeDuration log_input_processing_time_;
  OpenDDS::DCPS::TimeDuration log_output_processing_time_;
  OpenDDS::DCPS::TimeDuration log_max_queue_latency_;

  OpenDDS::DCPS::MonotonicTimePoint publish_last_report_;
  HandlerStatistics publish_handler_statistics_;
  OpenDDS::DCPS::TimeDuration publish_input_processing_time_;
  OpenDDS::DCPS::TimeDuration publish_output_processing_time_;
  OpenDDS::DCPS::TimeDuration publish_max_queue_latency_;

  HandlerStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
  RelayStatisticsReporter& relay_statistics_reporter_;
};

}

#endif // RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
