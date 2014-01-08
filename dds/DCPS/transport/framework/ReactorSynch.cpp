/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ReactorSynch.h"
#include "dds/DCPS/debug.h"

#ifndef DEVELOPMENT
#define DEVELOPMENT 0 // DEVELOPMENT DIAGNOSTICS ONLY
#endif

#if !defined (__ACE_INLINE__)
#include "ReactorSynch.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::ReactorSynch::~ReactorSynch()
{
  DBG_ENTRY_LVL("ReactorSynch","~ReactorSynch",6);

  if(DEVELOPMENT) {
    std::size_t id = 0;
    if( worker()) {
      id = worker()->id();
    }
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReactorSynch::~ReactorSynch() [%d] - ")
               ACE_TEXT("dismantling synch %C worker.\n"),
               id,(worker()?"with":"without")));
  }
}

void
OpenDDS::DCPS::ReactorSynch::work_available()
{
  DBG_ENTRY_LVL("ReactorSynch","work_available",6);

  // Schedule queued data to be sent by the reactor.
  scheduleOutputHandler_->schedule_output();
}

