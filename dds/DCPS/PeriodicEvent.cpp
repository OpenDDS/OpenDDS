/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "PeriodicEvent.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

PeriodicEvent::PeriodicEvent(EventDispatcher_rch dispatcher, EventBase_rch event)
 : dispatcher_(dispatcher)
 , event_(event)
 , timer_id_(0)
{
}

void PeriodicEvent::enable(const TimeDuration& period, bool strict_timing)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (timer_id_ < 1) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      const MonotonicTimePoint expiration = now + period;
      long id = dispatcher->schedule(rchandle_from(this), expiration);
      if (id > 0) {
        period_ = period;
        expiration_ = expiration;
        timer_id_ = id;
        strict_timing_ = strict_timing;
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
}

bool PeriodicEvent::enabled() const
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return timer_id_ > 0;
}

void PeriodicEvent::handle_event()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_id_ = 0;
  EventDispatcher_rch dispatcher = dispatcher_.lock();
  if (dispatcher) {
    const MonotonicTimePoint expiration = (strict_timing_ ? expiration_ : MonotonicTimePoint::now()) + period_;
    long id = dispatcher->schedule(rchandle_from(this), expiration);
    if (id > 0) {
      expiration_ = expiration;
      timer_id_ = id;
    }
  }
  if (event_) {
    RcHandle<EventBase> event_copy(event_);
    guard.release();
    event_copy->handle_event();
  }
}

void PeriodicEvent::handle_cancel()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (event_) {
    RcHandle<EventBase> event_copy(event_);
    guard.release();
    event_copy->handle_cancel();
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
