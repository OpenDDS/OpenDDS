// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Watchdog.h"

#include "ace/Reactor.h"
#include "ace/Time_Value.h"


OpenDDS::DCPS::Watchdog::Watchdog (ACE_Reactor * reactor,
                                   ACE_Time_Value const & interval)
  : reactor_ (reactor)
  , timer_ (this)
  , timer_id_ (-1)
{
  // Only schedule a timer if the interval is "reasonable".
  if (interval != ACE_Time_Value::zero
      && interval != ACE_Time_Value::max_time)
    this->timer_id_ = this->reactor_->schedule_timer (&timer_,
                                                      0,
                                                      interval,
                                                      interval);
}

OpenDDS::DCPS::Watchdog::~Watchdog ()
{
  (void) this->reactor_->cancel_timer (this->timer_id_);
}


void
OpenDDS::DCPS::Watchdog::reset_interval (ACE_Time_Value const & interval)
{
  // New interval takes effect after the next expiration.
  //
  // @@ We could cancel the old timer and reschedule a new one instead
  //    of resetting the interval of the old timer.  That would allow
  //    the new interval to take effect immediately, if that is
  //    desired or necessary.
  (void) this->reactor_->reset_timer_interval (this->timer_id_,
                                               interval);
}
