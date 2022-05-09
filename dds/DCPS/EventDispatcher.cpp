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

void EventDispatcher::shutdown()
{
  EventDispatcherLite_rch copy = dispatcher_;
  if (copy) {
    copy->shutdown();
  }
}

void EventDispatcher::dispatch(EventBase_rch event)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  dispatch_i(event);
}

long EventDispatcher::schedule(EventBase_rch event, const MonotonicTimePoint& expiration)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_list_.push_back(TimerCaller(event, rchandle_from(this)));
  TimerCaller& tc = timer_list_.back();
  tc.iter_ = timer_list_.end();
  --tc.iter_;
  const long id = dispatcher_->schedule(tc, expiration);
  if (id > 0) {
    tc.timer_id_ = id;
    timer_id_map_[id] = tc.iter_;
  } else {
    timer_list_.pop_back();
  }
  return id;
}

size_t EventDispatcher::cancel(long id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  size_t result = dispatcher_->cancel(id);
  TimerIdMap::iterator pos = timer_id_map_.find(id);
  if (pos != timer_id_map_.end()) {
    timer_list_.erase(pos->second);
    timer_id_map_.erase(pos);
  }
  return result;
}

void EventDispatcher::dispatch_i(EventBase_rch event)
{
  event_list_.push_back(EventCaller(event, rchandle_from(this)));
  EventCaller& ec = event_list_.back();
  ec.iter_ = event_list_.end();
  --ec.iter_;
  dispatcher_->dispatch(event_list_.back());
}

EventDispatcher::EventCaller::EventCaller(EventBase_rch event, RcHandle<EventDispatcher> dispatcher)
 : event_(event)
 , dispatcher_(dispatcher)
{
}

void EventDispatcher::EventCaller::operator()()
{
  RcHandle<EventDispatcher> dispatcher = dispatcher_.lock();
  if (dispatcher) {
    event_->handle_event();
    ACE_Guard<ACE_Thread_Mutex> guard(dispatcher->mutex_);
    dispatcher->event_list_.erase(iter_);
  }
}

EventDispatcher::TimerCaller::TimerCaller(EventBase_rch event, RcHandle<EventDispatcher> dispatcher)
 : event_(event)
 , dispatcher_(dispatcher)
{
}

void EventDispatcher::TimerCaller::operator()()
{
  RcHandle<EventDispatcher> dispatcher = dispatcher_.lock();
  if (dispatcher) {
    ACE_Guard<ACE_Thread_Mutex> guard(dispatcher->mutex_);
    dispatcher->dispatch_i(event_);
    dispatcher->timer_id_map_.erase(timer_id_);
    dispatcher->timer_list_.erase(iter_);
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
