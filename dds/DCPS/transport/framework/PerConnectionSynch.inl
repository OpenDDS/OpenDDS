/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::PerConnectionSynch::PerConnectionSynch(
  ThreadSynchResource* synch_resource,
  long                 priority,
  int                  scheduler)
  : ThreadSynch(synch_resource),
    condition_(this->lock_),
    work_available_(0),
    shutdown_(0),
    dds_priority_(priority),
    scheduler_(scheduler)
{
  DBG_ENTRY_LVL("PerConnectionSynch","PerConnectionSynch",6);
}
