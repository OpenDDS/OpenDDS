/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JOB_QUEUE_H
#define OPENDDS_DCPS_JOB_QUEUE_H

#include "RcEventHandler.h"
#include "PoolAllocator.h"

#include <ace/Reactor.h>
#include <ace/Thread_Mutex.h>
#include <ace/Reverse_Lock_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class JobQueue : public RcEventHandler {
public:
  class Job : public virtual RcObject {
  public:
    virtual ~Job() { }
    virtual void execute() = 0;
  };
  typedef RcHandle<Job> JobPtr;

  explicit JobQueue(ACE_Reactor* reactor)
  {
    this->reactor(reactor);
  }

  void enqueue(JobPtr job)
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    const bool empty = job_queue_.empty();
    job_queue_.push_back(job);
    if (empty) {
      guard.release();
      reactor()->notify(this);
    }
  }

private:
  ACE_Thread_Mutex mutex_;
  typedef OPENDDS_VECTOR(JobPtr) Queue;
  Queue job_queue_;

  int handle_exception(ACE_HANDLE /*fd*/)
  {
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
};

typedef RcHandle<JobQueue> JobQueue_rch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_JOB_QUEUE_H  */
