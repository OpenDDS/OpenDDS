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
#include "TimeTypes.h"

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
                                        CORBA::Long disposed_generation_count,
                                        CORBA::Long no_writers_generation_count,
                                        CORBA::Long sample_rank,
                                        CORBA::Long generation_rank,
                                        CORBA::Long absolute_generation_rank,
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
  si.opendds_reserved_publication_seq = 0;
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

class SampleInfoWrapper {
public:
  SampleInfoWrapper(const DDS::SampleInfo& sample_info)
    : si(sample_info)
  {}

  bool operator==(const SampleInfoWrapper& other) const
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

  explicit InternalDataReader(const DDS::DataReaderQos& qos,
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
      if (pos->second.is_publication(publication_handle)) {
        if (autodispose_unregistered_instances && pos->second.dispose(publication_handle, qos_)) {
          schedule = true;
        }
        if (pos->second.unregister_instance(publication_handle, qos_)) {
          schedule = true;
        }
      }
    }

    if (schedule) {
      const Listener_rch listener = listener_.lock();
      if (listener) {
        listener->schedule(rchandle_from(this));
        // TODO: If the listener doesn't do anything, then clean up then possibly clean up the instance.
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

    if (pos->second.dispose(publication_handle, qos_)) {
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

    if (pos->second.unregister_instance(publication_handle, qos_)) {
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

  void set_interesting_instances(const SampleSequence& instances)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    interesting_instances_ = instances;
  }

  const SampleSequence& get_interesting_instances() const
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, interesting_instances_);
    return interesting_instances_;
  }

  void read(SampleSequence& samples,
            InternalSampleInfoSequence& infos,
            CORBA::Long max_samples,
            DDS::SampleStateMask sample_states,
            DDS::ViewStateMask view_states,
            DDS::InstanceStateMask instance_states)
  {
    samples.clear();
    infos.clear();

    // TODO: Index to avoid the loop.
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ) {
      pos->second.read(pos->first, samples, infos, max_samples, sample_states, view_states, instance_states);
      pos->second.purge_samples(qos_);
      if (pos->second.can_purge_instance(qos_)) {
        instance_map_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void take(SampleSequence& samples,
            InternalSampleInfoSequence& infos,
            CORBA::Long max_samples,
            DDS::SampleStateMask sample_states,
            DDS::ViewStateMask view_states,
            DDS::InstanceStateMask instance_states)
  {
    samples.clear();
    infos.clear();

    // TODO: Index to avoid the loop.
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename InstanceMap::iterator pos = instance_map_.begin(), limit = instance_map_.end(); pos != limit; ) {
      pos->second.take(pos->first, samples, infos, max_samples, sample_states, view_states, instance_states);
      pos->second.purge_samples(qos_);
      if (pos->second.can_purge_instance(qos_)) {
        instance_map_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  void read_instance(SampleSequence& samples,
                     InternalSampleInfoSequence& infos,
                     CORBA::Long max_samples,
                     const T& key,
                     DDS::SampleStateMask sample_states,
                     DDS::ViewStateMask view_states,
                     DDS::InstanceStateMask instance_states)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    typename InstanceMap::iterator pos = instance_map_.find(key);
    if (pos != instance_map_.end()) {
      pos->second.read(pos->first, samples, infos, max_samples, sample_states, view_states, instance_states);
      pos->second.purge_samples(qos_);
      if (pos->second.can_purge_instance(qos_)) {
        instance_map_.erase(pos);
      }
    }
  }

  void take_instance(SampleSequence& samples,
                     InternalSampleInfoSequence& infos,
                     CORBA::Long max_samples,
                     const T& key,
                     DDS::SampleStateMask sample_states,
                     DDS::ViewStateMask view_states,
                     DDS::InstanceStateMask instance_states)
  {
    samples.clear();
    infos.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    typename InstanceMap::iterator pos = instance_map_.find(key);
    if (pos != instance_map_.end()) {
      pos->second.take(pos->first, samples, infos, max_samples, sample_states, view_states, instance_states);
      pos->second.purge_samples(qos_);
      if (pos->second.can_purge_instance(qos_)) {
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

  SampleSequence interesting_instances_;

  typedef OPENDDS_SET(InternalEntity_wrch) PublicationSet;

  class Instance {
  public:

    Instance()
      : view_state_(DDS::NEW_VIEW_STATE)
      , instance_state_(DDS::ALIVE_INSTANCE_STATE)
      , disposed_generation_count_(0)
      , no_writers_generation_count_(0)
      , informed_of_not_alive_(false)
    {
      disposed_expiration_date_.sec = 0;
      disposed_expiration_date_.nanosec = 0;
      no_writers_expiration_date_.sec = 0;
      no_writers_expiration_date_.nanosec = 0;
    }

    bool is_publication(InternalEntity_wrch publication_handle) const
    {
      return publication_set_.count(publication_handle);
    }

    DDS::ViewStateKind view_state() const { return view_state_; }

    DDS::InstanceStateKind instance_state() const { return instance_state_; }

    void purge_samples(const DDS::DataReaderQos& qos)
    {
      if (instance_state_ == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE &&
          !is_infinite(qos.reader_data_lifecycle.autopurge_disposed_samples_delay) &&
          SystemTimePoint::now().to_idl_struct() > disposed_expiration_date_) {
        not_read_samples_.clear();
        read_samples_.clear();
      }
    }

    bool can_purge_instance(const DDS::DataReaderQos& qos) const
    {
      if (instance_state_ != DDS::ALIVE_INSTANCE_STATE &&
          not_read_samples_.empty() &&
          read_samples_.empty() &&
          publication_set_.empty()) {
        return true;
      }

      if (instance_state_ == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE &&
          !is_infinite(qos.reader_data_lifecycle.autopurge_nowriter_samples_delay) &&
          SystemTimePoint::now().to_idl_struct() > no_writers_expiration_date_) {
        return true;
      }

      return false;
    }

    void read(const T& key,
              SampleSequence& samples,
              InternalSampleInfoSequence& infos,
              CORBA::Long max_samples,
              DDS::SampleStateMask sample_states,
              DDS::ViewStateMask view_states,
              DDS::InstanceStateMask instance_states)
    {
      if (!((view_states & view_state_) && (instance_states & instance_state_))) {
        return;
      }

      CORBA::Long sample_count = 0;

      if (sample_states & DDS::READ_SAMPLE_STATE) {
        for (typename SampleList::const_iterator pos = read_samples_.begin(), limit = read_samples_.end();
             pos != limit && (max_samples == DDS::LENGTH_UNLIMITED || sample_count < max_samples); ++pos) {
          samples.push_back(pos->sample);
          infos.push_back(make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, true));
          ++sample_count;
        }
      }

      if (sample_states & DDS::NOT_READ_SAMPLE_STATE) {
        typename SampleList::iterator pos = not_read_samples_.begin();
        for (typename SampleList::iterator limit = not_read_samples_.end();
             pos != limit && (max_samples == DDS::LENGTH_UNLIMITED || sample_count < max_samples); ++pos) {
          samples.push_back(pos->sample);
          infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, true));
          ++sample_count;
        }
        read_samples_.splice(read_samples_.end(), not_read_samples_, not_read_samples_.begin(), pos);

        // Generate a synthetic sample for not alive states.
        if (sample_count == 0 &&
            instance_state_ != DDS::ALIVE_INSTANCE_STATE &&
            !informed_of_not_alive_) {
          samples.push_back(key);
          infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, disposed_generation_count_, no_writers_generation_count_, 0, 0, 0, false));
          ++sample_count;
        }
      }

      compute_ranks(sample_count, infos);

      if (sample_count) {
        view_state_ = DDS::NOT_NEW_VIEW_STATE;
        informed_of_not_alive_ = true;
      }
    }

    void take(const T& key,
              SampleSequence& samples,
              InternalSampleInfoSequence& infos,
              CORBA::Long max_samples,
              DDS::SampleStateMask sample_states,
              DDS::ViewStateMask view_states,
              DDS::InstanceStateMask instance_states)
    {
      if (!((view_states & view_state_) && (instance_states & instance_state_))) {
        return;
      }

      CORBA::Long sample_count = 0;

      if (sample_states & DDS::READ_SAMPLE_STATE) {
        typename SampleList::iterator pos = read_samples_.begin();
        for (typename SampleList::iterator limit = read_samples_.end();
             pos != limit && (max_samples == DDS::LENGTH_UNLIMITED || sample_count < max_samples); ++pos) {
          samples.push_back(pos->sample);
          infos.push_back(make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, true));
          ++sample_count;
        }
        read_samples_.erase(read_samples_.begin(), pos);
      }

      if (sample_states & DDS::NOT_READ_SAMPLE_STATE) {
        typename SampleList::iterator pos = not_read_samples_.begin();
        for (typename SampleList::iterator limit = not_read_samples_.end();
             pos != limit && (max_samples == DDS::LENGTH_UNLIMITED || sample_count < max_samples); ++pos) {
          samples.push_back(pos->sample);
          infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, 0, true));
          ++sample_count;
        }
        not_read_samples_.erase(not_read_samples_.begin(), pos);

        // Generate a synthetic sample for not alive states.
        if (sample_count == 0 &&
            instance_state_ != DDS::ALIVE_INSTANCE_STATE &&
            !informed_of_not_alive_) {
          samples.push_back(key);
          infos.push_back(make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, disposed_generation_count_, no_writers_generation_count_, 0, 0, 0, false));
          ++sample_count;
        }
      }

