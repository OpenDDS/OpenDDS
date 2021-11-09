/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SPORADIC_TASK_H
#define OPENDDS_DCPS_SPORADIC_TASK_H

#include "RcEventHandler.h"
#include "ReactorInterceptor.h"
#include "TimeSource.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SporadicTask : public RcEventHandler {
public:
  SporadicTask(const TimeSource& time_source,
               RcHandle<ReactorInterceptor> interceptor)
    : time_source_(time_source)
    , interceptor_(interceptor)
    , desired_scheduled_(false)
    , actual_scheduled_(false)
    , sporadic_command_(make_rch<SporadicCommand>(rchandle_from(this)))
  {
    reactor(interceptor->reactor());
  }

  virtual ~SporadicTask() {}

  void schedule(const TimeDuration& delay)
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

    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      interceptor->execute_or_enqueue(sporadic_command_);
    } else if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: SporadicTask::schedule")
                 ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
    }
  }

  void cancel()
  {
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      if (!desired_scheduled_) {
        return;
      }

      desired_scheduled_ = false;
    }

    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      interceptor->execute_or_enqueue(sporadic_command_);
    } else if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: SporadicTask::cancel")
                 ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
    }
  }

  void cancel_and_wait()
  {
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      if (!desired_scheduled_) {
        return;
      }

      desired_scheduled_ = false;
    }

    RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
    if (interceptor) {
      ReactorInterceptor::CommandPtr command = interceptor->execute_or_enqueue(sporadic_command_);
      if (command) {
        command->wait();
      }
    } else if (DCPS_debug_level >= 1) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: SporadicTask::cancel_and_wait")
                 ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
    }
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  struct SporadicCommand : public ReactorInterceptor::Command {
    explicit SporadicCommand(WeakRcHandle<SporadicTask> sporadic_task)
      : sporadic_task_(sporadic_task)
    { }

    virtual void execute()
    {
      RcHandle<SporadicTask> st = sporadic_task_.lock();
      if (st) {
        st->execute_i();
      }
    }

    WeakRcHandle<SporadicTask> sporadic_task_;
  };

  const TimeSource& time_source_;
  WeakRcHandle<ReactorInterceptor> interceptor_;
  bool desired_scheduled_;
  MonotonicTimePoint desired_next_time_;
  TimeDuration desired_delay_;
  bool actual_scheduled_;
  MonotonicTimePoint actual_next_time_;
  RcHandle<SporadicCommand> sporadic_command_;
  mutable ACE_Thread_Mutex mutex_;

  void execute_i()
  {
    // Bump up our refcount so the Reactor doesn't delete us when canceling the timer.
    RcHandle<SporadicTask> x = rchandle_from(this);

    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

    if ((!desired_scheduled_ && actual_scheduled_) ||
        (desired_scheduled_ && actual_scheduled_ && desired_next_time_ != actual_next_time_)) {
      reactor()->cancel_timer(this);
      actual_scheduled_ = false;
    }

    if (desired_scheduled_ && !actual_scheduled_) {
      const long timer = reactor()->schedule_timer(this, 0, desired_delay_.value());
      if (timer == -1) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: SporadicTask::execute_i")
                   ACE_TEXT(" failed to schedule timer %p\n"),
                   ACE_TEXT("")));
      } else {
        actual_scheduled_ = true;
        actual_next_time_ = desired_next_time_;
      }
    }
  }

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    const MonotonicTimePoint now(tv);
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      desired_scheduled_ = false;
      actual_scheduled_ = false;
    }
    execute(now);
    return 0;
  }
};

template <typename Delegate>
class PmfSporadicTask : public SporadicTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfSporadicTask(const TimeSource& time_source,
                  RcHandle<ReactorInterceptor> interceptor,
                  const Delegate& delegate, PMF function)
    : SporadicTask(time_source, interceptor)
    , delegate_(delegate)
    , function_(function)
  {}

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

#endif /* OPENDDS_DCPS_SPORADIC_TASK_H  */
