// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PerConnectionSynchStrategy.h"
#include "PerConnectionSynch.h"


#if !defined (__ACE_INLINE__)
#include "PerConnectionSynchStrategy.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::PerConnectionSynchStrategy::~PerConnectionSynchStrategy()
{
  DBG_ENTRY_LVL("PerConnectionSynchStrategy","~PerConnectionSynchStrategy",5);
}


OpenDDS::DCPS::ThreadSynch*
OpenDDS::DCPS::PerConnectionSynchStrategy::create_synch_object
                                       (ThreadSynchResource* synch_resource)
{
  DBG_ENTRY_LVL("PerConnectionSynchStrategy","create_synch_object",5);
  return new PerConnectionSynch(synch_resource);
}
