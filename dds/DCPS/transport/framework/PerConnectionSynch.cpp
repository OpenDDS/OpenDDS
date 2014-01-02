/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PerConnectionSynch.h"
#include "dds/DCPS/debug.h"

#ifndef DEVELOPMENT
#define DEVELOPMENT 0 // DEVELOPMENT DIAGNOSTICS ONLY
#endif

#if !defined (__ACE_INLINE__)
#include "PerConnectionSynch.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::PerConnectionSynch::~PerConnectionSynch()
{
  DBG_ENTRY_LVL("PerConnectionSynch","~PerConnectionSynch",6);
    if(DEVELOPMENT) {
      std::size_t id = 0;
      if( worker()) {
        id = worker()->id();
      }
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) PerConnectionSynch::~PerConnectionSynch() [%d] - ")
                 ACE_TEXT("dismantling synch %C worker.\n"),
                 id,(worker()?"with":"without")));
    }
  
  // Belts and suspenders.
  if(worker()) {
    (int)worker()->cancel_wakeup( ACE_Event_Handler::WRITE_MASK);
  }
}

void
OpenDDS::DCPS::PerConnectionSynch::work_available()
{
  DBG_ENTRY_LVL("PerConnectionSynch","work_available",6);

  // Use the reactor thread to handle backpressure processing.
  if(worker()) {
    (int)worker()->schedule_wakeup( ACE_Event_Handler::WRITE_MASK);
  }
}

