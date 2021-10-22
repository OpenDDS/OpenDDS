#ifndef RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
#define RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_

#include "Config.h"
#include "RelayStatisticsReporter.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class HandlerStatisticsReporter {
public:
  HandlerStatisticsReporter(const Config& config,
                            const std::string& name,
                            HandlerStatisticsDataWriter_var writer,
                            RelayStatisticsReporter& relay_statistics_reporter)
    : config_(config)
    , writer_(writer)
    , relay_statistics_reporter_(relay_statistics_reporter)
  {
    DDS::Topic_var topic = writer->get_topic();
    topic_name_ = topic->get_name();
    log_handler_statistics_.relay_id(config.relay_id());
    log_handler_statistics_.name(name);
    publish_handler_statistics_.relay_id(config.relay_id());
    publish_handler_statistics_.name(name);
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now,
                     MessageType type)
  {
    relay_statistics_reporter_.input_message(byte_count, time, now, type);
    log_helper_.input_message(log_handler_statistics_, byte_count, time, type);
    publish_helper_.input_message(publish_handler_statistics_, byte_count, time, type);
    report(now);
  }

  void ignored_message(size_t byte_count,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    relay_statistics_reporter_.ignored_message(byte_count, now, type);
    log_helper_.ignored_message(log_handler_statistics_, byte_count, type);
    publish_helper_.ignored_message(publish_handler_statistics_, byte_count, type);
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      MessageType type)
  {
    relay_statistics_reporter_.output_message(byte_count, time, queue_latency, now, type);
    log_helper_.output_message(log_handler_statistics_, byte_count, time, queue_latency, type);
    publish_helper_.output_message(publish_handler_statistics_, byte_count, time, queue_latency, type);
    report(now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    relay_statistics_reporter_.dropped_message(byte_count, time, queue_latency, now, type);
    log_helper_.dropped_message(log_handler_statistics_, byte_count, time, queue_latency, type);
    publish_helper_.dropped_message(publish_handler_statistics_, byte_count, time, queue_latency, type);
    report(now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_gain(value, now);
    log_helper_.max_gain(log_handler_statistics_, value);
    publish_helper_.max_gain(publish_handler_statistics_, value);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.error(now);
    log_helper_.error(log_handler_statistics_);
    publish_helper_.error(publish_handler_statistics_);
    report(now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_queue_size(size, now);
    log_helper_.max_queue_size(log_handler_statistics_, size);
    publish_helper_.max_queue_size(publish_handler_statistics_, size);
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
    if (!log_helper_.prepare_report(log_handler_statistics_, now, force, config_.log_handler_statistics())) {
      return;
    }

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_handler_statistics_).c_str()));

    log_helper_.reset(log_handler_statistics_, now);
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (!publish_helper_.prepare_report(publish_handler_statistics_, now, force, config_.publish_handler_statistics())) {
      return;
    }

    const auto ret = writer_->write(publish_handler_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "HandlerStatisticsReporter::report %C failed to write handler statistics\n",
        publish_handler_statistics_.name().c_str()));
    }

    publish_helper_.reset(publish_handler_statistics_, now);
  }

  const Config& config_;

  typedef CommonIoStatsReportHelper<HandlerStatistics> Helper;

  HandlerStatistics log_handler_statistics_;
  Helper log_helper_;

  HandlerStatistics publish_handler_statistics_;
  Helper publish_helper_;

  HandlerStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
  RelayStatisticsReporter& relay_statistics_reporter_;
};

}

#endif // RTPSRELAY_HANDLER_STATISTICS_REPORTER_H_
