#pragma once

#ifndef STRESS_TESTS_TIMING_CHECKER_H
#define STRESS_TESTS_TIMING_CHECKER_H

#include <dds/DCPS/RcEventHandler.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/ConditionVariable.h>

namespace Utils {

struct TimingChecker : public virtual OpenDDS::DCPS::RcEventHandler
{
  TimingChecker();

  int handle_timeout(const ACE_Time_Value&, const void*);

  OpenDDS::DCPS::MonotonicTimePoint wait_timeout();

  typedef OpenDDS::DCPS::TimeDuration Duration;

  static bool check_timing(
    const Duration& epsilon = Duration::from_msec(5),
    const Duration& requested = Duration::from_msec(100));

  mutable ACE_Thread_Mutex mutex_;
  mutable OpenDDS::DCPS::ConditionVariable<ACE_Thread_Mutex> cv_;
  OpenDDS::DCPS::MonotonicTimePoint timeout_;
};

}

#endif // STRESS_TESTS_TIMING_CHECKER_H
