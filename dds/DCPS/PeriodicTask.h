/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PERIODIC_TASK_H
#define OPENDDS_DCPS_PERIODIC_TASK_H

#include "Service_Participant.h"
#include "RcEventHandler.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export PeriodicTask : public virtual RcEventHandler {
public:
  explicit PeriodicTask(ReactorTask_rch reactor_task)
    : user_enabled_(false)
    , enabled_(false)
    , reactor_task_(reactor_task)
    , timer_(-1)
  {
    reactor(reactor_task->get_reactor());
  }

  virtual ~PeriodicTask() {}

  void enable(bool reenable, const TimeDuration& period);

  void disable();

  bool enabled() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
    return user_enabled_;
  }

  virtual void execute(const MonotonicTimePoint& now) = 0;

private:
  mutable ACE_Thread_Mutex mutex_;
  bool user_enabled_;
  bool enabled_;
  const ReactorTask_wrch reactor_task_;
  long timer_;

  struct ScheduleEnableCommand : public ReactorTask::Command {
    ScheduleEnableCommand(WeakRcHandle<PeriodicTask> hb, bool reenable, const TimeDuration& period)
      : periodic_task_(hb)
      , reenable_(reenable)
      , period_(period)
    {}

    virtual void execute(ACE_Reactor*)
    {
      RcHandle<PeriodicTask> periodic_task = periodic_task_.lock();
      if (periodic_task) {
        periodic_task->enable_i(reenable_, period_);
      }
    }

    const WeakRcHandle<PeriodicTask> periodic_task_;
    const bool reenable_;
    const TimeDuration period_;
  };

  struct ScheduleDisableCommand : public ReactorTask::Command {
    explicit ScheduleDisableCommand(WeakRcHandle<PeriodicTask> hb)
      : periodic_task_(hb)
    {}

    virtual void execute(ACE_Reactor*)
    {
      RcHandle<PeriodicTask> periodic_task = periodic_task_.lock();
      if (periodic_task) {
        periodic_task->disable_i();
      }
    }

    const WeakRcHandle<PeriodicTask> periodic_task_;
  };

  int handle_timeout(const ACE_Time_Value& tv, const void*)
  {
    ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

    const MonotonicTimePoint now(tv);
    execute(now);
    return 0;
  }

  void enable_i(bool reenable, const TimeDuration& per);

  void disable_i();
};

template <typename Delegate>
class PmfPeriodicTask : public PeriodicTask {
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfPeriodicTask(ReactorTask_rch reactor_task, const Delegate& delegate, PMF function)
    : PeriodicTask(reactor_task)
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

#endif /* OPENDDS_DCPS_PERIODIC_TASK_H  */
