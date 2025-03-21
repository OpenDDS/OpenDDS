/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "SporadicTask.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void SporadicTask::schedule(const TimeDuration& delay)
{
  schedule_i(time_source_.monotonic_time_point_now() + delay, delay);
}

void SporadicTask::schedule_max(const MonotonicTimePoint& release_time,
                                const TimeDuration& minimum_delay)
{
  const MonotonicTimePoint now = time_source_.monotonic_time_point_now();
  const MonotonicTimePoint now_delay = now + minimum_delay;
  if (now_delay > release_time) {
    schedule_i(now_delay, minimum_delay);
  } else {
    schedule_i(release_time, release_time - now);
  }
}

void SporadicTask::schedule_i(const MonotonicTimePoint& next_time,
                              const TimeDuration& delay)
{
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
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: SporadicTask::schedule_i: "
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

void SporadicTask::update_schedule(ReactorWrapper& reactor_wrapper)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  if ((!desired_scheduled_ && timer_id_ != ReactorWrapper::InvalidTimerId) ||
      (desired_scheduled_ && timer_id_ != ReactorWrapper::InvalidTimerId && desired_next_time_ != actual_next_time_)) {
    reactor_wrapper.cancel(timer_id_);
    timer_id_ = ReactorWrapper::InvalidTimerId;
  }

  if (desired_scheduled_ && timer_id_ == ReactorWrapper::InvalidTimerId) {
    timer_id_ = reactor_wrapper.schedule(*this, 0, desired_delay_);
    if (timer_id_ == ReactorWrapper::InvalidTimerId) {
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
