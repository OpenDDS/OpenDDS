/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

#include <iostream> // DEVELOPMENT DIAGNOSTICS ONLY
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
    std::cerr << std::dec << getpid()
              << " ESTABLISHING SYNCH "
              << (worker()? "WITH ": "WITHOUT ") << " WORKER"
              << std::hex << this << std::endl;
  }
  
  // Belts and suspenders.
  if(worker()) {
    (int)worker()->cancel_wakeup( ACE_Event_Handler::WRITE_MASK);
  }
}

