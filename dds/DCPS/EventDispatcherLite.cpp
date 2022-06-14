/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "EventDispatcherLite.h"
#include "TimeDuration.h"

#include <ace/Reverse_Lock_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

EventDispatcherLite::EventDispatcherLite(size_t count)
 : cv_(mutex_)
 , allow_dispatch_(true)
 , stop_when_empty_(false)
 , running_(true)
 , running_threads_(0)
 , max_timer_id_(LONG_MAX)
 , pool_(count, run, this)
{
}

EventDispatcherLite::~EventDispatcherLite()
{
  shutdown();
}

void EventDispatcherLite::shutdown(bool immediate, EventQueue* const pending)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  allow_dispatch_ = false;
  stop_when_empty_ = true;
  running_ = running_ && !immediate; // && with existing state in case shutdown has already been called
  cv_.notify_all();

  if (pool_.contains(ACE_Thread::self())) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Warning :: EventDispatcherLite::shutdown: Contained Thread Attempting To Call Shutdown."));
    if (pending) {
      pending->clear();
    }
    return;
  }

  while (running_threads_) {
    cv_.wait(tsm_);
  }

  if (pending) {
    pending->clear();
    pending->swap(event_queue_);
    for (TimerQueueMap::const_iterator it = timer_queue_map_.begin(), limit = timer_queue_map_.end(); it != limit; ++it) {
      pending->push_back(it->second.first);
    }
  } else {
    event_queue_.clear();
  }
  timer_queue_map_.clear();
  timer_id_map_.clear();
}

EventDispatcherLite::DispatchStatus EventDispatcherLite::dispatch(FunPtr fun, void* arg)
{
  if (!fun) {
    return DS_ERROR;
  }

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (allow_dispatch_) {
    event_queue_.push_back(std::make_pair(fun, arg));
    cv_.notify_one();
    return DS_SUCCESS;
  }
  return DS_ERROR;
}

EventDispatcherLite::TimerId EventDispatcherLite::schedule(FunPtr fun, void* arg, const MonotonicTimePoint& expiration)
{
  if (!fun) {
    return -1;
  }

  TimerId id = 0;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (allow_dispatch_) {
    TimerQueueMap::iterator pos = timer_queue_map_.insert(std::make_pair(expiration, std::make_pair(std::make_pair(fun, arg), 0)));
    // Make it a loop in case we ever recycle timer ids
    const TimerId starting_id = max_timer_id_;
    do {
      id = max_timer_id_ = max_timer_id_ == LONG_MAX ? 1 : max_timer_id_ + 1;
      if (id == starting_id) {
        return -1; // all ids in use ?!
      }
      pos->second.second = id;
    } while (timer_id_map_.insert(std::make_pair(id, pos)).second == false);
    cv_.notify_one();
    return id;
  }
  return -1;
}

size_t EventDispatcherLite::cancel(EventDispatcherLite::TimerId id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  TimerIdMap::iterator pos = timer_id_map_.find(id);
  if (pos != timer_id_map_.end()) {
    if (pos->second == timer_queue_map_.begin()) {
      cv_.notify_all();
    }
    timer_queue_map_.erase(pos->second);
    timer_id_map_.erase(pos);
    return 1;
  }
  return 0;
}

size_t EventDispatcherLite::cancel(FunPtr fun, void* arg)
{
  OPENDDS_ASSERT(fun);
  size_t count = 0;
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  for (TimerQueueMap::iterator it = timer_queue_map_.begin(); it != timer_queue_map_.end();) {
    if (it->second.first.first == fun && it->second.first.second == arg) {
      if (it == timer_queue_map_.begin()) {
        cv_.notify_all();
      }
      timer_id_map_.erase(it->second.second);
      timer_queue_map_.erase(it++);
      ++count;
    } else {
      ++it;
    }
  }
  return count;
}

ACE_THR_FUNC_RETURN EventDispatcherLite::run(void* arg)
{
  EventDispatcherLite& dispatcher = *(static_cast<EventDispatcherLite*>(arg));
  dispatcher.run_event_loop();
  return 0;
}

void EventDispatcherLite::run_event_loop()
{
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ++running_threads_;
  while (running_) {

    // Logical Order:
    // - Move expired timer events into normal event queue
    // - Wait appropriate length if there's nothing to do
    // - Check for early exit before execution
    // - Run first task from event queue

    if (allow_dispatch_) {
      const MonotonicTimePoint now = MonotonicTimePoint::now();

      TimerQueueMap::iterator last = timer_queue_map_.upper_bound(now), pos = last;
      while (pos != timer_queue_map_.begin()) {
        --pos;
        event_queue_.push_back(pos->second.first);
        timer_id_map_.erase(pos->second.second);
      }
      if (last != timer_queue_map_.begin()) {
        timer_queue_map_.erase(timer_queue_map_.begin(), last);
      }
    }

    if (event_queue_.empty()) {
      if (stop_when_empty_) {
        running_ = false;
        cv_.notify_all();
      } else if (allow_dispatch_ && timer_queue_map_.size()) {
        MonotonicTimePoint deadline(timer_queue_map_.begin()->first);
        cv_.wait_until(deadline, tsm_);
      } else {
        cv_.wait(tsm_);
      }
    }

    if (!running_ || event_queue_.empty()) continue;

    FunArgPair pair = event_queue_.front();
    event_queue_.pop_front();
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    pair.first(pair.second);
  }
  --running_threads_;
  cv_.notify_all();
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
