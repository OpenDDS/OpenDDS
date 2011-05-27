// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::PoolSynch::PoolSynch(PoolSynchStrategy* strategy,
                                ThreadSynchResource* synch_resource)
  : ThreadSynch(synch_resource),
    strategy_(strategy)
{
  DBG_ENTRY("PoolSynch","PoolSynch");
  // TBD
}

