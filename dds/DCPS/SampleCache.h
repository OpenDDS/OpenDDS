/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SAMPLE_CACHE_H
#define OPENDDS_DCPS_SAMPLE_CACHE_H

#include <dds/DdsDcpsCoreC.h>

#include "RcObject.h"
#include "Util.h"

#include <list>
#include <set>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  const SampleStateKind NO_SAMPLE_STATE = 0;

  bool operator==(const DDS::Time_t& x, const DDS::Time_t& y)
  {
    return x.sec == y.sec &&
      x.nanosec == y.nanosec;
  }

  bool operator==(const DDS::SampleInfo& x, const DDS::SampleInfo& y)
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
}

namespace OpenDDS {
namespace DCPS {

inline
DDS::SampleInfo make_sample_info(DDS::SampleStateKind sample_state,
                                 DDS::ViewStateKind view_state,
                                 DDS::InstanceStateKind instance_state,
                                 const DDS::Time_t& source_timestamp,
                                 DDS::InstanceHandle_t instance_handle,
                                 DDS::InstanceHandle_t publication_handle,
                                 ACE_CDR::Long disposed_generation_count,
                                 ACE_CDR::Long no_writers_generation_count,
                                 ACE_CDR::Long sample_rank,
                                 ACE_CDR::Long generation_rank,
                                 ACE_CDR::Long absolute_generation_rank,
                                 ACE_CDR::Boolean valid_data)
{
  DDS::SampleInfo si;
  si.sample_state = sample_state;
  si.view_state = view_state;
  si.instance_state = instance_state;
  si.source_timestamp = source_timestamp;
  si.instance_handle = instance_handle;
  si.publication_handle = publication_handle;
  si.disposed_generation_count = disposed_generation_count;
  si.no_writers_generation_count = no_writers_generation_count;
  si.sample_rank = sample_rank;
  si.generation_rank = generation_rank;
  si.absolute_generation_rank = absolute_generation_rank;
  si.valid_data = valid_data;
  return si;
}

struct SampleCacheState {
  DDS::SampleStateKind sample_state;
  DDS::ViewStateKind view_state;
  DDS::InstanceStateKind instance_state;

  SampleCacheState(DDS::SampleStateKind a_sample_state,
                   DDS::ViewStateKind a_view_state,
                   DDS::InstanceStateKind a_instance_state)
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

  SampleCache(DDS::InstanceHandle_t instance_handle)
    : instance_handle_(instance_handle)
    , view_state_(DDS::NEW_VIEW_STATE)
    , instance_state_(DDS::ALIVE_INSTANCE_STATE)
    , instance_element_state_(NOT_READ)
    , disposed_generation_count_(0)
    , no_writers_generation_count_(0)
  { }

  void initialize(const SampleCache& other)
  {
    // All of the samples are new.
    new_samples_.insert(new_samples_.end(), other.not_new_samples_.begin(), other.not_new_samples_.end());
    new_samples_.insert(new_samples_.end(), other.new_samples_.begin(), other.new_samples_.end());
    view_state_ = DDS::NEW_VIEW_STATE;
    instance_state_ = other.instance_state_;
    instance_element_state_ = NOT_READ;
    instance_element_ = other.instance_element_;
    disposed_generation_count_ = 0;
    no_writers_generation_count_ = 0;
    publication_handles_ = other.publication_handles_;
  }

