// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"

ACE_INLINE
TAO::DCPS::PerConnectionSynch::PerConnectionSynch
                                        (ThreadSynchResource* synch_resource)
  : ThreadSynch(synch_resource),
    condition_(this->lock_),
    work_available_(0),
    shutdown_(0)
{
  DBG_ENTRY_LVL("PerConnectionSynch","PerConnectionSynch",5);
}

