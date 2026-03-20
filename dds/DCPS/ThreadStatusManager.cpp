/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ThreadStatusManager.h"
#include "SafetyProfileStreams.h"
#include "Hash.h"
#include "debug.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

String ThreadInfo::get_bit_key() const
{
  return to_dds_string(thread_id) + (name.empty() ? "" : (" (" + name + ")"));
}

ThreadStatusManager::Thread::Thread(const String& name, const ThreadId& thread_id, ACE_thread_t handle)
  : info_({name, thread_id, handle})
  , bit_key_(info_.get_bit_key())
  , timestamp_(SystemTimePoint::now())
  , last_update_(MonotonicTimePoint::now())
  , last_status_change_(MonotonicTimePoint::now())
  , status_(ThreadStatus_Active)
  , nesting_depth_(0)
  , detail1_(0)
  , detail2_(0)
  , current_bucket_(0)
{}

void ThreadStatusManager::Thread::update(const MonotonicTimePoint& m_now,
                                         const SystemTimePoint& s_now,
                                         ThreadStatus next_status,
                                         const TimeDuration& bucket_limit,
                                         bool nested, int detail1, int detail2)
{
  timestamp_ = s_now;

  if (nested) {
    (next_status == ThreadStatus_Active) ? ++nesting_depth_ : --nesting_depth_;
  }

  if (!nested ||
      (next_status == ThreadStatus_Active && nesting_depth_ == 1) ||
      (next_status == ThreadStatus_Idle && nesting_depth_ == 0)) {
    if (buckets_[current_bucket_].total_time() > bucket_limit) {
      current_bucket_ = (current_bucket_ + 1) % BUCKET_COUNT;
      Bucket& current = buckets_[current_bucket_];
      total_.active_time -= current.active_time;
      current.active_time = 0;
      total_.idle_time -= current.idle_time;
      current.idle_time = 0;
    }

    const TimeDuration t = m_now - last_update_;

    switch (status_) {
    case ThreadStatus_Active:
      buckets_[current_bucket_].active_time += t;
      total_.active_time += t;
      break;
    case ThreadStatus_Idle:
      buckets_[current_bucket_].idle_time += t;
      total_.idle_time += t;
      break;
    }

    last_status_change_ = m_now;
    status_ = next_status;
  }
  detail1_ = detail1;
  detail2_ = detail2;
  last_update_ = m_now;
}

namespace {
  TimeDuration bonus_time(const MonotonicTimePoint& now, const MonotonicTimePoint& last_change,
    ThreadStatusManager::Thread::ThreadStatus current_status, ThreadStatusManager::Thread::ThreadStatus target_status)
  {
    if (now > last_change && current_status == target_status) {
      return now - last_change;
    }
    return TimeDuration::zero_value;
  }
}

double ThreadStatusManager::Thread::utilization(const MonotonicTimePoint& now) const
{
  const TimeDuration active_bonus = bonus_time(now, last_status_change_, status_, ThreadStatus_Active),
    idle_bonus = bonus_time(now, last_status_change_, status_, ThreadStatus_Idle),
    denom = total_.total_time() + active_bonus + idle_bonus;

  if (!denom.is_zero()) {
    return (total_.active_time + active_bonus) / denom;
  }
  return 0;
}

void ThreadStatusManager::set_thread_status_listener(ThreadStatusListener* listener)
{
  ManagerInfo copy;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    manager_info_.set_thread_status_listener(listener);
    copy = manager_info_;
  }
  update_manager_info(copy);
}

void ThreadStatusManager::thread_status_interval(const TimeDuration& thread_status_interval)
{
  ManagerInfo copy;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    manager_info_.thread_status_interval(thread_status_interval);
    copy = manager_info_;
  }
  update_manager_info(copy);
}

const TimeDuration& ThreadStatusManager::thread_status_interval() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, TimeDuration::zero_value);
  return manager_info_.thread_status_interval();
}

bool ThreadStatusManager::update_thread_status() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  return manager_info_.update_thread_status();
}

