#ifndef RTPSRELAY_GOVERNOR_H_
#define RTPSRELAY_GOVERNOR_H_

#include <dds/DCPS/TimeTypes.h>

#include <ace/Thread_Mutex.h>

#include <cstddef>

namespace RtpsRelay {

class Governor {
public:
  Governor(std::size_t max_throughput /* in bytes per second */)
    : max_throughput_(max_throughput / 1000)
    , interval_start_(OpenDDS::DCPS::MonotonicTimePoint::now())
    , bytes_sent_(0)
  {}

  void add_bytes(std::size_t count)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const auto now = OpenDDS::DCPS::MonotonicTimePoint::now();
    const auto minimum_time = OpenDDS::DCPS::TimeDuration::from_msec(bytes_sent_ / max_throughput_);
    const auto actual_time = now - interval_start_;

    // Reset at 1s if not limiting.
    if (actual_time > minimum_time &&
        actual_time > OpenDDS::DCPS::TimeDuration(1)) {
      interval_start_ = now;
      bytes_sent_ = 0;
    }

    bytes_sent_ += count;
  }

  OpenDDS::DCPS::MonotonicTimePoint get_next_send_time() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, OpenDDS::DCPS::MonotonicTimePoint::zero_value);
    return interval_start_ + OpenDDS::DCPS::TimeDuration::from_msec(bytes_sent_ / max_throughput_);
  }

private:
  mutable ACE_Thread_Mutex mutex_;
  const std::size_t max_throughput_; // in bytes/millisecond
  OpenDDS::DCPS::MonotonicTimePoint interval_start_;
  std::size_t bytes_sent_;
};

}

#endif /* RTPSRELAY_GOVERNOR_H_ */
