/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ThreadStatusManager.h"
#include "ThreadMonitor.h"

#include <exception>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

String ThreadStatusManager::rtps_thr_key_("VSEDP");
double ThreadStatusManager::rtps_util_hwm_(0.500);
double ThreadStatusManager::rtps_util_lwm_(0.500);
TimeDuration ThreadStatusManager::rtps_max_wait_(10,0);

void ThreadStatusManager::init(const String& key, double high_water_mark, double low_water_mark, ACE_UINT64 secs)
{
  rtps_thr_key_ = key;
  rtps_util_hwm_ = high_water_mark;
  rtps_util_lwm_ = low_water_mark;
  rtps_max_wait_ = TimeDuration(secs, 0);
  if (ThreadMonitor::installed_monitor_) {
    ThreadMonitor::installed_monitor_->set_levels(rtps_util_hwm_,
                                                  rtps_util_lwm_);
  }
}

void ThreadStatusManager::get_levels(double& hwm, double& lwm)
{
  hwm = rtps_util_hwm_;
  lwm = rtps_util_lwm_;
}

const char* ThreadStatusManager::status_to_string(ThreadStatus status)
{
  switch (status) {
  case ThreadStatus_Running:
    return "Running";

  case ThreadStatus_Finished:
    return "Finished";

  default:
    if (DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ThreadStatusManager::status_to_string: ")
        ACE_TEXT("%d is either invalid or not recognized.\n"),
        status));
    }
    return "<Invalid thread status>";
  }
}

bool ThreadStatusManager::update_busy(const String& thread_key, double pbusy)
{
  const SystemTimePoint now = SystemTimePoint::now();
  ThreadStatus status = ThreadStatus_Running;
  bool sat = false;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  Map::iterator it = map_.find(thread_key);
  if (it != map_.end()) {
    sat = it->second.saturated;
    if (pbusy >= rtps_util_hwm_) {
      sat = true;
    } else if (pbusy <= rtps_util_lwm_) {
      sat = false;
    }
    it->second.timestamp = now;
    it->second.utilization = pbusy;
    it->second.saturated = sat;
  } else {
    sat = pbusy >= rtps_util_hwm_;
    map_[thread_key] = Thread(now, status, pbusy, sat);
  }
  if (thread_key == rtps_thr_key_) {
    rtps_set_flow_control(sat);
  }
  return true;
}

bool ThreadStatusManager::update(const String& thread_key,
                                 ThreadStatus status,
                                 double utilization,
                                 bool saturated)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  const SystemTimePoint now = SystemTimePoint::now();
  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) ThreadStatus::update: "
      "update for thread \"%C\" %C @ %d, utilization = %F saturated = %d\n",
      thread_key.c_str(), status_to_string(status), now.value().sec(), utilization, saturated));
  }
  switch (status) {
  case ThreadStatus_Finished:
    {
      Map::iterator it = map_.find(thread_key);
      if (it == map_.end()) {
        if (DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: ThreadStatus::update: "
            "Trying to remove \"%C\", but it's not an existing thread!\n",
            thread_key.c_str()));
        }
        return false;
      }
      map_.erase(it);
    }
    break;

  default:
    if (map_.find(thread_key) == map_.end() && ThreadMonitor::installed_monitor_) {
      ThreadMonitor::installed_monitor_->preset(this, thread_key.c_str());
    }
    map_[thread_key] = Thread(now, status, utilization, saturated);
    if (thread_key == rtps_thr_key_) {
      rtps_set_flow_control(saturated);
    }
  }

  return true;
}

String ThreadStatusManager::get_key(const char* safety_profile_tid, const String& name)
{
  String key;
#ifdef OPENDDS_SAFETY_PROFILE
  key = safety_profile_tid;
#else
  ACE_UNUSED_ARG(safety_profile_tid);
#  ifdef ACE_HAS_MAC_OSX
  unsigned long tid = 0;
  uint64_t u64_tid;
  if (!pthread_threadid_np(0, &u64_tid)) {
    tid = static_cast<unsigned long>(u64_tid);
  } else if (DCPS_debug_level) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: pthread_threadid_np failed\n")));
  }
#  elif defined ACE_HAS_GETTID
  const pid_t tid = gettid();
#  else
  const ACE_thread_t tid = ACE_OS::thr_self();
#  endif

  key = to_dds_string(tid);
#endif

  if (name.length()) {
    key += " (" + name + ")";
  }

  return key;
}

bool ThreadStatusManager::sync_with_parent(ThreadStatusManager& parent,
  ThreadStatusManager::Map& running, ThreadStatusManager::Map& finished)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g2, parent.lock_, false);
    running = parent.map_;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g1, lock_, false);
  ACE_DEBUG((LM_DEBUG, "%T TSM::sync_with_parent called\n"));
  // Figure out what threads were removed from parent.map_
  Map::iterator mi = map_.begin();
  Map::iterator ri = running.begin();
  while (mi != map_.end() || ri != running.end()) {
    const int cmp = mi != map_.end() && ri != running.end() ?
      std::strcmp(mi->first.c_str(), ri->first.c_str()) :
      ri != running.end() ? 1 : -1;
    if (cmp < 0) { // We're behind, this thread was removed
      finished.insert(*mi);
      ++mi;
    } else if (cmp > 0) { // We're ahead, this thread was added
      ++ri;
    } else { // Same thread, continue
      ++mi;
      ++ri;
    }
  }

  map_ = running;
  return true;
}

void ThreadStatusManager::rtps_set_flow_control(bool sat)
{
  ACE_DEBUG((LM_DEBUG, "%T rtps set flow_control, sat = %d\n", sat));
  if (!rtps_paused_) {
    if (sat) {
      GuardType g(rtps_lock_);
      rtps_paused_ = true;
    }
  } else if (!sat) {
    GuardType g(rtps_lock_);
    rtps_paused_ = false;
    rtps_pauser_.notify_all();
  }
}

void ThreadStatusManager::rtps_flow_control()
{
  GuardType g(rtps_lock_);
  while (rtps_paused_) {
    MonotonicTimePoint expire = MonotonicTimePoint::now() + rtps_max_wait_;
    CvStatus res = rtps_pauser_.wait_until(expire);
    if (res == CvStatus_Timeout) {
      if (DCPS::DCPS_debug_level >= 4) {
        ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) RTPS flow control wait timed out.\n")));
      }
      break;
    } else if (res == CvStatus_Error) {
      if (DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) Rtps flow control error, cannot wait.\n")));
      }
      break;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
