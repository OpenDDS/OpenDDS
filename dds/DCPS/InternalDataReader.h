/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_READER_H
#define OPENDDS_DCPS_INTERNAL_DATA_READER_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "RcObject.h"
#include "PoolAllocator.h"
#include "InternalDataReaderListener.h"

#include <dds/DdsDcpsCoreC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class InternalEntity : public RcObject {};
typedef WeakRcHandle<InternalEntity> InternalEntity_wrch;

enum InternalSampleInfoKind {
  ISIK_REGISTER,
  ISIK_SAMPLE,
  ISIK_UNREGISTER,
  ISIK_DISPOSE
};

struct InternalSampleInfo {
  InternalSampleInfoKind kind;
  InternalEntity_wrch publication_handle;

  InternalSampleInfo(InternalSampleInfoKind a_kind,
                     InternalEntity_wrch a_publication_handle)
    : kind(a_kind)
    , publication_handle(a_publication_handle)
  {}

  bool operator==(const InternalSampleInfo& other) const
  {
    return kind == other.kind &&
      publication_handle == other.publication_handle;
  }
};

typedef OPENDDS_VECTOR(InternalSampleInfo) InternalSampleInfoSequence;

template <typename T>
class InternalDataReader : public InternalEntity {
public:
  typedef OPENDDS_VECTOR(T) SampleSequence;
  typedef RcHandle<InternalDataReaderListener<T> > Listener_rch;
  typedef WeakRcHandle<InternalDataReaderListener<T> > Listener_wrch;

  explicit InternalDataReader(bool durable = false,
                              Listener_rch listener = Listener_rch())
    : durable_(durable)
    , listener_(listener)
  {}

  /// @name InternalTopic and InternalWriter Interface
  /// @{
  bool durable() const { return durable_; }

  void remove_publication(InternalEntity_wrch publication_handle)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    // FUTURE: Index by publication_handle to avoid the loop.
    bool schedule = false;
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ++pos) {
      if (pos->second.unregister_instance(pos->first, publication_handle)) {
        schedule = true;
      }
    }

    const Listener_rch listener = listener_.lock();
    if (schedule && listener) {
      listener->schedule(rchandle_from(this));
    }
  }

  void register_instance(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const std::pair<typename InstanceMap::iterator, bool> p = instance_map_.insert(std::make_pair(sample, Instance()));
    const std::pair<PublicationSet::iterator, bool> q = p.first->second.publication_set.insert(publication_handle);
    if (q.second) {
      // New publication.
      p.first->second.samples.push_back(sample);
      p.first->second.infos.push_back(InternalSampleInfo(ISIK_REGISTER, publication_handle));

      const Listener_rch listener = listener_.lock();
      if (listener) {
        listener->schedule(rchandle_from(this));
      }
    }

    if (p.second) {
      p.first->second.view_state = DDS::NEW_VIEW_STATE;
    }
    p.first->second.instance_state = DDS::ALIVE_INSTANCE_STATE;
  }

  void write(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const std::pair<typename InstanceMap::iterator, bool> p = instance_map_.insert(std::make_pair(sample, Instance()));
    p.first->second.publication_set.insert(publication_handle);
    p.first->second.samples.push_back(sample);
    p.first->second.infos.push_back(InternalSampleInfo(ISIK_SAMPLE, publication_handle));

    const Listener_rch listener = listener_.lock();
    if (listener) {
      listener->schedule(rchandle_from(this));
    }

    if (p.second ||
        (p.first->second.view_state == DDS::NOT_NEW_VIEW_STATE &&
         p.first->second.instance_state != DDS::ALIVE_INSTANCE_STATE)) {
      p.first->second.view_state = DDS::NEW_VIEW_STATE;
    }

    p.first->second.instance_state = DDS::ALIVE_INSTANCE_STATE;
  }

  void unregister_instance(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceMap::iterator pos = instance_map_.find(sample);
    if (pos == instance_map_.end()) {
      return;
    }

    const Listener_rch listener = listener_.lock();
    if (pos->second.unregister_instance(sample, publication_handle) && listener) {
      listener->schedule(rchandle_from(this));
    }
  }

  void dispose(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceMap::iterator pos = instance_map_.find(sample);
    if (pos == instance_map_.end()) {
      return;
    }

    pos->second.samples.push_back(sample);
    pos->second.infos.push_back(InternalSampleInfo(ISIK_DISPOSE, publication_handle));
    pos->second.instance_state = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;

    const Listener_rch listener = listener_.lock();
    if (listener) {
      listener->schedule(rchandle_from(this));
    }
  }
  /// @}

  /// @name User Interface
  /// @{
  void set_listener(Listener_rch listener)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    listener_ = listener;
  }

  Listener_rch get_listener() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, Listener_rch());
    return listener_.lock();
  }

  void take(SampleSequence& samples, InternalSampleInfoSequence& infos)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ) {
      samples.insert(samples.end(), pos->second.samples.begin(), pos->second.samples.end());
      pos->second.samples.clear();
      infos.insert(infos.end(), pos->second.infos.begin(), pos->second.infos.end());
      pos->second.infos.clear();

      pos->second.view_state = DDS::NOT_NEW_VIEW_STATE;

      if (pos->second.instance_state != DDS::ALIVE_INSTANCE_STATE) {
        instance_map_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }
  /// @}

private:
  const bool durable_;
  // Often, the listener will have the reader as a member.  Use a weak
  // pointer to prevent a cycle that prevents the listener from being
  // destroyed.
  Listener_wrch listener_;

  typedef OPENDDS_SET(InternalEntity_wrch) PublicationSet;

  struct Instance {
    OPENDDS_LIST(T) samples;
    OPENDDS_LIST(InternalSampleInfo) infos;

    PublicationSet publication_set;

    DDS::ViewStateKind view_state;
    DDS::InstanceStateKind instance_state;

    bool unregister_instance(const T& sample, InternalEntity_wrch publication_handle)
    {
      if (publication_set.erase(publication_handle)) {
        samples.push_back(sample);
        infos.push_back(InternalSampleInfo(ISIK_UNREGISTER, publication_handle));

        if (publication_set.empty() && instance_state != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
          instance_state = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
        }

        return true;
      }

      return false;
    }
  };

  typedef OPENDDS_MAP_T(T, Instance) InstanceMap;
  InstanceMap instance_map_;

  mutable ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_INTERNAL_DATA_READER_H */
