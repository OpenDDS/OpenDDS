#ifndef RTPSRELAY_PARTICIPANT_STATISTICS_WRITER_H_
#define RTPSRELAY_PARTICIPANT_STATISTICS_WRITER_H_

#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/TimeTypes.h"


namespace RtpsRelay {

class ParticipantStatisticsWriter : public RcObject {
public:

  virtual ~ParticipantStatisticsWriter() {}

  virtual void process_input_msg(size_t byte_count) = 0;

  virtual void process_output_msg(size_t byte_count) = 0;

  virtual void update_fan_out(uint32_t value) = 0;

  virtual void update_queue_latency(const Duration_t& updated_latency) = 0;

  virtual void update_queue_size(uint32_t value) = 0;

  //virtual void add_local_participant() = 0;
  //virtual void remove_local_participant() = 0;

  virtual void log_stats(const OpenDDS::DCPS::MonotonicTimePoint& time_now) = 0;
  virtual void publish_stats(const OpenDDS::DCPS::MonotonicTimePoint& time_now) = 0;

  virtual void reset_stats() = 0;

protected:

  explicit ParticipantStatisticsWriter() {}
};

typedef RcHandle<ParticipantStatisticsWriter> ParticipantStatisticsWriter_rch;
}

#endif // RTPSRELAY_STATISTICS_REPORTER_H_
