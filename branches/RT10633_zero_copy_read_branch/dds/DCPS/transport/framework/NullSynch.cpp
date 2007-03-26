// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "NullSynch.h"
#include "ThreadSynchResource.h"


#if !defined (__ACE_INLINE__)
#include "NullSynch.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::NullSynch::~NullSynch()
{
  DBG_ENTRY_LVL("NullSynch","~NullSynch",5);
}


void
TAO::DCPS::NullSynch::work_available()
{
  DBG_ENTRY_LVL("NullSynch","work_available",5);

  ACE_ERROR((LM_ERROR,
             "(%P|%t) INTERNAL ERROR - NullSynch::work_available() "
             "method should *NEVER* be called!\n"));
}
