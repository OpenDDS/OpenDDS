/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::PoolSynch::PoolSynch(PoolSynchStrategy* strategy,
                                    ThreadSynchResource* synch_resource)
  : ThreadSynch(synch_resource),
    strategy_(strategy)
{
  DBG_ENTRY_LVL("PoolSynch","PoolSynch",6);
  // TBD
}
