/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DURABILITY_SERVICE_H
#define OPENDDS_DCPS_DURABILITY_SERVICE_H

#include "SampleCache.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename Sample> class InternalDataReader;

template<typename Sample>
class DurabilityService : public RcObject {
public:
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  // Replay all durable samples for the reader.
  virtual void add_reader(InternalDataReaderPtr reader, DDS::InstanceHandle_t publication_handle) = 0;

  virtual void register_instance(DDS::InstanceHandle_t publication_handle,
                                 const Sample& sample,
                                 const DDS::Time_t& source_timestamp) = 0;
  virtual void store(const Sample& sample, const DDS::Time_t& source_timestamp) = 0;
  virtual void unregister_instance(DDS::InstanceHandle_t publication_handle,
                                   const Sample& sample,
                                   const DDS::Time_t& source_timestamp) = 0;
  virtual void dispose_instance(DDS::InstanceHandle_t publication_handle,
                                const Sample& sample,
                                const DDS::Time_t& source_timestamp) = 0;
};

template<typename Sample>
class VolatileDurabilityService : public DurabilityService<Sample> {
public:
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  void add_reader(InternalDataReaderPtr, DDS::InstanceHandle_t) {}

  void register_instance(DDS::InstanceHandle_t publication_handle,
                         const Sample& sample,
                         const DDS::Time_t& source_timestamp) {}
  void store(const Sample& sample, const DDS::Time_t& source_timestamp) {}
  void unregister_instance(DDS::InstanceHandle_t publication_handle,
                           const Sample& sample,
                           const DDS::Time_t& source_timestamp) {}
  void dispose_instance(DDS::InstanceHandle_t publication_handle,
                        const Sample& sample,
                        const DDS::Time_t& source_timestamp) {}
};

template<typename Sample>
class TransientLocalDurabilityService : public DurabilityService<Sample> {
public:
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  void add_reader(InternalDataReaderPtr reader, DDS::InstanceHandle_t publication_handle)
  {
    // TODO: Register all of the instances.

    for (typename SampleCache<Sample>::const_sample_iterator pos = sample_cache_.sample_begin(), limit = sample_cache_.sample_end();
         pos != limit; ++pos)
      {
        reader->store(pos->sample, pos->source_timestamp, publication_handle);
      }
  }

  void register_instance(DDS::InstanceHandle_t publication_handle,
                         const Sample& sample,
                         const DDS::Time_t& source_timestamp)
  {
    sample_cache_.register_instance(publication_handle, sample, source_timestamp);
  }

  void store(const Sample& sample, const DDS::Time_t& source_timestamp)
  {
    sample_cache_.store(sample, source_timestamp);
  }

  void unregister_instance(DDS::InstanceHandle_t publication_handle,
                           const Sample& sample,
                           const DDS::Time_t& source_timestamp)
  {
    sample_cache_.unregister_instance(publication_handle, sample, source_timestamp);
  }

  void dispose_instance(DDS::InstanceHandle_t publication_handle,
                        const Sample& sample,
                        const DDS::Time_t& source_timestamp)
  {
    sample_cache_.dispose_instance(publication_handle, sample, source_timestamp);
  }

private:
  SampleCache<Sample> sample_cache_;
};

template<typename Sample>
class TransientDurabilityService : public DurabilityService<Sample> {
public:
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  void add_reader(InternalDataReaderPtr, DDS::InstanceHandle_t) {}

  void register_instance(DDS::InstanceHandle_t publication_handle,
                         const Sample& sample,
                         const DDS::Time_t& source_timestamp) {}
  void store(const Sample& sample, const DDS::Time_t& source_timestamp) {}
  void unregister_instance(DDS::InstanceHandle_t publication_handle,
                           const Sample& sample,
                           const DDS::Time_t& source_timestamp) {}
  void dispose_instance(DDS::InstanceHandle_t publication_handle,
                        const Sample& sample,
                        const DDS::Time_t& source_timestamp) {}
};

template<typename Sample>
class PersistentDurabilityService : public DurabilityService<Sample> {
public:
  typedef RcHandle<InternalDataReader<Sample> > InternalDataReaderPtr;

  void add_reader(InternalDataReaderPtr, DDS::InstanceHandle_t) {}

  void register_instance(DDS::InstanceHandle_t publication_handle,
                         const Sample& sample,
                         const DDS::Time_t& source_timestamp) {}
  void store(const Sample& sample, const DDS::Time_t& source_timestamp) {}
  void unregister_instance(DDS::InstanceHandle_t publication_handle,
                           const Sample& sample,
                           const DDS::Time_t& source_timestamp) {}
  void dispose_instance(DDS::InstanceHandle_t publication_handle,
                        const Sample& sample,
                        const DDS::Time_t& source_timestamp) {}
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_INTERNAL_DATA_WRITER_H */
