/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

#ifndef DEVELOPMENT
#define DEVELOPMENT 0 // DEVELOPMENT DIAGNOSTICS ONLY
#endif

ACE_INLINE
OpenDDS::DCPS::PerConnectionSynch::PerConnectionSynch(
  ThreadSynchResource* synch_resource)
  : ThreadSynch(synch_resource)
{
  DBG_ENTRY_LVL("PerConnectionSynch","PerConnectionSynch",6);
  if(DEVELOPMENT) {
    std::size_t id = 0;
    if( worker()) {
      id = worker()->id();
    }
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) PerConnectionSynch::PerConnectionSynch() [%d] - ")
               ACE_TEXT("establishing synch %C worker.\n"),
               id,(worker()?"with":"without")));
  }
  
  // Belts and suspenders.
  if(worker()) {
    (int)worker()->cancel_wakeup( ACE_Event_Handler::WRITE_MASK);
  }
}

