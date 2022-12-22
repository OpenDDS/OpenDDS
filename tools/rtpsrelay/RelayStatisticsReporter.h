#ifndef RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_RELAY_STATISTICS_REPORTER_H_

#include "Config.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

// after RelayTypeSupportImpl.h so that set_default is available
#include "CommonIoStatsReportHelper.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class RelayStatisticsReporter {
public:
  RelayStatisticsReporter(const Config& config,
                          RelayStatisticsDataWriter_var writer)
    : config_(config)
    , writer_(writer)
  {
    DDS::Topic_var topic = writer_->get_topic();
    topic_name_ = topic->get_name();
    log_relay_statistics_.relay_id(config.relay_id());
    publish_relay_statistics_.relay_id(config.relay_id());
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     const OpenDDS::DCPS::MonotonicTimePoint& now,
                     MessageType type)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.input_message(log_relay_statistics_, byte_count, time, type);
    publish_helper_.input_message(publish_relay_statistics_, byte_count, time, type);
    report(guard, now);
  }

  void ignored_message(size_t byte_count,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.ignored_message(log_relay_statistics_, byte_count, type);
    publish_helper_.ignored_message(publish_relay_statistics_, byte_count, type);
    report(guard, now);
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      MessageType type)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.output_message(log_relay_statistics_, byte_count, time, queue_latency, type);
    publish_helper_.output_message(publish_relay_statistics_, byte_count, time, queue_latency, type);
    report(guard, now);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       const OpenDDS::DCPS::MonotonicTimePoint& now,
                       MessageType type)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.dropped_message(log_relay_statistics_, byte_count, time, queue_latency, type);
    publish_helper_.dropped_message(publish_relay_statistics_, byte_count, time, queue_latency, type);
    report(guard, now);
  }

  void max_gain(size_t value, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.max_gain(log_relay_statistics_, value);
    publish_helper_.max_gain(publish_relay_statistics_, value);
    report(guard, now);
  }

  void local_active_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_active_participants() = static_cast<uint32_t>(count);
    report(guard, now);
  }

  void error(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.error(log_relay_statistics_);
    publish_helper_.error(publish_relay_statistics_);
    report(guard, now);
  }

  void new_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++log_relay_statistics_.new_address_count();
    ++publish_relay_statistics_.new_address_count();
    report(guard, now);
  }

  void expired_address(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++log_relay_statistics_.expired_address_count();
    ++publish_relay_statistics_.expired_address_count();
    report(guard, now);
  }

  void max_queue_size(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_helper_.max_queue_size(log_relay_statistics_, size);
    publish_helper_.max_queue_size(publish_relay_statistics_, size);
    report(guard, now);
  }

  void local_participants(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.local_participants() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_participants() = static_cast<uint32_t>(count);
    report(guard, now);
  }

  void local_writers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.local_writers() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_writers() = static_cast<uint32_t>(count);
    report(guard, now);
  }

  void local_readers(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.local_readers() = static_cast<uint32_t>(count);
    publish_relay_statistics_.local_readers() = static_cast<uint32_t>(count);
    report(guard, now);
  }

  void relay_partitions_pub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_partitions_pub_count() = count;
    publish_relay_statistics_.relay_partitions_pub_count() = count;
    report(guard, now);
  }

  void relay_address_pub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_address_pub_count() = count;
    publish_relay_statistics_.relay_address_pub_count() = count;
    report(guard, now);
  }

  void spdp_replay_pub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.spdp_replay_pub_count() = count;
    publish_relay_statistics_.spdp_replay_pub_count() = count;
    report(guard, now);
  }

  void admission_deferral_count(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    ++log_relay_statistics_.admission_deferral_count();
    ++publish_relay_statistics_.admission_deferral_count();
    report(guard, now);
  }

  void report()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now(), true);
  }

private:

  void report(ACE_Guard<ACE_Thread_Mutex>& guard,
              const OpenDDS::DCPS::MonotonicTimePoint& now,
              bool force = false)
  {
    log_report(now, force);
    publish_report(guard, now, force);
  }

  void log_report(const OpenDDS::DCPS::MonotonicTimePoint& now,
                  bool force)
  {
    if (!log_helper_.prepare_report(log_relay_statistics_, now, force, config_.log_relay_statistics())) {
      return;
    }

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) STAT: %C %C\n"), topic_name_.in(), OpenDDS::DCPS::to_json(log_relay_statistics_).c_str()));

    log_helper_.reset(log_relay_statistics_, now);
    log_relay_statistics_.new_address_count(0);
    log_relay_statistics_.expired_address_count(0);
    log_relay_statistics_.admission_deferral_count(0);
  }

  void publish_report(ACE_Guard<ACE_Thread_Mutex>& guard,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force)
  {
    if (!publish_helper_.prepare_report(publish_relay_statistics_, now, force, config_.publish_relay_statistics())) {
      return;
    }

    const auto writer_copy = writer_;
    const auto stats_copy = publish_relay_statistics_;

    publish_helper_.reset(publish_relay_statistics_, now);
    publish_relay_statistics_.new_address_count(0);
    publish_relay_statistics_.expired_address_count(0);
    publish_relay_statistics_.admission_deferral_count(0);

    guard.release();

    const auto ret = writer_copy->write(stats_copy, DDS::HANDLE_NIL);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: "
        "RelayStatisticsReporter::publish_report failed to write relay statistics\n"));
    }
  }

  mutable ACE_Thread_Mutex mutex_;
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
