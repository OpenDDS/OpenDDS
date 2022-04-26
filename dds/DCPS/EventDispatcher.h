/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_EVENTDISPATCHER_H
#define OPENDDS_DCPS_EVENTDISPATCHER_H

#include "Definitions.h"
#include "ThreadPool.h"
#include "TimePoint_T.h"
#include "TimeDuration.h"

#include <ace/Thread_Mutex.h>
#include <ace/Reverse_Lock_T.h>
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

class OpenDDS_Dcps_Export EventDispatcher
{
public:
  enum DispatchStatus : uint32_t
  {
    DS_UNKNOWN = 0,
    DS_SUCCESS = 1,
    DS_ERROR = 2
  };

  EventDispatcher(size_t count = 1);
  virtual ~EventDispatcher();

  typedef void*(*FunPtr)(void*);

  DispatchStatus dispatch(FunPtr fun, void* arg = NULL);

  template <typename T>
  DispatchStatus dispatch(T& ref) {
    return dispatch(fun_ptr_proxy<T>, &ref);
  }

  DispatchStatus schedule(FunPtr fun, void* arg = NULL, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  template <typename T>
  DispatchStatus schedule(T& ref, const MonotonicTimePoint& expiration = MonotonicTimePoint::now()) {
    return schedule(fun_ptr_proxy<T>, &ref, expiration);
  }

private:

  static void* run(void* arg);
  void run_event_loop();

  typedef std::pair<FunPtr, void*> FunArgPair;
  typedef OPENDDS_QUEUE(FunArgPair) EventQueue;
  typedef OPENDDS_MULTIMAP(MonotonicTimePoint, FunArgPair) TimerQueueMap;

  mutable ACE_Thread_Mutex mutex_;
  mutable ConditionVariable<ACE_Thread_Mutex> cv_;
  bool running_;
  EventQueue event_queue_;
  TimerQueueMap timer_queue_map_;
  ThreadStatusManager tsm_;
  ThreadPool pool_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENTDISPATCHER_H
