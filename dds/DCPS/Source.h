/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SOURCE_H
#define OPENDDS_DCPS_SOURCE_H

#include "dds/DCPS/JobQueue.h"
#include "dds/DCPS/TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ObserverInterface : public virtual RcObject {
public:
  virtual void schedule() = 0;
};

template <typename Sample>
class Sink;

template <typename Sample>
class JobQueueObserver : public ObserverInterface, public JobQueue::Job {
public:
  typedef RcHandle<Sink<Sample>> SinkPtr;
  typedef WeakRcHandle<Sink<Sample>> WeakSinkPtr;

  JobQueueObserver()
    : scheduled_(false)
  {}

  void set_job_queue(WeakRcHandle<JobQueue> job_queue)
  {
    job_queue_ = job_queue;
  }

  void set_sink(WeakSinkPtr sink)
  {
    sink_ = sink;
  }

  virtual void schedule()
  {
    // Don't need a lock so long as the sink is locked.
    if (scheduled_ == true) {
      return;
    }

    RcHandle<JobQueue> job_queue = job_queue_.lock();
    if (job_queue) {
      job_queue->enqueue(rchandle_from(this));
      scheduled_ = true;
    }
  }

  virtual void execute()
  {
    scheduled_ = false;
    SinkPtr sink = sink_.lock();
    if (sink) {
      observe(sink);
    }
  }

  virtual void observe(SinkPtr sink) = 0;

private:
  WeakRcHandle<JobQueue> job_queue_;
  WeakSinkPtr sink_;
  bool scheduled_;
};

enum SampleState {
  READ_SAMPLE_STATE = 1 << 0,
  NOT_READ_SAMPLE_STATE = 1 << 1
};

const int NO_SAMPLE_STATE = 0;
const int ANY_SAMPLE_STATE = READ_SAMPLE_STATE | NOT_READ_SAMPLE_STATE;

enum ViewState {
  NEW_VIEW_STATE = 1 << 0,
  NOT_NEW_VIEW_STATE = 1 << 1
};

const int ANY_VIEW_STATE = NEW_VIEW_STATE | NOT_NEW_VIEW_STATE;

enum InstanceState {
  ALIVE_INSTANCE_STATE = 1 << 0,
  NOT_ALIVE_DISPOSED_INSTANCE_STATE = 1 << 1,
  NOT_ALIVE_NO_WRITERS_INSTANCE_STATE = 1 << 2
};

const int ANY_INSTANCE_STATE = ALIVE_INSTANCE_STATE | NOT_ALIVE_DISPOSED_INSTANCE_STATE | NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;

typedef const void* InstanceHandle;
typedef const void* PublicationHandle;
typedef std::set<PublicationHandle> PublicationHandleSet;

struct SampleInfo {
  SampleState sample_state;
  ViewState view_state;
  InstanceState instance_state;
  size_t disposed_generation_count;
  size_t no_writers_generation_count;
  size_t sample_rank;
  size_t generation_rank;
  size_t absolute_generation_rank;
  MonotonicTimePoint source_timestamp;
  InstanceHandle instance_handle;
  PublicationHandle publication_handle;
  bool valid_data;

  SampleInfo()
    : sample_state(READ_SAMPLE_STATE)
    , view_state(NEW_VIEW_STATE)
    , instance_state(ALIVE_INSTANCE_STATE)
    , disposed_generation_count(0)
    , no_writers_generation_count(0)
    , sample_rank(0)
    , generation_rank(0)
    , absolute_generation_rank(0)
    , instance_handle(0)
    , publication_handle(0)
    , valid_data(false)
  {}

