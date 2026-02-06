/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ThreadStatusManager.h"
#include "SafetyProfileStreams.h"
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
      current_bucket_ = (current_bucket_ + 1) % bucket_count;
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

ThreadId ThreadStatusManager::get_thread_id()
{
#ifdef ACE_WIN32
  return static_cast<unsigned>(ACE_Thread::self());
#elif defined ACE_HAS_GETTID
  return ACE_OS::thr_gettid();
#else
  char buffer[32];
  const size_t len = ACE_OS::thr_id(buffer, 32);
  return String(buffer, len);
#endif
}

void ThreadStatusManager::add_thread(const String& name)
{
  if (!update_thread_status()) {
    return;
  }

  const ThreadId thread_id = get_thread_id();
  const Thread thread(name, thread_id, ACE_OS::thr_self());

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) ThreadStatusManager::add_thread: "
               "adding thread %C\n", thread.bit_key().c_str()));
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  map_.insert(std::make_pair(thread_id, thread));
  if (thread_status_listener_) {
    thread_status_listener_->on_thread_started(thread.info());
  }
}

void ThreadStatusManager::set_thread_status_listener(ThreadStatusListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  thread_status_listener_ = listener;
}

void ThreadStatusManager::update_i(Thread::ThreadStatus status, bool finished,
                                   bool nested, int detail1, int detail2)
{
  if (!update_thread_status()) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  const MonotonicTimePoint m_now = MonotonicTimePoint::now();
  const SystemTimePoint s_now = SystemTimePoint::now();
  const ThreadId thread_id = get_thread_id();

  const Map::iterator pos = map_.find(thread_id);
  if (pos != map_.end()) {
    Thread& thread = pos->second;
    thread.update(m_now, s_now, status, bucket_limit_, nested, detail1, detail2);
    if (finished) {
      finished_.push_back(thread);
      map_.erase(pos);
      if (thread_status_listener_) {
        thread_status_listener_->on_thread_finished(thread.info());
      }
    }
  }
  cleanup(m_now);
}

void ThreadStatusManager::harvest(const MonotonicTimePoint& start,
                                  ThreadStatusManager::List& running,
                                  ThreadStatusManager::List& finished) const
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

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

void ThreadStatusManager::cleanup(const MonotonicTimePoint& now)
{
  const MonotonicTimePoint cutoff = now - 10 * thread_status_interval_;

  while (!finished_.empty() && finished_.front().last_update() < cutoff) {
    finished_.pop_front();
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
