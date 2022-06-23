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

#endif // OPENDDS_DCPS_PERIODIC_EVENT_H
