/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PERIODIC_TASK_H
#define OPENDDS_DCPS_PERIODIC_TASK_H

#include "RcEventHandler.h"
#include "ReactorInterceptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PeriodicTask : public RcEventHandler {
public:
  explicit PeriodicTask(RcHandle<ReactorInterceptor> interceptor)
    : user_enabled_(false)
    , interceptor_(interceptor)
    , enabled_(false)
  {
    reactor(interceptor->reactor());
  }

  virtual ~PeriodicTask() {}

  void enable(bool reenable, const TimeDuration& period)
  {
    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      user_enabled_ = true;
    }
    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      interceptor->execute_or_enqueue(make_rch<ScheduleEnableCommand>(rchandle_from(this), reenable, period));
    }
  }

  void disable()
  {
    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      user_enabled_ = false;
    }
    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      interceptor->execute_or_enqueue(make_rch<ScheduleDisableCommand>(rchandle_from(this)));
    }
  }

  bool enabled() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
    return user_enabled_;
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  mutable ACE_Thread_Mutex mutex_;
  bool user_enabled_;
  WeakRcHandle<ReactorInterceptor> interceptor_;
  bool enabled_;

  struct ScheduleEnableCommand : public ReactorInterceptor::Command {
    ScheduleEnableCommand(WeakRcHandle<PeriodicTask> hb, bool reenable, const TimeDuration& period)
      : periodic_task_(hb), reenable_(reenable), period_(period)
    { }

    virtual void execute()
    {
      RcHandle<PeriodicTask> periodic_task = periodic_task_.lock();
      if (periodic_task) {
        periodic_task->enable_i(reenable_, period_);
      }
    }

    WeakRcHandle<PeriodicTask> const periodic_task_;
    const bool reenable_;
    const TimeDuration period_;
  };

  struct ScheduleDisableCommand : public ReactorInterceptor::Command {
    explicit ScheduleDisableCommand(WeakRcHandle<PeriodicTask> hb)
      : periodic_task_(hb)
    { }

    virtual void execute()
    {
      RcHandle<PeriodicTask> periodic_task = periodic_task_.lock();
      if (periodic_task) {
        periodic_task->disable_i();
      }
    }

    WeakRcHandle<PeriodicTask> const periodic_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

    const MonotonicTimePoint now(tv);
    execute(now);
    return 0;
  }

  void enable_i(bool reenable, const TimeDuration& per)
  {
    if (!enabled_) {
      const long timer =
        reactor()->schedule_timer(this, 0, ACE_Time_Value::zero, per.value());

      if (timer == -1) {
        ACE_ERROR((LM_ERROR, "(%P|%t) PeriodicTask::enable"
                   " failed to schedule timer %p\n", ACE_TEXT("")));
      } else {
        enabled_ = true;
      }
    } else if (reenable) {
      disable_i();
      enable_i(false, per);
    }
  }

  void
  disable_i()
  {
    if (enabled_) {
      reactor()->cancel_timer(this);
      enabled_ = false;
    }
  }
};

template <typename Delegate>
class PmfPeriodicTask : public PeriodicTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfPeriodicTask(RcHandle<ReactorInterceptor> interceptor, const Delegate& delegate, PMF function)
    : PeriodicTask(interceptor)
    , delegate_(delegate)
    , function_(function) {}

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;

  void execute(const MonotonicTimePoint& now)
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)(now);
    }
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_PERIODIC_TASK_H  */
