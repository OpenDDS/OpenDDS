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
      EventCaller* ptr = static_cast<EventCaller*>(it->second);
      delete ptr;
    }
  }
}

bool EventDispatcher::dispatch(EventBase_rch event)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return false;
  }
  EventCaller* ptr = new EventCaller(event);
  const bool result = dispatcher_->dispatch(*ptr);
  if (!result) {
    delete ptr;
  }
  return result;
}

long EventDispatcher::schedule(EventBase_rch event, const MonotonicTimePoint& expiration)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return -1;
  }
  EventCaller* ptr = new EventCaller(event);
  const long result = dispatcher_->schedule(*ptr, expiration);
  if (result < 0) {
    delete ptr;
  }
  return result;
}

size_t EventDispatcher::cancel(long id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!dispatcher_) {
    return 0;
  }
  return dispatcher_->cancel(id);
}

EventDispatcher::EventCaller::EventCaller(EventBase_rch event)
  : event_(event)
{
}

void EventDispatcher::EventCaller::operator()()
{
  event_->handle_event();
  delete this;
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
      expiration_ = now + duration;
      long id = dispatcher->schedule(rchandle_from(this), expiration_);
      if (id > 0) {
        timer_id_ = id;
      }
    }
  } else {
    if (now + duration < expiration_) {
      EventDispatcher_rch dispatcher = dispatcher_.lock();
      if (dispatcher) {
        if (dispatcher->cancel(timer_id_)) {
          timer_id_ = 0;
          expiration_ = now + duration;
          long id = dispatcher->schedule(rchandle_from(this), expiration_);
          if (id > 0) {
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
  RcHandle<EventBase> event = event_.lock();
  if (event) {
    guard.release();
    event->handle_event();
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