  void register_instance(const Sample& key, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    std::pair<PublicationHandleSet::iterator, bool> p = publication_handles_.insert(publication_handle);

    switch (instance_state_) {
    case DDS::ALIVE_INSTANCE_STATE:
      if (p.second) {
        instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
      instance_element_ = Element(key, ++disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      break;
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      instance_element_ = Element(key, disposed_generation_count_, ++no_writers_generation_count_, source_timestamp, publication_handle);
      break;
    }

    if (view_state_ == DDS::NOT_NEW_VIEW_STATE) {
      if (instance_state_ != DDS::ALIVE_INSTANCE_STATE) {
        view_state_ = DDS::NEW_VIEW_STATE;
      }
    }

    if (instance_state_ != DDS::ALIVE_INSTANCE_STATE) {
      instance_element_state_ = NOT_READ;
      instance_state_ = DDS::ALIVE_INSTANCE_STATE;
    }
  }

  void write(const Sample& sample, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    register_instance(sample, source_timestamp, publication_handle);
    new_samples_.push_back(Element(sample, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle));
  }

  void unregister_instance(const Sample& key, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    publication_handles_.erase(publication_handle);

    if (instance_state_ == DDS::ALIVE_INSTANCE_STATE && publication_handles_.empty()) {
      instance_state_ = DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE;
      instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      instance_element_state_ = NOT_READ;
      // TODO: Do we need to change the view state?
    }
  }

  void dispose_instance(const Sample& key, const DDS::Time_t& source_timestamp, const DDS::InstanceHandle_t publication_handle)
  {
    if (instance_state_ == DDS::ALIVE_INSTANCE_STATE) {
      instance_state_ = DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE;
      instance_element_ = Element(key, disposed_generation_count_, no_writers_generation_count_, source_timestamp, publication_handle);
      instance_element_state_ = NOT_READ;
      // TODO: Do we need to change the view state?
    }
  }

  void read(SampleList& sample_list, DDS::SampleInfoSeq& sample_info_list, size_t max_samples, int sample_state_mask)
  {
    typename ListType::const_iterator new_samples_begin = new_samples_.begin();
    typename ListType::const_iterator new_samples_pos = new_samples_.begin();
    typename ListType::const_iterator new_samples_end = new_samples_.end();

    const size_t size_before = sample_list.size();
    // mrsic = most recent sample in returned collection
    size_t not_read_mrsic_idx = -1;
    size_t read_mrsic_idx = -1;

    // Prefer not read samples.
    if (sample_state_mask & DDS::NOT_READ_SAMPLE_STATE) {
      while (sample_list.size() != max_samples && new_samples_pos != new_samples_end) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(new_samples_pos->sample);
        push_back(sample_info_list, make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, new_samples_pos->source_timestamp, get_instance_handle(), new_samples_pos->publication_handle, new_samples_pos->disposed_generation_count, new_samples_pos->no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (new_samples_pos->disposed_generation_count + new_samples_pos->no_writers_generation_count), true));
        ++new_samples_pos;
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == NOT_READ) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        push_back(sample_info_list, make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), false));
        instance_element_state_ = READ;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = READ;
      }
    }

