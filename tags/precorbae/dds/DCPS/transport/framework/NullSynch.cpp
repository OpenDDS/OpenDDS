// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "NullSynch.h"
#include  "ThreadSynchResource.h"


#if !defined (__ACE_INLINE__)
#include "NullSynch.inl"
#endif /* __ACE_INLINE__ */

TAO::DCPS::NullSynch::~NullSynch()
{
  DBG_ENTRY("NullSynch","~NullSynch");
}


void
TAO::DCPS::NullSynch::work_available()
{
  DBG_ENTRY("NullSynch","work_available");

  ACE_ERROR((LM_ERROR,
             "(%P|%t) INTERNAL ERROR - NullSynch::work_available() "
             "method should *NEVER* be called!\n"));
}
