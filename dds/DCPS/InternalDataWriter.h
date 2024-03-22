/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_WRITER_H
#define OPENDDS_DCPS_INTERNAL_DATA_WRITER_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "RcObject.h"
#include "InternalDataReader.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/*
  The InternalDataWriter supports the following QoS:

  DurabilityQosPolicy durability; => VOLATILE, TRANSIENT_LOCAL
  DurabilityServiceQosPolicy durability_service; => None
  DeadlineQosPolicy deadline; => None
  LatencyBudgetQosPolicy latency_budget => None
  LivelinessQosPolicy liveliness => None
  ReliabilityQosPolicy reliability => RELIABLE
  DestinationOrderQosPolicy destination_order; => None
  HistoryQosPolicy history; => KEEP_LAST_HISTORY, KEEP_ALL_HISTORY
  ResourceLimitsQosPolicy resource_limits; => None
  TransportPriorityQosPolicy transport_priority; => None
  LifespanQosPolicy lifespan; => None
  UserDataQosPolicy user_data; => None
  OwnershipQosPolicy ownership; => None
  OwnershipStrengthQosPolicy ownership_strength; => None
  WriterDataLifecycleQosPolicy writer_data_lifecycle; => Yes
*/

template <typename T>
class InternalDataWriter : public InternalEntity {
public:
  typedef RcHandle<InternalDataReader<T> > InternalDataReader_rch;
  typedef WeakRcHandle<InternalDataReader<T> > InternalDataReader_wrch;
  typedef OPENDDS_VECTOR(T) SampleSequence;

  explicit InternalDataWriter(const DDS::DataWriterQos& qos)
    : qos_(qos)
  {}

  /// @name InternalTopic Interface
  /// @{
  void add_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const SampleSequence& instances = reader->get_interesting_instances();

    if (instances.empty()) {
      readers_.insert(reader);

      if (qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS && reader->durable()) {
        for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end();
             pos != limit; ++pos) {
          pos->second.add_reader(reader, static_rchandle_cast<InternalEntity>(rchandle_from(this)));
        }
      }
    } else {
      all_instance_readers_.insert(reader);
      for (typename SampleSequence::const_iterator pos = instances.begin(), limit = instances.end();
           pos != limit; ++ pos) {
        instance_readers_[*pos].insert(reader);
        if (qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS && reader->durable()) {
          typename InstanceMap::iterator pos2 = instance_map_.find(*pos);
          if (pos2 != instance_map_.end()) {
            pos2->second.add_reader(reader, static_rchandle_cast<InternalEntity>(rchandle_from(this)));
          }
        }
      }
    }
  }

  void remove_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (readers_.erase(reader)) {
      reader->remove_publication(static_rchandle_cast<InternalEntity>(rchandle_from(this)), qos_.writer_data_lifecycle.autodispose_unregistered_instances);
    }

    if (all_instance_readers_.erase(reader)) {
      const SampleSequence& instances = reader->get_interesting_instances();
      for (typename SampleSequence::const_iterator pos = instances.begin(), limit = instances.end();
           pos != limit; ++pos) {
        typename InstanceReaderSet::iterator pos2 = instance_readers_.find(*pos);
        if (pos2 != instance_readers_.end()) {
          if (pos2->second.erase(reader)) {
            reader->remove_publication(static_rchandle_cast<InternalEntity>(rchandle_from(this)), qos_.writer_data_lifecycle.autodispose_unregistered_instances);
          }
          if (pos2->second.empty()) {
            instance_readers_.erase(pos2);
          }
        }
      }
    }
  }

  bool has_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
    return readers_.count(reader) + all_instance_readers_.count(reader);
  }

  InternalEntity_wrch publication_handle()
  {
    return static_rchandle_cast<InternalEntity>(rchandle_from(this));
  }
  /// @}

  /// @name User Interface
  /// @{
  void write(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS) {
      const std::pair<typename InstanceMap::iterator, bool> p = instance_map_.insert(std::make_pair(sample, SampleHolder()));
      p.first->second.write(sample, qos_);
    }

    typename InstanceReaderSet::const_iterator pos = instance_readers_.find(sample);
    if (pos != instance_readers_.end()) {
      write_set(sample, pos->second);
    }

    write_set(sample, readers_);
  }

  void dispose(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS) {
      typename InstanceMap::iterator pos = instance_map_.find(sample);
      if (pos != instance_map_.end()) {
        pos->second.dispose();
      }
    }

    typename InstanceReaderSet::const_iterator pos = instance_readers_.find(sample);
    if (pos != instance_readers_.end()) {
      dispose_set(sample, pos->second);
    }

    dispose_set(sample, readers_);
  }

  void unregister_instance(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS) {
      instance_map_.erase(sample);
    }

    typename InstanceReaderSet::const_iterator pos = instance_readers_.find(sample);
    if (pos != instance_readers_.end()) {
      unregister_instance_set(sample, pos->second);
    }

    unregister_instance_set(sample, readers_);
  }
  /// @}

