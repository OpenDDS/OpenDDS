/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PROACTOR_EVENT_DISPATCHER_H
#define OPENDDS_DCPS_PROACTOR_EVENT_DISPATCHER_H

#include "EventDispatcher.h"
#include "ThreadPool.h"

#include <ace/Refcounted_Auto_Ptr.h>
#include <ace/Proactor.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ProactorEventDispatcher : public EventDispatcher, public virtual ACE_Handler {
public:
  /**
   * Create a ProactorEventDispatcher
   * @param count the requested size of the internal thread pool (see DispatchProactor)
   */
  explicit ProactorEventDispatcher(size_t count = 1);
  virtual ~ProactorEventDispatcher();

  void shutdown(bool immediate = false);

  bool dispatch(EventBase_rch event);

  long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  size_t cancel(long id);

  void handle_time_out(const ACE_Time_Value& tv, const void* act = 0);

private:

  typedef ACE_Refcounted_Auto_Ptr<ACE_Proactor, ACE_Thread_Mutex> Proactor_rap;

  static ACE_THR_FUNC_RETURN run(void* arg);

  mutable ACE_Thread_Mutex mutex_;
  mutable ConditionVariable<ACE_Thread_Mutex> cv_;
  size_t active_threads_;
  Proactor_rap proactor_;
  RcHandle<ThreadPool> pool_;
};
typedef RcHandle<ProactorEventDispatcher> ProactorEventDispatcher_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_PROACTOR_EVENT_DISPATCHER_H
