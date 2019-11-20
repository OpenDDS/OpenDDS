/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RAKERESULTS_T_CPP
#define RAKERESULTS_T_CPP

#include "dds/DCPS/RakeResults_T.h"
#include "dds/DCPS/SubscriptionInstance.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/QueryConditionImpl.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <class SampleSeq>
RakeResults<SampleSeq>::RakeResults(DataReaderImpl* reader,
                                    SampleSeq& received_data,
                                    DDS::SampleInfoSeq& info_seq,
                                    CORBA::Long max_samples,
                                    DDS::PresentationQosPolicy presentation,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                    DDS::QueryCondition_ptr cond,
#endif
                                    Operation_t oper)
  : reader_(reader)
  , received_data_(received_data)
  , info_seq_(info_seq)
  , max_samples_(max_samples)
#ifndef OPENDDS_NO_QUERY_CONDITION
  , cond_(cond)
#endif
  , oper_(oper)
  , do_sort_(false)
  , do_filter_(false)
{
#ifndef OPENDDS_NO_QUERY_CONDITION

  if (cond_) {
    const QueryConditionImpl* qci = dynamic_cast<QueryConditionImpl*>(cond_);
    if (!qci) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RakeResults(): ")
        ACE_TEXT("failed to obtain QueryConditionImpl\n")));
      return;
    }
    do_filter_ = qci->hasFilter();
    std::vector<OPENDDS_STRING> order_bys = qci->getOrderBys();
    do_sort_ = order_bys.size() > 0;

    if (do_sort_) {
      ComparatorBase::Ptr cmp;

      // Iterate in reverse over the comma-separated fields so that the
      // top-level comparison is the leftmost.  The others will be chained.
      for (size_t i = order_bys.size(); i > 0; --i) {
        const OPENDDS_STRING& fieldspec = order_bys[i - 1];
        //FUTURE: handle ASC / DESC as an extension to the DDS spec?
        cmp = getMetaStruct<typename SampleSeq::value_type>()
          .create_qc_comparator(fieldspec.c_str(), cmp);
      }

      SortedSetCmp comparator(cmp);
      SortedSet actual_sort(comparator);
      sorted_.swap(actual_sort);
    }

  } else {
#endif
    // PRESENTATION ordered access (TOPIC)
    this->do_sort_ = presentation.ordered_access == true &&
                     presentation.access_scope == DDS::TOPIC_PRESENTATION_QOS;
#ifndef OPENDDS_NO_QUERY_CONDITION
  }

#endif
}

template <class SampleSeq>
bool RakeResults<SampleSeq>::insert_sample(ReceivedDataElement* sample,
                                           SubscriptionInstance_rch instance,
                                           size_t index_in_instance)
{
#ifndef OPENDDS_NO_QUERY_CONDITION

  if (do_filter_) {
    const QueryConditionImpl* qci = dynamic_cast<QueryConditionImpl*>(cond_);
    typedef typename SampleSeq::value_type VT;
    const VT* typed_sample = static_cast<VT*>(sample->registered_data_);
    if (!qci || !typed_sample || !qci->filter(*typed_sample, !sample->valid_data_)) {
      return false;
    }
  }

#endif

  if (do_sort_) {
    // N.B. Until a better heuristic is found, non-valid
    // samples are elided when sorting by QueryCondition.
#ifndef OPENDDS_NO_QUERY_CONDITION
    if (cond_ && !sample->registered_data_) return false;
#endif

    RakeData rd = {sample, instance, index_in_instance};
    sorted_.insert(rd);

  } else {
    if (unsorted_.size() == max_samples_) return false;

    RakeData rd = {sample, instance, index_in_instance};
    unsorted_.push_back(rd);
  }

  return true;
}

