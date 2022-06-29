/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "SporadicEvent.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

SporadicEvent::SporadicEvent(EventDispatcher_rch dispatcher, EventBase_rch event)
 : dispatcher_(dispatcher)
 , event_(event)
 , timer_id_(0)
{
}

void SporadicEvent::schedule(const TimeDuration& duration)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (timer_id_ < 1) {
    EventDispatcher_rch dispatcher = dispatcher_.lock();
    if (dispatcher) {
      const MonotonicTimePoint expiration = now + duration;
      long id = dispatcher->schedule(rchandle_from(this), expiration);
      if (id > 0) {
        expiration_ = expiration;
        timer_id_ = id;
      }
    }
  } else {
    if (now + duration < expiration_) {
      EventDispatcher_rch dispatcher = dispatcher_.lock();
      if (dispatcher) {
        if (dispatcher->cancel(timer_id_)) {
          timer_id_ = 0;
          const MonotonicTimePoint expiration = now + duration;
          long id = dispatcher->schedule(rchandle_from(this), expiration);
          if (id > 0) {
            expiration_ = expiration;
            timer_id_ = id;
          }
        }
      }
    }
  }
}

void SporadicEvent::cancel()
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

void SporadicEvent::handle_event_scheduling()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_id_ = 0;
}

void SporadicEvent::handle_event()
{
  handle_event_scheduling();
  ACE_Guard<ACE_Thread_Mutex> guard(event_mutex_);
  if (event_) {
    RcHandle<EventBase> event_copy(event_);
    guard.release();
    event_copy->handle_event();
  }
}

void SporadicEvent::handle_cancel()
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
