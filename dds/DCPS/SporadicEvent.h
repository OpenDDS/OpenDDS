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

namespace OpenDDS
{
namespace DCPS
{

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

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SPORADIC_EVENT_H
