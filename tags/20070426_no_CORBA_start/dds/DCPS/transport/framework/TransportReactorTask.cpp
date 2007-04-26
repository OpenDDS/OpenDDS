// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReactorTask.h"


#if !defined (__ACE_INLINE__)
#include "TransportReactorTask.inl"
#endif /* __ACE_INLINE__ */

#include <ace/Select_Reactor.h>

TAO::DCPS::TransportReactorTask::TransportReactorTask()
  : state_ (STATE_NOT_RUNNING),
    condition_(this->lock_)
{
  DBG_ENTRY_LVL("TransportReactorTask","TransportReactorTask",5);
  // Set our reactor pointer to a new reactor object.
  this->reactor_ = new ACE_Reactor(new ACE_Select_Reactor, 1);
}


TAO::DCPS::TransportReactorTask::~TransportReactorTask()
{
  DBG_ENTRY_LVL("TransportReactorTask","~TransportReactorTask",5);
  delete this->reactor_;
}


int
TAO::DCPS::TransportReactorTask::open(void*)
{
  DBG_ENTRY_LVL("TransportReactorTask","open",5);

  GuardType guard(this->lock_);

  // Reset our state.
  this->state_ = STATE_NOT_RUNNING;

  // For now, we only support one thread to run the reactor event loop.
  // Parts of the logic in this class would need to change to support
  // more than one thread running the reactor.

  // Attempt to activate ourselves.  If successful, a new thread will be
  // started and it will invoke our svc() method.  Note that we still have
  // a hold on our lock while we do this.
  if (this->activate(THR_NEW_LWP | THR_JOINABLE,1) != 0)
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportReactorTask Failed to activate "
                        "itself.\n"),
                       -1);
    }

  // Here we need to wait until a condition is triggered by the new thread(s)
  // that we created.  Note that this will cause us to release the lock and
  // wait until the condition_ is signal()'ed.  When it is signaled, the
  // condition will attempt to obtain the lock again, and then return to us
  // here.  We can then go on.
  if (this->state_ == STATE_NOT_RUNNING)
    {
      this->state_ = STATE_OPENING;
      this->condition_.wait();
    }

  return 0;
}


int
TAO::DCPS::TransportReactorTask::svc()
{
  DBG_ENTRY_LVL("TransportReactorTask","svc",5);

  // First off - We need to obtain our own reference to ourselves such
  // that we don't get deleted while still running in our own thread.
  // In essence, our current thread "owns" a copy of our reference.
  // It's all done with the magic of intrusive reference counting!
  this->_add_ref();

  // VERY IMPORTANT!  Tell the reactor that this task's thread will be
  //                  its "owner".
  if (this->reactor_->owner(ACE_Thread_Manager::instance()->thr_self()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                  "(%P|%t) ERROR: Failed to change the reactor's owner().\n"));
    }

  {
    // Obtain the lock.  This should only happen once the open() has hit
    // the condition_.wait() line of code, and has released the lock.
    GuardType guard(this->lock_);

    if (this->state_ == STATE_OPENING)
      {
        // Advance the state.
        this->state_ = STATE_RUNNING;

        // Signal the condition_ that we are here.
        this->condition_.signal();
      }
  }

  //TBD: Should the reactor continue running if there are some exceptions
  //     are caught while handling events?
//MJM: Put this in a loop with a state conditional and use the state to
//MJM: indicate whether or not to terminate.  But I can think of no
//MJM: reason to have anything in the conditional, so just expire.
//MJM: Nevermind.
  try
  {
    // Tell the reactor to handle events.
    this->reactor_->run_reactor_event_loop ();
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: TransportReactorTask::svc caught exception.\n"));
  }

  return 0;
}


int
TAO::DCPS::TransportReactorTask::close(u_long flags)
{
  DBG_ENTRY_LVL("TransportReactorTask","close",5);
  ACE_UNUSED_ARG (flags);
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

  this->_remove_ref();
  return 0;
}


void
TAO::DCPS::TransportReactorTask::stop()
{
  DBG_ENTRY_LVL("TransportReactorTask","stop",5);
  {
    GuardType guard(this->lock_);
    if (this->state_ == STATE_NOT_RUNNING)
      {
        // We are already "stopped".  Just return.
        return;
      }
    this->state_ = STATE_NOT_RUNNING;
  }

  this->reactor_->end_reactor_event_loop ();

  // Let's wait for the reactor task's thread to complete before we
  // leave this stop method.
  this->wait();
}


