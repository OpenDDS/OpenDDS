// -*- C++ -*-
//
// $Id$
#include  "DCPS/DdsDcps_pch.h"
#include  "NullSynch.h"
#include  "NullSynchStrategy.h"

#include "EntryExit.h"


TAO::DCPS::NullSynchStrategy::NullSynchStrategy()
{
  DBG_ENTRY_LVL("NullSynchStrategy","NullSynchStrategy",5);
}


TAO::DCPS::NullSynchStrategy::~NullSynchStrategy()
{
  DBG_ENTRY_LVL("NullSynchStrategy","~NullSynchStrategy",5);
}


TAO::DCPS::ThreadSynch*
TAO::DCPS::NullSynchStrategy::create_synch_object
                                       (ThreadSynchResource* synch_resource)
{
  DBG_ENTRY_LVL("NullSynchStrategy","create_synch_object",5);

  if (synch_resource != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Coding Error - NullSynchStrategy::"
                 "create_synch_object() should always get a NULL pointer "
                 "(ThreadSynchResource*) argument.\n"));
    }

  return new NullSynch(0);
}
