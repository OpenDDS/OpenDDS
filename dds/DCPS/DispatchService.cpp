/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"
#include "DispatchService.h"
#include "Service_Participant.h"
#include "TimeDuration.h"

#include <ace/Reverse_Lock_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DispatchService::DispatchService(size_t count)
 : cv_(mutex_)
 , allow_dispatch_(true)
 , stop_when_empty_(false)
 , running_(true)
 , running_threads_(0)
 , max_timer_id_(LONG_MAX)
 , pool_(count, run, this)
{
}

DispatchService::~DispatchService()
{
  shutdown();
}

void DispatchService::shutdown(bool immediate, EventQueue* const pending)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  allow_dispatch_ = false;
  stop_when_empty_ = true;
  running_ = running_ && !immediate; // && with existing state in case shutdown has already been called
  cv_.notify_all();

  if (pool_.contains(ACE_Thread::self())) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR :: DispatchService::shutdown: Contained Thread Attempting To Call Shutdown.\n"));
    }
    if (pending) {
      pending->clear();
    }
    return;
  }

  while (running_threads_) {
    cv_.wait(TheServiceParticipant->get_thread_status_manager());
  }

  if (pending) {
    pending->clear();
    pending->swap(event_queue_);
    const TimerQueueMap& cmap = timer_queue_map_;
    for (TimerQueueMap::const_iterator it = cmap.begin(), limit = cmap.end(); it != limit; ++it) {
      pending->push_back(it->second.first);
    }
  } else {
    event_queue_.clear();
  }
  timer_queue_map_.clear();
  timer_id_map_.clear();
}

DispatchService::DispatchStatus DispatchService::dispatch(FunPtr fun, void* arg)
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

DispatchService::TimerId DispatchService::schedule(FunPtr fun, void* arg, const MonotonicTimePoint& expiration)
{
  if (!fun) {
    return TI_FAILURE;
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
        return TI_FAILURE; // all ids in use ?!
      }
      pos->second.second = id;
    } while (timer_id_map_.insert(std::make_pair(id, pos)).second == false);
    cv_.notify_one();
    return id;
  }
  return TI_FAILURE;
}

size_t DispatchService::cancel(DispatchService::TimerId id, void** arg)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  TimerIdMap::iterator pos = timer_id_map_.find(id);
  if (pos != timer_id_map_.end()) {
    if (pos->second == timer_queue_map_.begin()) {
      cv_.notify_all();
    }
    if (arg) {
      *arg = pos->second->second.first.second;
    }
    timer_queue_map_.erase(pos->second);
    timer_id_map_.erase(pos);
    return 1;
  }
  return 0;
}

size_t DispatchService::cancel(FunPtr fun, void* arg)
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

ACE_THR_FUNC_RETURN DispatchService::run(void* arg)
{
  DispatchService& dispatcher = *static_cast<DispatchService*>(arg);
  dispatcher.run_event_loop();
  return 0;
}

void DispatchService::run_event_loop()
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

    if (allow_dispatch_ && !timer_queue_map_.empty()) {
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
        cv_.wait_until(deadline, TheServiceParticipant->get_thread_status_manager());
      } else {
        cv_.wait(TheServiceParticipant->get_thread_status_manager());
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
