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

struct OpenDDS_Dcps_Export EventBase : public RcObject
{
  virtual void handle_event() = 0;
  virtual void handle_error();
  virtual void handle_cancel();

  void operator()();
};
typedef RcHandle<EventBase> EventBase_rch;

class OpenDDS_Dcps_Export EventDispatcher : public RcObject
{
public:

  EventDispatcher(size_t count = 1);
  virtual ~EventDispatcher();

  void shutdown(bool immediate = false);

  bool dispatch(EventBase_rch event);

  long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now());

  size_t cancel(long id);

private:

  mutable ACE_Thread_Mutex mutex_;
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
