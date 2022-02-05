/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_READER_LISTENER_H
#define OPENDDS_DCPS_INTERNAL_DATA_READER_LISTENER_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "JobQueue.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename T>
class InternalDataReader;

template <typename T>
class InternalDataReaderListener : public JobQueue::Job {
public:
  typedef RcHandle<InternalDataReader<T> > InternalDataReader_rch;

  InternalDataReaderListener()
  {}

  explicit InternalDataReaderListener(JobQueue_rch job_queue)
    : job_queue_(job_queue)
  {}

  void job_queue(JobQueue_rch job_queue)
  {
    job_queue_ = job_queue;
  }

  virtual void on_data_available(InternalDataReader_rch reader) = 0;

  void schedule(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    JobQueue_rch lock = job_queue_.lock();
    if (lock) {
      bool enqueue = readers_.empty();
      readers_.insert(reader);
      if (enqueue) {
        lock->enqueue(rchandle_from(this));
      }
    }
  }

private:
  JobQueue_wrch job_queue_;
  typedef WeakRcHandle<InternalDataReader<T> > Reader;
  typedef OPENDDS_SET(Reader) ReaderSet;
  ReaderSet readers_;

  mutable ACE_Thread_Mutex mutex_;

  void execute()
  {
    Reader reader;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

      typename ReaderSet::iterator pos = readers_.begin();
      reader = *pos;
      readers_.erase(pos);
    }

    InternalDataReader_rch lock = reader.lock();
    if (lock) {
      on_data_available(lock);
    }
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INTERNAL_DATA_READER_LISTENER_H */
