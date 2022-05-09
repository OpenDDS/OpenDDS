/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_EVENTDISPATCHERLITE_H
#define OPENDDS_DCPS_EVENTDISPATCHERLITE_H

#include "Definitions.h"
#include "RcObject.h"
#include "ThreadPool.h"
#include "TimePoint_T.h"

#include <ace/Thread_Mutex.h>
#include "ConditionVariable.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

template <typename T>
void* fun_ptr_proxy(void* arg)
{
  T& ref = *static_cast<T*>(arg);
  ref();
  return 0;
}

class OpenDDS_Dcps_Export EventDispatcherLite : public RcObject
{
public:

  typedef bool DispatchStatus;
  typedef long TimerId;

  const bool DS_SUCCESS = true;
  const bool DS_ERROR = false;

  EventDispatcherLite(size_t count = 1);
  virtual ~EventDispatcherLite();

  void shutdown();

  typedef void*(*FunPtr)(void*);

  DispatchStatus dispatch(FunPtr fun, void* arg = NULL);

  template <typename T>
  DispatchStatus dispatch(T& ref) {
    return dispatch(fun_ptr_proxy<T>, &ref);
  }

  TimerId schedule(FunPtr fun, void* arg = NULL, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  template <typename T>
  TimerId schedule(T& ref, const MonotonicTimePoint& expiration = MonotonicTimePoint::now()) {
    return schedule(fun_ptr_proxy<T>, &ref, expiration);
  }

  size_t cancel(TimerId id);

  size_t cancel(FunPtr fun, void* arg = NULL);

  template <typename T>
  size_t cancel(T& ref) {
    return cancel(fun_ptr_proxy<T>, &ref);
  }

private:

  static ACE_THR_FUNC_RETURN run(void* arg);
  void run_event_loop();

  typedef std::pair<FunPtr, void*> FunArgPair;
  typedef OPENDDS_QUEUE(FunArgPair) EventQueue;
  typedef std::pair<FunArgPair, TimerId> TimerPair;
  typedef OPENDDS_MULTIMAP(MonotonicTimePoint, TimerPair) TimerQueueMap;
  typedef OPENDDS_MAP(TimerId, TimerQueueMap::iterator) TimerIdMap;

  mutable ACE_Thread_Mutex mutex_;
  mutable ConditionVariable<ACE_Thread_Mutex> cv_;
  bool running_;
  size_t running_threads_;
  EventQueue event_queue_;
  TimerQueueMap timer_queue_map_;
  TimerIdMap timer_id_map_;
  TimerId max_timer_id_;
  ThreadStatusManager tsm_;
  ThreadPool pool_;
};
typedef RcHandle<EventDispatcherLite> EventDispatcherLite_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENTDISPATCHERLITE_H
