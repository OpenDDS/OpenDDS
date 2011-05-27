// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"

ACE_INLINE
TAO::DCPS::PoolSynchStrategy::PoolSynchStrategy()
  : condition_(this->lock_),
    shutdown_(0)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","PoolSynchStrategy",5);
}

