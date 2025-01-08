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

#include "RcHandle_T.h"
#include "Service_Participant.h"
#include "debug.h"

#include <ace/Select_Reactor.h>
#include <ace/WFMO_Reactor.h>
#include <ace/Proactor.h>
#include <ace/Proactor_Impl.h>
#include <ace/WIN32_Proactor.h>
#include <ace/OS_NS_Thread.h>

#include <exception>
#include <cstring>

#if OPENDDS_CONFIG_BOOTTIME_TIMERS
#  if defined __linux__ && __linux__
#    include <sys/timerfd.h>
#  else
#    error Unsupported platform for OPENDDS_CONFIG_BOOTTIME_TIMERS
#  endif
#endif

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
  , reactor_state_(RS_NONE)
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
  reactor_wrapper_.close();
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

int ReactorTask::open_reactor_task(ThreadStatusManager* thread_status_manager,
                                   const String& name,
                                   ACE_Reactor* reactor)
{
  GuardType guard(lock_);

  // If we've already been opened, let's clean up the old stuff
  cleanup();

  // thread status reporting support
  thread_status_manager_ = thread_status_manager;
  name_ = name;
  reactor_ = reactor;

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

  if (!reactor_wrapper_.open(reactor_)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) NOTICE: ReactorTask::open_reactor_task: could not open ReactorWrapper\n"));
    }
    return -1;
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

    ACE_Event_Handler::reactor(reactor_);

    // Advance the state.
    state_ = STATE_RUNNING;
    condition_.notify_all();
  }

  ReactorWrapper::TimerId thread_status_timer = ReactorWrapper::InvalidTimerId;
  RcHandle<RcEventHandler> tsm_updater_handler;

  if (thread_status_manager_->update_thread_status()) {
    tsm_updater_handler = make_rch<ThreadStatusManager::Updater>();
    const TimeDuration period = thread_status_manager_->thread_status_interval();
    thread_status_timer = reactor_wrapper_.schedule(*tsm_updater_handler, thread_status_manager_,
                                                    period, period);

    if (thread_status_timer == ReactorWrapper::InvalidTimerId) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ReactorTask::svc: failed to "
                              "schedule timer for ThreadStatusManager::Updater\n"));
      }
    }
  }

  ThreadStatusManager::Sleeper sleeper(*thread_status_manager_);
  reactor_->run_reactor_event_loop();

  if (thread_status_timer != ReactorWrapper::InvalidTimerId) {
    reactor_wrapper_.cancel(thread_status_timer);
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

  reactor_wrapper_.close();

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

bool ReactorTask::on_thread() const
{
  GuardType guard(lock_);
  return ACE_OS::thr_equal(reactor_owner_, ACE_Thread::self());
}

void ReactorTask::reactor(ACE_Reactor* reactor)
{
  GuardType guard(lock_);
  ACE_Event_Handler::reactor(reactor);
}

ACE_Reactor* ReactorTask::reactor() const
{
  GuardType guard(lock_);
  return ACE_Event_Handler::reactor();
}

ReactorTask::CommandPtr ReactorTask::execute_or_enqueue(CommandPtr command)
{
  OPENDDS_ASSERT(command);

  GuardType guard(lock_);

  // Only allow immediate execution if running on the reactor thread, otherwise we risk deadlock
  // when calling into the reactor object.
  const bool is_owner = ACE_OS::thr_equal(reactor_owner_, ACE_Thread::self());

  // If state is set to processing, the contents of command_queue_ have been swapped out
  // so immediate execution may run jobs out of the expected order.
  const bool is_not_processing = reactor_state_ != RS_PROCESSING;

  // If the command_queue_ is not empty, allowing execution will potentially run unexpected code
  // which is problematic since we may be holding locks used by the unexpected code.
  const bool is_empty = command_queue_.empty();

  // If all three of these conditions are met, it should be safe to execute
  const bool is_safe_to_execute = is_owner && is_not_processing && is_empty;

  // Even if it's not normally safe to execute, allow immediate execution if the reactor is shut down
  const bool immediate = is_safe_to_execute || (state_ == STATE_SHUT_DOWN);

  // Always set reactor and push to the queue
  ACE_Reactor* local_reactor = reactor_;
  command_queue_.push_back(command);

  // But depending on whether we're running it immediately or not, we either process or notify
  if (immediate) {
    process_command_queue_i(guard, local_reactor);
  } else if (reactor_state_ == RS_NONE) {
    reactor_state_ = RS_NOTIFIED;
    guard.release();
    local_reactor->notify(this);
  }
  return command;
}

void ReactorTask::wait_until_empty()
{
  GuardType guard(lock_);
  while (reactor_state_ != RS_NONE || !command_queue_.empty()) {
    condition_.wait(*thread_status_manager_);
  }
}

int ReactorTask::handle_exception(ACE_HANDLE /*fd*/)
{
  ThreadStatusManager::Event ev(*thread_status_manager_);

  GuardType guard(lock_);
  process_command_queue_i(guard, reactor_);
  return 0;
}

void ReactorTask::process_command_queue_i(ACE_Guard<ACE_Thread_Mutex>& guard,
                                          ACE_Reactor* reactor)
{
  Queue cq;
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(lock_);

  reactor_state_ = RS_PROCESSING;
  if (!command_queue_.empty()) {
    cq.swap(command_queue_);
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    for (Queue::const_iterator pos = cq.begin(), limit = cq.end(); pos != limit; ++pos) {
      (*pos)->execute(reactor_wrapper_);
    }
  }
  if (!command_queue_.empty()) {
    reactor_state_ = RS_NOTIFIED;
    guard.release();
    reactor->notify(this);
  } else {
    reactor_state_ = RS_NONE;
    condition_.notify_all();
  }
}

bool ReactorWrapper::open(ACE_Reactor* reactor)
{
  reactor_ = reactor;

#if OPENDDS_CONFIG_BOOTTIME_TIMERS
  timer_fd_ = timerfd_create(CLOCK_BOOTTIME, TFD_CLOEXEC);
  if (timer_fd_ == -1) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ReactorWrapper::open: timerfd_create %m\n"));
    }
    return false;
  }

  if (reactor_->register_handler(this, ACE_Event_Handler::READ_MASK) == -1) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ReactorWrapper::open: register_handler %m\n"));
    }
    return false;
  }
