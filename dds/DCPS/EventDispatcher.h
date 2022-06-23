/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_EVENT_DISPATCHER_H
#define OPENDDS_DCPS_EVENT_DISPATCHER_H

#include "Definitions.h"
#include "RcObject.h"
#include "TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

/**
 * EventBase is an abstract base class for events
 *
 * EventBase is an abstract base class for events dispatched or scheduled by
 * an EventDispatcher. Derived classes are required to implement handle_event
 * which will be called when the event is dispatched by the event dispatcher
 */
struct OpenDDS_Dcps_Export EventBase : public virtual RcObject
{
  virtual ~EventBase();

  /// Called when the event is dispatched by an EventDispatcher
  virtual void handle_event() = 0;

  /// Only called when an exception is caught during handle_event
  virtual void handle_error();

  /// Called when the dispatch or schedule of an event is canceled by an
  /// EventDispatcher, for example when immediate shutdown is requested
  virtual void handle_cancel();

  void operator()();
};
typedef RcHandle<EventBase> EventBase_rch;

/**
 * PmfEvent is a helper class for adapting void member funtions of existing
 * classes into dispatchable events
 */
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

/**
 * PmfNowEvent is a helper class for adapting MonotonicTimePoint-accepting member
 * funtions of existing classes into dispatchable events. (c.f. PmfEvent)
 */
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

/**
 * EventDispatcher is an interface for the scheduling and dispatching of events
 *
 * Implementations of EventDispatcher will accept reference counted handles to
 * event objects derived from EventBase either for immediate dispatch (dispatch)
 * or for future scheduled dispatch (schedule). Scheduled dispatches return id
 * values which can be used to cancel the dispatch before the scheduled time.
 */
class OpenDDS_Dcps_Export EventDispatcher : public virtual RcObject
{
public:

  EventDispatcher();
  virtual ~EventDispatcher();

  /**
   * Request shutdown of this EventDispatcher, which prevents sucessful future
   * calls to either dispatch or schedule and cancels all scheduled events.
   * @param immediate prevent any further dispatches from event queue, otherwise allow current queue to empty
   */
  virtual void shutdown(bool immediate = false) = 0;

  /*
   * Dispatch an event.
   * @param event the event to dispatch
   * @return true if event successfully enqueue, otherwise false
   */
  virtual bool dispatch(EventBase_rch event) = 0;

  /*
   * Schedule the future dispatch of an event.
   * @param event the event to dispatch
   * @param expiration the requested dispatch time (no earlier than)
   * @return -1 on a failure, otherwise the timer id for the future dispatch
   */
  virtual long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now()) = 0;

  /*
   * Cancel a scheduled event dispatch.
   * @param id the id of the event timer to cancel
   * @return the number of canceled events (0 for failure to cancel, 1 for success)
   */
  virtual size_t cancel(long id) = 0;

};
typedef RcHandle<EventDispatcher> EventDispatcher_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENT_DISPATCHER_H
