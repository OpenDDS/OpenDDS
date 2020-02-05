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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MultiTask : public RcEventHandler {
public:
  explicit MultiTask(RcHandle<ReactorInterceptor> interceptor, const TimeDuration& period)
    : interceptor_(interceptor)
    , period_(period)
    , timer_(-1)
    , next_time_()
    , cancel_estimate_()
  {
    reactor(interceptor->reactor());
  }

  virtual ~MultiTask() {}

  void enable(const TimeDuration& period)
  {
    interceptor_->execute_or_enqueue(new ScheduleEnableCommand(this, period));
  }

  void disable()
  {
    interceptor_->execute_or_enqueue(new ScheduleDisableCommand(this));
  }

  void disable_and_wait()
  {
    ReactorInterceptor::CommandPtr command = interceptor_->execute_or_enqueue(new ScheduleDisableCommand(this));
    command->wait();
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  RcHandle<ReactorInterceptor> interceptor_;
  const TimeDuration period_;
  long timer_;
  MonotonicTimePoint next_time_;
  TimeDuration cancel_estimate_;
  mutable ACE_Thread_Mutex mutex_;

  struct ScheduleEnableCommand : public ReactorInterceptor::Command {
    ScheduleEnableCommand(MultiTask* multi_task, const TimeDuration& period)
      : multi_task_(multi_task), period_(period)
    { }

    virtual void execute()
    {
      multi_task_->enable_i(period_);
    }

    MultiTask* const multi_task_;
    const TimeDuration period_;
  };

  struct ScheduleDisableCommand : public ReactorInterceptor::Command {
    explicit ScheduleDisableCommand(MultiTask* multi_task)
      : multi_task_(multi_task)
    { }

    virtual void execute()
    {
      multi_task_->disable_i();
    }

    MultiTask* const multi_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    const MonotonicTimePoint now(tv);
    execute(now);
    return 0;
  }

  void enable_i(const TimeDuration& per)
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    const MonotonicTimePoint now = MonotonicTimePoint::now();
    if (timer_ == -1) {
      timer_ = reactor()->schedule_timer(this, 0, per.value(), period_.value());

      if (timer_ == -1) {
        ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable"
                   " failed to schedule timer %p\n", ACE_TEXT("")));
      } else {
        next_time_ = now + per;
      }
    } else {
      while (next_time_ < now) {
        next_time_ = next_time_ + period_;
      }
      if ((now + per) < (next_time_ + cancel_estimate_)) {
        reactor()->cancel_timer(timer_);
        const MonotonicTimePoint now2 = MonotonicTimePoint::now();
        timer_ = reactor()->schedule_timer(this, 0, per.value(), period_.value());

        if (timer_ == -1) {
          ACE_ERROR((LM_ERROR, "(%P|%t) MultiTask::enable"
                     " failed to reschedule timer %p\n", ACE_TEXT("")));
        } else {
          next_time_ = now2 + per;
          cancel_estimate_ = now2 - now;
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

  PmfMultiTask(RcHandle<ReactorInterceptor> interceptor, const TimeDuration& period, Delegate& delegate, PMF function)
    : MultiTask(interceptor, period)
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