      compute_ranks(sample_count, infos);

      if (sample_count) {
        view_state_ = DDS::NOT_NEW_VIEW_STATE;
        informed_of_not_alive_ = true;
      }
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
        while (read_samples_.size() + not_read_samples_.size() >= static_cast<size_t>(qos.history.depth)) {
          if (!read_samples_.empty()) {
            read_samples_.pop_front();
          } else {
            not_read_samples_.pop_front();
          }
        }
      }

      not_read_samples_.push_back(SampleHolder(sample, disposed_generation_count_, no_writers_generation_count_));
    }

    bool dispose(InternalEntity_wrch publication_handle,
                 const DDS::DataReaderQos& qos)
    {
      publication_set_.insert(publication_handle);

      if (instance_state_ == DDS::ALIVE_INSTANCE_STATE) {
        instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
        disposed_expiration_date_ = SystemTimePoint::now().to_idl_struct() + qos.reader_data_lifecycle.autopurge_disposed_samples_delay;
        informed_of_not_alive_ = false;
        return true;
      }

      return false;
    }

    bool unregister_instance(InternalEntity_wrch publication_handle,
                             const DDS::DataReaderQos& qos)
    {
      publication_set_.erase(publication_handle);

      if (publication_set_.empty() && instance_state_ == DDS::ALIVE_INSTANCE_STATE) {
        instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
        no_writers_expiration_date_ = SystemTimePoint::now().to_idl_struct() + qos.reader_data_lifecycle.autopurge_nowriter_samples_delay;
        informed_of_not_alive_ = false;
        return true;
      }

      return false;
    }

  private:
    struct SampleHolder {
      T sample;
      CORBA::Long disposed_generation_count;
      CORBA::Long no_writers_generation_count;

      SampleHolder(const T& s,
                   CORBA::Long dgc,
                   CORBA::Long nwgc)
        : sample(s)
        , disposed_generation_count(dgc)
        , no_writers_generation_count(nwgc)
      {}
    };

    typedef OPENDDS_LIST(SampleHolder) SampleList;
    SampleList read_samples_;
    SampleList not_read_samples_;

    PublicationSet publication_set_;

    DDS::ViewStateKind view_state_;
    DDS::InstanceStateKind instance_state_;
    DDS::Time_t disposed_expiration_date_;
    DDS::Time_t no_writers_expiration_date_;
    CORBA::Long disposed_generation_count_;
    CORBA::Long no_writers_generation_count_;
    bool informed_of_not_alive_;

    void compute_ranks(CORBA::Long sample_count, InternalSampleInfoSequence& infos)
    {
      if (sample_count == 0) {
        return;
      }

      typename InternalSampleInfoSequence::reverse_iterator pos = infos.rbegin();
      const CORBA::Long mrsic = pos->disposed_generation_count + pos->no_writers_generation_count;
      const CORBA::Long mrs = disposed_generation_count_ + no_writers_generation_count_;

      for (CORBA::Long rank = 0; rank != sample_count; ++rank, ++pos) {
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
