// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "PoolSynchStrategy.h"
#include  "PoolSynch.h"


#if !defined (__ACE_INLINE__)
#include "PoolSynchStrategy.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::PoolSynchStrategy::~PoolSynchStrategy()
{
  DBG_ENTRY("PoolSynchStrategy","~PoolSynchStrategy");
}


TAO::DCPS::ThreadSynch*
TAO::DCPS::PoolSynchStrategy::create_synch_object
                                      (ThreadSynchResource* synch_resource)
{
  DBG_ENTRY("PoolSynchStrategy","create_synch_object");
  PoolSynch* synch_object = new PoolSynch(this,synch_resource);

  // TBD - We need to remember the synch_object here, because if this
  //       PoolSynchStrategy (an active object) is shutdown, it will
  //       need these... right?  Or is it the other way around?  In any
  //       regard, we know that the PoolSynch needs a pointer to us
  //       because we are the active object.
  
  return synch_object;
}


int
TAO::DCPS::PoolSynchStrategy::open(void*)
{
  DBG_ENTRY("PoolSynchStrategy","open");
  // TBD
  return 0;
}


int
TAO::DCPS::PoolSynchStrategy::svc()
{
  DBG_ENTRY("PoolSynchStrategy","svc");
  // TBD
  return 0;
}


int
TAO::DCPS::PoolSynchStrategy::close(u_long)
{
  DBG_ENTRY("PoolSynchStrategy","close");
  // TBD
  return 0;
}
