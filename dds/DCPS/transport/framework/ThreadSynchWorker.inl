/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ThreadSynchWorker::ThreadSynchWorker( std::size_t id)
 : id_(id)
{
  DBG_ENTRY_LVL("ThreadSynchWorker","ThreadSynchWorker",6);
}

ACE_INLINE
std::size_t
OpenDDS::DCPS::ThreadSynchWorker::id() const
{
  DBG_ENTRY_LVL("ThreadSynchWorker","id",6);

  return id_;
}

ACE_INLINE
void
OpenDDS::DCPS::ThreadSynchWorker::schedule_output()
{
  DBG_ENTRY_LVL("ThreadSynchWorker","schedule_output",6);
}

