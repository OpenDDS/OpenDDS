#ifndef RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
#define RTPSRELAY_RELAY_STATISTICS_REPORTER_H_

#include "Config.h"

#include <dds/rtpsrelaylib/RelayTypeSupportImpl.h>
#include <dds/rtpsrelaylib/Utility.h>

// after RelayTypeSupportImpl.h so that set_default is available
#include "CommonIoStatsReportHelper.h"

#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/Statistics.h>

namespace RtpsRelay {

class RelayStatisticsReporter {
public:
  RelayStatisticsReporter(const Config& config,
                          RelayStatisticsDataWriter_var writer);

  ~RelayStatisticsReporter();

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

  void total_client_ips(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto size32 = static_cast<uint32_t>(size);
    log_relay_statistics_.total_client_ips(size32);
    publish_relay_statistics_.total_client_ips(size32);
    report(guard, now);
  }

  void total_client_ports(size_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto size32 = static_cast<uint32_t>(size);
    log_relay_statistics_.total_client_ports(size32);
    publish_relay_statistics_.total_client_ports(size32);
    report(guard, now);
  }

  void rejected_address_map_size(uint32_t size, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    log_relay_statistics_.rejected_address_map_size(std::max(log_relay_statistics_.rejected_address_map_size(), size));
    publish_relay_statistics_.rejected_address_map_size(std::max(publish_relay_statistics_.rejected_address_map_size(), size));
    report(guard, now);
  }

  void deactivation_queue_size(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.deactivation_queue_size() = count32;
    publish_relay_statistics_.deactivation_queue_size() = count32;
    report(guard, now);
  }

  void expiration_queue_size(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.expiration_queue_size() = count32;
    publish_relay_statistics_.expiration_queue_size() = count32;
    report(guard, now);
  }

  void admission_queue_size(size_t count, const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.admission_queue_size() = count32;
    publish_relay_statistics_.admission_queue_size() = count32;
    report(guard, now);
  }

  void partition_slots(size_t used, size_t free)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto used32 = static_cast<uint32_t>(used),
      free32 = static_cast<uint32_t>(free);
    log_relay_statistics_.client_partitions().slots() = used32;
    publish_relay_statistics_.client_partitions().slots() = used32;
    log_relay_statistics_.client_partitions().free_slots() = free32;
    publish_relay_statistics_.client_partitions().free_slots() = free32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void partitions(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.client_partitions().count() = count32;
    publish_relay_statistics_.client_partitions().count() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void partition_index_cache(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.client_partitions().partition_index_cache() = count32;
    publish_relay_statistics_.client_partitions().partition_index_cache() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void partition_index_nodes(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.client_partitions().partition_index_nodes() = count32;
    publish_relay_statistics_.client_partitions().partition_index_nodes() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void partition_guids(size_t count_main, size_t count_cache)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count_main),
      cache32 = static_cast<uint32_t>(count_cache);
    log_relay_statistics_.client_partitions().guids() = count32;
    publish_relay_statistics_.client_partitions().guids() = count32;
    log_relay_statistics_.client_partitions().guids_cache() = cache32;
    publish_relay_statistics_.client_partitions().guids_cache() = cache32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void relay_partition_addresses(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.relay_partitions().addresses() = count32;
    publish_relay_statistics_.relay_partitions().addresses() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void relay_partition_index_cache(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.relay_partitions().partition_index_cache() = count32;
    publish_relay_statistics_.relay_partitions().partition_index_cache() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void relay_partition_index_nodes(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.relay_partitions().partition_index_nodes() = count32;
    publish_relay_statistics_.relay_partitions().partition_index_nodes() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void relay_partition_slots(size_t count)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const auto count32 = static_cast<uint32_t>(count);
    log_relay_statistics_.relay_partitions().slots() = count32;
    publish_relay_statistics_.relay_partitions().slots() = count32;
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
  }

  void admission_state_changed(bool admitting)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (admitting) {
      ++log_relay_statistics_.transitions_to_admitting();
      ++publish_relay_statistics_.transitions_to_admitting();
    } else {
      ++log_relay_statistics_.transitions_to_nonadmitting();
      ++publish_relay_statistics_.transitions_to_nonadmitting();
    }
    report(guard, OpenDDS::DCPS::MonotonicTimePoint::now());
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

  void log_report(const OpenDDS::DCPS::MonotonicTimePoint& now, bool force);
  void publish_report(ACE_Guard<ACE_Thread_Mutex>& guard,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force);

  void get_opendds_stats(std::vector<OpenDDSModuleStatistics>& out);
  static void get_process_stats(RelayStatistics& out);

  static void log_json(const std::string& label, const std::string& json);

  mutable ACE_Thread_Mutex mutex_;
  const Config& config_;

  using Helper = CommonIoStatsReportHelper<RelayStatistics>;

  RelayStatistics log_relay_statistics_;
  Helper log_helper_;

  RelayStatistics publish_relay_statistics_;
  Helper publish_helper_;

  RelayStatisticsDataWriter_var writer_;
  std::string topic_name_;

  OpenDDS::DCPS::StatisticsDataReader_rch internal_reader_;
};

}

#endif // RTPSRELAY_RELAY_STATISTICS_REPORTER_H_
