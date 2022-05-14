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
public:
  SporadicEvent(EventDispatcher_rch dispatcher, EventBase_rch event);

  void schedule(const TimeDuration& duration);
  void cancel();

  void handle_event();

private:
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
