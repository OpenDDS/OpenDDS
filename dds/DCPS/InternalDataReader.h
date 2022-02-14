/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INTERNAL_DATA_READER_H
#define OPENDDS_DCPS_INTERNAL_DATA_READER_H

#include "DisjointSequence.h"
#include "JobQueue.h"
#include "SampleCache.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// A collection of samples belonging to different instances.
template <typename Sample>
class InternalDataReader : public RcObject {
public:
  typedef std::vector<Sample> SampleList;
  typedef Schedulable Observer;
  typedef RcHandle<Observer> ObserverPtr;
  typedef WeakRcHandle<Observer> WeakObserverPtr;

  InternalDataReader(size_t depth = -1)
    : next_instance_handle_(0)
    , depth_(depth)
  {}

  void set_observer(ObserverPtr observer)
  {
    observer_ = observer;
  }


  // All of the samples in other should belong to a single publication.
  void initialize(const InternalDataReader& other)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g1, mutex_);
      {
        ACE_GUARD(ACE_Thread_Mutex, g2, other.mutex_);
        for (typename KeyCache::const_iterator pos = other.key_to_cache_.begin(), limit = other.key_to_cache_.end(); pos != limit; ++pos) {
          SampleCache2Ptr s = make_rch<SampleCache2>(get_next_instance_handle());
          s->initialize(*pos->second);
          s->resize(depth_);
          key_to_cache_[pos->first] = s;
          instance_to_cache_[s->get_instance_handle()] = s;
          state_to_caches_[s->state()].insert(s);
        }
      }

      observer = observer_.lock();
    }

    if (observer) {
      observer->schedule();
    }
  }

  void register_instance(const Sample& sample, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCache2Ptr sample_cache = insert_instance_i(sample);
      const SampleCache2State s1 = sample_cache->state();
      sample_cache->register_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCache2State s2 = sample_cache->state();
      if (s1 != s2) {
        state_to_caches_[s1].erase(sample_cache);
        state_to_caches_[s2].insert(sample_cache);
      }
      observer = observer_.lock();
    }

    if (observer) {
      observer->schedule();
    }
  }

  void store(const Sample& sample, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCache2Ptr sample_cache = insert_instance_i(sample);
      const SampleCache2State s1 = sample_cache->state();
      sample_cache->store(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCache2State s2 = sample_cache->state();
      if (s1 != s2) {
        state_to_caches_[s1].erase(sample_cache);
        state_to_caches_[s2].insert(sample_cache);
      }
      observer = observer_.lock();
    }

    if (observer) {
      observer->schedule();
    }
  }

  void unregister_instance(const Sample& sample, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCache2Ptr sample_cache = lookup_instance_i(sample);
      if (!sample_cache) {
        return;
      }
      const SampleCache2State s1 = sample_cache->state();
      sample_cache->unregister_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCache2State s2 = sample_cache->state();
      if (s1 != s2) {
        state_to_caches_[s1].erase(sample_cache);
        state_to_caches_[s2].insert(sample_cache);
      }
      observer = observer_.lock();
    }

    if (observer) {
      observer->schedule();
    }
  }

  void dispose_instance(const Sample& sample, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCache2Ptr sample_cache = lookup_instance_i(sample);
      if (!sample_cache) {
        return;
      }
      const SampleCache2State s1 = sample_cache->state();
      sample_cache->dispose_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCache2State s2 = sample_cache->state();
      if (s1 != s2) {
        state_to_caches_[s1].erase(sample_cache);
        state_to_caches_[s2].insert(sample_cache);
      }
      observer = observer_.lock();
    }

    if (observer) {
      observer->schedule();
    }
  }

  void read(SampleList& sample_list,
            DDS::SampleInfoSeq& sample_info_list,
            size_t max_samples,
            int sample_state_mask,
            int view_state_mask,
            int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.length(0);

    SampleCache2PtrSet reclassify;

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (typename StateCache::iterator pos = state_to_caches_.begin(), limit = state_to_caches_.end(); sample_list.size() != max_samples && pos != limit; ++pos) {
      if ((pos->first.sample_state & sample_state_mask) &&
          (pos->first.view_state & view_state_mask) &&
          (pos->first.instance_state & instance_state_mask)) {
        const SampleCache2State& state = pos->first;
        SampleCache2PtrSet& s = pos->second;
        for (typename SampleCache2PtrSet::const_iterator pos = s.begin(), limit = s.end(); sample_list.size() != max_samples && pos != limit;) {
          SampleCache2Ptr scp = *pos;
          scp->read(sample_list, sample_info_list, max_samples, sample_state_mask);
          if (scp->state() != state) {
            s.erase(pos++);
            reclassify.insert(scp);
          } else {
            ++pos;
          }
        }
      }
    }

    for (typename SampleCache2PtrSet::const_iterator pos = reclassify.begin(), limit = reclassify.end(); pos != limit; ++pos) {
      state_to_caches_[(*pos)->state()].insert(*pos);
    }
  }

  void take(SampleList& sample_list,
            DDS::SampleInfoSeq& sample_info_list,
            size_t max_samples,
            int sample_state_mask,
            int view_state_mask,
            int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.length(0);

    SampleCache2PtrSet reclassify;

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (typename StateCache::iterator pos = state_to_caches_.begin(), limit = state_to_caches_.end(); sample_list.size() != max_samples && pos != limit; ++pos) {
      if ((pos->first.sample_state & sample_state_mask) &&
          (pos->first.view_state & view_state_mask) &&
          (pos->first.instance_state & instance_state_mask)) {
        const SampleCache2State& state = pos->first;
        SampleCache2PtrSet& s = pos->second;
        for (typename SampleCache2PtrSet::const_iterator pos = s.begin(), limit = s.end(); sample_list.size() != max_samples && pos != limit;) {
          SampleCache2Ptr scp = *pos;
          scp->take(sample_list, sample_info_list, max_samples, sample_state_mask);
          if (scp->state() != state) {
            s.erase(pos++);
            reclassify.insert(scp);
          } else {
            ++pos;
          }
        }
      }
    }

    for (typename SampleCache2PtrSet::const_iterator pos = reclassify.begin(), limit = reclassify.end(); pos != limit; ++pos) {
      state_to_caches_[(*pos)->state()].insert(*pos);
    }
  }

  // Not implemented.
  // void read_w_condition(SampleList& sample_list,
  //                       DDS::SampleInfoSeq& sample_info_list,
  //                       size_t max_samples,
  //                       Condition condition);

  // Not implemented.
  // void take_w_condition(SampleList& sample_list,
  //                       DDS::SampleInfoSeq& sample_info_list,
  //                       size_t max_samples,
  //                       Condition condition);

  bool read_next_sample(Sample& sample,
                        DDS::SampleInfo& sample_info)
  {
    SampleList sample_list;
    DDS::SampleInfoSeq sample_info_list;
    read(sample_list, sample_info_list, 1, DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!sample_list.empty()) {
      sample = sample_list[0];
      sample_info = sample_info_list[0];
      return true;
    }
    return false;
  }

  bool take_next_sample(Sample& sample,
                        DDS::SampleInfo& sample_info)
  {
    SampleList sample_list;
    DDS::SampleInfoSeq sample_info_list;
    take(sample_list, sample_info_list, 1, DDS::NOT_READ_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    if (!sample_list.empty()) {
      sample = sample_list[0];
      sample_info = sample_info_list[0];
      return true;
    }
    return false;
  }

  void read_instance(SampleList& sample_list,
                     DDS::SampleInfoSeq& sample_info_list,
                     size_t max_samples,
                     DDS::InstanceHandle_t a_handle,
                     int sample_state_mask,
                     int view_state_mask,
                     int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.length(0);

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceCache::const_iterator pos = instance_to_cache_.find(a_handle);
    if (pos == instance_to_cache_.end()) {
      return;
    }

    read_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
  }

  void take_instance(SampleList& sample_list,
                     DDS::SampleInfoSeq& sample_info_list,
                     size_t max_samples,
                     DDS::InstanceHandle_t a_handle,
                     int sample_state_mask,
                     int view_state_mask,
                     int instance_state_mask)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    sample_list.clear();
    sample_info_list.length(0);

    typename InstanceCache::const_iterator pos = instance_to_cache_.find(a_handle);
    if (pos == instance_to_cache_.end()) {
      return;
    }

    SampleCache2Ptr scp = pos->second;
    const SampleCache2State state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->take(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCache2State state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

  bool read_next_instance(SampleList& sample_list,
                          DDS::SampleInfoSeq& sample_info_list,
                          size_t max_samples,
                          DDS::InstanceHandle_t previous_handle,
                          int sample_state_mask,
                          int view_state_mask,
                          int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.length(0);

    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    typename InstanceCache::const_iterator pos = instance_to_cache_.upper_bound(previous_handle);
    if (pos == instance_to_cache_.end()) {
      return false;
    }

    read_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
    return !sample_list.empty();
  }

  void take_next_instance(SampleList& sample_list,
                          DDS::SampleInfoSeq& sample_info_list,
                          size_t max_samples,
                          DDS::InstanceHandle_t previous_handle,
                          int sample_state_mask,
                          int view_state_mask,
                          int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.length(0);

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceCache::const_iterator pos = instance_to_cache_.upper_bound(previous_handle);
    if (pos == instance_to_cache_.end()) {
      return;
    }

    take_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
  }

  // Not implemented.
  // void read_next_instance_w_condition(SampleList& sample_list,
  //                                     DDS::SampleInfoSeq& sample_info_list,
  //                                     size_t max_samples,
  //                                     InstanceHandle previous_handle,
  //                                     Condition condition);

  // Not implemented.
  // void take_next_instance_w_condition(SampleList& sample_list,
  //                                     DDS::SampleInfoSeq& sample_info_list,
  //                                     size_t max_samples,
  //                                     InstanceHandle previous_handle,
  //                                     Condition condition);

  bool get_key_value(Sample& key_holder,
                     DDS::InstanceHandle_t handle)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    typename InstanceCache::const_iterator pos = instance_to_cache_.find(handle);
    if (pos != instance_to_cache_.end()) {
      return pos->second->get_key_value(key_holder);
    }

    return false;
  }

  DDS::InstanceHandle_t lookup_instance(const Sample& sample)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos != key_to_cache_.end()) {
      return pos->second->get_instance_handle();
    }
    return 0;
  }

