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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SporadicTask : public RcEventHandler {
public:
  explicit SporadicTask(RcHandle<ReactorInterceptor> interceptor)
    : interceptor_(interceptor)
    , scheduled_(false)
  {
    reactor(interceptor->reactor());
  }

  virtual ~SporadicTask() {}

  void schedule(const TimeDuration& delay)
  {
    if (interceptor_) {
      RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
      if (interceptor) {
        interceptor->execute_or_enqueue(new ScheduleCommand(this, delay));
      }
      else if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: SporadicTask::schedule")
          ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
      }
    }
  }

  void cancel()
  {
    if (interceptor_) {
      RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
      if (interceptor) {
        interceptor->execute_or_enqueue(new CancelCommand(this));
      }
      else if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: SporadicTask::cancel")
          ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
      }
    }
  }

  void cancel_and_wait()
  {
    if (interceptor_) {
      RcHandle<ReactorInterceptor> interceptor = interceptor_.lock();
      if (interceptor) {
        ReactorInterceptor::CommandPtr command = interceptor->execute_or_enqueue(new CancelCommand(this));
        command->wait();
      }
      else if (DCPS_debug_level >= 1) {
        ACE_ERROR((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: SporadicTask::cancel_and_wait")
          ACE_TEXT(" failed to receive ReactorInterceptor handle\n")));
      }
    }
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  WeakRcHandle<ReactorInterceptor> interceptor_;
  bool scheduled_;

  struct ScheduleCommand : public ReactorInterceptor::Command {
    ScheduleCommand(SporadicTask* hb, const TimeDuration& delay)
      : sporadic_task_(hb), delay_(delay)
    { }

    virtual void execute()
    {
      sporadic_task_->schedule_i(delay_);
    }

    SporadicTask* const sporadic_task_;
    const TimeDuration delay_;
  };

  struct CancelCommand : public ReactorInterceptor::Command {
    CancelCommand(SporadicTask* hb)
      : sporadic_task_(hb)
    { }

    virtual void execute()
    {
      sporadic_task_->cancel_i();
    }

    SporadicTask* const sporadic_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    const MonotonicTimePoint now(tv);
    scheduled_ = false;
    execute(now);
    return 0;
  }

  void schedule_i(const TimeDuration& delay)
  {
    if (!scheduled_ && reactor()) {
      const long timer = reactor()->schedule_timer(this, 0, delay.value());

      if (timer == -1) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: SporadicTask::enable")
                   ACE_TEXT(" failed to schedule timer %p\n"),
                   ACE_TEXT("")));
      } else {
        scheduled_ = true;
      }
    }
  }

  void
  cancel_i()
  {
    if (scheduled_ && reactor()) {
      reactor()->cancel_timer(this);
      scheduled_ = false;
    }
  }
};

template <typename Delegate>
class PmfSporadicTask : public SporadicTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfSporadicTask(RcHandle<ReactorInterceptor> interceptor, Delegate& delegate, PMF function)
    : SporadicTask(interceptor)
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
