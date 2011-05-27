// -*- C++ -*-
//
// $Id$

#include  "EntryExit.h"


ACE_INLINE
TAO::DCPS::NullSynch::NullSynch(ThreadSynchResource* resource)
  : ThreadSynch(0)
{
  DBG_ENTRY_LVL("NullSynch","NullSynch",5);
  ACE_UNUSED_ARG(resource);
}

