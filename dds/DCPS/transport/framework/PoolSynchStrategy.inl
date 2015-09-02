/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::PoolSynchStrategy::PoolSynchStrategy()
  : condition_(this->lock_)
{
  DBG_ENTRY_LVL("PoolSynchStrategy","PoolSynchStrategy",6);
}
