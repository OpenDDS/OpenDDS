/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/EventDispatcher.h"

#include <ace/Reverse_Lock_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

EventDispatcher::EventDispatcher(size_t count)
 : cv_(mutex_)
 , running_(true)
 , pool_(count, run, this)
{
}

EventDispatcher::~EventDispatcher()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  running_ = false;
  cv_.notify_all();
}

EventDispatcher::DispatchStatus EventDispatcher::dispatch(FunPtr fun, void* arg)
{
  OPENDDS_ASSERT(fun);
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  event_queue_.push(std::make_pair(fun, arg));
  cv_.notify_one();
  return DS_SUCCESS;
}

EventDispatcher::DispatchStatus EventDispatcher::schedule(FunPtr fun, void* arg, const MonotonicTimePoint& expiration)
{
  OPENDDS_ASSERT(fun);
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  timer_queue_map_.insert(std::make_pair(expiration, std::make_pair(fun, arg)));
  cv_.notify_one();
  return DS_SUCCESS;
}

void* EventDispatcher::run(void* arg)
{
  EventDispatcher& dispatcher = *(static_cast<EventDispatcher*>(arg));
  dispatcher.run_event_loop();
  return 0;
}

void EventDispatcher::run_event_loop()
{
  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  while (running_) {

    // Logical Order:
    // - Move expired timer events into normal event queue
    // - Wait appropriate length if there's nothing to do
    // - Check for early exit before execution
    // - Run first task from event queue

    const MonotonicTimePoint now = MonotonicTimePoint::now();

    TimerQueueMap::iterator last = timer_queue_map_.upper_bound(now), pos = last;
    while (pos != timer_queue_map_.begin()) {
      --pos;
      event_queue_.push(pos->second);
    }
    if (last != timer_queue_map_.begin()) {
      timer_queue_map_.erase(timer_queue_map_.begin(), last);
    }

    if (event_queue_.empty()) {
      if (timer_queue_map_.size()) {
        MonotonicTimePoint deadline(timer_queue_map_.begin()->first);
        cv_.wait_until(deadline, tsm_);
      } else {
        cv_.wait(tsm_);
      }
    }

    if (!running_ || event_queue_.empty()) continue;

    FunArgPair pair = event_queue_.front();
    event_queue_.pop();
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rev_guard(rev_lock);
    pair.first(pair.second);
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
