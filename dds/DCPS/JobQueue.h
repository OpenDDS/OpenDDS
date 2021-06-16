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

class Schedulable : public virtual RcObject {
public:
  virtual void schedule() = 0;
};

class Executable : public virtual RcObject {
public:
  virtual void execute() = 0;
};

class JobQueueSchedulable : public Schedulable, public JobQueue::Job {
public:
  typedef RcHandle<Executable> ExecutablePtr;
  typedef WeakRcHandle<Executable> WeakExecutablePtr;

  JobQueueSchedulable()
    : scheduled_(false)
  {}

  void set_job_queue(WeakRcHandle<JobQueue> job_queue)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    job_queue_ = job_queue;
  }

  void set_executable(WeakExecutablePtr executable)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    executable_ = executable;
  }

  virtual void schedule()
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    if (scheduled_ == true) {
      return;
    }

    RcHandle<JobQueue> job_queue = job_queue_.lock();
    if (job_queue) {
      job_queue->enqueue(rchandle_from(this));
      scheduled_ = true;
    }
  }

  virtual void execute()
  {
    ExecutablePtr executable;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      scheduled_ = false;
      executable = executable_.lock();
    }
    if (executable) {
      executable->execute();
    }
  }

private:
  WeakRcHandle<JobQueue> job_queue_;
  WeakExecutablePtr executable_;
  mutable ACE_Thread_Mutex mutex_;
  bool scheduled_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_JOB_QUEUE_H  */
