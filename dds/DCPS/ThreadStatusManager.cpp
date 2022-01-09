/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ThreadStatusManager.h"
#include "ThreadMonitor.h"
#include "Service_Participant.h"

#include <ace/Arg_Shifter.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Argv_Type_Converter.h>

#include <exception>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

static const ACE_TCHAR COMMON_SECTION_NAME[] = ACE_TEXT("common");
static bool got_rtps_thread_key = false;
static bool got_rtps_max_wait = false;

int ThreadStatusManager::parse_args(int &argc, ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-RTPSThreadKey"))) != 0) {
      rtps_thr_key_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      arg_shifter.consume_arg();
      got_rtps_thread_key = true;

    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-RTPSMaxDelay"))) != 0) {
      rtps_max_wait_ = ACE_OS::atoi(ACE_TEXT_ALWAYS_CHAR(currentArg));
      arg_shifter.consume_arg();
      got_rtps_max_wait = true;

    } else {
      arg_shifter.ignore_arg();
    }
  }

  return 0;
}

int ThreadStatusManager::load_common_configuration(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key sect;

  if (cf.open_section(root, COMMON_SECTION_NAME, false, sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // a common section. The code default configuration will be used.
      ACE_DEBUG((LM_NOTICE, ACE_TEXT(
      "(%P|%t) NOTICE: Service_Participant::load_common_configuration ")
      ACE_TEXT("failed to open section %s\n"), COMMON_SECTION_NAME));
    }

    return 0;

  } else {
    const ACE_TCHAR* message = ACE_TEXT("(%P|%t) NOTICE: using %s value from command option (overrides value if it's in config file)\n");
    if (got_rtps_thread_key) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("RTPSThreadKey")));
    } else {
      ACE_TString key(ACE_TEXT("VSEDP"));
      GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("RTPSThreadKey"), key);
      rtps_thr_key_ = ACE_TEXT_ALWAYS_CHAR(key.c_str());
    }
    if (got_rtps_max_wait) {
      ACE_DEBUG((LM_NOTICE, message, ACE_TEXT("RTPSMaxWait")));
    } else {
      int delay = 0;
      GET_CONFIG_VALUE(cf, sect, ACE_TEXT("RTPSMaxWait"), delay, int)
      rtps_max_wait_ = TimeDuration(delay);
    }
  }

  return 0;
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

bool ThreadStatusManager::update_busy(const String& thread_key, double pbusy, bool saturated)
{
  const SystemTimePoint now = SystemTimePoint::now();
  ThreadStatus status = ThreadStatus_Running;
  bool sat = false;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  Map::iterator it = map_.find(thread_key);
  if (it != map_.end()) {
    sat = it->second.saturated;
    if (saturated != sat) {
      sat = saturated;
    }
    it->second.timestamp = now;
    it->second.utilization = pbusy;
    it->second.saturated = saturated;
  } else {
    map_[thread_key] = Thread(now, status, pbusy, saturated);
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
