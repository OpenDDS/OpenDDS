/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PERIODIC_EVENT_H
#define OPENDDS_DCPS_PERIODIC_EVENT_H

#include "EventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * PeriodicEvent is an event that is dispatched with a regular, repeating period
 *
 * PeriodicEvent serves as a helpful utility class for repeatedly dispatching
 * another base event at a regular, fixed interval. The PeriodicEvent takes both
 * an EventDispatcher and a base event (c.f. EventBase) and handles the logic
 * of scheduling, rescheduling, and canceling the base event with the
 * EventDispatcher.
 */
class OpenDDS_Dcps_Export PeriodicEvent : public EventBase
{
public:

  /**
   * Creates a PeriodicEvent to handle scheduling a base event with an EventDispatcher
   * @param dispatcher the EventDispatcher to use for scheduling
   * @param event the base event (c.f EventBase) to schedule for dispatch
   */
  PeriodicEvent(EventDispatcher_rch dispatcher, EventBase_rch event);

  /**
   * Enable the PeriodicEvent to begin scheduling / rescheduling the base event
   * using the internal EventDispatcher and the provided time period. If strict
   * scheduling is requested, the elapsed time since the previous dispatch will
   * not impact the time point provided for the next scheduled dispatch. Note:
   * strict timing may potentially lead to CPU starvation if the dispatch takes
   * longer than the requested period.
   * @param period the time period used to schedule / reschedule the base event
   * @param strict_timing set to true to strictly calculate scheduled dispatch times
   */
  void enable(const TimeDuration& period, bool strict_timing = true);

  /**
   * Disable the PeriodicEvent, canceling the scheduled base event if enabled.
   */
  void disable();

  /**
   * Check to see if the PeriodicEvent is currently enabled
   * @return true if the PeriodicEvent is currently enabled
   */
  bool enabled() const;

  /**
   * For use by EventDispatcher
   */
  void handle_event();

  /**
   * For use by EventDispatcher
   */
  void handle_cancel();

private:

  void handle_event_scheduling();

  mutable ACE_Thread_Mutex mutex_;
  mutable ACE_Thread_Mutex event_mutex_;
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

#endif // OPENDDS_DCPS_PERIODIC_EVENT_H
