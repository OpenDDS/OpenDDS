/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SPORADIC_TASK_H
#define OPENDDS_DCPS_SPORADIC_TASK_H

#include "RcEventHandler.h"
#include "ReactorTask_rch.h"
#include "Service_Participant.h"
#include "TimeSource.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export SporadicTask : public virtual RcEventHandler {
public:
  SporadicTask(const TimeSource& time_source,
               ReactorTask_rch reactor_task)
    : time_source_(time_source)
    , reactor_task_(reactor_task)
    , desired_scheduled_(false)
    , timer_id_(-1)
    , sporadic_command_(make_rch<SporadicCommand>(rchandle_from(this)))
  {
    reactor(reactor_task->get_reactor());
  }

  virtual ~SporadicTask() {}

  void schedule(const TimeDuration& delay);

  void cancel();

  virtual void execute(const MonotonicTimePoint& now) = 0;

protected:
  long get_timer_id() const { return timer_id_; }

private:
  struct SporadicCommand : ReactorTask::Command {
    explicit SporadicCommand(WeakRcHandle<SporadicTask> sporadic_task)
      : sporadic_task_(sporadic_task)
    {}

    virtual void execute(ACE_Reactor*)
    {
      RcHandle<SporadicTask> st = sporadic_task_.lock();
      if (st) {
        st->update_schedule();
      }
    }

    const WeakRcHandle<SporadicTask> sporadic_task_;
  };

  const TimeSource& time_source_;
  const ReactorTask_wrch reactor_task_;
  bool desired_scheduled_;
  MonotonicTimePoint desired_next_time_;
  TimeDuration desired_delay_;
  long timer_id_;
  MonotonicTimePoint actual_next_time_;
  const RcHandle<SporadicCommand> sporadic_command_;
  mutable ACE_Thread_Mutex mutex_;

  void update_schedule();

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

    const MonotonicTimePoint now(tv);
    {
      ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
      desired_scheduled_ = false;
      timer_id_ = -1;
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
                  ReactorTask_rch reactor_task,
                  RcHandle<Delegate> delegate,
                  PMF function)
    : SporadicTask(time_source, reactor_task)
    , delegate_(delegate)
    , function_(function)
  {}

private:
  const WeakRcHandle<Delegate> delegate_;
  const PMF function_;

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
