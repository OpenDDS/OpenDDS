/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_TOPIC_H
#define OPENDDS_DCPS_INTERNAL_TOPIC_H

#include "InternalDataReader.h"
#include "InternalDataWriter.h"
#include "PoolAllocator.h"
#include "InstanceHandle.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename Sample>
class InternalTopic : public RcObject {
public:
  typedef InternalDataWriter<Sample> InternalDataWriter;
  typedef RcHandle<InternalDataWriter> InternalDataWriterPtr;
  typedef InternalDataReader<Sample> InternalDataReader;
  typedef RcHandle<InternalDataReader> InternalDataReaderPtr;

  // TODO: depth should come from QoS.
  InternalDataWriterPtr create_writer(const DDS::DataWriterQos& qos)
  {
    InternalDataWriterPtr writer;
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, writer);

    const DDS::InstanceHandle_t publication_handle =
      reusable_publication_handles_.empty() ? publication_handle_generator_.next() : reusable_publication_handles_.pop_front();

    writer = make_rch<InternalDataWriterType>(rchandle_from(this), publication_handle, qos);
    writers_.insert(writer);

    return writer;
  }

  // TODO: depth should come from QoS.
  InternalDataReaderPtr create_reader(size_t depth)
  {
    InternalDataReaderPtr reader;
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, reader);

    reader = make_rch<InternalDataReaderType>(depth);
    readers_.insert(reader);

    for (typename InternalDataWriters::const_iterator pos = writers_.begin(), limit = writers_.end();
         pos != limit; ++pos) {
      (*pos)->add_reader(reader);
    }

    return reader;
  }

  void remove_writer(InternalDataWriterPtr writer)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (writers_.count(writer) == 0) {
      return;
    }

    reusable_publication_handles_.add(writer->get_publication_handle());

    writers_.erase(writer);
  }

  void remove_reader(InternalDataReaderPtr reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    readers_.erase(reader);
  }

  bool has_writer(InternalDataWriterPtr writer)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    return writers_.count(writer);
  }

  bool has_reader(InternalDataReaderPtr reader)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    return readers_.count(reader);
  }

  // Support InternalDataWriters.

  void register_instance(const Sample& sample,
                         const DDS::Time_t& source_timestamp,
                         const DDS::InstanceHandle_t publication_handle)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = readers_.begin(), limit = readers_.end();
         pos != limit; ++pos) {
      (*pos)->register_instance(sample, source_timestamp, publication_handle);
    }
  }

  void store(const Sample& sample,
             const DDS::Time_t& source_timestamp,
             const DDS::InstanceHandle_t publication_handle)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = readers_.begin(), limit = readers_.end();
         pos != limit; ++pos) {
      (*pos)->store(sample, source_timestamp, publication_handle);
    }
  }

  void unregister_instance(const Sample& sample,
                           const DDS::Time_t& source_timestamp,
                           const DDS::InstanceHandle_t publication_handle)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = readers_.begin(), limit = readers_.end();
         pos != limit; ++pos) {
      (*pos)->unregister_instance(sample, source_timestamp, publication_handle);
    }
  }

  void dispose_instance(const Sample& sample,
                        const DDS::Time_t& source_timestamp,
                        const DDS::InstanceHandle_t publication_handle)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = readers_.begin(), limit = readers_.end();
         pos != limit; ++pos) {
      (*pos)->dispose_instance(sample, source_timestamp, publication_handle);
    }
  }

private:
  mutable ACE_Thread_Mutex mutex_;
  typedef OPENDDS_SET(InternalDataWriterPtr) InternalDataWriters;
  InternalDataWriters writers_;
  typedef OPENDDS_SET(InternalDataReaderPtr) InternalDataReaders;
  InternalDataReaders readers_;

  InstanceHandleGenerator publication_handle_generator_;
  DisjointSequence::OrderedRanges<DDS::InstanceHandle_t> reusable_publication_handles_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_INTERNAL_TOPIC_H */
