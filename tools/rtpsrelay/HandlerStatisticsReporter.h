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
    , log_helper_(log_handler_statistics_)
    , publish_helper_(publish_handler_statistics_)
    , writer_(writer)
    , relay_statistics_reporter_(relay_statistics_reporter)
  {
    DDS::Topic_var topic = writer->get_topic();
    topic_name_ = topic->get_name();
    log_handler_statistics_.application_participant_guid(rtps_guid_to_relay_guid(config.application_participant_guid()));
    log_handler_statistics_.name(name);
    publish_handler_statistics_.application_participant_guid(rtps_guid_to_relay_guid(config.application_participant_guid()));
    publish_handler_statistics_.name(name);
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now,
                     MessageType type)
  {
    relay_statistics_reporter_.input_message(byte_count, time, now, type);
    log_helper_.input_message(byte_count, time, type);
    publish_helper_.input_message(byte_count, time, type);
    report(now);
  }

  void ignored_message(size_t byte_count,
                       const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.ignored_message(byte_count, now);
    log_helper_.ignored_message(byte_count);
    publish_helper_.ignored_message(byte_count);
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      MessageType type)
  {
    relay_statistics_reporter_.output_message(byte_count, time, queue_latency, now, type);
    log_helper_.output_message(byte_count, time, queue_latency, type);
    publish_helper_.output_message(byte_count, time, queue_latency, type);
    report(now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    relay_statistics_reporter_.dropped_message(byte_count, time, queue_latency, now, type);
    log_helper_.dropped_message(byte_count, time, queue_latency, type);
    publish_helper_.dropped_message(byte_count, time, queue_latency, type);
    report(now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_gain(value, now);
    log_helper_.max_gain(value);
    publish_helper_.max_gain(value);
    report(now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.error(now);
    log_helper_.error();
    publish_helper_.error();
    report(now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    relay_statistics_reporter_.max_queue_size(size, now);
    log_helper_.max_queue_size(size);
    publish_helper_.max_queue_size(size);
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
    if (!log_helper_.prepare_report(now, force, config_.log_handler_statistics())) {
      return;
    }

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_handler_statistics_).c_str()));

    log_helper_.reset(now);
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (!publish_helper_.prepare_report(now, force, config_.publish_handler_statistics())) {
      return;
    }

    const auto ret = writer_->write(publish_handler_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "HandlerStatisticsReporter::report %C failed to write handler statistics\n",
        publish_handler_statistics_.name().c_str()));
    }

    publish_helper_.reset(now);
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
