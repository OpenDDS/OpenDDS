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
  StatisticsType& statistics_;
  OpenDDS::DCPS::TimeDuration rtps_input_processing_time_;
  OpenDDS::DCPS::TimeDuration rtps_output_processing_time_;
  OpenDDS::DCPS::TimeDuration stun_input_processing_time_;
  OpenDDS::DCPS::TimeDuration stun_output_processing_time_;

  explicit ProtocolStatisticsReportHelper(StatisticsType& statistics)
    : statistics_(statistics)
  {
  }

  void input_message(size_t byte_count,
                     const OpenDDS::DCPS::TimeDuration& time,
                     MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics_.rtps().bytes_in() += byte_count;
      ++statistics_.rtps().messages_in();
      rtps_input_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics_.stun().bytes_in() += byte_count;
      ++statistics_.stun().messages_in();
      stun_input_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::input_message: "
                 "MessageType is Unknown\n"));
    }
  }

  void ignored_message(size_t byte_count,
                       MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics_.rtps().bytes_ignored() += byte_count;
      ++statistics_.rtps().messages_ignored();
      break;
    case MessageType::Stun:
      statistics_.stun().bytes_ignored() += byte_count;
      ++statistics_.stun().messages_ignored();
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::ignored_message: "
                 "MessageType is Unknown\n"));
    }
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics_.rtps().bytes_out() += byte_count;
      ++statistics_.rtps().messages_out();
      rtps_output_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics_.stun().bytes_out() += byte_count;
      ++statistics_.stun().messages_out();
      stun_output_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::output_message: "
        "MessageType is Unknown\n"));
    }
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       MessageType type)
  {
    switch (type) {
    case MessageType::Rtps:
      statistics_.rtps().bytes_dropped() += byte_count;
      ++statistics_.rtps().messages_dropped();
      rtps_output_processing_time_ += time;
      break;
    case MessageType::Stun:
      statistics_.stun().bytes_dropped() += byte_count;
      ++statistics_.stun().messages_dropped();
      stun_output_processing_time_ += time;
      break;
    case MessageType::Unknown:
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ProtocolStatisticsReportHelper::dropped_message: "
        "MessageType is Unknown\n"));
    }
  }

  void prepare_report()
  {
    statistics_.rtps().input_processing_time(time_diff_to_duration(rtps_input_processing_time_));
    statistics_.rtps().output_processing_time(time_diff_to_duration(rtps_output_processing_time_));
    statistics_.stun().input_processing_time(time_diff_to_duration(stun_input_processing_time_));
    statistics_.stun().output_processing_time(time_diff_to_duration(stun_output_processing_time_));
  }

  void reset()
  {
    OpenDDS::DCPS::set_default(statistics_.rtps());
    OpenDDS::DCPS::set_default(statistics_.stun());
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

  explicit CommonIoStatsReportHelper(StatisticsType& statistics)
    : Helper(statistics)
  {
  }

  void output_message(size_t byte_count,
                      const OpenDDS::DCPS::TimeDuration& time,
                      const OpenDDS::DCPS::TimeDuration& queue_latency,
                      MessageType type)
  {
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
    Helper::output_message(byte_count, time, type);
  }

  void dropped_message(size_t byte_count,
                       const OpenDDS::DCPS::TimeDuration& time,
                       const OpenDDS::DCPS::TimeDuration& queue_latency,
                       MessageType type)
  {
    Helper::dropped_message(byte_count, time, type);
    max_queue_latency_ = std::max(max_queue_latency_, queue_latency);
  }

  void max_gain(size_t value)
  {
    this->statistics_.max_gain() = std::max(this->statistics_.max_gain(), static_cast<uint32_t>(value));
  }

  void error()
  {
    ++this->statistics_.error_count();
  }

  void max_queue_size(size_t size)
  {
    this->statistics_.max_queue_size() = std::max(
      this->statistics_.max_queue_size(), static_cast<ACE_CDR::ULong>(size));
  }

  /// Returns false if it's not time to make a report.
  bool prepare_report(const OpenDDS::DCPS::MonotonicTimePoint& now, bool force,
    const OpenDDS::DCPS::TimeDuration& min_interval)
  {
    if (min_interval.is_zero()) {
      return false;
    }

    const auto interval = now - last_report_;
    if (!force && interval < min_interval) {
      return false;
    }

    Helper::prepare_report();
    this->statistics_.interval(time_diff_to_duration(interval));
    this->statistics_.max_queue_latency(time_diff_to_duration(max_queue_latency_));

    return true;
  }

  void reset(const OpenDDS::DCPS::MonotonicTimePoint& now)
  {
    Helper::reset();
    last_report_ = now;
    this->statistics_.max_gain(0);
    this->statistics_.error_count(0);
    this->statistics_.max_queue_size(0);
    max_queue_latency_ = OpenDDS::DCPS::TimeDuration::zero_value;
  }
};

}

#endif // RTPSRELAY_RELAY_COMMON_IO_STATS_REPORT_HELPER_H_
