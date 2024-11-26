/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "SporadicTask.h"

#include "Timers.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void SporadicTask::schedule(const TimeDuration& delay)
{
  const MonotonicTimePoint next_time = time_source_.monotonic_time_point_now() + delay;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (!desired_scheduled_ || next_time < desired_next_time_) {
      desired_scheduled_ = true;
      desired_next_time_ = next_time;
      desired_delay_ = delay;
    } else {
      return;
    }
  }

  const ReactorTask_rch reactor_task = reactor_task_.lock();
  if (reactor_task) {
    reactor_task->execute_or_enqueue(sporadic_command_);
  } else if (log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SporadicTask::schedule: "
               "failed to receive ReactorTask handle\n"));
  }
}

void SporadicTask::cancel()
{
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (!desired_scheduled_) {
      return;
    }

    desired_scheduled_ = false;
  }

  const ReactorTask_rch reactor_task = reactor_task_.lock();
  if (reactor_task) {
    reactor_task->execute_or_enqueue(sporadic_command_);
  } else if (log_level >= LogLevel::Error) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SporadicTask::cancel: "
               "failed to receive ReactorTask handle\n"));
  }
}

void SporadicTask::update_schedule()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  if ((!desired_scheduled_ && timer_id_ != Timers::InvalidTimerId) ||
      (desired_scheduled_ && timer_id_ != Timers::InvalidTimerId && desired_next_time_ != actual_next_time_)) {
    Timers::cancel(reactor(), timer_id_);
    timer_id_ = Timers::InvalidTimerId;
  }

  if (desired_scheduled_ && timer_id_ == Timers::InvalidTimerId) {
    timer_id_ = Timers::schedule(reactor(), *this, 0, desired_delay_);
    if (timer_id_ == Timers::InvalidTimerId) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: SporadicTask::execute_i: "
                   "failed to schedule timer %p\n", ACE_TEXT("")));
      }
    } else {
      actual_next_time_ = desired_next_time_;
    }
  }
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
