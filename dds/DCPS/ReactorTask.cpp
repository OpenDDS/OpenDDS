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
#include "ThreadMonitor.h"
#include "ThreadStatusManager.h"

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
  : state_(STATE_NOT_RUNNING)
  , condition_(lock_)
  , reactor_(0)
  , reactor_owner_(ACE_OS::NULL_thread)
  , proactor_(0)
#ifdef OPENDDS_REACTOR_TASK_ASYNC
  , use_async_send_(useAsyncSend)
#endif
  , timer_queue_(0)
  , thread_status_manager_(0)
  , timeout_(TimeDuration(0))
{
  ACE_UNUSED_ARG(useAsyncSend);
}

ReactorTask::~ReactorTask()
{
  cleanup();
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

int ReactorTask::open_reactor_task(void*, TimeDuration timeout,
  ThreadStatusManager* thread_status_manager, const String& name)
{
  GuardType guard(lock_);

  // If we've already been opened, let's clean up the old stuff
  cleanup();

  // thread status reporting support
  timeout_ = timeout;
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
    condition_.wait();
  }

  return 0;
}

int ReactorTask::svc()
{
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

    interceptor_ = make_rch<Interceptor>(this);

    // Advance the state.
    state_ = STATE_RUNNING;
    condition_.notify_all();
  }

  const bool update_thread_status = thread_status_manager_ && !timeout_.is_zero();
  const String thread_key = ThreadStatusManager::get_key("ReactorTask", name_);

  try {
    // Tell the reactor to handle events.
    if (update_thread_status) {
      while (state_ == STATE_RUNNING) {
        ACE_Time_Value t = timeout_.value();
        reactor_->run_reactor_event_loop(t, 0);
        if (DCPS_debug_level > 4) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) ReactorTask::svc. Updating thread status.\n"));
        }
        if (!thread_status_manager_->update(thread_key) && DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ReactorTask::svc: updated failed\n"));
        }
      }

    } else {
      reactor_->run_reactor_event_loop();
    }
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: ReactorTask::svc caught exception - %C.\n",
               e.what()));
    throw;
  } catch (...) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: ReactorTask::svc caught exception.\n"));
    throw;
  }

  if (update_thread_status) {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) ReactorTask::svc: "
        "Updating thread status for the last time\n"));
    }
    if (!thread_status_manager_->update(thread_key, ThreadStatus_Finished) &&
        DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ReactorTask::svc: final update failed\n"));
    }
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

  {
    GuardType guard(lock_);

    if (state_ == STATE_NOT_RUNNING) {
      // We are already "stopped".  Just return.
      return;
    }

    state_ = STATE_NOT_RUNNING;

#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
    // Remove the proactor handler so the reactor stops forwarding messages.
    if (proactor_) {
      reactor_->remove_handler(
        proactor_->implementation()->get_handle(),
        ACE_Event_Handler::DONT_CALL);
    }
#endif

    reactor_->end_reactor_event_loop();

    // Let's wait for the reactor task's thread to complete before we
    // leave this stop method.
    wait();

    // Reset the thread manager in case it goes away before the next open.
    this->thr_mgr(0);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