private:
  const DDS::DataWriterQos qos_;

  typedef OPENDDS_SET(InternalDataReader_wrch) ReaderSet;
  ReaderSet readers_;
  ReaderSet all_instance_readers_;
  typedef OPENDDS_MAP_T(T, ReaderSet) InstanceReaderSet;
  InstanceReaderSet instance_readers_;

  void write_set(const T& sample,
                 const ReaderSet& readers)
  {
    for (typename ReaderSet::const_iterator pos = readers.begin(), limit = readers.end(); pos != limit; ++pos) {
      InternalDataReader_rch reader = pos->lock();
      if (reader) {
        reader->write(static_rchandle_cast<InternalEntity>(rchandle_from(this)), sample);
      }
    }
  }

  void dispose_set(const T& sample,
                   const ReaderSet& readers)
  {
    for (typename ReaderSet::const_iterator pos = readers.begin(), limit = readers.end(); pos != limit; ++pos) {
      InternalDataReader_rch reader = pos->lock();
      if (reader) {
        reader->dispose(static_rchandle_cast<InternalEntity>(rchandle_from(this)), sample);
      }
    }
  }

  void unregister_instance_set(const T& sample,
                               const ReaderSet& readers)
  {
    for (typename ReaderSet::const_iterator pos = readers.begin(), limit = readers.end(); pos != limit; ++pos) {
      InternalDataReader_rch reader = pos->lock();
      if (reader) {
        if (qos_.writer_data_lifecycle.autodispose_unregistered_instances) {
          reader->dispose(static_rchandle_cast<InternalEntity>(rchandle_from(this)), sample);
        }
        reader->unregister_instance(static_rchandle_cast<InternalEntity>(rchandle_from(this)), sample);
      }
    }
  }

  class SampleHolder {
  public:
    bool empty() const { return samples_.empty(); }

    void add_reader(InternalDataReader_rch reader, RcHandle<InternalEntity> writer)
    {
      for (typename SampleList::const_iterator pos = samples_.begin(), limit = samples_.end(); pos != limit; ++pos) {
        reader->write(writer, *pos);
      }
    }

    void write(const T& sample,
               const DDS::DataWriterQos& qos)
    {
      samples_.push_back(sample);
      if (qos.history.kind == DDS::KEEP_LAST_HISTORY_QOS) {
        while (samples_.size() > static_cast<std::size_t>(qos.history.depth)) {
          samples_.pop_front();
        }
      }
    }

    void dispose()
    {
      samples_.clear();
    }

  private:
    typedef OPENDDS_LIST(T) SampleList;
    SampleList samples_;
  };

  typedef OPENDDS_MAP_T(T, SampleHolder) InstanceMap;
  InstanceMap instance_map_;

  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INTERNAL_DATA_WRITER_H */
