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

struct OpenDDS_Dcps_Export EventBase : public virtual RcObject
{
  virtual ~EventBase();

  virtual void handle_event() = 0;
  virtual void handle_error();
  virtual void handle_cancel();

  void operator()();
};
typedef RcHandle<EventBase> EventBase_rch;

template <typename Delegate>
class PmfEvent : public EventBase
{
public:
  typedef void (Delegate::*PMF)();

  PmfEvent(RcHandle<Delegate> delegate, PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  void handle_event()
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)();
    }
  }

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;
};

template <typename Delegate>
class PmfNowEvent : public EventBase
{
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfNowEvent(RcHandle<Delegate> delegate, PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  void handle_event()
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)(MonotonicTimePoint::now());
    }
  }

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;
};

class OpenDDS_Dcps_Export EventDispatcher : public virtual RcObject
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
  mutable ACE_Thread_Mutex mutex_;
  WeakRcHandle<EventDispatcher> dispatcher_;
  RcHandle<EventBase> event_;
  MonotonicTimePoint expiration_;
  long timer_id_;
};

class OpenDDS_Dcps_Export PeriodicEvent : public EventBase
{
public:
  PeriodicEvent(EventDispatcher_rch dispatcher, EventBase_rch event);

  void enable(const TimeDuration& period, bool strict_timing = true);
  void disable();
  bool enabled() const;

  void handle_event();

private:
  mutable ACE_Thread_Mutex mutex_;
  WeakRcHandle<EventDispatcher> dispatcher_;
  RcHandle<EventBase> event_;
  TimeDuration period_;
  bool strict_timing_;
  MonotonicTimePoint expiration_;
  long timer_id_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENTDISPATCHER_H
