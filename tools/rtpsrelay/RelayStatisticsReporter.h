#ifndef RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_RELAY_STATISTICS_REPORTER_H_

#include "Config.h"

#include "lib/RelayTypeSupportImpl.h"
#include "lib/Utility.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class RelayStatisticsReporter {
public:
  RelayStatisticsReporter(const Config& config,
                            RelayStatisticsDataWriter_var writer)
    : config_(config)
    , log_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , publish_last_report_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , writer_(writer)
  {
    DDS::Topic_var topic = writer_->get_topic();
    topic_name_ = topic->get_name();
    log_relay_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
    publish_relay_statistics_.application_participant_guid(repoid_to_guid(config.application_participant_guid()));
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.bytes_in() += byte_count;
    ++log_relay_statistics_.messages_in();
    log_input_processing_time_ += time;
    publish_relay_statistics_.bytes_in() += byte_count;
    ++publish_relay_statistics_.messages_in();
    publish_input_processing_time_ += time;
    report(now);
  }

  void ignored_message(size_t byte_count,
                     const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.bytes_ignored() += byte_count;
    ++log_relay_statistics_.messages_ignored();
    publish_relay_statistics_.bytes_ignored() += byte_count;
    ++publish_relay_statistics_.messages_ignored();
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.bytes_out() += byte_count;
    ++log_relay_statistics_.messages_out();
    log_output_processing_time_ += time;
    log_max_queue_latency_ = std::max(log_max_queue_latency_, queue_latency);
    publish_relay_statistics_.bytes_out() += byte_count;
    ++publish_relay_statistics_.messages_out();
    publish_output_processing_time_ += time;
    publish_max_queue_latency_ = std::max(publish_max_queue_latency_, queue_latency);
    report(now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.bytes_dropped() += byte_count;
    ++log_relay_statistics_.messages_dropped();
    log_output_processing_time_ += time;
    log_max_queue_latency_ = std::max(log_max_queue_latency_, queue_latency);
    publish_relay_statistics_.bytes_dropped() += byte_count;
    ++publish_relay_statistics_.messages_dropped();
    publish_output_processing_time_ += time;
    publish_max_queue_latency_ = std::max(publish_max_queue_latency_, queue_latency);
    report(now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.max_gain() = std::max(log_relay_statistics_.max_gain(), static_cast<uint32_t>(value));
    publish_relay_statistics_.max_gain() = std::max(publish_relay_statistics_.max_gain(), static_cast<uint32_t>(value));
    report(now);
  }

  void local_active_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_relay_statistics_.error_count();
    ++publish_relay_statistics_.error_count();
    report(now);
  }

  void new_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_relay_statistics_.new_address_count();
    ++publish_relay_statistics_.new_address_count();
    report(now);
  }

  void expired_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_relay_statistics_.expired_address_count();
    ++publish_relay_statistics_.expired_address_count();
    report(now);
  }

  void expired_pending(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ++log_relay_statistics_.expired_pending_count();
    ++publish_relay_statistics_.expired_pending_count();
    report(now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.max_queue_size() = std::max(log_relay_statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
    publish_relay_statistics_.max_queue_size() = std::max(publish_relay_statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
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
    if (config_.log_relay_statistics().is_zero()) {
      return;
    }

    const auto d = now - log_last_report_;
    if (!force && d < config_.log_relay_statistics()) {
      return;
    }

    log_relay_statistics_.interval(time_diff_to_duration(d));
    log_relay_statistics_.input_processing_time(time_diff_to_duration(log_input_processing_time_));
    log_relay_statistics_.output_processing_time(time_diff_to_duration(log_output_processing_time_));
    log_relay_statistics_.max_queue_latency(time_diff_to_duration(log_max_queue_latency_));

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_relay_statistics_).c_str()));

    log_last_report_ = now;

    log_relay_statistics_.messages_in(0);
    log_relay_statistics_.bytes_in(0);
    log_relay_statistics_.messages_ignored(0);
    log_relay_statistics_.bytes_ignored(0);
    log_relay_statistics_.messages_out(0);
    log_relay_statistics_.bytes_out(0);
    log_relay_statistics_.messages_dropped(0);
    log_relay_statistics_.bytes_dropped(0);
    log_relay_statistics_.max_gain(0);
    log_relay_statistics_.error_count(0);
    log_relay_statistics_.new_address_count(0);
    log_relay_statistics_.expired_address_count(0);
    log_relay_statistics_.expired_pending_count(0);
    log_relay_statistics_.max_queue_size(0);
    log_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    log_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    log_max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (config_.publish_relay_statistics().is_zero()) {
      return;
    }

    const auto d = now - publish_last_report_;
    if (!force && d < config_.publish_relay_statistics()) {
      return;
    }

    publish_relay_statistics_.interval(time_diff_to_duration(d));
    publish_relay_statistics_.input_processing_time(time_diff_to_duration(publish_input_processing_time_));
    publish_relay_statistics_.output_processing_time(time_diff_to_duration(publish_output_processing_time_));
    publish_relay_statistics_.max_queue_latency(time_diff_to_duration(publish_max_queue_latency_));

    const auto ret = writer_->write(publish_relay_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RelayStatisticsReporter::report failed to write relay statistics\n")));
    }

    publish_last_report_ = now;

    publish_relay_statistics_.messages_in(0);
    publish_relay_statistics_.bytes_in(0);
    publish_relay_statistics_.messages_ignored(0);
    publish_relay_statistics_.bytes_ignored(0);
    publish_relay_statistics_.messages_out(0);
    publish_relay_statistics_.bytes_out(0);
    publish_relay_statistics_.messages_dropped(0);
    publish_relay_statistics_.bytes_dropped(0);
    publish_relay_statistics_.max_gain(0);
    publish_relay_statistics_.error_count(0);
    publish_relay_statistics_.new_address_count(0);
    publish_relay_statistics_.expired_address_count(0);
    publish_relay_statistics_.expired_pending_count(0);
    publish_relay_statistics_.max_queue_size(0);
    publish_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    publish_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    publish_max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

  const Config& config_;

  OpenDDS::DCPS::MonotonicTimePoint log_last_report_;
  RelayStatistics log_relay_statistics_;
  OpenDDS::DCPS::TimeDuration log_input_processing_time_;
  OpenDDS::DCPS::TimeDuration log_output_processing_time_;
  OpenDDS::DCPS::TimeDuration log_max_queue_latency_;

  OpenDDS::DCPS::MonotonicTimePoint publish_last_report_;
  RelayStatistics publish_relay_statistics_;
  OpenDDS::DCPS::TimeDuration publish_input_processing_time_;
  OpenDDS::DCPS::TimeDuration publish_output_processing_time_;
  OpenDDS::DCPS::TimeDuration publish_max_queue_latency_;

  RelayStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
};

}

#endif // RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
