/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DISPATCH_SERVICE_H
#define OPENDDS_DCPS_DISPATCH_SERVICE_H

#include "ConditionVariable.h"
#include "Definitions.h"
#include "RcObject.h"
#include "ThreadPool.h"
#include "TimePoint_T.h"

#include <ace/Thread_Mutex.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export DispatchService : public virtual RcObject {
public:

  /// Helper function for adapting arbitrary function objects (with operator())
  template <typename T>
  static void fun_ptr_proxy(void* arg)
  {
    T& ref = *static_cast<T*>(arg);
    ref();
  }

  /// Typedef for Dispatch Return Values
  typedef bool DispatchStatus;

  /// DispatchStatus Success Constant
  static const bool DS_SUCCESS = true;

  /// DispatchStatus Error Constant
  static const bool DS_ERROR = false;

  /// Typedef for Schedule Return Values
  typedef long TimerId;

  /// TimerId Failure Constant
  static const long TI_FAILURE = -1;

  typedef void (*FunPtr)(void*);
  typedef std::pair<FunPtr, void*> FunArgPair;
  typedef OPENDDS_DEQUE(FunArgPair) EventQueue;

  /**
   * Create a DispatchService
   * @param count the requested size of the internal thread pool
   */
  explicit DispatchService(size_t count = 1);

  virtual ~DispatchService();

  /**
   * Request shutdown of this DispatchService, which prevents sucessful future
   * calls to either dispatch or schedule and cancels all scheduled events.
   * @param immediate prevent any further dispatches from event queue, otherwise allow current queue to empty
   * @param pending An EventQueue object in which to store canceled events in case extra processing is required
   */
  void shutdown(bool immediate = false, EventQueue* pending = 0);

  /**
   * Dispatch an event
   * @param fun the function pointer to dispatch
   * @param arg the argument to pass during dispatch
   * @return true if event successfully enqueue, otherwise false
   */
  DispatchStatus dispatch(FunPtr fun, void* arg = 0);

  /// Helper function to dispatch arbitrary function objects (see fun_ptr_proxy)
  template <typename T>
  DispatchStatus dispatch(T& ref)
  {
    return dispatch(fun_ptr_proxy<T>, &ref);
  }

  /**
   * Schedule the future dispatch of an event
   * @param fun the function pointer to schedule
   * @param arg the argument to pass during dispatch
   * @param expiration the requested dispatch time (no earlier than)
   * @return -1 on a failure, otherwise the timer id for the future dispatch
   */
  TimerId schedule(FunPtr fun, void* arg = 0, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  /// Helper function to schedule arbitrary function objects (see fun_ptr_proxy)
  template <typename T>
  TimerId schedule(T& ref, const MonotonicTimePoint& expiration = MonotonicTimePoint::now())
  {
    return schedule(fun_ptr_proxy<T>, &ref, expiration);
  }

  /**
   * Cancel a scheduled event by id
   * @param id the scheduled timer id to cancel
   * @param arg if specified, arg will be set to the arg value passed in at time of scheduling
   * @return the number of timers successfully canceled (0 or 1)
   */
  size_t cancel(TimerId id, void** arg = 0);

  /**
   * Cancel a scheduled event by function pointer
   * @param fun the function pointer of the event/s to cancel
   * @param arg the argument passed at time of scheduling
   * @return the number of timers successfully canceled (potentially several)
   */
  size_t cancel(FunPtr fun, void* arg = 0);

  /// Helper function to cancel arbitrary function objects (see fun_ptr_proxy)
  template <typename T>
  size_t cancel(T& ref)
  {
    return cancel(fun_ptr_proxy<T>, &ref);
  }

private:

  static ACE_THR_FUNC_RETURN run(void* arg);
  void run_event_loop();

  typedef std::pair<FunArgPair, TimerId> TimerPair;
  typedef OPENDDS_MULTIMAP(MonotonicTimePoint, TimerPair) TimerQueueMap;
  typedef OPENDDS_MAP(TimerId, TimerQueueMap::iterator) TimerIdMap;

  mutable ACE_Thread_Mutex mutex_;
  mutable ConditionVariable<ACE_Thread_Mutex> cv_;
  bool allow_dispatch_;
  bool stop_when_empty_;
  bool running_;
  size_t running_threads_;
  EventQueue event_queue_;
  TimerQueueMap timer_queue_map_;
  TimerIdMap timer_id_map_;
  TimerId max_timer_id_;
  ThreadPool pool_;
};
typedef RcHandle<DispatchService> DispatchService_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_DISPATCH_SERVICE_H
