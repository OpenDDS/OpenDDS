// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "WatchdogTimer.h"
#include "Watchdog.h"

OpenDDS::DCPS::WatchdogTimer::WatchdogTimer (Watchdog * dog)
  : watchdog_ (dog)
{
}

OpenDDS::DCPS::WatchdogTimer::~WatchdogTimer ()
{
}

int
OpenDDS::DCPS::WatchdogTimer::handle_timeout (
  ACE_Time_Value const & /* current_time */,
  void const * act)
{
  this->watchdog_->execute (act, true); 
  return 0;
}

