/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTORTASK_H
#define OPENDDS_DCPS_REACTORTASK_H

#include "ConditionVariable.h"
#include "RcObject.h"
#include "SafetyProfileStreams.h"
#include "ThreadStatusManager.h"
#include "TimeTypes.h"
#include "dcps_export.h"

#include <ace/Task.h>
#include <ace/Synch_Traits.h>
#include <ace/Timer_Heap_T.h>
#include <ace/Event_Handler_Handle_Timeout_Upcall.h>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Proactor;
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReactorTask
  : public virtual ACE_Task_Base
  , public virtual RcObject
{
public:
  // Takes ownership of the reactor.
  explicit ReactorTask(bool useAsyncSend);
  virtual ~ReactorTask();

  int open_reactor_task(ThreadStatusManager* thread_status_manager = 0,
                        const String& name = "",
                        ACE_Reactor* reactor = 0);

  virtual int open(void*) { return open_reactor_task(); }
  virtual int svc();
  virtual int close(u_long flags = 0);

  void stop();

  bool on_thread() const;

  ACE_Reactor* get_reactor();
  const ACE_Reactor* get_reactor() const;

  ACE_Proactor* get_proactor();
  const ACE_Proactor* get_proactor() const;

  class OpenDDS_Dcps_Export Command
    : public virtual RcObject {
  public:
    Command() { }
    virtual ~Command() { }

    virtual void execute(ACE_Reactor* reactor) = 0;
  };
  typedef RcHandle<Command> CommandPtr;

  CommandPtr execute_or_enqueue(CommandPtr command);
  void wait_until_empty();

  OPENDDS_POOL_ALLOCATION_FWD

private:
  virtual void reactor(ACE_Reactor* reactor);
  virtual ACE_Reactor* reactor() const;

  void cleanup();
  void wait_for_startup_i() const;

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;
  typedef ConditionVariable<LockType> ConditionVariableType;
  typedef ACE_Timer_Heap_T<
    ACE_Event_Handler*, ACE_Event_Handler_Handle_Timeout_Upcall,
    ACE_SYNCH_RECURSIVE_MUTEX, MonotonicClock> TimerQueueType;

  mutable LockType lock_;
  mutable ConditionVariableType condition_;
  enum State { STATE_UNINITIALIZED, STATE_OPENING, STATE_RUNNING, STATE_SHUT_DOWN };
  State state_;
  ACE_Reactor* reactor_;
  ACE_thread_t reactor_owner_;
  ACE_Proactor* proactor_;

#if defined ACE_WIN32 && defined ACE_HAS_WIN32_OVERLAPPED_IO
#define OPENDDS_REACTOR_TASK_ASYNC
  bool use_async_send_;
#endif

  TimerQueueType* timer_queue_;

  typedef OPENDDS_VECTOR(CommandPtr) Queue;
  Queue command_queue_;

  enum ReactorState {
    RS_NONE,
    RS_NOTIFIED,
    RS_PROCESSING
  };
  ReactorState reactor_state_;

  int handle_exception(ACE_HANDLE /*fd*/);
  void process_command_queue_i(ACE_Guard<ACE_Thread_Mutex>& guard,
                               ACE_Reactor* reactor);

  // thread status reporting
  String name_;
  ThreadStatusManager* thread_status_manager_;
};

class OpenDDS_Dcps_Export RegisterHandler : public ReactorTask::Command {
public:
  RegisterHandler(ACE_HANDLE io_handle,
                  ACE_Event_Handler* event_handler,
                  ACE_Reactor_Mask mask)
    : io_handle_(io_handle)
    , event_handler_(event_handler)
    , mask_(mask)
  {}

private:
  ACE_HANDLE io_handle_;
  ACE_Event_Handler* event_handler_;
  ACE_Reactor_Mask mask_;

  void execute(ACE_Reactor* reactor);
};

class OpenDDS_Dcps_Export RemoveHandler : public ReactorTask::Command {
public:
  RemoveHandler(ACE_HANDLE io_handle,
                ACE_Reactor_Mask mask)
    : io_handle_(io_handle)
    , mask_(mask)
  {}

private:
  ACE_HANDLE io_handle_;
  ACE_Reactor_Mask mask_;

  void execute(ACE_Reactor* reactor);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ReactorTask.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_REACTORTASK_H */
