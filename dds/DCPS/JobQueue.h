/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JOB_QUEUE_H
#define OPENDDS_DCPS_JOB_QUEUE_H

#include "RcEventHandler.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class JobQueue : public RcEventHandler {
public:
  class Job : public RcObject {
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
    job_queue_.push(job);
    if (empty) {
      guard.release();
      reactor()->notify(this);
    }
  }

private:
  ACE_Thread_Mutex mutex_;
  OPENDDS_QUEUE(JobPtr) job_queue_;

  int handle_exception(ACE_HANDLE /*fd*/)
  {
    ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, -1);
    size_t count = job_queue_.size();
    while (count--) {
      JobPtr job = job_queue_.front();
      job_queue_.pop();
      {
        ACE_GUARD_RETURN(ACE_Reverse_Lock<ACE_Thread_Mutex>, rev_guard, rev_lock, -1);
        job->execute();
      }
    }

    if (!job_queue_.empty()) {
      guard.release();
      reactor()->notify(this);
    }

    return 0;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_JOB_QUEUE_H  */
