/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "PeriodicTask.h"

#include "Timers.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void PeriodicTask::enable(bool reenable, const TimeDuration& period)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    user_enabled_ = true;
  }
  ReactorTask_rch reactor_task = reactor_task_.lock();
  if (reactor_task) {
    reactor_task->execute_or_enqueue(make_rch<ScheduleEnableCommand>(rchandle_from(this), reenable, period));
  }
}

void PeriodicTask::disable()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    user_enabled_ = false;
  }
  ReactorTask_rch reactor_task = reactor_task_.lock();
  if (reactor_task) {
    reactor_task->execute_or_enqueue(make_rch<ScheduleDisableCommand>(rchandle_from(this)));
  }
}

void PeriodicTask::enable_i(bool reenable, const TimeDuration& per)
{
  if (!enabled_) {
    timer_ = Timers::schedule(reactor(), *this, 0, TimeDuration(), per);
    if (timer_ == Timers::InvalidTimerId) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) PeriodicTask::enable_i: "
                   "failed to schedule timer %p\n", ACE_TEXT("")));
      }
    } else {
      enabled_ = true;
    }
  } else if (reenable) {
    disable_i();
    enable_i(false, per);
  }
}

void PeriodicTask::disable_i()
{
  if (enabled_) {
    Timers::cancel(reactor(), timer_);
    enabled_ = false;
  }
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL
