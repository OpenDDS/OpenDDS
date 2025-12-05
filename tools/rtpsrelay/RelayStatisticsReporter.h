#ifndef RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_RELAY_STATISTICS_REPORTER_H_

#include "Config.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

// after RelayTypeSupportImpl.h so that set_default is available
#include "CommonIoStatsReportHelper.h"

#include <dds/DCPS/JsonValueWriter.h>

namespace RtpsRelay {

class RelayStatisticsReporter : public OpenDDS::DCPS::ConfigListener {
public:
  RelayStatisticsReporter(const Config& config,
                          RelayStatisticsDataWriter_var writer);

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

  void handler_statistics_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.handler_statistics_sub_count() = count;
    publish_relay_statistics_.handler_statistics_sub_count() = count;
    report(guard, now);
  }

  void relay_statistics_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_statistics_sub_count() = count;
    publish_relay_statistics_.relay_statistics_sub_count() = count;
    report(guard, now);
  }

  void participant_statistics_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.participant_statistics_sub_count() = count;
    publish_relay_statistics_.participant_statistics_sub_count() = count;
    report(guard, now);
  }

  void relay_partitions_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_partitions_sub_count() = count;
    publish_relay_statistics_.relay_partitions_sub_count() = count;
    report(guard, now);
  }

  void relay_participant_status_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_participant_status_sub_count() = count;
    publish_relay_statistics_.relay_participant_status_sub_count() = count;
    report(guard, now);
  }

  void spdp_replay_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.spdp_replay_sub_count() = count;
    publish_relay_statistics_.spdp_replay_sub_count() = count;
    report(guard, now);
  }

  void relay_address_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_address_sub_count() = count;
    publish_relay_statistics_.relay_address_sub_count() = count;
    report(guard, now);
  }

  void relay_status_sub_count(uint32_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.relay_status_sub_count() = count;
    publish_relay_statistics_.relay_status_sub_count() = count;
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

  void remote_map_size(uint32_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.remote_map_size(size);
    publish_relay_statistics_.remote_map_size(size);
    report(guard, now);
  }

  void max_ips_per_client(uint32_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.max_ips_per_client(std::max(log_relay_statistics_.max_ips_per_client(), size));
    publish_relay_statistics_.max_ips_per_client(std::max(publish_relay_statistics_.max_ips_per_client(), size));
    report(guard, now);
  }

  void rejected_address_map_size(uint32_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.rejected_address_map_size(std::max(log_relay_statistics_.rejected_address_map_size(), size));
    publish_relay_statistics_.rejected_address_map_size(std::max(publish_relay_statistics_.rejected_address_map_size(), size));
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
                  bool force);

  void publish_report(ACE_Guard<ACE_Thread_Mutex>& guard,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force);

  void on_data_available(InternalDataReader_rch reader) override;
  void configure_stats_period(const OpenDDS::DCPS::TimeDuration& log,
                              const OpenDDS::DCPS::TimeDuration& publish) const;

  static void log_json(const std::string& label, const std::string& json);

  mutable ACE_Thread_Mutex mutex_;
  const Config& config_;

  typedef CommonIoStatsReportHelper<RelayStatistics> Helper;

  RelayStatistics log_relay_statistics_;
  Helper log_helper_;

  RelayStatistics publish_relay_statistics_;
  Helper publish_helper_;

  RelayStatisticsDataWriter_var writer_;
  std::string topic_name_;
};

}

#endif // RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
