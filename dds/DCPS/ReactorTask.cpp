/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ReactorTask.h"
#if !defined (__ACE_INLINE__)
#include "ReactorTask.inl"
#endif /* __ACE_INLINE__ */

#include "Service_Participant.h"

#include <ace/Select_Reactor.h>
#include <ace/WFMO_Reactor.h>
#include <ace/Proactor.h>
#include <ace/Proactor_Impl.h>
#include <ace/WIN32_Proactor.h>
#include <ace/OS_NS_Thread.h>

#include <exception>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ReactorTask::ReactorTask(bool useAsyncSend)
  : condition_(lock_)
  , state_(STATE_UNINITIALIZED)
  , reactor_(0)
  , reactor_owner_(ACE_OS::NULL_thread)
  , proactor_(0)
#ifdef OPENDDS_REACTOR_TASK_ASYNC
  , use_async_send_(useAsyncSend)
#endif
  , timer_queue_(0)
  , thread_status_manager_(0)
{
  ACE_UNUSED_ARG(useAsyncSend);
}

ReactorTask::~ReactorTask()
{
  cleanup();
}

void ReactorTask::wait_for_startup_i() const
{
  while (state_ == STATE_UNINITIALIZED || state_ == STATE_OPENING) {
    condition_.wait(thread_status_manager_ ?
                      *thread_status_manager_ :
                      TheServiceParticipant->get_thread_status_manager());
  }
}

void ReactorTask::cleanup()
{
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  if (proactor_) {
    reactor_->remove_handler(
      proactor_->implementation()->get_handle(),
      ACE_Event_Handler::DONT_CALL);
    delete proactor_;
    proactor_ = 0;
  }
#endif

  delete reactor_;
  reactor_ = 0;
  delete timer_queue_;
  timer_queue_ = 0;
}

int ReactorTask::open_reactor_task(void*,
                                   ThreadStatusManager* thread_status_manager,
                                   const String& name)
{
  GuardType guard(lock_);

  // If we've already been opened, let's clean up the old stuff
  cleanup();

  // thread status reporting support
  thread_status_manager_ = thread_status_manager;
  name_ = name;

  // Set our reactor and proactor pointers to a new reactor/proactor objects.
#ifdef OPENDDS_REACTOR_TASK_ASYNC
  if (use_async_send_ && !reactor_) {
    reactor_ = new ACE_Reactor(new ACE_WFMO_Reactor, 1);

    ACE_WIN32_Proactor* proactor_impl = new ACE_WIN32_Proactor(0, 1);
    proactor_ = new ACE_Proactor(proactor_impl, 1);
    reactor_->register_handler(proactor_impl, proactor_impl->get_handle());
  } else
#endif
  if (!reactor_) {
    reactor_ = new ACE_Reactor(new ACE_Select_Reactor, true);
    proactor_ = 0;
  }

  if (!timer_queue_) {
    timer_queue_ = new TimerQueueType();
    reactor_->timer_queue(timer_queue_);
  }

  state_ = STATE_OPENING;
  condition_.notify_all();

  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: ReactorTask Failed to activate "
                      "itself.\n"),
                     -1);
  }

  while (state_ != STATE_RUNNING) {
    condition_.wait(*thread_status_manager);
  }

  return 0;
}

int ReactorTask::svc()
{
  ThreadStatusManager::Start s(*thread_status_manager_, name_);

  {
    GuardType guard(lock_);

    // First off - We need to obtain our own reference to ourselves such
    // that we don't get deleted while still running in our own thread.
    // In essence, our current thread "owns" a copy of our reference.
    // It's all done with the magic of intrusive reference counting!
    _add_ref();

    // Ignore all signals to avoid
    //     ERROR: <something descriptive> Interrupted system call
    // The main thread will handle signals.
    sigset_t set;
    ACE_OS::sigfillset(&set);
    ACE_OS::thr_sigsetmask(SIG_SETMASK, &set, NULL);

    // Tell the reactor that this thread will be its owner
    if (reactor_->owner(ACE_Thread_Manager::instance()->thr_self()) != 0) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: Failed to change the reactor's owner().\n"));
    }
    reactor_owner_ = ACE_Thread_Manager::instance()->thr_self();

    interceptor_ = make_rch<Interceptor>(this, reactor_, reactor_owner_);

    // Advance the state.
    state_ = STATE_RUNNING;
    condition_.notify_all();
  }

  // Tell the reactor to handle events.
  if (thread_status_manager_->update_thread_status()) {
    while (state_ == STATE_RUNNING) {
      ACE_Time_Value t = thread_status_manager_->thread_status_interval().value();
      ThreadStatusManager::Sleeper sleeper(*thread_status_manager_);
      reactor_->run_reactor_event_loop(t, 0);
    }

  } else {
    reactor_->run_reactor_event_loop();
  }

  return 0;
}

int ReactorTask::close(u_long flags)
{
  ACE_UNUSED_ARG(flags);
  // This is called after the reactor threads exit.
  // We should not set state here since we are not
  // sure how many reactor threads we will use.
  // If there is one reactor thread then we should
  // set the state so the stop will not call
  // end_reactor_event_loop.
  // If there are multiple reactor threads, we still
  // need call end_reactor_event_loop in stop() while
  // one reactor thread already exited.
//MJM: Right.

  _remove_ref();
  return 0;
}

void ReactorTask::stop()
{
  ACE_Reactor* reactor = 0;
  {
    GuardType guard(lock_);

    if (state_ == STATE_UNINITIALIZED || state_ == STATE_SHUT_DOWN) {
      // We are already "stopped".  Just return.
      return;
    }

    state_ = STATE_SHUT_DOWN;

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
    // Remove the proactor handler so the reactor stops forwarding messages.
    if (proactor_) {
      reactor_->remove_handler(
        proactor_->implementation()->get_handle(),
        ACE_Event_Handler::DONT_CALL);
    }
#endif
    reactor = reactor_;
  }

  if (reactor) {
    // We can't hold the lock when we call this, because the reactor threads may need to
    // access the lock as part of normal execution before they return to the reactor control loop
    reactor->end_reactor_event_loop();
  }

  {
    GuardType guard(lock_);

    // In the future, we will likely want to replace this assert with a new "SHUTTING_DOWN" state
    // which can be used to delay any potential new calls to open_reactor_task()
    OPENDDS_ASSERT(state_ == STATE_SHUT_DOWN);

    // Let's wait for the reactor task's thread to complete before we
    // leave this stop method.
    if (thread_status_manager_) {
      ThreadStatusManager::Sleeper sleeper(*thread_status_manager_);
      wait();

      // Reset the thread manager in case it goes away before the next open.
      thr_mgr(0);
    }
  }
}

void ReactorTask::reactor(ACE_Reactor* reactor)
{
  ACE_Event_Handler::reactor(reactor);
}

ACE_Reactor* ReactorTask::reactor() const
{
  return ACE_Event_Handler::reactor();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
