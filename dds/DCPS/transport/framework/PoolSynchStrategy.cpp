/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PoolSynchStrategy.h"
#include "PoolSynch.h"

#if !defined (__ACE_INLINE__)
#include "PoolSynchStrategy.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::PoolSynchStrategy::~PoolSynchStrategy()
{
  DBG_ENTRY_LVL("PoolSynchStrategy","~PoolSynchStrategy",6);
}

OpenDDS::DCPS::ThreadSynch*
OpenDDS::DCPS::PoolSynchStrategy::create_synch_object(
  ThreadSynchResource* synch_resource,
  long                 /* priority */,
  int                  /* scheduler */)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","create_synch_object",6);
  PoolSynch* synch_object = new PoolSynch(this,synch_resource);

  // TBD - We need to remember the synch_object here, because if this
  //       PoolSynchStrategy (an active object) is shutdown, it will
  //       need these... right?  Or is it the other way around?  In any
  //       regard, we know that the PoolSynch needs a pointer to us
  //       because we are the active object.

  return synch_object;
}

int
OpenDDS::DCPS::PoolSynchStrategy::open(void*)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","open",6);
  // TBD
  return 0;
}

int
OpenDDS::DCPS::PoolSynchStrategy::svc()
{
  DBG_ENTRY_LVL("PoolSynchStrategy","svc",6);
  // TBD
  return 0;
}

int
OpenDDS::DCPS::PoolSynchStrategy::close(u_long)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","close",6);
  // TBD
  return 0;
}