  SampleInfo(SampleState a_sample_state,
             ViewState a_view_state,
             InstanceState a_instance_state,
             size_t a_disposed_generation_count,
             size_t a_no_writers_generation_count,
             size_t a_sample_rank,
             size_t a_generation_rank,
             size_t a_absolute_generation_rank,
             const MonotonicTimePoint& a_source_timestamp,
             InstanceHandle a_instance_handle,
             PublicationHandle a_publication_handle,
             bool a_valid_data)
    : sample_state(a_sample_state)
    , view_state(a_view_state)
    , instance_state(a_instance_state)
    , disposed_generation_count(a_disposed_generation_count)
    , no_writers_generation_count(a_no_writers_generation_count)
    , sample_rank(a_sample_rank)
    , generation_rank(a_generation_rank)
    , absolute_generation_rank(a_absolute_generation_rank)
    , source_timestamp(a_source_timestamp)
    , instance_handle(a_instance_handle)
    , publication_handle(a_publication_handle)
    , valid_data(a_valid_data)
  {}

  bool operator==(const SampleInfo& other)
  {
    return sample_state == other.sample_state &&
      view_state == other.view_state &&
      instance_state == other.instance_state &&
      disposed_generation_count == other.disposed_generation_count &&
      no_writers_generation_count == other.no_writers_generation_count &&
      sample_rank == other.sample_rank &&
      generation_rank == other.generation_rank &&
      absolute_generation_rank == other.absolute_generation_rank &&
      source_timestamp == other.source_timestamp &&
      instance_handle == other.instance_handle &&
      publication_handle == other.publication_handle &&
      valid_data == other.valid_data;
  }
};

inline std::ostream& operator<<(std::ostream& out, const SampleInfo& sample_info)
{
  out << "SampleInfo("
      << sample_info.sample_state << ','
      << sample_info.view_state << ','
      << sample_info.instance_state << ','
      << sample_info.disposed_generation_count << ','
      << sample_info.no_writers_generation_count << ','
      << sample_info.sample_rank << ','
      << sample_info.generation_rank << ','
      << sample_info.absolute_generation_rank << ','
      << sample_info.source_timestamp.value().sec() << '.' << sample_info.source_timestamp.value().usec() << ','
      << sample_info.instance_handle << ','
      << sample_info.publication_handle << ','
      << sample_info.valid_data << ")";
  return out;
}

typedef std::vector<SampleInfo> SampleInfoList;

struct SampleCacheState {
  int sample_state;
  ViewState view_state;
  InstanceState instance_state;

  SampleCacheState(int a_sample_state,
                   ViewState a_view_state,
                   InstanceState a_instance_state)
    : sample_state(a_sample_state)
    , view_state(a_view_state)
    , instance_state(a_instance_state)
  {}

  bool operator==(const SampleCacheState& other) const
  {
    return sample_state == other.sample_state && view_state == other.view_state && instance_state == other.instance_state;
  }

  bool operator!=(const SampleCacheState& other) const
  {
    return !(*this == other);
  }

  bool operator<(const SampleCacheState& other) const
  {
    if (sample_state != other.sample_state) {
      return sample_state > other.sample_state;
    }
    if (view_state != other.view_state) {
      return view_state < other.view_state;
    }
    return instance_state < other.instance_state;
  }
};

// A collection of samples belonging to the same instance.
template<typename Sample>
class SampleCache : public RcObject {
public:
  typedef std::vector<Sample> SampleList;
  typedef RcHandle<SampleCache> SampleCachePtr;

  SampleCache()
    : view_state_(NEW_VIEW_STATE)
    , instance_state_(ALIVE_INSTANCE_STATE)
    , instance_element_state_(NOT_READ)
    , disposed_generation_count_(0)
    , no_writers_generation_count_(0)
  { }

  void initialize(const SampleCache& other)
  {
    // All of the samples are new.
    new_samples_.insert(new_samples_.end(), other.not_new_samples_.begin(), other.not_new_samples_.end());
    new_samples_.insert(new_samples_.end(), other.new_samples_.begin(), other.new_samples_.end());
    view_state_ = NEW_VIEW_STATE;
    instance_state_ = other.instance_state_;
    instance_element_state_ = NOT_READ;
    instance_element_ = other.instance_element_;
    disposed_generation_count_ = 0;
    no_writers_generation_count_ = 0;
    publication_handles_ = other.publication_handles_;
  }

