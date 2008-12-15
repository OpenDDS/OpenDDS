// -*- C++ -*-
//
// $Id$

#include "EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::NullSynch::NullSynch(ThreadSynchResource* resource)
  : ThreadSynch(0)
{
  DBG_ENTRY_LVL("NullSynch","NullSynch",6);
  ACE_UNUSED_ARG(resource);
}

