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
#include <ace/OS_NS_Thread.h>

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
  , thread_status_(0)
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
  ThreadStatus* thread_stat, const String& name)
{
  GuardType guard(lock_);

  // If we've already been opened, let's clean up the old stuff
  cleanup();

  // thread status reporting support
  timeout_ = timeout;
  thread_status_ = thread_stat;
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

  try {
    // Tell the reactor to handle events.
    if (timeout_ == TimeDuration(0)) {
      reactor_->run_reactor_event_loop();
    } else {
      const String thread_key = ThreadStatus::get_key("ReactorTask", name_);

      while (state_ == STATE_RUNNING) {
        ACE_Time_Value t = timeout_.value();
        reactor_->run_reactor_event_loop(t, 0);
        if (thread_status_) {
          if (DCPS_debug_level >= 4) {
            ACE_DEBUG((LM_DEBUG,
                       "(%P|%t) ReactorTask::svc. Updating thread status.\n"));
          }
          if (!thread_status_->update(thread_key) && DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ReactorTask::svc: updated failed\n"));
          }
        }
      }
    }
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

bool ThreadStatus::update(const String& thread_key)
{
  ACE_WRITE_GUARD_RETURN(ACE_Thread_Mutex, g, lock, false);
  const SystemTimePoint now = SystemTimePoint::now();
  map[thread_key] = Thread(now);
  if (DCPS_debug_level >= 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) ThreadStatus::update: "
      "update for thread \"%C\" @ %d\n",
      thread_key.c_str(), now.value().sec()));
  }
  return true;
}

String ThreadStatus::get_key(const char* safety_profile_tid, const String& name)
{
  String key;
#ifdef OPENDDS_SAFETY_PROFILE
  key = safety_profile_tid;
#else
  ACE_UNUSED_ARG(safety_profile_tid);
#  ifdef ACE_HAS_MAC_OSX
  unsigned long tid = 0;
  uint64_t u64_tid;
  if (!pthread_threadid_np(0, &u64_tid)) {
    tid = static_cast<unsigned long>(u64_tid);
  } else if (DCPS_debug_level) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: pthread_threadid_np failed\n")));
  }
#  elif defined ACE_HAS_GETTID
  const pid_t tid = gettid();
#  else
  const ACE_thread_t tid = ACE_OS::thr_self();
#  endif

  key = to_dds_string(tid);
#endif

  if (name.length()) {
    key += " (" + name + ")";
  }

  return key;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
