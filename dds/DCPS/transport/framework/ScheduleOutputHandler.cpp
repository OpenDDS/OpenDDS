/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ScheduleOutputHandler.h"

#include "TransportSendStrategy.h"

#if !defined (__ACE_INLINE__)
#include "ScheduleOutputHandler.inl"
#endif /* __ACE_INLINE__ */

#ifndef DEVELOPMENT
#define DEVELOPMENT 0 // DEVELOPMENT DIAGNOSTICS ONLY
#endif

void
OpenDDS::DCPS::ScheduleOutputHandler::schedule_output()
{
  DBG_ENTRY_LVL("ScheduleOutputHandler","schedule_output",6);

  // Only emit once into the notification queue since we check which
  // operation to perform when we process.
  if( reference_count_ == 1) {
    /// Filter the notifications here to reduce load.
    TransportSendStrategy::SendMode mode = strategy_->mode();

    if( ( (state_ ==  Enabled) && (mode == TransportSendStrategy::MODE_DIRECT))
     || ( (state_ == Disabled) && ( (mode == TransportSendStrategy::MODE_QUEUE)
                                 || (mode == TransportSendStrategy::MODE_SUSPEND)))) {
      (int)reactor()->notify(this);
    }
  }
}

int
OpenDDS::DCPS::ScheduleOutputHandler::handle_exception(ACE_HANDLE)
{
  DBG_ENTRY_LVL("ScheduleOutputHandler","handle_exception",6);

  if( reference_count_ == 1) {
    // The containing TransportSendStrategy has unregistered, so we are
    // going away and can't determine the mode, don't process.
    return -1;
  }

  // Check the *current* mode value as it might have been changed since
  // we were scheduled to run.
  //
  // We already hold the Reactor::token_ (since we are being called from
  // the reactor), and it will see the recursion and allow the call back
  // into the reactor.  We hold no other locks, and so avoid deadlock.
  TransportSendStrategy::SendMode mode = strategy_->mode();
  bool changed = false;

  // We need to recheck the mode here since it might have already changed.
  if( mode == TransportSendStrategy::MODE_DIRECT) {
    // Don't cancel a canceled handle.
    if( state_ == Enabled) {
      (int)reactor()->cancel_wakeup( handle_, ACE_Event_Handler::WRITE_MASK);
      state_ = Disabled;
      changed = true;
    }

  } else if( (mode == TransportSendStrategy::MODE_QUEUE)
          || (mode == TransportSendStrategy::MODE_SUSPEND)) {

    // Don't schedule a scheduled handle.
    if( state_ == Disabled) {
      (int)reactor()->schedule_wakeup( handle_, ACE_Event_Handler::WRITE_MASK);
      state_ = Enabled;
      changed = true;
    }
  }

  if(DEVELOPMENT) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ScheduleOutputHandler::handle_exception() - [%d] ")
               ACE_TEXT("%C data queueing for handle %d.\n"),
               strategy_->id(),
               (changed? ((state_ == Enabled)? "starting": "canceling"): "declining to change"),
               handle_));
  }

  // Terminate the upcall and remove from the reactor, if there (and
  // decrement_reference()).
  return -1;
}

