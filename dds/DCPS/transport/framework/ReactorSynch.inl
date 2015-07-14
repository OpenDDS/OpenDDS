/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "dds/DCPS/debug.h"

ACE_INLINE
OpenDDS::DCPS::ReactorSynch::ReactorSynch(
  ThreadSynchResource* synch_resource,
  TransportSendStrategy* strategy,
  ACE_Reactor* reactor
)
  : ThreadSynch(synch_resource),
    scheduleOutputHandler_( new ScheduleOutputHandler( strategy, reactor))
{
  DBG_ENTRY_LVL("ReactorSynch","ReactorSynch",6);

  // Manage the handler storage using the provided _var.
  safeHandler_ = scheduleOutputHandler_;

  if (DCPS_debug_level > 4) {
    std::size_t id = 0;
    if( worker()) {
      id = worker()->id();
    }
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ReactorSynch::ReactorSynch() [%d] - ")
               ACE_TEXT("establishing synch %C a worker.\n"),
               id,(worker()?"with":"without")));
  }
}

