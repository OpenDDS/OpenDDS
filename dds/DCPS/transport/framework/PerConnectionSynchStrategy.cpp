// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "PerConnectionSynchStrategy.h"
#include  "PerConnectionSynch.h"


#if !defined (__ACE_INLINE__)
#include "PerConnectionSynchStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::PerConnectionSynchStrategy::~PerConnectionSynchStrategy()
{
  DBG_ENTRY_LVL("PerConnectionSynchStrategy","~PerConnectionSynchStrategy",5);
}


TAO::DCPS::ThreadSynch*
TAO::DCPS::PerConnectionSynchStrategy::create_synch_object
                                       (ThreadSynchResource* synch_resource)
{
  DBG_ENTRY_LVL("PerConnectionSynchStrategy","create_synch_object",5);
  return new PerConnectionSynch(synch_resource);
}