    if (sample_state_mask & DDS::READ_SAMPLE_STATE) {
      for (typename ListType::const_iterator pos = not_new_samples_.begin(), limit = not_new_samples_.end();
           sample_list.size() != max_samples && pos != limit; ++pos) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(pos->sample);
        push_back(sample_info_list, make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, pos->source_timestamp, get_instance_handle(), pos->publication_handle, pos->disposed_generation_count, pos->no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (pos->disposed_generation_count + pos->no_writers_generation_count), true));
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == READ) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        push_back(sample_info_list, make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), false));
      }
    }

    // Transfer not read to read.
    not_new_samples_.splice(not_new_samples_.end(), new_samples_, new_samples_begin, new_samples_pos);

    view_state_ = DDS::NOT_NEW_VIEW_STATE;

    size_t sample_rank = sample_list.size() - size_before;
    if (sample_rank != 0) {
      const DDS::SampleInfo& mrsic = not_read_mrsic_idx != size_t(-1) ? sample_info_list[not_read_mrsic_idx] : sample_info_list[read_mrsic_idx];
      for (size_t idx = size_before; idx != sample_list.size(); ++idx) {
        sample_info_list[idx].sample_rank = --sample_rank;
        sample_info_list[idx].generation_rank =
          (mrsic.disposed_generation_count + mrsic.no_writers_generation_count) -
          (sample_info_list[idx].disposed_generation_count + mrsic.no_writers_generation_count);
      }
    }
  }

  void take(SampleList& sample_list, DDS::SampleInfoSeq& sample_info_list, size_t max_samples, int sample_state_mask)
  {
    const size_t size_before = sample_list.size();
    size_t not_read_mrsic_idx = -1;
    size_t read_mrsic_idx = -1;

    // Prefer read samples.
    if (sample_state_mask & DDS::READ_SAMPLE_STATE) {
      while (!not_new_samples_.empty() && sample_list.size() != max_samples) {
        read_mrsic_idx = sample_list.size();
        const Element& front = not_new_samples_.front();
        sample_list.push_back(front.sample);
        push_back(sample_info_list, make_sample_info(DDS::READ_SAMPLE_STATE, view_state_, instance_state_, front.source_timestamp, get_instance_handle(), front.publication_handle, front.disposed_generation_count, front.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (front.disposed_generation_count + front.no_writers_generation_count), true));
        not_new_samples_.pop_front();
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == READ) {
        read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        push_back(sample_info_list, make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), false));
        instance_element_state_ = TAKEN;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = TAKEN;
      }
    }

    if (sample_state_mask & DDS::NOT_READ_SAMPLE_STATE) {
      while (!new_samples_.empty() && sample_list.size() != max_samples) {
        not_read_mrsic_idx = sample_list.size();
        const Element& front = new_samples_.front();
        sample_list.push_back(front.sample);
        push_back(sample_info_list, make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, front.source_timestamp, get_instance_handle(), front.publication_handle, front.disposed_generation_count, front.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (front.disposed_generation_count + front.no_writers_generation_count), true));
        new_samples_.pop_front();
      }

      if (sample_list.size() == size_before && sample_list.size() != max_samples && instance_element_state_ == NOT_READ) {
        not_read_mrsic_idx = sample_list.size();
        sample_list.push_back(instance_element_.sample);
        push_back(sample_info_list, make_sample_info(DDS::NOT_READ_SAMPLE_STATE, view_state_, instance_state_, instance_element_.source_timestamp, get_instance_handle(), instance_element_.publication_handle, instance_element_.disposed_generation_count, instance_element_.no_writers_generation_count, 0, 0, (disposed_generation_count_ + no_writers_generation_count_) - (instance_element_.disposed_generation_count + instance_element_.no_writers_generation_count), false));
        instance_element_state_ = TAKEN;
      } else if (sample_list.size() != size_before) {
        instance_element_state_ = TAKEN;
      }
    }

    view_state_ = DDS::NOT_NEW_VIEW_STATE;

    size_t sample_rank = sample_list.size() - size_before;
    if (sample_rank != 0) {
      const DDS::SampleInfo& mrsic = not_read_mrsic_idx != size_t(-1) ? sample_info_list[not_read_mrsic_idx] : sample_info_list[read_mrsic_idx];
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

  DDS::InstanceHandle_t get_instance_handle() const { return instance_handle_; }
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
      ((instance_element_state_ == NOT_READ || !new_samples_.empty()) ? DDS::NOT_READ_SAMPLE_STATE : 0) |
      ((instance_element_state_ == READ || !not_new_samples_.empty()) ? DDS::READ_SAMPLE_STATE : 0);
    return SampleCacheState(sample_state, view_state_, instance_state_);
  }

private:
  const DDS::InstanceHandle_t instance_handle_;
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
    DDS::Time_t source_timestamp;
    DDS::InstanceHandle_t publication_handle;

    Element()
      : disposed_generation_count(0)
      , no_writers_generation_count(0)
      , publication_handle(0)
    {}

    Element(const Sample& a_sample,
            size_t a_disposed_generation_count,
            size_t a_no_writers_generation_count,
            const DDS::Time_t& a_source_timestamp,
            DDS::InstanceHandle_t a_publication_handle)
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
  DDS::ViewStateKind view_state_;
  DDS::InstanceStateKind instance_state_;
  enum {
    NOT_READ,
    READ,
    TAKEN
  } instance_element_state_;
  Element instance_element_;
  size_t disposed_generation_count_;
  size_t no_writers_generation_count_;
  typedef std::set<DDS::InstanceHandle_t> PublicationHandleSet;
  PublicationHandleSet publication_handles_;

  static bool valid_data_pred(const Element& element)
  {
    return element.valid_data;
  }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_SAMPLE_CACHE_H */
