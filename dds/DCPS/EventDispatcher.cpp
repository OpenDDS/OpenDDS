/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "EventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

EventBase::~EventBase()
{
}

void EventBase::handle_error()
{
}

void EventBase::handle_cancel()
{
}

void EventBase::operator()()
{
  try {
    handle_event();
  } catch (...) {
    handle_error();
  }
  this->_remove_ref();
}

EventDispatcher::EventDispatcher(size_t count)
 : dispatcher_(make_rch<EventDispatcherLite>(count))
{
}

EventDispatcher::~EventDispatcher()
{
  shutdown();
}

void EventDispatcher::shutdown(bool immediate)
{
  EventDispatcherLite_rch local;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    local.swap(dispatcher_);
  }
  if (local) {
    EventDispatcherLite::EventQueue remaining;
    local->shutdown(immediate, &remaining);
    for (EventDispatcherLite::EventQueue::iterator it = remaining.begin(), limit = remaining.end(); it != limit; ++it) {
      EventBase* ptr = static_cast<EventBase*>(it->second);
      if (ptr) {
        ptr->handle_cancel();
        ptr->_remove_ref();
      }
    }
  }
}

bool EventDispatcher::dispatch(EventBase_rch event)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return false;
  }
  event->_add_ref();
  const bool result = dispatcher_->dispatch(*event);
  if (!result) {
    event->_remove_ref();
  }
  return result;
}

long EventDispatcher::schedule(EventBase_rch event, const MonotonicTimePoint& expiration)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return -1;
  }
  event->_add_ref();
  const long result = dispatcher_->schedule(*event, expiration);
  if (result < 0) {
    event->_remove_ref();
  }
  return result;
}

size_t EventDispatcher::cancel(long id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return 0;
  }
  void* arg = 0;
  const size_t result = dispatcher_->cancel(id, &arg);
  if (result) {
    EventBase* ptr = static_cast<EventBase*>(arg);
    if (ptr) {
      ptr->handle_cancel();
      ptr->_remove_ref();
    }
  }
  return result;
}

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

void SporadicEvent::handle_event()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_id_ = 0;
  if (event_) {
    guard.release();
    event_->handle_event();
  }
}

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
    guard.release();
    event_->handle_event();
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
