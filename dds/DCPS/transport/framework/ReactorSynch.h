/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REACTORSYNCH_H
#define OPENDDS_DCPS_REACTORSYNCH_H

#include "ThreadSynch.h"
#include "ScheduleOutputHandler.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace DCPS {

/**
 * @class ReactorSynch
 *
 * @brief Send thread synchronization utilizing a reactor.
 *
 * This class implements sending thread synchronization by scheduling
 * qeueued data sending to be handled by a reactor.  The initial
 * implementation will share the TransportReactorTask reactor for all
 * sending threads; which means that all receiving and queued sends will
 * be sharing the same thread.  Or thread pool.
 *
 * Calls to enable and disable sending on queued data by the reactor are
 * scheduled to be enabled and disabled on the reactor by notifying the
 * reactor to make the update.  This ensures that no locks will be held
 * (other than the Reactor::token_) when the call into the reactor is
 * made.
 *
 * References to the containing TransportSendStrategy and the reactor are
 * held as raw pointers since they are guaranteed to be valid for the
 * lifetime of this object (this is held in the same containing object).
 */
class ReactorSynch : public ThreadSynch {
public:
  /// Construct with raw pointers from the containing
  /// TransportSendStrategy.
  ReactorSynch(ThreadSynchResource* synch_resource,
               TransportSendStrategy* strategy,
               ACE_Reactor* reactor);

  virtual ~ReactorSynch();

  virtual void work_available();

private:
  /// Notification event handler.
  ScheduleOutputHandler* scheduleOutputHandler_;

  /// Memory management using the provided (kluge) var type.
  ACE_Event_Handler_var safeHandler_;
};

}} // End namespace OpenDDS::DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ReactorSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_REACTORSYNCH_H */

