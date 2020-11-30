#ifndef OPENDDS_DCPS_CONDITION_H
#define OPENDDS_DCPS_CONDITION_H

#include "TimeTypes.h"

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Condition_Recursive_Thread_Mutex.h>
#include <ace/Condition_T.h>
#include <ace/Condition_Attributes.h>
#include <ace/OS_NS_Thread.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ACE_Condition wrapper based on std::condition_variable that enforces
 * monotonic time behavior.
 *
 * Besides the fact that it only works with ACE Mutexes, the major difference
 * between this and std::condition_variable_any, the generalized form of
 * std::condition_variable, is that it takes the mutex as a constructor
 * argument, where the std::condition_variables take them as method arguments.
 */
template <typename Mutex>
class Condition {
public:
  Condition(Mutex& mutex)
  : impl_(mutex, ACE_Condition_Attributes_T<MonotonicClock>())
  {
  }

  enum WaitStatus {
    NoTimeout,
    Timeout,
    WaitError
  };

  WaitStatus wait()
  {
    if (impl_.wait() == 0) {
      return NoTimeout;
    }
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Condition::wait: %p\n"));
    return WaitError;
  }

  WaitStatus wait_until(const MonotonicTimePoint& expire_at)
  {
    if (impl_.wait(&expire_at.value()) == 0) {
      return NoTimeout;
    } else if (errno == ETIME) {
      return Timeout;
    }
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Condition::wait_until: %p\n"));
    return WaitError;
  }

  WaitStatus wait_for(const TimeDuration& expire_in)
  {
    return wait_until(MonotonicTimePoint::now() + expire_in);
  }

  bool notify_one()
  {
    if (impl_.signal() == 0) {
      return true;
    }
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Condition::notify_one: %p\n"));
    return false;
  }

  bool notify_all()
  {
    if (impl_.broadcast() == 0) {
      return true;
    }
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: Condition::notify_all: %p\n"));
    return false;
  }

protected:
  ACE_Condition<Mutex> impl_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_CONDITION_H
