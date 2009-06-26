#ifndef RAKERESULTS_T_CPP
#define RAKERESULTS_T_CPP

#include "dds/DCPS/RakeResults_T.h"
#include "dds/DCPS/SubscriptionInstance.h"
#include "dds/DCPS/DataReaderImpl.h"

namespace OpenDDS
{
    namespace DCPS
    {

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

const char ORDER_BY[] = "ORDER BY ";
const size_t ORDER_BY_SIZE = sizeof(ORDER_BY) - 1 /*no null*/;
const char WHITESPACE[] = " \t\f\v\n\r"; //from std::isspace() in <cctype>

inline void trim(std::string& str)
{
  size_t start = str.find_first_not_of(WHITESPACE);
  size_t end = str.find_last_not_of(WHITESPACE);
  std::string sub = str.substr(start, end - start + 1);
  str.swap(sub);
}

#endif

template <class SampleSeq>
RakeResults<SampleSeq>::RakeResults(SampleSeq& received_data,
                                    ::DDS::SampleInfoSeq& info_seq,
                                    ::CORBA::Long max_samples,
                                    ::DDS::QueryCondition_ptr cond,
                                    Operation_t oper)
  : received_data_(received_data)
  , info_seq_(info_seq)
  , max_samples_(max_samples)
  , cond_(cond)
  , oper_(oper)
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  , do_sort_(false)
{
  if (cond_)
    {
      CORBA::String_var query_var = cond_->get_query_expression();
      std::string query(query_var);
      //FUTURE: Parse the "WHERE" clause, including get_query_parameters()
      size_t idx_orderby = query.find(ORDER_BY);
      do_sort_ = (idx_orderby != std::string::npos);
      if (do_sort_ && query.size() > idx_orderby + ORDER_BY_SIZE)
        {
          typename SampleSeq::value_type* sample = 0;
          std::string order_by_str(query, idx_orderby + ORDER_BY_SIZE);
          ComparatorBase::Ptr cmp = 0;
          // Iterate in reverse over the comma-separated fields so that the
          // top-level comparison is the leftmost.  The others will be chained.
          for (size_t idx = std::string::npos, end = order_by_str.size(); idx;)
            {
              idx = order_by_str.rfind(',', end - 1) + 1; //npos -> 0
              std::string fieldspec(order_by_str, idx, end - idx);
              end = idx - 1;
              trim(fieldspec);
              //FUTURE: handle ASC / DESC as an extension to the DDS spec?
              cmp = create_qc_comparator(sample, fieldspec.c_str(), cmp);
            }
          QueryConditionCmp comparator(cmp);
          SortedSet actual_sort(comparator);
          sorted_.swap(actual_sort);
        }
    }
}
#else // excluding the content subscription profile
{}
#endif


template <class SampleSeq>
bool RakeResults<SampleSeq>::insert_sample(ReceivedDataElement* sample,
                                           SubscriptionInstance* instance,
                                           size_t index_in_instance)
{
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  if (!where_filter(static_cast<typename SampleSeq::value_type*>
                    (sample->registered_data_))) return false;
#endif

  RakeData rd = {sample, instance, index_in_instance};

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  if (do_sort_)
    {
      sorted_.insert(rd);
    }
  else
    {
#endif
      if (unsorted_.size() == max_samples_) return false;
      unsorted_.push_back(rd);
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    }
#endif

  return true;
}


template <class SampleSeq>
template <class FwdIter>
bool RakeResults<SampleSeq>::copy_into(FwdIter iter, FwdIter end,
  typename SampleSeq::PrivateMemberAccess& received_data_p)
{
  typedef typename SampleSeq::value_type Sample;
  typedef std::map<SubscriptionInstance*, InstanceData> InstanceMap;
  InstanceMap inst_map;

  for (CORBA::ULong idx = 0; iter != end && idx < max_samples_; ++idx, ++iter)
    {
      // 1. Populate the Received Data sequence
      ReceivedDataElement* rde = iter->rde_;
      if (received_data_.maximum() != 0)
        {
          if (rde->registered_data_ == 0)
            {
              received_data_p.assign_sample(idx, Sample());
            }
          else
            {
              received_data_p.assign_sample(idx,
                *static_cast<Sample*> (rde->registered_data_));
            }
        }
      else
        {
          received_data_p.assign_ptr(idx, rde);
        }

      // 2. Per-sample SampleInfo (not the three *_rank variables) and state
      SubscriptionInstance& inst = *iter->si_;
      inst.instance_state_.sample_info(info_seq_[idx], rde);
      rde->sample_state_ = ::DDS::READ_SAMPLE_STATE;

      // 3. Record some info about per-instance SampleInfo (*_rank) so that
      //    we can fill in the ranks after the loop has completed
      std::pair<typename InstanceMap::iterator, bool> result =
        inst_map.insert(std::make_pair(&inst, InstanceData()));
      InstanceData& id = result.first->second;
      if (result.second) // first time we've seen this Instance
        {
          ReceivedDataElement& mrs = *inst.rcvd_sample_.tail_;
          id.MRS_disposed_gc_ = mrs.disposed_generation_count_;
          id.MRS_nowriters_gc_ = mrs.no_writers_generation_count_;
        }
      if (iter->index_in_instance_ >= id.MRSIC_index_)
        {
          id.MRSIC_index_ = iter->index_in_instance_;
          id.MRSIC_disposed_gc_ = rde->disposed_generation_count_;
          id.MRSIC_nowriters_gc_ = rde->no_writers_generation_count_;
        }
      if (!id.most_recent_generation_)
        {
          id.most_recent_generation_ =
            inst.instance_state_.most_recent_generation(rde);
        }
      id.sampleinfo_positions_.push_back(idx);

      // 4. Take
      if (oper_ == DDS_OPERATION_TAKE)
        {
          inst.rcvd_sample_.remove(rde);
          inst.instance_state_.data_reader()->dec_ref_data_element(rde);
        }
    }

  // Fill in the *_ranks in the SampleInfo, and set instance state (mrg)
  for (typename InstanceMap::iterator i_iter(inst_map.begin()),
         i_end(inst_map.end()); i_iter != i_end; ++i_iter)
    {
      SubscriptionInstance& inst = *i_iter->first;
      InstanceData& id = i_iter->second;
      if (id.most_recent_generation_) inst.instance_state_.accessed();
      CORBA::Long sample_rank = id.sampleinfo_positions_.size();
      for (IndexList::iterator s_iter(id.sampleinfo_positions_.begin()),
             s_end(id.sampleinfo_positions_.end()); s_iter != s_end; ++s_iter)
        {
          ::DDS::SampleInfo& si = info_seq_[*s_iter];
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
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  if (do_sort_)
    {
      size_t len = std::min(sorted_.size(), static_cast<size_t>(max_samples_));
      received_data_p.internal_set_length(len);
      info_seq_.length(len);
      return copy_into(sorted_.begin(), sorted_.end(), received_data_p);
    }
  else
    {
#endif
      size_t len = unsorted_.size(); //can't be larger than max_samples_
      received_data_p.internal_set_length(len);
      info_seq_.length(len);
      return copy_into(unsorted_.begin(), unsorted_.end(), received_data_p);
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
    }
#endif
}

    } // namespace DCPS
} // namespace OpenDDS


#endif /* RAKERESULTS_H  */



