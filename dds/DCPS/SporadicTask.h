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
#include "Service_Participant.h"

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
    } else if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: SporadicTask::schedule: "
                 "failed to receive ReactorInterceptor handle\n"));
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
    } else if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: SporadicTask::cancel: "
                 "failed to receive ReactorInterceptor handle\n"));
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
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

    if ((!desired_scheduled_ && actual_scheduled_) ||
        (desired_scheduled_ && actual_scheduled_ && desired_next_time_ != actual_next_time_)) {
      reactor()->cancel_timer(this);
      actual_scheduled_ = false;
    }

    if (desired_scheduled_ && !actual_scheduled_) {
      const long timer = reactor()->schedule_timer(this, 0, desired_delay_.value());
      if (timer == -1) {
        if (log_level >= LogLevel::Error) {
          ACE_ERROR((LM_ERROR,
                     "(%P|%t) ERROR: SporadicTask::execute_i: "
                     "failed to schedule timer %p\n", ""));
        }
      } else {
        actual_scheduled_ = true;
        actual_next_time_ = desired_next_time_;
      }
    }
  }

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

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
                  RcHandle<Delegate> delegate,
                  PMF function)
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
