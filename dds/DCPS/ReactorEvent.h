/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTOR_EVENT_H
#define OPENDDS_DCPS_REACTOR_EVENT_H

#include "EventDispatcher.h"
#include "RcEventHandler.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * ReactorEvent is an event which, when triggered, will attempt to run the specified handle_event
 * function within a reactor callback (i.e. on the reactor thread). This is handy for avoiding
 * deadlocks when calling into ACE_Reactor-aware functions (e.g. calling join on a multicast socket).
 */
class OpenDDS_Dcps_Export ReactorEvent : public virtual EventBase, public virtual RcEventHandler {
public:

  /**
   * Creates a ReactorEvent to handle scheduling a base event with an EventDispatcher
   * @param reactor the ACE_Reactor to use for notifications
   * @param event the base event (c.f EventBase) to schedule for dispatch
   */
  ReactorEvent(ACE_Reactor* reactor, EventBase_rch event);

  /**
   * For use by EventDispatcher
   */
  void handle_event();

private:

  int handle_exception(ACE_HANDLE fd);

  ACE_Reactor* reactor_;
  RcHandle<EventBase> event_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_REACTOR_EVENT_H