ThreadId ThreadStatusManager::get_thread_id()
{
#ifdef ACE_WIN32
  return static_cast<ThreadId>(ACE_Thread::self());
#elif defined ACE_HAS_GETTID
  return ACE_OS::thr_gettid();
#else
  char buffer[32];
  const size_t len = ACE_OS::thr_id(buffer, 32);
  return String(buffer, len);
#endif
}

void ThreadStatusManager::ThreadContainer::set_manager_info(const ManagerInfo& manager_info)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  manager_info_ = manager_info;
}

void ThreadStatusManager::ThreadContainer::add_thread(const Thread& thread)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  map_.insert(std::make_pair(thread.info().thread_id, thread));
  manager_info_.on_thread_started(thread);
}

void ThreadStatusManager::ThreadContainer::update(
  Thread::ThreadStatus status, bool finished, bool nested, int detail1, int detail2,
  const MonotonicTimePoint& m_now, const SystemTimePoint& s_now, const ThreadId& thread_id)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  if (!manager_info_.update_thread_status()) {
    return;
  }

  const Map::iterator pos = map_.find(thread_id);
  const bool use_finished = manager_info_.thread_status_interval();
  if (pos != map_.end()) {
    Thread& thread = pos->second;
    thread.update(m_now, s_now, status, manager_info_.bucket_limit(), nested, detail1, detail2);
    if (finished) {
      manager_info_.on_thread_finished(thread);
      if (use_finished) {
        finished_.push_back(thread);
      }
      map_.erase(pos);
    }
  }

  // Cleanup finished, empty it if the thread_status_interval has been disabled
  if (use_finished || !finished_.empty()) {
    const MonotonicTimePoint cutoff = m_now - 10 * manager_info_.thread_status_interval();
    while (!finished_.empty() && finished_.front().last_update() < cutoff) {
      finished_.pop_front();
    }
  }
}

void ThreadStatusManager::ThreadContainer::harvest(const MonotonicTimePoint& start,
  ThreadStatusManager::List& running, ThreadStatusManager::List& finished) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  for (Map::const_iterator pos = map_.begin(), limit = map_.end(); pos != limit; ++pos) {
    if (pos->second.last_update() > start) {
      running.push_back(pos->second);
    }
  }

  for (List::const_iterator pos = finished_.begin(), limit = finished_.end(); pos != limit; ++pos) {
    if (pos->last_update() > start) {
      finished.push_back(*pos);
    }
  }
}

ThreadStatusManager::ThreadContainer& ThreadStatusManager::get_container(const ThreadId& tid)
{
#if defined (ACE_WIN32) || defined (ACE_HAS_GETTID)
  const size_t hash_key = static_cast<size_t>(tid);
#else
  const unsigned char* data = reinterpret_cast<const unsigned char*>(tid.c_str());
  const unsigned hash = fnv_1a_hash(data, tid.length());
  const size_t hash_key = static_cast<size_t>(hash);
#endif
  return containers_[hash_key % NUM_CONTAINERS];
}

void ThreadStatusManager::update_manager_info(const ManagerInfo& copy)
{
  for (size_t i = 0; i < NUM_CONTAINERS; ++i) {
    containers_[i].set_manager_info(copy);
  }
}

void ThreadStatusManager::add_thread(const String& name)
{
  const ThreadId thread_id = get_thread_id();
  const Thread thread(name, thread_id, ACE_OS::thr_self());

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) ThreadStatusManager::add_thread: "
               "adding thread %C\n", thread.bit_key().c_str()));
  }

  if (!update_thread_status()) {
    return;
  }

  get_container(thread_id).add_thread(thread);
}

void ThreadStatusManager::update_i(Thread::ThreadStatus status, bool finished,
                                   bool nested, int detail1, int detail2)
{
  const MonotonicTimePoint m_now = MonotonicTimePoint::now();
  const SystemTimePoint s_now = SystemTimePoint::now();
  const ThreadId thread_id = get_thread_id();

  get_container(thread_id).update(status, finished, nested, detail1, detail2, m_now, s_now, thread_id);
}

void ThreadStatusManager::harvest(const MonotonicTimePoint& start,
                                  ThreadStatusManager::List& running,
                                  ThreadStatusManager::List& finished) const
{
  for (size_t i = 0; i < NUM_CONTAINERS; ++i) {
    containers_[i].harvest(start, running, finished);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
