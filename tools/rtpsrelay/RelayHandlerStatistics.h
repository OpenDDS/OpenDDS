#ifndef RTPSRELAY_RELAY_HANDLER_STATISTICS_IMPL_H_
#define RTPSRELAY_RELAY_HANDLER_STATISTICS_IMPL_H_

#include "lib/RelayTypeSupportImpl.h"

#include "dds/DCPS/TimeTypes.h"

#include <ace/Thread_Mutex.h>

namespace RtpsRelay {

class RelayHandlerStatistics {
public:

  explicit RelayHandlerStatistics(const OpenDDS::DCPS::RepoId& participant_guid,
                                  const std::string& name,
                                  HandlerStatisticsDataWriter_ptr writer,
                                  bool log_stats);

  virtual ~RelayHandlerStatistics();

  void update_input_msgs(size_t byte_count);
  void update_output_msgs(size_t byte_count);

  void update_fan_out(size_t count);
  void update_queue_size(uint32_t value);
  void update_queue_latency(const Duration_t& updated_latency);

  void update_local_participants(uint32_t num_participants);

  void governor_active();
  void report_error();

  void report(const OpenDDS::DCPS::MonotonicTimePoint& time_now);

  void reset_stats();

private:

  HandlerStatisticsDataWriter_ptr writer_;

  bool log_stats_;

  HandlerStatistics handler_stats_;

  std::string particpant_name_;

  OpenDDS::DCPS::MonotonicTimePoint last_report_time_;

  ACE_Thread_Mutex stats_mutex_;
};

}

#endif // RTPSRELAY_RELAY_HANDLER_STATISTICS_IMPL_H_
