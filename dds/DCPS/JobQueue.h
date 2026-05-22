/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_JOB_QUEUE_H
#define OPENDDS_DCPS_JOB_QUEUE_H

#include "dcps_export.h"

#include "EventDispatcher.h"

#include <ace/Reactor.h>
#include <ace/Thread_Mutex.h>
#include <ace/Reverse_Lock_T.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class Job : public EventBase {
public:
  virtual ~Job() { }
  virtual void execute() = 0;
  void handle_event() { execute(); }
};
typedef RcHandle<Job> JobPtr;

template <typename Delegate>
class PmfJob : public Job {
public:
  typedef void (Delegate::*PMF)();

  PmfJob(RcHandle<Delegate> delegate,
         PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  virtual ~PmfJob() {}

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;

  void execute()
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)();
    }
  }
};

class OpenDDS_Dcps_Export JobQueue : public virtual RcObject {
public:
  explicit JobQueue(EventDispatcher_rch event_dispatcher);

  void enqueue(JobPtr job)
  {
    event_dispatcher_->dispatch(dynamic_rchandle_cast<EventBase>(job));
  }

  size_t size() const
  {
    return event_dispatcher_->queue_size();
  }

private:
  EventDispatcher_rch event_dispatcher_;
};

typedef RcHandle<JobQueue> JobQueue_rch;
typedef WeakRcHandle<JobQueue> JobQueue_wrch;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_JOB_QUEUE_H  */
