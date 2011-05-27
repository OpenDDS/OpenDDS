// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::PoolSynch::PoolSynch(PoolSynchStrategy* strategy,
                                ThreadSynchResource* synch_resource)
  : ThreadSynch(synch_resource),
    strategy_(strategy)
{
  DBG_ENTRY_LVL("PoolSynch","PoolSynch",5);
  // TBD
}

