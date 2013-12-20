/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ThreadSynchWorker::ThreadSynchWorker()
{
  DBG_ENTRY_LVL("ThreadSynchWorker","ThreadSynchWorker",6);
}

ACE_INLINE
int
OpenDDS::DCPS::ThreadSynchWorker::schedule_wakeup( ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("ThreadSynchWorker","schedule_wakeup",6);

  return 0;
}

ACE_INLINE
int
OpenDDS::DCPS::ThreadSynchWorker::cancel_wakeup( ACE_Reactor_Mask)
{
  DBG_ENTRY_LVL("ThreadSynchWorker","cancel_wakeup",6);

  return 0;
}

