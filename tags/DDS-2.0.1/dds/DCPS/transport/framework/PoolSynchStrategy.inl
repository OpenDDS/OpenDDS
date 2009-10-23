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
OpenDDS::DCPS::PoolSynchStrategy::PoolSynchStrategy()
  : condition_(this->lock_),
    shutdown_(0)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","PoolSynchStrategy",6);
}