  void register_instance(const Sample& key, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    std::pair<PublicationHandleSet::iterator, bool> p = publication_handles_.insert(publication_handle);

    switch (instance_state_) {
    case ALIVE_INSTANCE_STATE:
      if (p.second) {
        instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      }
      break;
    case NOT_ALIVE_DISPOSED_INSTANCE_STATE:
      instance_element_ = Element(key, ++disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      break;
    case NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      instance_element_ = Element(key, disposed_generation_count_, ++no_writers_generation_count_, source_timestamp, publication_handle);
      break;
    }

    if (view_state_ == NOT_NEW_VIEW_STATE) {
      if (instance_state_ != ALIVE_INSTANCE_STATE) {
        view_state_ = NEW_VIEW_STATE;
      }
    }

    if (instance_state_ != ALIVE_INSTANCE_STATE) {
      instance_element_state_ = NOT_READ;
      instance_state_ = ALIVE_INSTANCE_STATE;
    }
  }

  void write(const Sample& sample, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    register_instance(sample, source_timestamp, publication_handle);
    new_samples_.push_back(Element(sample, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle));
  }

  void unregister_instance(const Sample& key, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    publication_handles_.erase(publication_handle);

    if (instance_state_ == ALIVE_INSTANCE_STATE && publication_handles_.empty()) {
      instance_state_ = NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
      instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      instance_element_state_ = NOT_READ;
      // TODO: Do we need to change the view state?
    }
  }

  void dispose_instance(const Sample& key, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    if (instance_state_ == ALIVE_INSTANCE_STATE) {
      instance_state_ = NOT_ALIVE_DISPOSED_INSTANCE_STATE;
      instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      instance_element_state_ = NOT_READ;
      // TODO: Do we need to change the view state?
    }
  }

  void read(SampleList& sample_list, SampleInfoList& sample_info_list, size_t max_samples, int sample_state_mask)
  {
    typename ListType::const_iterator new_samples_begin = new_samples_.begin();
    typename ListType::const_iterator new_samples_pos = new_samples_.begin();
    typename ListType::const_iterator new_samples_end = new_samples_.end();

    const size_t size_before = sample_list.size();
    // mrsic = most recent sample in returned collection
    size_t not_read_mrsic_idx = -1;
    size_t read_mrsic_idx = -1;

    // Prefer not read samples.
    if (sample_state_mask & NOT_READ_SAMPLE_STATE) {
      while (sample_list.size() != max_samples && new_samples_pos != new_samples_end) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(new_samples_pos->sample);
        sample_info_list.push_back(SampleInfo(NOT_READ_SAMPLE_STATE, view_state_, instance_state_, new_samples_pos->disposed_generation_count, new_samples_pos->no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (new_samples_pos->disposed_generation_count + new_samples_pos->no_writers_generation_count), new_samples_pos->source_timestamp, get_instance_handle(), new_samples_pos->publication_handle, true));
        ++new_samples_pos;
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == NOT_READ) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        sample_info_list.push_back(SampleInfo(NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, false));
        instance_element_state_ = READ;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = READ;
      }
    }

