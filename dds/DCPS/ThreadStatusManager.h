/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADSTATUSMANAGER_H
#define OPENDDS_DCPS_THREADSTATUSMANAGER_H

#include "dcps_export.h"
#include "RcObject.h"
#include "TimeTypes.h"
#include "ConditionVariable.h"
#include "SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

enum ThreadStatus {
  ThreadStatus_Running,
  ThreadStatus_Finished
};

struct OpenDDS_Dcps_Export ThreadStatusManager {
  struct Thread {
    Thread() {}
    Thread(const SystemTimePoint& time, ThreadStatus status, double util, bool sat)
      : timestamp(time)
      , status(status)
      , utilization(util)
      , saturated(sat)
    {}
    SystemTimePoint timestamp;
    ThreadStatus status;
    double utilization;
    bool saturated;
    // TODO(iguessthislldo): Add Participant GUID
  };
  typedef OPENDDS_MAP(String, Thread) Map;

  static const char* status_to_string(ThreadStatus status);

  /// Get key for map and update.
  /// safety_profile_tid is the thread id under safety profile, otherwise unused.
  /// name is for a more human-friendly name that will be appended to the key.
  static String get_key(const char* safety_profile_tid = "", const String& name = "");

  /// Update the busy percent for the identified thread
  bool update_busy(const String& key, double pbusy);

  /// Update the status of a thread to indicate it was able to check in at the
  /// given time. Returns false if failed.
  bool update(const String& key,
              ThreadStatus status = ThreadStatus_Running,
              double utilization = 0.0,
              bool saturated = false);

  /// To support multiple readers determining that a thread finished without
  /// having to do something more complicated to cleanup that fact, have a
  /// Manager for each reader use this to get the information the readers need.
  bool sync_with_parent(ThreadStatusManager& parent, Map& running, Map& finished);

  /// Pauses caller when the rtpsrelay thread utilization exceeds the high
  /// water mark until it drops below the low water mark.
  void rtps_flow_control();

  /// update the flow control setting based on the rtps relay thread saturation
  /// status.
  void rtps_set_flow_control(bool saturated);

#ifdef ACE_HAS_GETTID
  static inline pid_t gettid()
  {
    return syscall(SYS_gettid);
  }
#endif

  ThreadStatusManager()
  : map_()
  , lock_()
  , rtps_lock_()
  , rtps_pauser_(rtps_lock_)
  , rtps_paused_(false)
  {

  }

  static void init(const String& key, double hwm, double lwm, ACE_UINT64 secs);

  static void get_levels(double& hwm, double& lwm);


private:
  static String rtps_thr_key_;
  static double rtps_util_hwm_;
  static double rtps_util_lwm_;
  static TimeDuration rtps_max_wait_;

  Map map_;

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;
  typedef ConditionVariable<LockType> ConditionVariableType;

  LockType lock_;
  LockType rtps_lock_;
  ConditionVariableType rtps_pauser_;
  bool rtps_paused_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_THREADSTATUSMANAGER_H */
