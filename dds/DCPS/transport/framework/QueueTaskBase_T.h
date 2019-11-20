/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QUEUE_TASK_BASE_T_H
#define OPENDDS_DCPS_QUEUE_TASK_BASE_T_H

#include /**/ "ace/pre.h"

#include "EntryExit.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ace/Condition_T.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Task.h"
#include "ace/Unbounded_Queue.h"
#include "ace/INET_Addr.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class QueueTaskBase
 *
 * @brief A simple ACE task that manages a queue of request.
 */
template <typename T>
class QueueTaskBase : public ACE_Task_Base {
public:
  QueueTaskBase()
  : work_available_(lock_),
      shutdown_initiated_(false),
      opened_(false),
      thr_id_(ACE_OS::NULL_thread) {
    DBG_ENTRY("QueueTaskBase","QueueTaskBase");
  }

  virtual ~QueueTaskBase() {
    DBG_ENTRY("QueueTaskBase","~QueueTaskBase");
  }

  /// Put the request to the request queue.
  /// Returns 0 if successful, -1 otherwise (it has been "rejected" or this
  /// task is shutdown).
  int add(const T& req) {
    DBG_ENTRY("QueueTaskBase","add");
    GuardType guard(this->lock_);

    if (this->shutdown_initiated_)
      return -1;

    int result = this->queue_.enqueue_tail(req);

    if (result == 0) {
      this->work_available_.signal();

    } else
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: QueueTaskBase::add %p\n",
       ACE_TEXT("enqueue_tail")));

    return result;
  }

  /// Activate the worker threads
  virtual int open(void* = 0) {
    DBG_ENTRY("QueueTaskBase","open");

    GuardType guard(this->lock_);

    // We can assume that we are in the proper state to handle this open()
    // call as long as we haven't been open()'ed before.
    if (this->opened_) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) QueueTaskBase failed to open.  "
                        "Task has previously been open()'ed.\n"),
                       -1);
    }

    // Activate this task object with one worker thread.
    if (this->activate(THR_NEW_LWP | THR_JOINABLE, 1) != 0) {
      // Assumes that when activate returns non-zero return code that
      // no threads were activated.
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) QueueTaskBase failed to activate "
                        "the worker threads.\n"),
                       -1);
    }

    // Now we have past the point where we can say we've been open()'ed before.
    this->opened_ = true;

    return 0;
  }

  /// The "mainline" executed by the worker thread.
  virtual int svc() {
    DBG_ENTRY("QueueTaskBase","svc");

    this->thr_id_ = ACE_OS::thr_self();

    // Start the "GetWork-And-PerformWork" loop for the current worker thread.
    while (!this->shutdown_initiated_) {
      T req;
      {
        GuardType guard(this->lock_);

        if (this->queue_.is_empty()) {
          this->work_available_.wait();
        }

        if (this->shutdown_initiated_)
          break;

        int result = queue_.dequeue_head(req);

        if (result != 0) {
          //I'm not sure why this thread got more signals than actual signals
          //when using thread_per_connection and the user application thread
          //send requests without interval. We just need ignore the dequeue
          //failure.
          //ACE_ERROR ((LM_ERROR, "(%P|%t) ERROR: QueueTaskBase::svc  %p\n",
          //  ACE_TEXT("dequeue_head")));
          continue;
        }
      }

      this->execute(req);
    }

    // This will never get executed.
    return 0;
  }

  /// Called when the thread exits.
  virtual int close(u_long flag = 0) {
    DBG_ENTRY("QueueTaskBase","close");

    if (flag == 0)
      return 0;

    {
      GuardType guard(this->lock_);

      if (this->shutdown_initiated_)
        return 0;

      // Set the shutdown flag to true.
      this->shutdown_initiated_ = true;
      this->work_available_.signal();
    }

    if (this->opened_ && !ACE_OS::thr_equal(this->thr_id_, ACE_OS::thr_self()))
      this->wait();

    return 0;
  }

  bool is_shutdown_initiated() const {
    GuardType guard(lock_);
    return shutdown_initiated_;
  }

  /// The subclass should implement this function to handle the
  /// dequeued request.
  virtual void execute(T& req) = 0;

private:

  typedef ACE_SYNCH_MUTEX         LockType;
  typedef ACE_Guard<LockType>     GuardType;
  typedef ACE_Condition<LockType> ConditionType;

  typedef ACE_Unbounded_Queue<T>  Queue;

  /// Lock to protect the "state" (all of the data members) of this object.
  mutable LockType lock_;

  /// The request queue.
  Queue queue_;

  /// Condition used to signal the worker threads that they may be able to
  /// find a request in the queue_ that needs to be executed.
  /// This condition will be signal()'ed each time a request is
  /// added to the queue_, and also when this task is shutdown.
  ConditionType work_available_;

  /// Flag used to initiate a shutdown request to all worker threads.
  bool shutdown_initiated_;

  /// Flag used to avoid multiple open() calls.
  bool opened_;

  /// The id of the thread created by this task.
  ACE_thread_t thr_id_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_QUEUE_TASK_BASE_T_H */
