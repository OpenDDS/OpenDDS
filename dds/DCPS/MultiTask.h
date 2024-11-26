/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTI_TASK_H
#define OPENDDS_DCPS_MULTI_TASK_H

#include "RcEventHandler.h"
#include "Service_Participant.h"
#include "TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MultiTask : public virtual RcEventHandler {
public:
  MultiTask(ReactorTask_rch reactor_task, const TimeDuration& delay)
    : reactor_task_(reactor_task)
    , delay_(delay)
    , timer_(-1)
    , next_time_()
    , cancel_estimate_()
  {
    reactor(reactor_task->get_reactor());
  }

  virtual ~MultiTask() {}

  void enable(const TimeDuration& delay);

  void disable()
  {
    ReactorTask_rch reactor_task = reactor_task_.lock();
    if (reactor_task) {
      reactor_task->execute_or_enqueue(make_rch<ScheduleDisableCommand>(rchandle_from(this)));
    }
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  const ReactorTask_wrch reactor_task_;
  const TimeDuration delay_;
  long timer_;
  MonotonicTimePoint next_time_;
  TimeDuration cancel_estimate_;
  mutable ACE_Thread_Mutex mutex_;

  struct ScheduleEnableCommand : public ReactorTask::Command {
    ScheduleEnableCommand(WeakRcHandle<MultiTask> multi_task, const TimeDuration& delay)
      : multi_task_(multi_task)
      , delay_(delay)
    {}

    virtual void execute(ACE_Reactor*)
    {
      RcHandle<MultiTask> multi_task = multi_task_.lock();
      if (multi_task) {
        multi_task->enable_i(delay_);
      }
    }

    const WeakRcHandle<MultiTask> multi_task_;
    const TimeDuration delay_;
  };

  struct ScheduleDisableCommand : public ReactorTask::Command {
    explicit ScheduleDisableCommand(WeakRcHandle<MultiTask> multi_task)
      : multi_task_(multi_task)
    {}

    virtual void execute(ACE_Reactor*)
    {
      RcHandle<MultiTask> multi_task = multi_task_.lock();
      if (multi_task) {
        multi_task->disable_i();
      }
    }

    const WeakRcHandle<MultiTask> multi_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*);

  void enable_i(const TimeDuration& per);

  void disable_i();
};

template <typename Delegate>
class PmfMultiTask : public MultiTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfMultiTask(ReactorTask_rch reactor_task,
               const TimeDuration& delay,
               RcHandle<Delegate> delegate,
               PMF function)
    : MultiTask(reactor_task, delay)
    , delegate_(delegate)
    , function_(function) {}

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

#endif /* OPENDDS_DCPS_MULTI_TASK_H  */
