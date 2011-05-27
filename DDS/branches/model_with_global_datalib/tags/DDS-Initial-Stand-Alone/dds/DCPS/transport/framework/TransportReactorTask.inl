// -*- C++ -*-
//
// $Id$
#include  "ace/Reactor.h"
#include  "EntryExit.h"

ACE_INLINE 
TAO::DCPS::TransportReactorTask::TransportReactorTask()
  : state_ (STATE_NOT_RUNNING),
    condition_(this->lock_)
{
  DBG_ENTRY("TransportReactorTask","TransportReactorTask");
  // Set our reactor pointer to a new reactor object.
  this->reactor_ = new ACE_Reactor();
}


ACE_INLINE void
TAO::DCPS::TransportReactorTask::stop()
{
  DBG_ENTRY("TransportReactorTask","stop");
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


ACE_INLINE ACE_Reactor*
TAO::DCPS::TransportReactorTask::get_reactor()
{
  DBG_SUB_ENTRY("TransportReactorTask","get_reactor",1);
  return this->reactor_;
}


ACE_INLINE const ACE_Reactor*
TAO::DCPS::TransportReactorTask::get_reactor() const
{
  DBG_SUB_ENTRY("TransportReactorTask","get_reactor",2);
  return this->reactor_;
}

