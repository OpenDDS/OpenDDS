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

template <typename T>
class InternalDataWriter : public RcObject {
public:
  typedef RcHandle<InternalDataReader<T> > InternalDataReader_rch;

  InternalDataWriter(bool durable)
    : durable_(durable)
  {}

  void add_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    readers_.insert(reader);

    if (durable_ && reader->durable()) {
      for (typename InstanceMap::const_iterator pos = instance_map_.begin(), limit = instance_map_.end();
           pos != limit; ++pos) {
        if (pos->second.valid_data) {
          reader->write(this, pos->second.sample);
        } else {
          reader->register_instance(this, pos->first);
        }
      }
    }
  }

  void remove_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    if (readers_.erase(reader)) {
      reader->remove_publication(this);
    }
  }

  bool has_reader(InternalDataReader_rch reader)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
    return readers_.count(reader);
  }

  void register_instance(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (durable_) {
      instance_map_.insert(std::make_pair(sample, SampleHolder()));
    }

    for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
      (*pos)->register_instance(this, sample);
    }
  }

  void write(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (durable_) {
      const std::pair<typename InstanceMap::iterator, bool> p = instance_map_.insert(std::make_pair(sample, SampleHolder()));
      p.first->second.sample = sample;
      p.first->second.valid_data = true;
    }

    for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
      (*pos)->write(this, sample);
    }
  }

  void unregister_instance(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (durable_) {
      instance_map_.erase(sample);
    }

    for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
      (*pos)->unregister_instance(this, sample);
    }
  }

  void dispose(const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (durable_) {
      instance_map_.erase(sample);
    }

    for (typename ReaderSet::const_iterator pos = readers_.begin(), limit = readers_.end(); pos != limit; ++pos) {
      (*pos)->dispose(this, sample);
    }
  }

  const void* publication_handle() const
  {
    return this;
  }

private:
  const bool durable_;

  typedef OPENDDS_SET(InternalDataReader_rch) ReaderSet;
  ReaderSet readers_;

  struct SampleHolder {
    T sample;
    bool valid_data;

    SampleHolder()
      : valid_data(false)
    {}
  };

  typedef OPENDDS_MAP(T, SampleHolder) InstanceMap;
  InstanceMap instance_map_;

  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INTERNAL_DATA_WRITER_H */