    if (sample_state_mask & READ_SAMPLE_STATE) {
      for (typename ListType::const_iterator pos = not_new_samples_.begin(), limit = not_new_samples_.end();
           sample_list.size() != max_samples && pos != limit; ++pos) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(pos->sample);
        sample_info_list.push_back(SampleInfo(READ_SAMPLE_STATE, view_state_, instance_state_, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (pos->disposed_generation_count + pos->no_writers_generation_count), pos->source_timestamp, get_instance_handle(), pos->publication_handle, true));
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == READ) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        sample_info_list.push_back(SampleInfo(READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, false));
      }
    }

    // Transfer not read to read.
    not_new_samples_.splice(not_new_samples_.end(), new_samples_, new_samples_begin, new_samples_pos);

    view_state_ = NOT_NEW_VIEW_STATE;

    size_t sample_rank = sample_list.size() - size_before;
    if (sample_rank != 0) {
      const SampleInfo& mrsic = not_read_mrsic_idx != size_t(-1) ? sample_info_list[not_read_mrsic_idx] : sample_info_list[read_mrsic_idx];
      for (size_t idx = size_before; idx != sample_list.size(); ++idx) {
        sample_info_list[idx].sample_rank = --sample_rank;
        sample_info_list[idx].generation_rank =
          (mrsic.disposed_generation_count + mrsic.no_writers_generation_count) -
          (sample_info_list[idx].disposed_generation_count + mrsic.no_writers_generation_count);
      }
    }
  }

  void take(SampleList& sample_list, SampleInfoList& sample_info_list, size_t max_samples, int sample_state_mask)
  {
    const size_t size_before = sample_list.size();
    size_t not_read_mrsic_idx = -1;
    size_t read_mrsic_idx = -1;

    // Prefer read samples.
    if (sample_state_mask & READ_SAMPLE_STATE) {
      while (!not_new_samples_.empty() && sample_list.size() != max_samples) {
        read_mrsic_idx = sample_list.size();
        const Element& front = not_new_samples_.front();
        sample_list.push_back(front.sample);
        sample_info_list.push_back(SampleInfo(READ_SAMPLE_STATE, view_state_, instance_state_, front.disposed_generation_count, front.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (front.disposed_generation_count + front.no_writers_generation_count), front.source_timestamp, get_instance_handle(), front.publication_handle, true));
        not_new_samples_.pop_front();
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == READ) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        sample_info_list.push_back(SampleInfo(NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, false));
        instance_element_state_ = TAKEN;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = TAKEN;
      }
    }

    if (sample_state_mask & NOT_READ_SAMPLE_STATE) {
      while (!new_samples_.empty() && sample_list.size() != max_samples) {
        not_read_mrsic_idx = sample_list.size();
        const Element& front = new_samples_.front();
        sample_list.push_back(front.sample);
        sample_info_list.push_back(SampleInfo(NOT_READ_SAMPLE_STATE, view_state_, instance_state_, front.disposed_generation_count, front.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (front.disposed_generation_count + front.no_writers_generation_count), front.source_timestamp, get_instance_handle(), front.publication_handle, true));
        new_samples_.pop_front();
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == NOT_READ) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        sample_info_list.push_back(SampleInfo(NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, false));
        instance_element_state_ = TAKEN;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = TAKEN;
      }
    }

    view_state_ = NOT_NEW_VIEW_STATE;

    size_t sample_rank = sample_list.size() - size_before;
    if (sample_rank != 0) {
      const SampleInfo& mrsic = not_read_mrsic_idx != size_t(-1) ? sample_info_list[not_read_mrsic_idx] : sample_info_list[read_mrsic_idx];
      for (size_t idx = size_before; idx != sample_list.size(); ++idx) {
        sample_info_list[idx].sample_rank = --sample_rank;
        sample_info_list[idx].generation_rank =
          (mrsic.disposed_generation_count + mrsic.no_writers_generation_count) -
          (sample_info_list[idx].disposed_generation_count + mrsic.no_writers_generation_count);
      }
    }
  }

  void resize(size_t a_size)
  {
    while (!not_new_samples_.empty() && size() > a_size) {
      not_new_samples_.pop_front();
    }

    while (!new_samples_.empty() && size() > a_size) {
      new_samples_.pop_front();
    }
  }

  InstanceHandle get_instance_handle() const { return this; }
  bool get_key_value(Sample& sample) const
  {
    sample = instance_element_.sample;
    return true;
  }
  bool empty() const { return not_new_samples_.empty() && new_samples_.empty(); }
  size_t size() const { return not_new_samples_.size() + new_samples_.size(); }
  size_t not_new_size() const { return not_new_samples_.size(); }
  size_t new_size() const { return new_samples_.size(); }
  size_t writer_count() const { return publication_handles_.size(); }

  SampleCacheState state() const
  {
    const int sample_state =
      ((instance_element_state_ == NOT_READ || !new_samples_.empty()) ? NOT_READ_SAMPLE_STATE : 0) |
      ((instance_element_state_ == READ || !not_new_samples_.empty()) ? READ_SAMPLE_STATE : 0);
    return SampleCacheState(sample_state, view_state_, instance_state_);
  }

