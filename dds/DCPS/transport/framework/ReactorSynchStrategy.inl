/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReactorSynchStrategy::ReactorSynchStrategy(
  TransportSendStrategy* strategy,
  ACE_Reactor* reactor
)
 : strategy_( strategy),
   reactor_( reactor)
{
  DBG_ENTRY_LVL("ReactorSynchStrategy","ReactorSynchStrategy",6);
}

