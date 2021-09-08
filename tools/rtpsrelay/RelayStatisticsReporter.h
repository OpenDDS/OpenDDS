#ifndef RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_RELAY_STATISTICS_REPORTER_H_

#include "Config.h"
#include "CommonIoStatsReportHelper.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class RelayStatisticsReporter {
public:
  RelayStatisticsReporter(const Config& config,
                          RelayStatisticsDataWriter_var writer)
    : config_(config)
    , log_helper_(log_relay_statistics_)
    , publish_helper_(publish_relay_statistics_)
    , writer_(writer)
  {
    DDS::Topic_var topic = writer_->get_topic();
    topic_name_ = topic->get_name();
    log_relay_statistics_.application_participant_guid(rtps_guid_to_relay_guid(config.application_participant_guid()));
    publish_relay_statistics_.application_participant_guid(rtps_guid_to_relay_guid(config.application_participant_guid()));
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now,
                     MessageType type)
  {
    log_helper_.input_message(byte_count, time, type);
    publish_helper_.input_message(byte_count, time, type);
    report(now);
  }

  void ignored_message(size_t byte_count,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    log_helper_.ignored_message(byte_count, type);
    publish_helper_.ignored_message(byte_count, type);
    report(now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      MessageType type)
  {
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
    log_helper_.dropped_message(byte_count, time, queue_latency, type);
    publish_helper_.dropped_message(byte_count, time, queue_latency, type);
    report(now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_helper_.max_gain(value);
    publish_helper_.max_gain(value);
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
    log_helper_.error();
    publish_helper_.error();
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
    log_helper_.max_queue_size(size);
    publish_helper_.max_queue_size(size);
    report(now);
  }

  void local_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.local_participants() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_participants() = static_cast<uint32_t>(count);
    report(now);
  }

  void local_writers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.local_writers() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_writers() = static_cast<uint32_t>(count);
    report(now);
  }

  void local_readers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    log_relay_statistics_.local_readers() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_readers() = static_cast<uint32_t>(count);
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
    if (!log_helper_.prepare_report(now, force, config_.log_relay_statistics())) {
      return;
    }

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_relay_statistics_).c_str()));

    log_helper_.reset(now);
    log_relay_statistics_.new_address_count(0);
    log_relay_statistics_.expired_address_count(0);
    log_relay_statistics_.expired_pending_count(0);
  }

  void publish_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (!publish_helper_.prepare_report(now, force, config_.publish_relay_statistics())) {
      return;
    }

    const auto ret = writer_->write(publish_relay_statistics_, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "RelayStatisticsReporter::publish_report failed to write relay statistics\n"));
    }

    publish_helper_.reset(now);
    publish_relay_statistics_.new_address_count(0);
    publish_relay_statistics_.expired_address_count(0);
    publish_relay_statistics_.expired_pending_count(0);
  }

  const Config& config_;

  typedef CommonIoStatsReportHelper<RelayStatistics> Helper;

  RelayStatistics log_relay_statistics_;
  Helper log_helper_;

  RelayStatistics publish_relay_statistics_;
  Helper publish_helper_;

  RelayStatisticsDataWriter_var writer_;
  CORBA::String_var topic_name_;
};

}

#endif // RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
