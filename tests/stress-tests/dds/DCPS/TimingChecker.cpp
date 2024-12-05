#include "TimingChecker.h"

#include <dds/DCPS/ReactorTask.h>

#include <ace/Reactor.h>

namespace Utils {

TimingChecker::TimingChecker() : cv_(mutex_), timeout_(OpenDDS::DCPS::MonotonicTimePoint::zero_value)
{
}

int TimingChecker::handle_timeout(const ACE_Time_Value&, const void*)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timeout_ = OpenDDS::DCPS::MonotonicTimePoint::now();
  cv_.notify_all();
  return 0;
}

OpenDDS::DCPS::MonotonicTimePoint TimingChecker::wait_timeout()
{
  OpenDDS::DCPS::ThreadStatusManager tsm;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (timeout_ == OpenDDS::DCPS::MonotonicTimePoint::zero_value) {
    cv_.wait(tsm);
  }
  return timeout_;
}

bool TimingChecker::check_timing(const OpenDDS::DCPS::TimeDuration& epsilon, const OpenDDS::DCPS::TimeDuration& requested)
{
  using namespace OpenDDS::DCPS;

  bool result = true;

  ThreadStatusManager tsm;
  ReactorTask reactor_task(false);
  reactor_task.open_reactor_task(&tsm);

  for (int i = 0; result && i < 5; ++i) {
    RcHandle<TimingChecker> checker = make_rch<TimingChecker>();
    const MonotonicTimePoint start = MonotonicTimePoint::now();
    reactor_task.get_reactor()->schedule_timer(checker.in(), 0, requested.value());
    const MonotonicTimePoint stop = checker->wait_timeout();
    const TimeDuration elapsed = stop - start;
    if (elapsed < requested) {
      const TimeDuration delta = requested - elapsed;
      ACE_DEBUG((LM_DEBUG, "Checking that timer with requested delay of %C falls within epsilon value of %C: requested - elapsed = %C (%C)\n",
        requested.str().c_str(), epsilon.str().c_str(), delta.str().c_str(), delta < epsilon ? "PASS" : "FAIL"));
      result = result && delta < epsilon;
    } else {
      const TimeDuration delta = elapsed - requested;
      ACE_DEBUG((LM_DEBUG, "Checking that timer with requested delay of %C falls within epsilon value of %C: elapsed - requested = %C (%C)\n",
        requested.str().c_str(), epsilon.str().c_str(), delta.str().c_str(), delta < epsilon ? "PASS" : "FAIL"));
      result = result && delta < epsilon;
    }
  }

  reactor_task.stop();

  return result;
}

}
