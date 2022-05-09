/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_EVENTDISPATCHER_H
#define OPENDDS_DCPS_EVENTDISPATCHER_H

#include "EventDispatcherLite.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

struct EventBase : public RcObject
{
  virtual void handle_event() = 0;
};
typedef RcHandle<EventBase> EventBase_rch;

class OpenDDS_Dcps_Export EventDispatcher : public RcObject
{
public:

  EventDispatcher(size_t count = 1);
  virtual ~EventDispatcher();

  void shutdown();

  void dispatch(EventBase_rch event);

  long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  size_t cancel(long id);

private:

  void dispatch_i(EventBase_rch event);

  struct EventCaller
  {
    EventCaller(EventBase_rch event, RcHandle<EventDispatcher> dispatcher);

    void operator()();

    EventBase_rch event_;
    WeakRcHandle<EventDispatcher> dispatcher_;
    OPENDDS_LIST(EventCaller)::iterator iter_;
  };

  struct TimerCaller
  {
    TimerCaller(EventBase_rch event, RcHandle<EventDispatcher> dispatcher);

    void operator()();

    EventBase_rch event_;
    WeakRcHandle<EventDispatcher> dispatcher_;
    OPENDDS_LIST(TimerCaller)::iterator iter_;
    long timer_id_;
  };

  mutable ACE_Thread_Mutex mutex_;
  typedef OPENDDS_LIST(EventCaller) EventList;
  EventList event_list_;
  typedef OPENDDS_LIST(TimerCaller) TimerList;
  TimerList timer_list_;
  typedef OPENDDS_MAP(long, TimerList::iterator) TimerIdMap;
  TimerIdMap timer_id_map_;

  EventDispatcherLite_rch dispatcher_;
};
typedef RcHandle<EventDispatcher> EventDispatcher_rch;

class OpenDDS_Dcps_Export SporadicEvent : public EventBase
{
  SporadicEvent(EventDispatcher_rch dispatcher, EventBase_rch event) : dispatcher_(dispatcher), event_(event), timer_id_(0) {}

  void schedule(const TimeDuration& duration)
  {
    const MonotonicTimePoint now = MonotonicTimePoint::now();
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (timer_id_ < 1) {
      EventDispatcher_rch dispatcher = dispatcher_.lock();
      if (dispatcher) {
        expiration_ = now + duration;
        long id = dispatcher->schedule(rchandle_from(this), expiration_);
        if (id > 0) {
          timer_id_ = id;
        }
      }
    } else {
      if (now + duration < expiration_) {
        EventDispatcher_rch dispatcher = dispatcher_.lock();
        if (dispatcher) {
          if (dispatcher->cancel(timer_id_)) {
            timer_id_ = 0;
            expiration_ = now + duration;
            long id = dispatcher->schedule(rchandle_from(this), expiration_);
            if (id > 0) {
              timer_id_ = id;
            }
          }
        }
      }
    }
  }

  void cancel()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    if (timer_id_ > 0) {
      EventDispatcher_rch dispatcher = dispatcher_.lock();
      if (dispatcher) {
        if (dispatcher->cancel(timer_id_)) {
          timer_id_ = 0;
        }
      }
    }
  }

  void handle_event()
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    timer_id_ = 0;
    RcHandle<EventBase> event = event_.lock();
    if (event) {
      guard.release();
      event->handle_event();
    }
  }

  ACE_Thread_Mutex mutex_;
  WeakRcHandle<EventDispatcher> dispatcher_;
  WeakRcHandle<EventBase> event_;
  MonotonicTimePoint expiration_;
  long timer_id_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENTDISPATCHER_H
