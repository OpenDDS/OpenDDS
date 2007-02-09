// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "PoolSynch.h"
#include  "PoolSynchStrategy.h"
#include  "ThreadSynchResource.h"


#if !defined (__ACE_INLINE__)
#include "PoolSynch.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::PoolSynch::~PoolSynch()
{
  DBG_ENTRY_LVL("PoolSynch","~PoolSynch",5);
  // TBD
}


void
TAO::DCPS::PoolSynch::work_available()
{
  DBG_ENTRY_LVL("PoolSynch","work_available",5);
  // TBD
}


void
TAO::DCPS::PoolSynch::unregister_worker_i()
{
  DBG_ENTRY_LVL("PoolSynch","unregister_worker_i",5);
  // TBD
}
