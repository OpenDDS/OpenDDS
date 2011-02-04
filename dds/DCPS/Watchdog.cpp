/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "Watchdog.h"

#include "ace/Reactor.h"
#include "ace/Time_Value.h"

OpenDDS::DCPS::Watchdog::Watchdog(ACE_Reactor * reactor,
                                  ACE_Time_Value const & interval)
  : reactor_(reactor)
  , timer_(this)
  , interval_(interval)
{
}

OpenDDS::DCPS::Watchdog::~Watchdog()
{
  this->cancel_all();
}

void
OpenDDS::DCPS::Watchdog::reset_interval(ACE_Time_Value const & interval)
{
  if (this->interval_ != interval) {
    this->interval_ = interval;
    this->reschedule_deadline();
  }
}

long
OpenDDS::DCPS::Watchdog::schedule_timer(void* const act, const ACE_Time_Value & interval)
{
  return this->reactor_->schedule_timer(&this->timer_,
                                        act,
                                        interval,
                                        interval);
}

int
OpenDDS::DCPS::Watchdog::cancel_timer(long const & timer_id)
{
  return this->reactor_->cancel_timer(timer_id);
}

void
OpenDDS::DCPS::Watchdog::cancel_all()
{
  (void) this->reactor_->cancel_timer(&this->timer_);
}

int
OpenDDS::DCPS::Watchdog::reset_timer_interval(long const & timer_id)
{
  return this->reactor_->reset_timer_interval(timer_id, this->interval_);
}
