#ifndef OPENDDS_DCPS_CONDITIONVARIABLE_H
#define OPENDDS_DCPS_CONDITIONVARIABLE_H

#include "TimeTypes.h"
#include "debug.h"

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
 * This is the return type of ConditionVariable::wait* functions.
 */
enum CvStatus {
  CvStatus_NoTimeout, ///< The wait has returned because it was woken up
  CvStatus_Timeout, ///< The wait has returned because of a timeout
  CvStatus_Error /**<
    * The wait has returned because of an error.
    * The errno-given reason was logged.
    */
};

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
class ConditionVariable {
public:
  explicit ConditionVariable(Mutex& mutex)
  : impl_(mutex, ACE_Condition_Attributes_T<MonotonicClock>())
  {
  }

  CvStatus wait()
  {
    if (impl_.wait() == 0) {
      return CvStatus_NoTimeout;
    }
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ConditionVariable::wait: %p\n"));
    }
    return CvStatus_Error;
  }

  // Blocks forever if expire_at is zero.
  CvStatus wait_until(const MonotonicTimePoint& expire_at)
  {
    if (impl_.wait(expire_at.is_zero() ? 0 : &expire_at.value()) == 0) {
      return CvStatus_NoTimeout;
    } else if (errno == ETIME) {
      return CvStatus_Timeout;
    }
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ConditionVariable::wait_until: %p\n"));
    }
    return CvStatus_Error;
  }

  CvStatus wait_for(const TimeDuration& expire_in)
  {
    return wait_until(MonotonicTimePoint::now() + expire_in);
  }

  bool notify_one()
  {
    if (impl_.signal() == 0) {
      return true;
    }
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ConditionVariable::notify_one: %p\n"));
    }
    return false;
  }

  bool notify_all()
  {
    if (impl_.broadcast() == 0) {
      return true;
    }
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ConditionVariable::notify_all: %p\n"));
    }
    return false;
  }

protected:
  ACE_Condition<Mutex> impl_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_CONDITIONVARIABLE_H
