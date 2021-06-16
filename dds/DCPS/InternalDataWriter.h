/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_WRITER_H
#define OPENDDS_DCPS_INTERNAL_DATA_WRITER_H

#include "InternalDataReader.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename Sample>
class InternalDataWriter : public RcObject {
public:
  typedef RcHandle<InternalDataWriter> InternalDataWriterPtr;
  typedef InternalDataReader<Sample> InternalDataReader;
  typedef RcHandle<InternalDataReader> InternalDataReaderPtr;

  InternalDataWriter(DDS::InstanceHandle_t publication_handle,
                     size_t depth)
    : publication_handle_(publication_handle)
  {
    durability_sink_ = make_rch<InternalDataReaderType>(depth);
    sinks_.insert(durability_sink_);
  }

  DDS::InstanceHandle_t get_publication_handle() const { return publication_handle_; }

  void register_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      InternalDataReaderPtr sink = *pos;
      sink->register_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void write(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      InternalDataReaderPtr sink = *pos;
      sink->write(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void unregister_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      InternalDataReaderPtr sink = *pos;
      sink->unregister_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void dispose_instance(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InternalDataReaders::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      InternalDataReaderPtr sink = *pos;
      sink->dispose_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void connect(InternalDataReaderPtr sink)
  {
    {
      ACE_GUARD(ACE_Thread_Mutex, g, update_mutex_);
      to_insert_.insert(sink);
      to_erase_.erase(sink);
    }
    if (mutex_.tryacquire() == 0) {
      process_updates();
      mutex_.release();
    }
  }

  void disconnect(InternalDataReaderPtr sink)
  {
    {
      ACE_GUARD(ACE_Thread_Mutex, g, update_mutex_);
      to_erase_.insert(sink);
      to_insert_.erase(sink);
    }
    if (mutex_.tryacquire() == 0) {
      process_updates();
      mutex_.release();
    }
  }

private:
  typedef std::set<InternalDataReaderPtr> InternalDataReaders;
  mutable ACE_Thread_Mutex update_mutex_; // Second in locking order.
  InternalDataReaders to_insert_;
  InternalDataReaders to_erase_;
  mutable ACE_Thread_Mutex mutex_; // First in locking order.
  InternalDataReaders sinks_;
  InternalDataReaderPtr durability_sink_;
  const DDS::InstanceHandle_t publication_handle_;

  void process_updates()
  {
    ACE_GUARD(ACE_Thread_Mutex, g, update_mutex_);
    for (typename InternalDataReaders::const_iterator pos = to_erase_.begin(), limit = to_erase_.end(); pos != limit; ++pos) {
      InternalDataReaderPtr sink = *pos;

      SampleList sample_list;
      DDS::SampleInfoSeq sample_info_list;
      DDS::InstanceHandle_t instance_handle = 0;
      while (durability_sink_->read_next_instance(sample_list, sample_info_list, 1, instance_handle, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE)) {
        sink->unregister_instance(sample_list[0], source_timestamp, get_publication_handle());
        instance_handle = sample_info_list[0].instance_handle;
      }

      sinks_.erase(sink);
    }
    sinks_.insert(to_insert_.begin(), to_insert_.end());
    for (typename InternalDataReaders::const_iterator pos = to_insert_.begin(), limit = to_insert_.end(); pos != limit; ++pos) {
      (*pos)->initialize(*durability_sink_);
    }
    to_erase_.clear();
    to_insert_.clear();
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_INTERNAL_DATA_WRITER_H */
