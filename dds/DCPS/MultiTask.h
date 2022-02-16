/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTI_TASK_H
#define OPENDDS_DCPS_MULTI_TASK_H

#include "RcEventHandler.h"
#include "ReactorInterceptor.h"
#include "TimeTypes.h"
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MultiTask : public RcEventHandler {
public:
  explicit MultiTask(RcHandle<ReactorInterceptor> interceptor, const TimeDuration& delay)
    : interceptor_(interceptor)
    , delay_(delay)
    , timer_(-1)
    , next_time_()
    , cancel_estimate_()
  {
    reactor(interceptor->reactor());
  }

  virtual ~MultiTask() {}

  void enable(const TimeDuration& delay)
  {
    bool worth_passing_along = false;
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      worth_passing_along = (timer_ == -1) || ((MonotonicTimePoint::now() + delay + cancel_estimate_) < next_time_);
    }
    if (worth_passing_along) {
      RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
      if (interceptor) {
        interceptor->execute_or_enqueue(make_rch<ScheduleEnableCommand>(rchandle_from(this), delay));
      }
    }
  }

  void disable()
  {
    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      interceptor->execute_or_enqueue(make_rch<ScheduleDisableCommand>(rchandle_from(this)));
    }
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  WeakRcHandle<ReactorInterceptor> interceptor_;
  const TimeDuration delay_;
  long timer_;
  MonotonicTimePoint next_time_;
  TimeDuration cancel_estimate_;
  mutable ACE_Thread_Mutex mutex_;

  struct ScheduleEnableCommand : public ReactorInterceptor::Command {
    ScheduleEnableCommand(WeakRcHandle<MultiTask> multi_task, const TimeDuration& delay)
      : multi_task_(multi_task), delay_(delay)
    { }

    virtual void execute()
    {
      RcHandle<MultiTask> multi_task = multi_task_.lock();
      if (multi_task) {
        multi_task->enable_i(delay_);
      }
    }

    WeakRcHandle<MultiTask> const multi_task_;
    const TimeDuration delay_;
  };

  struct ScheduleDisableCommand : public ReactorInterceptor::Command {
    explicit ScheduleDisableCommand(WeakRcHandle<MultiTask> multi_task)
      : multi_task_(multi_task)
    { }

    virtual void execute()
    {
      RcHandle<MultiTask> multi_task = multi_task_.lock();
      if (multi_task) {
        multi_task->disable_i();
      }
    }

    WeakRcHandle<MultiTask> const multi_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*)
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

  void enable_i(const TimeDuration& per)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const MonotonicTimePoint now = MonotonicTimePoint::now();
    if (timer_ == -1) {
      timer_ = reactor()->schedule_timer(this, 0, per.value(), delay_.value());

      if (timer_ == -1) {
        ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable"
                   " failed to schedule timer %p\n", ACE_TEXT("")));
      } else {
        next_time_ = now + per;
      }
    } else {
      const MonotonicTimePoint estimated_next_time = now + per + cancel_estimate_;
      if (estimated_next_time < next_time_) {
        reactor()->cancel_timer(timer_);
        const MonotonicTimePoint now2 = MonotonicTimePoint::now();
        timer_ = reactor()->schedule_timer(this, 0, per.value(), delay_.value());
        cancel_estimate_ = now2 - now;

        if (timer_ == -1) {
          ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable"
                     " failed to reschedule timer %p\n", ACE_TEXT("")));
        } else {
          next_time_ = now2 + per;
        }
      }
    }
  }

  void
  disable_i()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (timer_ != -1) {
      reactor()->cancel_timer(timer_);
      timer_ = -1;
    }
  }
};

template <typename Delegate>
class PmfMultiTask : public MultiTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfMultiTask(RcHandle<ReactorInterceptor> interceptor,
               const TimeDuration& delay,
               RcHandle<Delegate> delegate,
               PMF function)
    : MultiTask(interceptor, delay)
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

#endif /* OPENDDS_DCPS_MULTI_TASK_H  */
