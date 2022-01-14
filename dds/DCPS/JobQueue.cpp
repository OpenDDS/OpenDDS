/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "JobQueue.h"

#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

JobQueue::JobQueue(ACE_Reactor* reactor)
{
  this->reactor(reactor);
}

int JobQueue::handle_exception(ACE_HANDLE /*fd*/)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  Queue q;

  ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);
  q.swap(job_queue_);
  for (Queue::const_iterator pos = q.begin(), limit = q.end(); pos != limit; ++pos) {
    ACE_GUARD_RETURN(ACE_Reverse_Lock<ACE_Thread_Mutex>, rev_guard, rev_lock, -1);
    (*pos)->execute();
  }

  if (!job_queue_.empty()) {
    guard.release();
    reactor()->notify(this);
  }

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