private:
  typedef SampleCache2<Sample> SampleCache2;
  typedef RcHandle<SampleCache2> SampleCache2Ptr;
  typedef std::map<Sample, SampleCache2Ptr> KeyCache;
  typedef std::map<DDS::InstanceHandle_t, SampleCache2Ptr> InstanceCache;
  typedef std::set<SampleCache2Ptr> SampleCache2PtrSet;
  typedef std::map<SampleCache2State, SampleCache2PtrSet> StateCache;

  mutable ACE_Thread_Mutex mutex_;
  DDS::InstanceHandle_t next_instance_handle_;
  DisjointSequence free_instance_handles_;
  KeyCache key_to_cache_; // For per-instance operations.
  InstanceCache instance_to_cache_; // For per-instance operations.
  StateCache state_to_caches_;
  size_t depth_;
  WeakObserverPtr observer_;

  DDS::InstanceHandle_t get_next_instance_handle()
  {
    if (!free_instance_handles_.empty()) {
      SequenceNumber low = free_instance_handles_.low();
      free_instance_handles_.erase(low);
      // TODO: Templatize DisjointSequence.
      return low.getValue();
    } else {
      return ++next_instance_handle_;
    }
  }

  SampleCache2Ptr insert_instance_i(const Sample& sample)
  {
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos == key_to_cache_.end()) {
      pos = key_to_cache_.insert(std::make_pair(sample, make_rch<SampleCache2>(get_next_instance_handle()))).first;
      instance_to_cache_[pos->second->get_instance_handle()] = pos->second;
      state_to_caches_[pos->second->state()].insert(pos->second);
    }
    return pos->second;
  }

  SampleCache2Ptr lookup_instance_i(const Sample& sample)
  {
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos != key_to_cache_.end()) {
      return pos->second;
    }
    return SampleCache2Ptr();
  }

  void read_instance_i(SampleList& sample_list,
                       DDS::SampleInfoSeq& sample_info_list,
                       size_t max_samples,
                       SampleCache2Ptr scp,
                       int sample_state_mask,
                       int view_state_mask,
                       int instance_state_mask)
  {
    const SampleCache2State state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->read(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCache2State state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

  void take_instance_i(SampleList& sample_list,
                       DDS::SampleInfoSeq& sample_info_list,
                       size_t max_samples,
                       SampleCache2Ptr scp,
                       int sample_state_mask,
                       int view_state_mask,
                       int instance_state_mask)
  {
    const SampleCache2State state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->take(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCache2State state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_INTERNAL_DATA_READER_H */
