/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "PoolSynch.h"
#include "PoolSynchStrategy.h"
#include "ThreadSynchResource.h"

#if !defined (__ACE_INLINE__)
#include "PoolSynch.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::PoolSynch::~PoolSynch()
{
  DBG_ENTRY_LVL("PoolSynch","~PoolSynch",6);
  // TBD
}

void
OpenDDS::DCPS::PoolSynch::work_available()
{
  DBG_ENTRY_LVL("PoolSynch","work_available",6);
  // TBD
}

void
OpenDDS::DCPS::PoolSynch::unregister_worker_i()
{
  DBG_ENTRY_LVL("PoolSynch","unregister_worker_i",6);
  // TBD
}
