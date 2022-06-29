/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SPORADIC_EVENT_H
#define OPENDDS_DCPS_SPORADIC_EVENT_H

#include "EventDispatcher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * SporadicEvent is an event which can be scheduled multiple times but will only dispatch once
 *
 * SporadicEvent serves as a helpful utility class for allowing repeated calls
 * to schedule (when the application is unsure if the event has already been
 * scheduled, but cumulative calls to schedule will only result in a single
 * dispatch. After the base event dispatches, additional calls to schedule
 * will result in another dispatch. Additional calls to schedule may, however,
 * shorten the scheduled time until the base event dispatches. The SporadicEvent
 * takes both an EventDispatcher and a base event (c.f. EventBase) and handles
 * the logic of scheduling, rescheduling, and canceling the base event with the
 * EventDispatcher.
 */
class OpenDDS_Dcps_Export SporadicEvent : public EventBase
{
public:

  /**
   * Creates a SporadicEvent to handle scheduling a base event with an EventDispatcher
   * @param dispatcher the EventDispatcher to use for scheduling
   * @param event the base event (c.f EventBase) to schedule for dispatch
   */
  SporadicEvent(EventDispatcher_rch dispatcher, EventBase_rch event);

  /**
   * Schedule the SporadicEvent to dispatch the base event after the specified
   * duration. If the SporadicEvent is already scheduled, the duration is
   * compared with the existing scheduled time and the shorter (sooner) of the
   * two durations is used.
   * @param duration the time period used to schedule / reschedule the base event
   */
  void schedule(const TimeDuration& duration);

  /**
   * Cancel the SporadicEvent, canceling the scheduled base event if scheduled.
   */
  void cancel();

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
  MonotonicTimePoint expiration_;
  long timer_id_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SPORADIC_EVENT_H
