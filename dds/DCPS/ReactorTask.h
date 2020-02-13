/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTORTASK_H
#define OPENDDS_DCPS_REACTORTASK_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/TimeTypes.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "ace/Task.h"
#include "ace/Barrier.h"
#include "ace/Synch_Traits.h"
#include "ace/Condition_T.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Timer_Heap_T.h"
#include "ace/Event_Handler_Handle_Timeout_Upcall.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Proactor;
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorTask : public virtual ACE_Task_Base,
public virtual RcObject {
public:

  explicit ReactorTask(bool useAsyncSend);
  virtual ~ReactorTask();

  virtual int open(void*);
  virtual int svc();
  virtual int close(u_long flags = 0);

  void stop();

  ACE_Reactor* get_reactor();
  const ACE_Reactor* get_reactor() const;

  ACE_thread_t get_reactor_owner() const;

  ACE_Proactor* get_proactor();
  const ACE_Proactor* get_proactor() const;

  void wait_for_startup() { barrier_.wait(); }

  bool is_shut_down() const { return state_ == STATE_NOT_RUNNING; }

  ReactorInterceptor_rch interceptor() const { return interceptor_; }

  OPENDDS_POOL_ALLOCATION_FWD

private:

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;
  typedef ACE_Timer_Heap_T<
    ACE_Event_Handler*, ACE_Event_Handler_Handle_Timeout_Upcall,
    ACE_SYNCH_RECURSIVE_MUTEX, MonotonicClock> TimerQueueType;

  enum State { STATE_NOT_RUNNING, STATE_OPENING, STATE_RUNNING };

  class Interceptor : public DCPS::ReactorInterceptor {
  public:
    explicit Interceptor(DCPS::ReactorTask* task)
     : ReactorInterceptor(task->get_reactor(), task->get_reactor_owner())
     , task_(task)
     {}
    bool reactor_is_shut_down() const
    {
      return task_->is_shut_down();
    }

  private:
    DCPS::ReactorTask* const task_;
  };

  ACE_Barrier   barrier_;
  LockType      lock_;
  State         state_;
  ConditionType condition_;
  ACE_Reactor*  reactor_;
  ACE_thread_t  reactor_owner_;
  ACE_Proactor* proactor_;
  bool          use_async_send_;
  TimerQueueType* timer_queue_;

  ReactorInterceptor_rch interceptor_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ReactorTask.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_REACTORTASK_H */
