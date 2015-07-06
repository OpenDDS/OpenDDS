/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::ScheduleOutputHandler::ScheduleOutputHandler(
  TransportSendStrategy* strategy,
  ACE_Reactor* reactor
) : ACE_Event_Handler( reactor),
    strategy_( strategy),
    state_( Disabled)
{
  DBG_ENTRY_LVL("ScheduleOutputHandler","ScheduleOutputHandler",6);

  reference_counting_policy().value(
    ACE_Event_Handler::Reference_Counting_Policy::ENABLED);
}

