/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReactorTask.h"

#if !defined (__ACE_INLINE__)
#include "ReactorTask.inl"
#endif /* __ACE_INLINE__ */

#include <ace/Select_Reactor.h>
#include <ace/WFMO_Reactor.h>
#include <ace/Proactor.h>
#include <ace/Proactor_Impl.h>
#include <ace/WIN32_Proactor.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::ReactorTask::ReactorTask(bool useAsyncSend)
  : barrier_(2)
  , state_(STATE_NOT_RUNNING)
  , condition_(lock_)
  , reactor_(0)
  , reactor_owner_(ACE_OS::NULL_thread)
  , proactor_(0)
  , use_async_send_(useAsyncSend)
  , timer_queue_(0)
{
}

OpenDDS::DCPS::ReactorTask::~ReactorTask()
{
#if defined (ACE_HAS_WIN32_OVERLAPPED_IO) || defined (ACE_HAS_AIO_CALLS)
  if (proactor_) {
    reactor_->remove_handler(
      proactor_->implementation()->get_handle(),
      ACE_Event_Handler::DONT_CALL);
    delete proactor_;
  }
#endif

  delete reactor_;
  delete timer_queue_;
}

int
OpenDDS::DCPS::ReactorTask::open(void*)
{

  // Set our reactor and proactor pointers to a new reactor/proactor objects.
  if (use_async_send_) {
#if defined (ACE_WIN32) && defined (ACE_HAS_WIN32_OVERLAPPED_IO)
    reactor_ = new ACE_Reactor(new ACE_WFMO_Reactor, 1);

    ACE_WIN32_Proactor* proactor_impl = new ACE_WIN32_Proactor(0, 1);
    proactor_ = new ACE_Proactor(proactor_impl, 1);
    reactor_->register_handler(proactor_impl, proactor_impl->get_handle());
#else
    reactor_ = new ACE_Reactor(new ACE_Select_Reactor, true);
    proactor_ = 0;
#endif
  } else {
    reactor_ = new ACE_Reactor(new ACE_Select_Reactor, true);
    proactor_ = 0;
  }

  timer_queue_ = new TimerQueueType();
  reactor_->timer_queue(timer_queue_);

  GuardType guard(lock_);

  // Reset our state.
  state_ = STATE_NOT_RUNNING;

  // For now, we only support one thread to run the reactor event loop.
  // Parts of the logic in this class would need to change to support
  // more than one thread running the reactor.

  // Attempt to activate ourselves.  If successful, a new thread will be
  // started and it will invoke our svc() method.  Note that we still have
  // a hold on our lock while we do this.
  if (activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: ReactorTask Failed to activate "
                      "itself.\n"),
                     -1);
  }

  wait_for_startup();

  // Here we need to wait until a condition is triggered by the new thread(s)
  // that we created.  Note that this will cause us to release the lock and
  // wait until the condition_ is signal()'ed.  When it is signaled, the
  // condition will attempt to obtain the lock again, and then return to us
  // here.  We can then go on.
  if (state_ == STATE_NOT_RUNNING) {
    state_ = STATE_OPENING;
    condition_.wait();
  }

  return 0;
}

int
OpenDDS::DCPS::ReactorTask::svc()
{
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

  // VERY IMPORTANT!Tell the reactor that this task's thread will be
  //                  its "owner".
  if (reactor_->owner(ACE_Thread_Manager::instance()->thr_self()) != 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: Failed to change the reactor's owner().\n"));
  }
  reactor_owner_ = ACE_Thread_Manager::instance()->thr_self();

  interceptor_ = make_rch<Interceptor>(this);

  wait_for_startup();

  {
    // Obtain the lock.  This should only happen once the open() has hit
    // the condition_.wait() line of code, and has released the lock.
    GuardType guard(lock_);

    if (state_ == STATE_OPENING) {
      // Advance the state.
      state_ = STATE_RUNNING;

      // Signal the condition_ that we are here.
      condition_.signal();
    }
  }

  //TBD: Should the reactor continue running if there are some exceptions
  //     are caught while handling events?
//MJM: Put this in a loop with a state conditional and use the state to
//MJM: indicate whether or not to terminate.  But I can think of no
//MJM: reason to have anything in the conditional, so just expire.
//MJM: Nevermind.
  try {
    // Tell the reactor to handle events.
    reactor_->run_reactor_event_loop();
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: ReactorTask::svc caught exception - %C.\n",
               e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: ReactorTask::svc caught exception.\n"));
  }

  return 0;
}

int
OpenDDS::DCPS::ReactorTask::close(u_long flags)
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

void
OpenDDS::DCPS::ReactorTask::stop()
{

  {
    GuardType guard(lock_);

    if (state_ == STATE_NOT_RUNNING) {
      // We are already "stopped".  Just return.
      return;
    }

    state_ = STATE_NOT_RUNNING;
  }

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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