private:
  // We can handle two extremes relatively easily.  First, if the set
  // of instances are partitioned across the writers, then each
  // instance will maintain a set that adds to the empty set.  Second,
  // if all writers write all instances, then each instance will
  // maintain a set that subtracts from the full set.  The worst case
  // is where each writer writes a random subset (half) of the
  // instances.  In this case, there is no way to encode it
  // efficiently.  A technique like binary PCA would be an optimal way
  // to encode the various subsets but the performance might not be
  // acceptable.  For now, the sets will be additive.
  struct Element {
    Sample sample;
    size_t disposed_generation_count;
    size_t no_writers_generation_count;
    MonotonicTimePoint source_timestamp;
    PublicationHandle publication_handle;

    Element()
      : disposed_generation_count(0)
      , no_writers_generation_count(0)
      , publication_handle(0)
    {}

    Element(const Sample& a_sample,
            size_t a_disposed_generation_count,
            size_t a_no_writers_generation_count,
            const MonotonicTimePoint& a_source_timestamp,
            PublicationHandle a_publication_handle)
      : sample(a_sample)
      , disposed_generation_count(a_disposed_generation_count)
      , no_writers_generation_count(a_no_writers_generation_count)
      , source_timestamp(a_source_timestamp)
      , publication_handle(a_publication_handle)
    {}
  };
  typedef std::list<Element> ListType;
  ListType not_new_samples_;
  ListType new_samples_;
  ViewState view_state_;
  InstanceState instance_state_;
  enum {
    NOT_READ,
    READ,
    TAKEN
  } instance_element_state_;
  Element instance_element_;
  size_t disposed_generation_count_;
  size_t no_writers_generation_count_;
  PublicationHandleSet publication_handles_;

  static bool valid_data_pred(const Element& element)
  {
    return element.valid_data;
  }
};

// A collection of samples belonging to different instances.
template <typename Sample>
class Sink : public RcObject {
public:
  typedef std::vector<Sample> SampleList;
  typedef ObserverInterface Observer;
  typedef RcHandle<Observer> ObserverPtr;
  typedef WeakRcHandle<Observer> WeakObserverPtr;

  Sink(size_t depth = -1)
    : depth_(depth)
  {}

  void set_observer(ObserverPtr observer)
  {
    observer_ = observer;
  }


