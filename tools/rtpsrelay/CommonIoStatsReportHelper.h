#ifndef RTPSRELAY_RELAY_COMMON_IO_STATS_REPORT_HELPER_H_
#define RTPSRELAY_RELAY_COMMON_IO_STATS_REPORT_HELPER_H_

#include <dds/rtpsrelaylib/Utility.h>

#include <dds/DCPS/TimeTypes.h>

#include <ace/Log_Msg.h>

namespace RtpsRelay {

/**
 * Helper class for working with statistics report structs with rtps and stun
 * ProtocolStatisticsReport fields.
 */
template <typename StatisticsType>
class ProtocolStatisticsReportHelper {
public:
  OpenDDS::DCPS::TimeDuration rtps_input_processing_time_;
  OpenDDS::DCPS::TimeDuration rtps_output_processing_time_;
  OpenDDS::DCPS::TimeDuration stun_input_processing_time_;
  OpenDDS::DCPS::TimeDuration stun_output_processing_time_;

  void input_message(StatisticsType& statistics,
                     size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics.rtps().bytes_in() += byte_count;
      ++statistics.rtps().messages_in();
      rtps_input_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics.stun().bytes_in() += byte_count;
      ++statistics.stun().messages_in();
      stun_input_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::input_message: "
                 "MessageType is Unknown\n"));
    }
  }

  void ignored_message(StatisticsType& statistics,
                       size_t byte_count,
                       MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics.rtps().bytes_ignored() += byte_count;
      ++statistics.rtps().messages_ignored();
      break;
    case MessageType::Stun:
      statistics.stun().bytes_ignored() += byte_count;
      ++statistics.stun().messages_ignored();
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::ignored_message: "
                 "MessageType is Unknown\n"));
    }
  }

  void output_message(StatisticsType& statistics,
                      size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics.rtps().bytes_out() += byte_count;
      ++statistics.rtps().messages_out();
      rtps_output_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics.stun().bytes_out() += byte_count;
      ++statistics.stun().messages_out();
      stun_output_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::output_message: "
        "MessageType is Unknown\n"));
    }
  }

  void dropped_message(StatisticsType& statistics,
                       size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics.rtps().bytes_dropped() += byte_count;
      ++statistics.rtps().messages_dropped();
      rtps_output_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics.stun().bytes_dropped() += byte_count;
      ++statistics.stun().messages_dropped();
      stun_output_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::dropped_message: "
        "MessageType is Unknown\n"));
    }
  }

  void prepare_report(StatisticsType& statistics)
  {
    statistics.rtps().input_processing_time(time_diff_to_duration(rtps_input_processing_time_));
    statistics.rtps().output_processing_time(time_diff_to_duration(rtps_output_processing_time_));
    statistics.stun().input_processing_time(time_diff_to_duration(stun_input_processing_time_));
    statistics.stun().output_processing_time(time_diff_to_duration(stun_output_processing_time_));
  }

  void reset(StatisticsType& statistics)
  {
    OpenDDS::DCPS::set_default(statistics.rtps());
    OpenDDS::DCPS::set_default(statistics.stun());
    rtps_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    rtps_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    stun_input_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
    stun_output_processing_time_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }

};

/**
 * Helper class for working with the common I/O statistics fields of
 * HandlerStatistics and RelayStatistics.
 */
template <typename StatisticsType>
class CommonIoStatsReportHelper : public ProtocolStatisticsReportHelper<StatisticsType> {
public:
  using Helper = ProtocolStatisticsReportHelper<StatisticsType>;
  OpenDDS::DCPS::MonotonicTimePoint last_report_ = OpenDDS::DCPS::MonotonicTimePoint::now();
  OpenDDS::DCPS::TimeDuration max_queue_latency_;

  void output_message(StatisticsType& statistics,
                      size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      MessageType type)
  {
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
    Helper::output_message(statistics, byte_count, time, type);
  }

  void dropped_message(StatisticsType& statistics,
                       size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       MessageType type)
  {
    Helper::dropped_message(statistics, byte_count, time, type);
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
  }

  void max_gain(StatisticsType& statistics,
                size_t value)
  {
    statistics.max_gain() = std::max(statistics.max_gain(), static_cast<uint32_t>(value));
  }

  void error(StatisticsType& statistics)
  {
    ++statistics.error_count();
  }

  void max_queue_size(StatisticsType& statistics,
                      size_t size)
  {
    statistics.max_queue_size() = std::max(
      statistics.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
  }

  /// Returns false if it's not time to make a report.
  bool prepare_report(StatisticsType& statistics,
                      const OpenDDS::DCPS::MonotonicTimePoint& now,
                      bool force,
                      const OpenDDS::DCPS::TimeDuration& min_interval)
  {
    if (min_interval.is_zero()) {
      return false;
    }

    const auto interval = now - last_report_;
    if (!force && interval < min_interval) {
      return false;
    }

    Helper::prepare_report(statistics);
    statistics.interval(time_diff_to_duration(interval));
    statistics.max_queue_latency(time_diff_to_duration(max_queue_latency_));

    return true;
  }

  void reset(StatisticsType& statistics,
             const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    Helper::reset(statistics);
    last_report_ = now;
    statistics.max_gain(0);
    statistics.error_count(0);
    statistics.max_queue_size(0);
    max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }
};

}

#endif // RTPSRELAY_RELAY_COMMON_IO_STATS_REPORT_HELPER_H_
