/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PerConnectionSynch.h"
#include "dds/DCPS/debug.h"

#if !defined (__ACE_INLINE__)
#include "PerConnectionSynch.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::PerConnectionSynch::~PerConnectionSynch()
{
  DBG_ENTRY_LVL("PerConnectionSynch","~PerConnectionSynch",6);
}

void
OpenDDS::DCPS::PerConnectionSynch::work_available()
{
  DBG_ENTRY_LVL("PerConnectionSynch","work_available",6);
  GuardType guard(this->lock_);
  this->work_available_ = 1;
  this->condition_.signal();
}

int
OpenDDS::DCPS::PerConnectionSynch::open(void*)
{
  DBG_ENTRY_LVL("PerConnectionSynch","open",6);
  // Activate this object to start a new thread that will call
  // our svc() method, and then our close() method.
  this->shutdown_ = 0;

  long flags;
  flags  = THR_NEW_LWP | THR_JOINABLE ;//|THR_SCOPE_PROCESS | THR_SCOPE_THREAD;

  if (this->scheduler_ >= 0) {
    flags |= THR_EXPLICIT_SCHED | this->scheduler_;

  } else {
    flags |= THR_INHERIT_SCHED;
  }

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) PerConnectionSynch::open(): ")
               ACE_TEXT("activating thread with flags 0x%08.8x ")
               ACE_TEXT("and priority %d.\n"),
               flags,
               this->dds_priority_));
  }

  return this->activate(flags, 1, 0, this->dds_priority_);
}

int
OpenDDS::DCPS::PerConnectionSynch::svc()
{
  DBG_ENTRY_LVL("PerConnectionSynch","svc",6);

  // Ignore all signals to avoid
  // ERROR: ACE::handle_write_ready return -1 while waiting  to unclog. handle_write_ready: Interrupted system call
  // The main thread will handle signals.
  sigset_t set;
  ACE_OS::sigfillset(&set);
  ACE_OS::thr_sigsetmask(SIG_SETMASK, &set, NULL);

  ThreadSynchWorker::WorkOutcome work_outcome =
    ThreadSynchWorker::WORK_OUTCOME_NO_MORE_TO_DO;

  // Loop until we honor the shutdown_ flag.
  while (1) {
    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
          "Top of infinite svc() loop\n"));

    {
      GuardType guard(this->lock_);

      VDBG((LM_DEBUG,"(%P|%t) DBG:   "
            "Lock acquired.  Check to see what to do next.\n"));

      // We will wait on the condition_ if all of the following are true:
      //
      //   1) The last time the perform_work() method was called, it
      //      indicated that there was no more work to do.
      //   2) Since we last invoked perform_work(), we have not been
      //      informed that there is work_available().
      //   3) We have not been asked to shutdown_ the svc() loop.
      //
      while ((work_outcome ==
              ThreadSynchWorker::WORK_OUTCOME_NO_MORE_TO_DO) &&
             (this->work_available_ == 0) &&
             (this->shutdown_       == 0)) {
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "No work to do.  Just wait on the condition.\n"));
        this->condition_.wait();
        VDBG((LM_DEBUG,"(%P|%t) DBG:   "
              "We are awake from waiting on the condition.\n"));
      }

      // Maybe we have been asked to shutdown_ the svc() loop.
      if (this->shutdown_ == 1) {
        VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
                  "Honoring the shutdown request.\n"), 5);
        // We are honoring the request to shutdown_ the svc() loop.
        break;
      }

      // Or, perhaps we experienced a fatal error on our last call to
      // perform_work().
      if (work_outcome == ThreadSynchWorker::WORK_OUTCOME_BROKEN_RESOURCE) {
        VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
                  "Fatal error - Broken SynchResounce.\n"), 5);
        // Stop the svc() loop.
        break;
      }

      VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
                "Reset our work_available_ flag to 0, and release lock.\n"), 5);

      // Set our work_available_ flag to false (0) before we release the
      // lock so that we will only count any work_available() calls that
      // happen after this point.
      this->work_available_ = 0;
    }

    if (work_outcome == ThreadSynchWorker::WORK_OUTCOME_CLOGGED_RESOURCE) {
      VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   Need to wait for clogged resources to open up.\n"), 5);

      // Ask the ThreadSynchResource to block us until the clog situation
      // clears up.
      if (this->wait_on_clogged_resource() == -1) {
        VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
                  "Fatal error - wait_on_clogged_resource fails.\n"), 5);
        break;
      }
    }

    VDBG((LM_DEBUG,"(%P|%t) DBG:   "
          "Call perform_work()\n"));

    // Without the lock, ask the worker to perform some work.  It tells
    // us if it completed with more work to still be performed (or not).
    work_outcome = this->perform_work();

    VDBG_LVL((LM_DEBUG,"(%P|%t) DBG:   "
              "call to perform_work() returned %d\n",work_outcome), 5);
  }

  return 0;
}

int
OpenDDS::DCPS::PerConnectionSynch::close(u_long)
{
  DBG_ENTRY_LVL("PerConnectionSynch","close",6);
  return 0;
}

int
OpenDDS::DCPS::PerConnectionSynch::register_worker_i()
{
  DBG_ENTRY_LVL("PerConnectionSynch","register_worker_i",6);
  return this->open(0);
}

void
OpenDDS::DCPS::PerConnectionSynch::unregister_worker_i()
{
  DBG_ENTRY_LVL("PerConnectionSynch","unregister_worker_i",6);
  // It is at this point that we need to stop the thread that
  // was activated when our open() method was called.
  {
    // Acquire the lock
    GuardType guard(this->lock_);

    // Set the shutdown_ flag to false to shutdown the svc() method loop.
    this->shutdown_ = 1;

    // Signal the condition_ object in case the svc() method is currently
    // blocked wait()'ing on the condition.
    this->condition_.signal();
  }

  // Wait for all threads running this task (there should just be one thread)
  // to finish.
  this->wait();
}
