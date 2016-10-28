/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_SCHEDULEOUTPUTHANDER_H
#define OPENDDS_SCHEDULEOUTPUTHANDER_H

#include <ace/Reactor.h>
#include <ace/Event_Handler.h>
#include "dds/DCPS/PoolAllocationBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace DCPS {

class TransportSendStrategy;

/**
 * @class ScheduleOutputHandler
 *
 * @brief event handler used to enable and disable output processing.
 *
 * This class implements a simple notification handler that is used to
 * schedule or cancel output processing for queued data according to the
 * current mode state of the TransportSendStrategy.  If the send strategy
 * is queueing data, then the reactor is enabled to process on output
 * events.  Otherwise the output processing callbacks are cancelled.
 */
class ScheduleOutputHandler : public ACE_Event_Handler, public PoolAllocationBase {
  public:
    /// Construct with the reactor and strategy.
    ScheduleOutputHandler( TransportSendStrategy* strategy,
                           ACE_Reactor* reactor);

    /// @{ @name ACE_Event_Handler methods

    /// modify the reactor mask for the handle.
    virtual int handle_exception( ACE_HANDLE);

    /// @}

    /// Update output processing in the reactor.
    void schedule_output();

  private:
    /// Strategy sending data to be scheduled (or not).
    TransportSendStrategy* strategy_;

    /// Cache the state that we have set the reactor into.
    enum HandlerState { Disabled, Enabled };
    HandlerState state_;
};

}} // End of namespace OpenDDS::DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "ScheduleOutputHandler.inl"
#endif /* __ACE_INLINE__ */

#endif /* OPENDDS_SCHEDULEOUTPUTHANDER_H */