#endif

  return true;
}

void ReactorWrapper::close()
{
#if OPENDDS_CONFIG_BOOTTIME_TIMERS
  if (reactor_) {
    reactor_->remove_handler(this, ACE_Event_Handler::ALL_EVENTS_MASK);
  }

  if (timer_fd_ != ACE_INVALID_HANDLE) {
    ::close(timer_fd_);
    timer_fd_ = ACE_INVALID_HANDLE;
  }
#endif

  // Current shutdown logic uses the reactor after close.
  // reactor_ = 0;
}

const ReactorWrapper::TimerId ReactorWrapper::InvalidTimerId = -1;

ReactorWrapper::TimerId ReactorWrapper::schedule(RcEventHandler& handler,
                                                 const void* arg,
                                                 const TimeDuration& delay,
                                                 const TimeDuration& interval)
{
#if OPENDDS_CONFIG_BOOTTIME_TIMERS
  const ACE_Time_Value earliest_before = timer_heap_.is_empty() ? ACE_Time_Value::max_time : timer_heap_.earliest_time();
  const TimerId tid = timer_heap_.schedule(&handler, arg, MonotonicTimePoint::now().value() + delay.value(), interval.value());
  const ACE_Time_Value earliest_after = timer_heap_.earliest_time();
  if (earliest_after < earliest_before) {
    if (!arm()) {
      return InvalidTimerId;
    }
  }
  return tid;
#else
  return reactor_->schedule_timer(&handler, arg, delay.value(), interval.value());
#endif
}

void ReactorWrapper::cancel(TimerId timer)
{
#if OPENDDS_CONFIG_BOOTTIME_TIMERS
  timer_heap_.cancel(timer);
#else
  reactor_->cancel_timer(timer);
#endif
}

int ReactorWrapper::register_handler(ACE_HANDLE io_handle,
                                     ACE_Event_Handler* event_handler,
                                     ACE_Reactor_Mask mask)
{
  return reactor_->register_handler(io_handle, event_handler, mask);
}

int ReactorWrapper::register_handler(ACE_Event_Handler* event_handler,
                                     ACE_Reactor_Mask mask)
{
  return reactor_->register_handler(event_handler, mask);
}

int ReactorWrapper::remove_handler(ACE_HANDLE io_handle,
                                   ACE_Reactor_Mask mask)
{
  return reactor_->remove_handler(io_handle, mask);
}

int ReactorWrapper::remove_handler(ACE_Event_Handler* event_handler,
                                   ACE_Reactor_Mask mask)
{
  return reactor_->remove_handler(event_handler, mask);
}

#if OPENDDS_CONFIG_BOOTTIME_TIMERS
int ReactorWrapper::handle_input(ACE_HANDLE)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  uint64_t unused;
  const ssize_t readReturned = read(timer_fd_, &unused, sizeof unused);
  if (readReturned != sizeof unused) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: ReactorWrapper::handle_input: read %m\n"));
    }
    return -1;
  }
  timer_heap_.expire(now.value());
  if (!timer_heap_.is_empty()) {
    if (!arm()) {
      return -1;
    }
  }
  return 0;
}

bool ReactorWrapper::arm()
{
  static const timespec one_ns = {0, 1};
  const ACE_Time_Value delay = timer_heap_.earliest_time() - MonotonicTimePoint::now().value();
  itimerspec ts;
  ts.it_interval = ACE_Time_Value::zero;
  // expiration in the past, execute as soon as possible
  // avoid zeros since that would disarm the timer
  ts.it_value = (delay <= ACE_Time_Value::zero) ? one_ns : delay;
  if (timerfd_settime(timer_fd_, 0, &ts, 0) == -1) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: ReactorWrapper::arm: timerfd_settime %m\n"));
    }
    return false;
  }
  return true;
}
#endif

void RegisterHandler::execute(ReactorWrapper& reactor_wrapper)
{
  if (reactor_wrapper.register_handler(io_handle_, event_handler_, mask_) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: RegisterHandler::execute: failed to register handler for socket %d\n",
                 io_handle_));
    }
  }
}

void RemoveHandler::execute(ReactorWrapper& reactor_wrapper)
{
  if (reactor_wrapper.remove_handler(io_handle_, mask_) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) ERROR: RemoveHandler::execute: failed to remove handler for socket %d\n",
                 io_handle_));
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