  // All of the samples in other should belong to a single publication.
  void initialize(const Sink& other)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g1, mutex_);
      {
        ACE_GUARD(ACE_Thread_Mutex, g2, other.mutex_);
        for (typename KeyCache::const_iterator pos = other.key_to_cache_.begin(), limit = other.key_to_cache_.end(); pos != limit; ++pos) {
          SampleCachePtr s = make_rch<SampleCache>();
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

  void register_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCachePtr sample_cache = insert_instance_i(sample);
      const SampleCacheState s1 = sample_cache->state();
      sample_cache->register_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCacheState s2 = sample_cache->state();
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

  void write(const Sample& sample, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCachePtr sample_cache = insert_instance_i(sample);
      const SampleCacheState s1 = sample_cache->state();
      sample_cache->write(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCacheState s2 = sample_cache->state();
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

  void unregister_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCachePtr sample_cache = lookup_instance_i(sample);
      if (!sample_cache) {
        return;
      }
      const SampleCacheState s1 = sample_cache->state();
      sample_cache->unregister_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCacheState s2 = sample_cache->state();
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

  void dispose_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp, const PublicationHandle publication_handle)
  {
    ObserverPtr observer;

    {
      ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
      SampleCachePtr sample_cache = lookup_instance_i(sample);
      if (!sample_cache) {
        return;
      }
      const SampleCacheState s1 = sample_cache->state();
      sample_cache->dispose_instance(sample, source_timestamp, publication_handle);
      sample_cache->resize(depth_);
      const SampleCacheState s2 = sample_cache->state();
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
            SampleInfoList& sample_info_list,
            size_t max_samples,
            int sample_state_mask,
            int view_state_mask,
            int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.clear();

    SampleCachePtrSet reclassify;

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (typename StateCache::iterator pos = state_to_caches_.begin(), limit = state_to_caches_.end(); sample_list.size() != max_samples && pos != limit; ++pos) {
      if ((pos->first.sample_state & sample_state_mask) &&
          (pos->first.view_state & view_state_mask) &&
          (pos->first.instance_state & instance_state_mask)) {
        const SampleCacheState& state = pos->first;
        SampleCachePtrSet& s = pos->second;
        for (typename SampleCachePtrSet::const_iterator pos = s.begin(), limit = s.end(); sample_list.size() != max_samples && pos != limit;) {
          SampleCachePtr scp = *pos;
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

    for (typename SampleCachePtrSet::const_iterator pos = reclassify.begin(), limit = reclassify.end(); pos != limit; ++pos) {
      state_to_caches_[(*pos)->state()].insert(*pos);
    }
  }

  void take(SampleList& sample_list,
            SampleInfoList& sample_info_list,
            size_t max_samples,
            int sample_state_mask,
            int view_state_mask,
            int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.clear();

    SampleCachePtrSet reclassify;

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    for (typename StateCache::iterator pos = state_to_caches_.begin(), limit = state_to_caches_.end(); sample_list.size() != max_samples && pos != limit; ++pos) {
      if ((pos->first.sample_state & sample_state_mask) &&
          (pos->first.view_state & view_state_mask) &&
          (pos->first.instance_state & instance_state_mask)) {
        const SampleCacheState& state = pos->first;
        SampleCachePtrSet& s = pos->second;
        for (typename SampleCachePtrSet::const_iterator pos = s.begin(), limit = s.end(); sample_list.size() != max_samples && pos != limit;) {
          SampleCachePtr scp = *pos;
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

    for (typename SampleCachePtrSet::const_iterator pos = reclassify.begin(), limit = reclassify.end(); pos != limit; ++pos) {
      state_to_caches_[(*pos)->state()].insert(*pos);
    }
  }

  // Not implemented.
  // void read_w_condition(SampleList& sample_list,
  //                       SampleInfoList& sample_info_list,
  //                       size_t max_samples,
  //                       Condition condition);

  // Not implemented.
  // void take_w_condition(SampleList& sample_list,
  //                       SampleInfoList& sample_info_list,
  //                       size_t max_samples,
  //                       Condition condition);

  bool read_next_sample(Sample& sample,
                        SampleInfo& sample_info)
  {
    SampleList sample_list;
    SampleInfoList sample_info_list;
    read(sample_list, sample_info_list, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    if (!sample_list.empty()) {
      sample = sample_list[0];
      sample_info = sample_info_list[0];
      return true;
    }
    return false;
  }

  bool take_next_sample(Sample& sample,
                        SampleInfo& sample_info)
  {
    SampleList sample_list;
    SampleInfoList sample_info_list;
    take(sample_list, sample_info_list, 1, NOT_READ_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE);
    if (!sample_list.empty()) {
      sample = sample_list[0];
      sample_info = sample_info_list[0];
      return true;
    }
    return false;
  }

  void read_instance(SampleList& sample_list,
                     SampleInfoList& sample_info_list,
                     size_t max_samples,
                     InstanceHandle a_handle,
                     int sample_state_mask,
                     int view_state_mask,
                     int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceCache::const_iterator pos = instance_to_cache_.find(a_handle);
    if (pos == instance_to_cache_.end()) {
      return;
    }

    read_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
  }

  void take_instance(SampleList& sample_list,
                     SampleInfoList& sample_info_list,
                     size_t max_samples,
                     InstanceHandle a_handle,
                     int sample_state_mask,
                     int view_state_mask,
                     int instance_state_mask)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    sample_list.clear();
    sample_info_list.clear();

    SampleCachePtr scp = rchandle_from(const_cast<SampleCache*>(static_cast<const SampleCache*>(a_handle)));
    const SampleCacheState state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->take(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCacheState state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

  bool read_next_instance(SampleList& sample_list,
                          SampleInfoList& sample_info_list,
                          size_t max_samples,
                          InstanceHandle previous_handle,
                          int sample_state_mask,
                          int view_state_mask,
                          int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.clear();

    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    typename InstanceCache::const_iterator pos = instance_to_cache_.upper_bound(previous_handle);
    if (pos == instance_to_cache_.end()) {
      return false;
    }

    read_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
    return !sample_list.empty();
  }

  void take_next_instance(SampleList& sample_list,
                          SampleInfoList& sample_info_list,
                          size_t max_samples,
                          InstanceHandle previous_handle,
                          int sample_state_mask,
                          int view_state_mask,
                          int instance_state_mask)
  {
    sample_list.clear();
    sample_info_list.clear();

    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    typename InstanceCache::const_iterator pos = instance_to_cache_.upper_bound(previous_handle);
    if (pos == instance_to_cache_.end()) {
      return;
    }

    take_instance_i(sample_list, sample_info_list, max_samples, pos->second, sample_state_mask, view_state_mask, instance_state_mask);
  }

  // Not implemented.
  // void read_next_instance_w_condition(SampleList& sample_list,
  //                                     SampleInfoList& sample_info_list,
  //                                     size_t max_samples,
  //                                     InstanceHandle previous_handle,
  //                                     Condition condition);

  // Not implemented.
  // void take_next_instance_w_condition(SampleList& sample_list,
  //                                     SampleInfoList& sample_info_list,
  //                                     size_t max_samples,
  //                                     InstanceHandle previous_handle,
  //                                     Condition condition);

  bool get_key_value(Sample& key_holder,
                     InstanceHandle handle)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

    typename InstanceCache::const_iterator pos = instance_to_cache_.find(handle);
    if (pos != instance_to_cache_.end()) {
      return pos->second->get_key_value(key_holder);
    }

    return false;
  }

  InstanceHandle lookup_instance(const Sample& sample)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos != key_to_cache_.end()) {
      return pos->second->get_instance_handle();
    }
    return 0;
  }

private:
  typedef SampleCache<Sample> SampleCache;
  typedef RcHandle<SampleCache> SampleCachePtr;
  typedef std::map<Sample, SampleCachePtr> KeyCache;
  typedef std::map<InstanceHandle, SampleCachePtr> InstanceCache;
  typedef std::set<SampleCachePtr> SampleCachePtrSet;
  typedef std::map<SampleCacheState, SampleCachePtrSet> StateCache;

  mutable ACE_Thread_Mutex mutex_;
  KeyCache key_to_cache_; // For per-instance operations.
  InstanceCache instance_to_cache_; // For per-instance operations.
  StateCache state_to_caches_;
  size_t depth_;
  WeakObserverPtr observer_;

  SampleCachePtr insert_instance_i(const Sample& sample)
  {
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos == key_to_cache_.end()) {
      pos = key_to_cache_.insert(std::make_pair(sample, make_rch<SampleCache>())).first;
      instance_to_cache_[pos->second->get_instance_handle()] = pos->second;
      state_to_caches_[pos->second->state()].insert(pos->second);
    }
    return pos->second;
  }

  SampleCachePtr lookup_instance_i(const Sample& sample)
  {
    typename KeyCache::iterator pos = key_to_cache_.find(sample);
    if (pos != key_to_cache_.end()) {
      return pos->second;
    }
    return SampleCachePtr();
  }

  void read_instance_i(SampleList& sample_list,
                       SampleInfoList& sample_info_list,
                       size_t max_samples,
                       SampleCachePtr scp,
                       int sample_state_mask,
                       int view_state_mask,
                       int instance_state_mask)
  {
    const SampleCacheState state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->read(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCacheState state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

  void take_instance_i(SampleList& sample_list,
                       SampleInfoList& sample_info_list,
                       size_t max_samples,
                       SampleCachePtr scp,
                       int sample_state_mask,
                       int view_state_mask,
                       int instance_state_mask)
  {
    const SampleCacheState state_before = scp->state();
    if ((state_before.sample_state & sample_state_mask) &&
        (state_before.view_state & view_state_mask) &&
        (state_before.instance_state & instance_state_mask)) {
      scp->take(sample_list, sample_info_list, max_samples, sample_state_mask);
      const SampleCacheState state_after = scp->state();
      if (state_after != state_before) {
        // Reclassify.
        state_to_caches_[state_before].erase(scp);
        state_to_caches_[state_after].insert(scp);
      }
    }
  }

};

template<typename Sample>
class Source : public RcObject {
public:
  typedef RcHandle<Source> SourcePtr;
  typedef Sink<Sample> Sink;
  typedef RcHandle<Sink> SinkPtr;

  Source(size_t depth = 1)
  {
    durability_sink_ = make_rch<SinkType>(depth);
    sinks_.insert(durability_sink_);
  }

  PublicationHandle get_publication_handle() const { return this; }

  void register_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename Sinks::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      SinkPtr sink = *pos;
      sink->register_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void write(const Sample& sample, const MonotonicTimePoint& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename Sinks::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      SinkPtr sink = *pos;
      sink->write(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void unregister_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename Sinks::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      SinkPtr sink = *pos;
      sink->unregister_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void dispose_instance(const Sample& sample, const MonotonicTimePoint& source_timestamp)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    for (typename Sinks::const_iterator pos = sinks_.begin(), limit = sinks_.end(); pos != limit; ++pos) {
      SinkPtr sink = *pos;
      sink->dispose_instance(sample, source_timestamp, get_publication_handle());
    }
    process_updates();
  }

  void connect(SinkPtr sink)
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

  void disconnect(SinkPtr sink)
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
  typedef std::set<SinkPtr> Sinks;
  ACE_Thread_Mutex update_mutex_; // Second in locking order.
  Sinks to_insert_;
  Sinks to_erase_;
  ACE_Thread_Mutex mutex_; // First in locking order.
  Sinks sinks_;
  SinkPtr durability_sink_;

  void process_updates()
  {
    ACE_GUARD(ACE_Thread_Mutex, g, update_mutex_);
    for (typename Sinks::const_iterator pos = to_erase_.begin(), limit = to_erase_.end(); pos != limit; ++pos) {
      SinkPtr sink = *pos;

      SampleList sample_list;
      SampleInfoList sample_info_list;
      InstanceHandle instance_handle = 0;
      while (durability_sink_->read_next_instance(sample_list, sample_info_list, 1, instance_handle, ANY_SAMPLE_STATE, ANY_VIEW_STATE, ANY_INSTANCE_STATE)) {
        sink->unregister_instance(sample_list[0], source_timestamp, get_publication_handle());
        instance_handle = sample_info_list[0].instance_handle;
      }

      sinks_.erase(sink);
    }
    sinks_.insert(to_insert_.begin(), to_insert_.end());
    for (typename Sinks::const_iterator pos = to_insert_.begin(), limit = to_insert_.end(); pos != limit; ++pos) {
      (*pos)->initialize(*durability_sink_);
    }
    to_erase_.clear();
    to_insert_.clear();
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_SOURCE_H */
