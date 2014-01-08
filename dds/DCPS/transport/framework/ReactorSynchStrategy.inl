/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ReactorSynchStrategy::ReactorSynchStrategy(
  TransportSendStrategy* strategy,
  ACE_Reactor* reactor,
  ACE_HANDLE handle
)
 : strategy_( strategy),
   reactor_( reactor),
   handle_( handle)
{
  DBG_ENTRY_LVL("ReactorSynchStrategy","ReactorSynchStrategy",6);
}

