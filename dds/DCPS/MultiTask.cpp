/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "MultiTask.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void MultiTask::enable(const TimeDuration& delay)
{
  bool worth_passing_along = false;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    worth_passing_along = (timer_ == ReactorWrapper::InvalidTimerId) ||
      ((MonotonicTimePoint::now() + delay + cancel_estimate_) < next_time_);
  }
  if (worth_passing_along) {
    ReactorTask_rch reactor_task= reactor_task_.lock();
    if (reactor_task) {
      reactor_task->execute_or_enqueue(make_rch<ScheduleEnableCommand>(rchandle_from(this), delay));
    }
  }
}

void MultiTask::enable_i(const TimeDuration& per,
                         ReactorWrapper& reactor_wrapper)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  if (timer_ == ReactorWrapper::InvalidTimerId) {
    timer_ = reactor_wrapper.schedule(*this, 0, per, delay_);

    if (timer_ == ReactorWrapper::InvalidTimerId) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable_i: "
                   "failed to schedule timer %p\n", ACE_TEXT("")));
      }
    } else {
      next_time_ = now + per;
    }
  } else {
    const MonotonicTimePoint estimated_next_time = now + per + cancel_estimate_;
    if (estimated_next_time < next_time_) {
      reactor_wrapper.cancel(timer_);
      const MonotonicTimePoint now2 = MonotonicTimePoint::now();
      timer_ = reactor_wrapper.schedule(*this, 0, per, delay_);
      cancel_estimate_ = now2 - now;

      if (timer_ == ReactorWrapper::InvalidTimerId) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable_i: "
                     "failed to reschedule timer %p\n", ACE_TEXT("")));
        }
      } else {
        next_time_ = now2 + per;
      }
    }
  }
}

int MultiTask::handle_timeout(const ACE_Time_Value& tv, const void*)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  const MonotonicTimePoint now(tv);
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    next_time_ = now + delay_;
  }
  execute(now);
  return 0;
}

void MultiTask::disable_i(ReactorWrapper& reactor_wrapper)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  if (timer_ != ReactorWrapper::InvalidTimerId) {
    reactor_wrapper.cancel(timer_);
    timer_ = ReactorWrapper::InvalidTimerId;
  }
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
