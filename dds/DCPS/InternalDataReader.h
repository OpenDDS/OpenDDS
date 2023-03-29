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
#include "Time_Helper.h"

#include <dds/DdsDcpsCoreC.h>
#include <dds/DdsDcpsInfrastructureC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class InternalEntity : public virtual RcObject {};
typedef WeakRcHandle<InternalEntity> InternalEntity_wrch;

typedef OPENDDS_VECTOR(DDS::SampleInfo) InternalSampleInfoSequence;

inline DDS::SampleInfo make_sample_info(DDS::SampleStateKind sample_state,
                                        DDS::ViewStateKind view_state,
                                        DDS::InstanceStateKind instance_state,
                                        size_t disposed_generation_count,
                                        size_t no_writers_generation_count,
                                        size_t sample_rank,
                                        size_t generation_rank,
                                        size_t absolute_generation_rank,
                                        bool valid_data)
{
  DDS::SampleInfo si;
  si.sample_state = sample_state;
  si.view_state = view_state;
  si.instance_state = instance_state;
  si.source_timestamp = make_time_t(0, 0); // TODO
  si.instance_handle = DDS::HANDLE_NIL; // TODO
  si.publication_handle = DDS::HANDLE_NIL; // TODO
  si.disposed_generation_count = disposed_generation_count;
  si.no_writers_generation_count = no_writers_generation_count;
  si.sample_rank = sample_rank;
  si.generation_rank = generation_rank;
  si.absolute_generation_rank = absolute_generation_rank;
  si.valid_data = valid_data;
  return si;
}

#ifndef OPENDDS_SAFETY_PROFILE
inline bool operator==(const DDS::SampleInfo& x, const DDS::SampleInfo& y)
{
  return x.sample_state == y.sample_state &&
    x.view_state == y.view_state &&
    x.instance_state == y.instance_state &&
    x.source_timestamp == y.source_timestamp &&
    x.instance_handle == y.instance_handle &&
    x.publication_handle == y.publication_handle &&
    x.disposed_generation_count == y.disposed_generation_count &&
    x.no_writers_generation_count == y.no_writers_generation_count &&
    x.sample_rank == y.sample_rank &&
    x.generation_rank == y.generation_rank &&
    x.absolute_generation_rank == y.absolute_generation_rank &&
    x.valid_data == y.valid_data;
}
#endif

class SIW {
public:
  SIW(const DDS::SampleInfo& sample_info)
    : si(sample_info)
  {}

  bool operator==(const SIW& other) const
  {
    return si == other.si;
  }

  DDS::SampleInfo si;
};

template <typename T>
class InternalDataReader : public InternalEntity {
public:
  typedef OPENDDS_VECTOR(T) SampleSequence;
  typedef RcHandle<InternalDataReaderListener<T> > Listener_rch;
  typedef WeakRcHandle<InternalDataReaderListener<T> > Listener_wrch;

  explicit InternalDataReader(const DDS::DataReaderQos qos,
                              Listener_rch listener = Listener_rch())
    : qos_(qos)
    , listener_(listener)
  {}

  /// @name InternalTopic and InternalWriter Interface
  /// @{
  bool durable() const { return qos_.durability.kind == DDS::TRANSIENT_LOCAL_DURABILITY_QOS; }

