// -*- C++ -*-
//
// $Id$
#include "ThreadSynch.h"

ACE_INLINE int
TAO::DCPS::TransportSendStrategy::start()
{
  DBG_ENTRY_LVL("TransportSendStrategy","start",5);

  // Since we (the TransportSendStrategy object) are a reference-counted
  // object, but the synch_ object doesn't necessarily know this, we need
  // to give a "copy" of a reference to ourselves to the synch_ object here.
  // We will do the reverse when we unregister ourselves (as a worker) from
  // the synch_ object.
  //MJM: The synch thingie knows to not "delete" us, right?
  this->_add_ref();
  if (this->synch_->register_worker(this) == -1)
    {
      // Take back our "copy".
      this->_remove_ref();
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportSendStrategy failed to register "
                        "as a worker with the ThreadSynch object.\n"),
                       -1);
    }

  return 0;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::stop()
{
  DBG_ENTRY_LVL("TransportSendStrategy","stop",5);

  this->synch_->unregister_worker();

  // Since we gave the synch_ a "copy" of a reference to ourselves, we need
  // to take it back now.
  this->_remove_ref();

  {
    GuardType guard(this->lock_);

    this->stop_i ();
  }

  // TBD SOON - What about all of the samples that may still be stuck in
  //            our queue_ and/or elems_?
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::send_start()
{
  DBG_ENTRY_LVL("TransportSendStrategy","send_start",5);

  GuardType guard(this->lock_);
  if (! this->link_released_)
    ++this->start_counter_;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::link_released (bool flag)
{
  DBG_ENTRY_LVL("TransportSendStrategy","link_released",5);

  GuardType guard(this->lock_);
  this->link_released_ = flag;
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::relink (bool)
{
  DBG_ENTRY_LVL("TransportSendStrategy","relink",5);
  // The subsclass needs implement this function for re-establishing
  // the link upon send failure.
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::suspend_send ()
{
  DBG_ENTRY_LVL("TransportSendStrategy","suspend_send",5);
  GuardType guard(this->lock_);

  if (this->mode_ != MODE_TERMINATED && this->mode_ != MODE_SUSPEND)
    {
      this->mode_before_suspend_ = this->mode_;
      this->mode_ = MODE_SUSPEND;
    }
}


ACE_INLINE void
TAO::DCPS::TransportSendStrategy::resume_send ()
{
  DBG_ENTRY_LVL("TransportSendStrategy","resume_send",5);
  GuardType guard(this->lock_);
  // If this send strategy is reused when the connection is reestablished, then
  // we need re-initialize the mode_ and mode_before_suspend_.
  if (this->mode_ == MODE_TERMINATED)
    {
      this->header_.length_ = 0;
      this->pkt_chain_ = 0;
      this->header_complete_ = 0;
      this->start_counter_ = 0;
      this->mode_ = MODE_DIRECT;
      this->mode_before_suspend_ = MODE_NOT_SET;
      this->num_delayed_notifications_ = 0;
    }
  else if (this->mode_before_suspend_ == MODE_NOT_SET)
    {
      ACE_ERROR((LM_ERROR, "(%P|%t)TransportSendStrategy::resume_send  The suspend_send()"
        " is not called previously.\n"));
    }
  else
    {
      this->mode_ = this->mode_before_suspend_;
    }
}


ACE_INLINE const char*
TAO::DCPS::TransportSendStrategy::mode_as_str (SendMode mode)
{
  static const char* SendModeStr[] = { "MODE_NOT_SET",
                                       "MODE_DIRECT",
                                       "MODE_QUEUE",
                                       "MODE_SUSPEND",
                                       "MODE_TERMINATED",
                                       "UNKNOWN" };

  return SendModeStr [mode];
}
