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
class InternalDataReaderListener : public virtual RcObject {
public:
  typedef RcHandle<InternalDataReader<T> > InternalDataReader_rch;

  InternalDataReaderListener()
    : job_(make_rch<Job>(rchandle_from(this)))
  {}

  explicit InternalDataReaderListener(JobQueue_rch job_queue)
    : job_(make_rch<Job>(rchandle_from(this)))
    , job_queue_(job_queue)
  {}

  void job_queue(JobQueue_rch job_queue)
  {
    job_queue_ = job_queue;
  }

  virtual void on_data_available(InternalDataReader_rch reader) = 0;

  /// @name InternalDataReader Interface
  /// @{
  void schedule(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    JobQueue_rch lock = job_queue_.lock();
    if (lock) {
      const bool enqueue = readers_.empty();
      readers_.insert(reader);
      if (enqueue) {
        lock->enqueue(job_);
      }
    }
  }
  /// @}

private:
  class Job : public JobQueue::Job {
  public:
    explicit Job(RcHandle<InternalDataReaderListener> listener)
      : listener_(listener)
    {}

    void execute()
    {
      RcHandle<InternalDataReaderListener> listener = listener_.lock();
      if (listener) {
        listener->execute();
      }
    }

  private:
    WeakRcHandle<InternalDataReaderListener> listener_;
  };

  JobQueue::JobPtr job_;
  JobQueue_wrch job_queue_;
  typedef WeakRcHandle<InternalDataReader<T> > Reader;
  typedef OPENDDS_SET(Reader) ReaderSet;
  ReaderSet readers_;

  mutable ACE_Thread_Mutex mutex_;

  void execute()
  {
    ReaderSet readers;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      std::swap(readers, readers_);
    }

    for (typename ReaderSet::const_iterator pos = readers.begin(), limit = readers.end();
         pos != limit; ++pos) {
      InternalDataReader_rch reader = pos->lock();
      if (reader) {
        on_data_available(reader);
      }
    }
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INTERNAL_DATA_READER_LISTENER_H */
