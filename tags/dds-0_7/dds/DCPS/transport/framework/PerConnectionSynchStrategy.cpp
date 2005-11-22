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
  DBG_ENTRY("PerConnectionSynchStrategy","~PerConnectionSynchStrategy");
}


TAO::DCPS::ThreadSynch*
TAO::DCPS::PerConnectionSynchStrategy::create_synch_object
                                       (ThreadSynchResource* synch_resource)
{
  DBG_ENTRY("PerConnectionSynchStrategy","create_synch_object");
  return new PerConnectionSynch(synch_resource);
}