template <class SampleSeq>
template <class FwdIter>
bool RakeResults<SampleSeq>::copy_into(FwdIter iter, FwdIter end,
                                       typename SampleSeq::PrivateMemberAccess& received_data_p)
{
  typedef typename SampleSeq::value_type Sample;
  typedef OPENDDS_MAP(SubscriptionInstance*, InstanceData) InstanceMap;
  InstanceMap inst_map;

  typedef OPENDDS_SET(SubscriptionInstance*) InstanceSet;
  InstanceSet released_instances;

  for (CORBA::ULong idx = 0; iter != end && idx < max_samples_; ++idx, ++iter) {
    // 1. Populate the Received Data sequence
    ReceivedDataElement* rde = iter->rde_;

    if (received_data_.maximum() != 0) {
      if (rde->registered_data_ == 0) {
        received_data_p.assign_sample(idx, Sample());

      } else {
        received_data_p.assign_sample(idx,
                                      *static_cast<Sample*>(rde->registered_data_));
      }

    } else {
      received_data_p.assign_ptr(idx, rde);
    }

    // 2. Per-sample SampleInfo (not the three *_rank variables) and state
    SubscriptionInstance& inst = *iter->si_;
    inst.instance_state_->sample_info(info_seq_[idx], rde);
    rde->sample_state_ = DDS::READ_SAMPLE_STATE;

    // 3. Record some info about per-instance SampleInfo (*_rank) so that
    //    we can fill in the ranks after the loop has completed
    std::pair<typename InstanceMap::iterator, bool> result =
      inst_map.insert(std::make_pair(&inst, InstanceData()));
    InstanceData& id = result.first->second;

    if (result.second) { // first time we've seen this Instance
      ReceivedDataElement& mrs = *inst.rcvd_samples_.tail_;
      id.MRS_disposed_gc_ =
        static_cast<CORBA::Long>(mrs.disposed_generation_count_);
      id.MRS_nowriters_gc_ =
        static_cast<CORBA::Long>(mrs.no_writers_generation_count_);
    }

    if (iter->index_in_instance_ >= id.MRSIC_index_) {
      id.MRSIC_index_ = iter->index_in_instance_;
      id.MRSIC_disposed_gc_ =
        static_cast<CORBA::Long>(rde->disposed_generation_count_);
      id.MRSIC_nowriters_gc_ =
        static_cast<CORBA::Long>(rde->no_writers_generation_count_);
    }

    if (!id.most_recent_generation_) {
      id.most_recent_generation_ =
        inst.instance_state_->most_recent_generation(rde);
    }

    id.sampleinfo_positions_.push_back(idx);

    // 4. Take
    if (oper_ == DDS_OPERATION_TAKE) {
      // If removing the sample releases it
      if (inst.rcvd_samples_.remove(rde)) {
        // Prevent access of the SampleInfo, below
        released_instances.insert(&inst);
      }
      rde->dec_ref();
    }
  }

  // Fill in the *_ranks in the SampleInfo, and set instance state (mrg)
  for (typename InstanceMap::iterator i_iter(inst_map.begin()),
       i_end(inst_map.end()); i_iter != i_end; ++i_iter) {

    InstanceData& id = i_iter->second;
    {  // Danger, limit visibility of inst
      SubscriptionInstance& inst = *i_iter->first;
      // If this instance has not been released
      if (released_instances.find(&inst) == released_instances.end()) {
        if (id.most_recent_generation_) {
          inst.instance_state_->accessed();
        }
      }
    }

    CORBA::Long sample_rank =
      static_cast<CORBA::Long>(id.sampleinfo_positions_.size());

    for (IndexList::iterator s_iter(id.sampleinfo_positions_.begin()),
         s_end(id.sampleinfo_positions_.end()); s_iter != s_end; ++s_iter) {
      DDS::SampleInfo& si = info_seq_[*s_iter];
      si.sample_rank = --sample_rank;
      si.generation_rank = id.MRSIC_disposed_gc_
                           + id.MRSIC_nowriters_gc_ - si.generation_rank;
      si.absolute_generation_rank = id.MRS_disposed_gc_ +
                                    id.MRS_nowriters_gc_ - si.absolute_generation_rank;
    }
  }

  return true;
}

template <class SampleSeq>
bool RakeResults<SampleSeq>::copy_to_user()
{
  typename SampleSeq::PrivateMemberAccess received_data_p(received_data_);

  if (do_sort_) {
    size_t len = std::min(static_cast<size_t>(sorted_.size()),
                          static_cast<size_t>(max_samples_));
    received_data_p.internal_set_length(static_cast<CORBA::ULong>(len));
    info_seq_.length(static_cast<CORBA::ULong>(len));
    return copy_into(sorted_.begin(), sorted_.end(), received_data_p);

  } else {
    size_t len = unsorted_.size(); //can't be larger than max_samples_
    received_data_p.internal_set_length(static_cast<CORBA::ULong>(len));
    info_seq_.length(static_cast<CORBA::ULong>(len));
    return copy_into(unsorted_.begin(), unsorted_.end(), received_data_p);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RAKERESULTS_H  */
