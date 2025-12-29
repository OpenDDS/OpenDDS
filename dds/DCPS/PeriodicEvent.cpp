/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"
#include "PeriodicEvent.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

PeriodicEvent::PeriodicEvent(EventDispatcher_rch dispatcher, EventBase_rch event)
 : dispatcher_(dispatcher)
 , event_(event)
 , strict_timing_(true)
 , timer_id_(0)
{
}

bool PeriodicEvent::enable(const TimeDuration& period, bool immediate_dispatch, bool strict_timing)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!enabled_i()) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      const MonotonicTimePoint expiration = now + period;
      if (immediate_dispatch) {
        period_ = period;
        expiration_ = now;
        strict_timing_ = strict_timing;
        const bool okay = dispatcher->dispatch(rchandle_from(this));
        if (!okay) {
          period_ = TimeDuration::zero_value;
        }
        return okay;
      } else {
        long id = dispatcher->schedule(rchandle_from(this), expiration);
        if (id > 0) {
          period_ = period;
          expiration_ = expiration;
          timer_id_ = id;
          strict_timing_ = strict_timing;
          return true;
        } else if (log_level >= LogLevel::Warning) {
          ACE_ERROR((LM_WARNING, "(%P|%t) PeriodicEvent::enable: failed to schedule\n"));
        }
      }
    }
    return false;
  }
  return true;
}

void PeriodicEvent::shorten_current_wait(const TimeDuration& period)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (timer_id_ > 0) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      const MonotonicTimePoint expiration = now + period;
      if (expiration < expiration_ && dispatcher->cancel(timer_id_)) {
        long id = dispatcher->schedule(rchandle_from(this), expiration);
        if (id > 0) {
          expiration_ = expiration;
          timer_id_ = id;
        } else {
          ACE_ERROR((LM_WARNING, "(%P|%t) PeriodicEvent::shorten_current_wait: failed to reschedule\n"));
        }
      }
    }
  }
}

void PeriodicEvent::disable()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (timer_id_ > 0) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      if (dispatcher->cancel(timer_id_)) {
        timer_id_ = 0;
      }
    }
  }
  period_ = TimeDuration::zero_value;
}

bool PeriodicEvent::enabled() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return enabled_i();
}

RcHandle<EventBase> PeriodicEvent::event() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(event_mutex_);
  return event_;
}

void PeriodicEvent::handle_event_scheduling()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_id_ = 0;
  if (enabled_i()) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      const MonotonicTimePoint expiration = (strict_timing_ ? expiration_ : MonotonicTimePoint::now()) + period_;
      long id = dispatcher->schedule(rchandle_from(this), expiration);
      if (id > 0) {
        expiration_ = expiration;
        timer_id_ = id;
      }
    }
  }
}

void PeriodicEvent::handle_event()
{
  handle_event_scheduling();
  ACE_Guard<ACE_Thread_Mutex> guard(event_mutex_);
  if (event_) {
    RcHandle<EventBase> event_copy(event_);
    guard.release();
    event_copy->handle_event();
  }
}

void PeriodicEvent::handle_cancel()
{
  ACE_Guard<ACE_Thread_Mutex> guard(event_mutex_);
  if (event_) {
    RcHandle<EventBase> event_copy(event_);
    guard.release();
    event_copy->handle_cancel();
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
