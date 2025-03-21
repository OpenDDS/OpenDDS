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
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Event ev(thread_status_manager);

  Queue q;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);
    q.swap(job_queue_);
  }

  for (Queue::const_iterator pos = q.begin(), limit = q.end(); pos != limit; ++pos) {
    ThreadStatusManager::Event event(thread_status_manager);
    (*pos)->execute();
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);
  if (!job_queue_.empty()) {
    guard.release();
    reactor()->notify(this);
  }

  return 0;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