  void remove_publication(InternalEntity_wrch publication_handle, bool autodispose_unregistered_instances)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    // FUTURE: Index by publication_handle to avoid the loop.
    bool schedule = false;
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ++pos) {
      if (autodispose_unregistered_instances && pos->second.dispose(publication_handle, pos->first)) {
        schedule = true;
      }
      if (pos->second.unregister_instance(publication_handle, pos->first)) {
        schedule = true;
      }
    }

    if (schedule) {
      const Listener_rch listener = listener_.lock();
      if (listener) {
        listener->schedule(rchandle_from(this));
      }
    }
  }

  void write(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    const std::pair<typename InstanceMap::iterator, bool> p = instance_map_.insert(std::make_pair(sample, Instance()));
    p.first->second.write(publication_handle, sample, qos_);

    const Listener_rch listener = listener_.lock();
    if (listener) {
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

    if (pos->second.dispose(publication_handle, pos->first)) {
      const Listener_rch listener = listener_.lock();
      if (listener) {
        listener->schedule(rchandle_from(this));
      }
    }
  }

  void unregister_instance(InternalEntity_wrch publication_handle, const T& sample)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceMap::iterator pos = instance_map_.find(sample);
    if (pos == instance_map_.end()) {
      return;
    }

    if (pos->second.unregister_instance(publication_handle, pos->first)) {
      const Listener_rch listener = listener_.lock();
      if (listener) {
        listener->schedule(rchandle_from(this));
      }
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

  void read(SampleSequence& samples, InternalSampleInfoSequence& infos)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ) {
      pos->second.read(samples, infos);
      if (pos->second.instance_state() != DDS::ALIVE_INSTANCE_STATE) {
        instance_map_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void take(SampleSequence& samples, InternalSampleInfoSequence& infos)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ) {
      pos->second.take(samples, infos);
      if (pos->second.instance_state() != DDS::ALIVE_INSTANCE_STATE) {
        instance_map_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void read_instance(SampleSequence& samples, InternalSampleInfoSequence& infos, const T& key)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    typename InstanceMap::iterator pos = instance_map_.find(key);
    if (pos != instance_map_.end()) {
      pos->second.read(samples, infos);
      if (pos->second.instance_state() != DDS::ALIVE_INSTANCE_STATE) {
        instance_map_.erase(pos);
      }
    }
  }

  void take_instance(SampleSequence& samples, InternalSampleInfoSequence& infos, const T& key)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    typename InstanceMap::iterator pos = instance_map_.find(key);
    if (pos != instance_map_.end()) {
      pos->second.take(samples, infos);
      if (pos->second.instance_state() != DDS::ALIVE_INSTANCE_STATE) {
        instance_map_.erase(pos);
      }
    }
  }
/// @}

private:
  const DDS::DataReaderQos qos_;
  // Often, the listener will have the reader as a member.  Use a weak
  // pointer to prevent a cycle that prevents the listener from being
  // destroyed.
  Listener_wrch listener_;

  typedef OPENDDS_SET(InternalEntity_wrch) PublicationSet;

  class Instance {
  public:

    Instance()
      : view_state_(DDS::NEW_VIEW_STATE)
      , instance_state_(DDS::ALIVE_INSTANCE_STATE)
      , disposed_generation_count_(0)
      , no_writers_generation_count_(0)
    {}

    DDS::ViewStateKind view_state() const { return view_state_; }

    DDS::InstanceStateKind instance_state() const { return instance_state_; }

    void read(SampleSequence& samples, InternalSampleInfoSequence& infos)
    {
      size_t sample_count = 0;

      for (typename SampleList::const_iterator pos = not_read_samples_.begin(), limit = not_read_samples_.end();
           pos != limit; ++pos) {
        samples.push_back(pos->sample);
        infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, pos->valid_data));
        ++sample_count;
      }

      for (typename SampleList::const_iterator pos = read_samples_.begin(), limit = read_samples_.end();
           pos != limit; ++pos) {
        samples.push_back(pos->sample);
        infos.push_back(make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, pos->valid_data));
        ++sample_count;
      }

      read_samples_.splice(read_samples_.end(), not_read_samples_);

      compute_ranks(sample_count, infos);

      view_state_ = DDS::NOT_NEW_VIEW_STATE;
    }

    void take(SampleSequence& samples, InternalSampleInfoSequence& infos)
    {
      size_t sample_count = 0;

      for (typename SampleList::const_iterator pos = read_samples_.begin(), limit = read_samples_.end();
           pos != limit; ++pos) {
        samples.push_back(pos->sample);
        infos.push_back(make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, pos->valid_data));
        ++sample_count;
      }
      read_samples_.clear();

      for (typename SampleList::const_iterator pos = not_read_samples_.begin(), limit = not_read_samples_.end();
           pos != limit; ++pos) {
        samples.push_back(pos->sample);
        infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, pos->valid_data));
        ++sample_count;
      }
      not_read_samples_.clear();

      compute_ranks(sample_count, infos);

      view_state_ = DDS::NOT_NEW_VIEW_STATE;
    }

    void write(InternalEntity_wrch publication_handle,
               const T& sample,
               const DDS::DataReaderQos& qos)
    {
      publication_set_.insert(publication_handle);

      if (view_state_ == DDS::NOT_NEW_VIEW_STATE && instance_state_ != DDS::ALIVE_INSTANCE_STATE) {
        view_state_ = DDS::NEW_VIEW_STATE;
      }

      switch (instance_state_) {
      case DDS::ALIVE_INSTANCE_STATE:
        break;
      case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
        ++disposed_generation_count_;
        break;
      case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
        ++no_writers_generation_count_;
        break;
      }

      instance_state_ = DDS::ALIVE_INSTANCE_STATE;

      if (qos.history.kind == DDS::KEEP_LAST_HISTORY_QOS) {
        while (read_samples_.size() + not_read_samples_.size() >= qos.history.depth) {
          if (!read_samples_.empty()) {
            read_samples_.pop_front();
          } else {
            not_read_samples_.pop_front();
          }
        }
      }

      not_read_samples_.push_back(SampleHolder(sample, disposed_generation_count_, no_writers_generation_count_, true));
    }

    bool dispose(InternalEntity_wrch publication_handle,
                 const T& key)
    {
      publication_set_.insert(publication_handle);

      if (instance_state_ != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
        not_read_samples_.push_back(SampleHolder(key, disposed_generation_count_, no_writers_generation_count_, false));
        instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
        return true;
      }

      return false;
    }

    bool unregister_instance(InternalEntity_wrch publication_handle,
                             const T& key)
    {
      publication_set_.erase(publication_handle);

      if (publication_set_.empty() && instance_state_ == DDS::ALIVE_INSTANCE_STATE) {
        not_read_samples_.push_back(SampleHolder(key, disposed_generation_count_, no_writers_generation_count_, false));
        instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
        return true;
      }

      return false;
    }

  private:
    struct SampleHolder {
      T sample;
      size_t disposed_generation_count;
      size_t no_writers_generation_count;
      bool valid_data;

      SampleHolder(const T& s,
                   size_t dgc,
                   size_t nwgc,
                   bool vd)
        : sample(s)
        , disposed_generation_count(dgc)
        , no_writers_generation_count(nwgc)
        , valid_data(vd)
      {}
    };

    typedef OPENDDS_LIST(SampleHolder) SampleList;
    SampleList read_samples_;
    SampleList not_read_samples_;

    PublicationSet publication_set_;

    DDS::ViewStateKind view_state_;
    DDS::InstanceStateKind instance_state_;
    size_t disposed_generation_count_;
    size_t no_writers_generation_count_;

    void compute_ranks(size_t sample_count, InternalSampleInfoSequence& infos)
    {
      if (sample_count == 0) {
        return;
      }

      typename InternalSampleInfoSequence::reverse_iterator pos = infos.rbegin();
      const size_t mrsic = pos->disposed_generation_count + pos->no_writers_generation_count;
      size_t mrs = mrsic;
      if (!read_samples_.empty()) {
        mrs = read_samples_.back().disposed_generation_count + read_samples_.back().no_writers_generation_count;
      }
      if (!not_read_samples_.empty()) {
        mrs = not_read_samples_.back().disposed_generation_count + not_read_samples_.back().no_writers_generation_count;
      }

      for (size_t rank = 0; rank != sample_count; ++rank, ++pos) {
        pos->sample_rank = rank;
        pos->generation_rank = mrsic - (pos->disposed_generation_count + pos->no_writers_generation_count);
        pos->absolute_generation_rank = mrs - (pos->disposed_generation_count + pos->no_writers_generation_count);
      }
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
