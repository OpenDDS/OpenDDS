/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADER_T_CPP
#define OPENDDS_DCPS_MULTITOPICDATAREADER_T_CPP

#ifndef OPENDDS_NO_MULTI_TOPIC

#include <stdexcept>
#include <sstream>


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::init_typed(DataReaderEx* dr)
{
  typed_reader_ = TypedDataReader::Interface::_narrow(dr);
}

template<typename Sample, typename TypedDataReader>
const MetaStruct&
MultiTopicDataReader_T<Sample, TypedDataReader>::getResultingMeta()
{
  return getMetaStruct<Sample>();
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::assign_fields(Sample& resulting,
  void* incoming, const MultiTopicDataReaderBase::QueryPlan& qp, const MetaStruct& meta)
{
  using namespace std;
  const vector<SubjectFieldSpec>& proj = qp.projection_;
  const MetaStruct& resulting_meta = getResultingMeta();

  typedef vector<SubjectFieldSpec>::const_iterator iter_t;
  for (iter_t iter = proj.begin(); iter != proj.end(); ++iter) {
    resulting_meta.assign(&resulting, iter->resulting_name_.c_str(),
                          incoming, iter->incoming_name_.c_str(), meta);
  }

  const vector<OPENDDS_STRING>& proj_out = qp.keys_projected_out_;
  for (vector<OPENDDS_STRING>::const_iterator iter = proj_out.begin();
       iter != proj_out.end(); ++iter) {
    resulting_meta.assign(&resulting, iter->c_str(),
                          incoming, iter->c_str(), meta);
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::assign_resulting_fields(
  Sample& target, const Sample& source, const TopicSet& other_topics)
{
  using namespace std;
  const MetaStruct& resulting_meta = getResultingMeta();

  for (TopicSet::const_iterator iterTopic = other_topics.begin();
       iterTopic != other_topics.end(); ++iterTopic) {
    const vector<SubjectFieldSpec>& proj = query_plans_[*iterTopic].projection_;
    typedef vector<SubjectFieldSpec>::const_iterator iter_t;
    for (iter_t iter = proj.begin(); iter != proj.end(); ++iter) {
      resulting_meta.assign(&target, iter->resulting_name_.c_str(),
                            &source, iter->resulting_name_.c_str(), resulting_meta);
    }
  }
}

template<typename Sample, typename TypedDataReader>
bool
MultiTopicDataReader_T<Sample, TypedDataReader>::join(
  SampleVec& resulting, const SampleWithInfo& prototype,
  const std::vector<OPENDDS_STRING>& key_names, const void* key_data,
  DDS::DataReader_ptr other_dr, const MetaStruct& other_meta)
{
  using namespace DDS;
  DataReaderImpl* other_dri = dynamic_cast<DataReaderImpl*>(other_dr);
  if (!other_dri) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReader_T::join: ")
      ACE_TEXT("Failed to get DataReaderImpl.\n")));
    return false;
  }

  TopicDescription_var other_td = other_dri->get_topicdescription();
  CORBA::String_var other_topic = other_td->get_name();
  const QueryPlan& other_qp = query_plans_[other_topic.in()];
  const size_t n_keys = key_names.size();

  if (n_keys > 0 && other_meta.numDcpsKeys() == n_keys) { // complete key
    InstanceHandle_t ih = other_dri->lookup_instance_generic(key_data);
    if (ih != HANDLE_NIL) {
      GenericData other_data(other_meta, false);
      SampleInfo info;
      const ReturnCode_t ret = other_dri->read_instance_generic(other_data.ptr_,
        info, ih, READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_NO_DATA) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MultiTopicDataReader_T::join: read_instance_generic"
                     " for topic %C returns %C\n", other_topic.in(), retcode_to_string(ret)));
        }
        return false;
      }
      if (ret == DDS::RETCODE_NO_DATA || !info.valid_data) {
        return false;
      }

      resulting.push_back(prototype);
      resulting.back().combine(SampleWithInfo(other_topic.in(), info));
      assign_fields(resulting.back().sample_, other_data.ptr_, other_qp, other_meta);
    }
  } else { // incomplete key or cross-join (0 key fields)
    ReturnCode_t ret = RETCODE_OK;
    for (InstanceHandle_t ih = HANDLE_NIL; ret != RETCODE_NO_DATA;) {
      GenericData other_data(other_meta, false);
      SampleInfo info;
      const ReturnCode_t ret = other_dri->read_next_instance_generic(other_data.ptr_,
        info, ih, READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MultiTopicDataReader_T::join:"
                     " read_next_instance_generic for topic %C returns %C\n",
                     other_topic.in(), retcode_to_string(ret)));
        }
        return false;
      }
      if (ret == RETCODE_NO_DATA || !info.valid_data) {
        break;
      }
      ih = info.instance_handle;

      bool match = true;
      for (size_t i = 0; match && i < key_names.size(); ++i) {
        if (!other_meta.compare(key_data, other_data.ptr_, key_names[i].c_str())) {
          match = false;
        }
      }

      if (match) {
        resulting.push_back(prototype);
        resulting.back().combine(SampleWithInfo(other_topic.in(), info));
        assign_fields(resulting.back().sample_, other_data.ptr_, other_qp, other_meta);
      }
    }
  }
  return true;
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::combine(
  SampleVec& resulting, const SampleVec& other,
  const std::vector<OPENDDS_STRING>& key_names, const TopicSet& other_topics)
{
  const MetaStruct& meta = getResultingMeta();
  SampleVec new_data;
  for (typename SampleVec::iterator it_res = resulting.begin();
       it_res != resulting.end(); /*incremented in loop*/) {
    bool found_one_match = false;
    for (typename SampleVec::const_iterator it_other = other.begin();
         it_other != other.end(); ++it_other) {
      bool match = true;
      for (size_t i = 0; match && i < key_names.size(); ++i) {
        if (!meta.compare(&*it_res, &*it_other, key_names[i].c_str())) {
          match = false;
        }
      }
      if (!match) {
        continue;
      }
      if (found_one_match) {
        new_data.push_back(*it_res);
        new_data.back().combine(*it_other);
        assign_resulting_fields(new_data.back().sample_, it_other->sample_, other_topics);
      } else {
        found_one_match = true;
        it_res->combine(*it_other);
        assign_resulting_fields(it_res->sample_, it_other->sample_, other_topics);
      }
    }
    if (found_one_match) {
      ++it_res;
    } else {
      // no match found in 'other' so data must not appear in result set
      it_res = resulting.erase(it_res);
    }
  }
  resulting.insert(resulting.end(), new_data.begin(), new_data.end());
}

/**
 * Two constituent topics are joinable directly if they have some common join keys,
 * or indirectly via a third topic which has common join keys with each of them.
 * The constituent topics form one or more groups of connected topics. In each groups,
 * topics are connected. But any two groups are not connected (otherwise, they would
 * have been a single group). And so groups are cross-joined with each other.
 * Within a group, topics are joined using process_joins(). Starting from an incoming
 * sample of a given topic, it computes partial resulting samples from the samples of
 * the constituent topics in the same group that are already received, in a DFS manner.
 * @partial_results contains entries for sets of topics, each corresponding to a path
 * of topics which have been visited, starting from the incoming topic.
 * For example, if the graph of visited topics looks like this
 *                              A (incoming topic)
 *                            /   \
 *                           B     C
 *                          /    / | \
 *                         D    E  F  G
 *                        /     .  .  .
 *                       H      .  .  .
 *                       .      .  .  .
 * then @a partial_results contains entries for topic sets: {A,B,D,H}, {A,C,E},
 * {A,C,F}, {A,C,G}.
 * When traverse a path, if the next adjacent topic appears on a different path, i.e.,
 * there is an entry corresponding to that path in @a partial_results that has it in
 * its topic set, then the two entries are combined into a new entry which contains
 * topics from both of them. For example, if we are traversing path (A,C,G) and
 * the next adjacent topic to G is B, which appears in path (A,B,D,H), then entries
 * keyed by (A,C,G) and (A,B,D,H) are combined into a single entry with key
 * (A,B,C,D,G,H). Entries (A,C,G) and (A,B,D,H) are removed from @a partial_results.
 */
template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::process_joins(
  std::map<TopicSet, SampleVec>& partial_results, SampleVec starting,
  const TopicSet& seen, const QueryPlan& qp)
{
  using namespace std;
  const MetaStruct& resulting_meta = getResultingMeta();
  OPENDDS_STRING this_topic;
  {
    ACE_GUARD_RETURN(ACE_RW_Thread_Mutex, read_guard, qp_lock_, DDS::RETCODE_OUT_OF_RESOURCES);
    this_topic = topicNameFor(qp.data_reader_);
  }
  typedef multimap<OPENDDS_STRING, OPENDDS_STRING>::const_iterator iter_t;
  for (iter_t iter = qp.adjacent_joins_.begin(); iter != qp.adjacent_joins_.end();) {
    // for each topic we're joining
    const OPENDDS_STRING& other_topic = iter->first;
    iter_t range_end = qp.adjacent_joins_.upper_bound(other_topic);
    const QueryPlan& other_qp = query_plans_[other_topic];
    DDS::DataReader_ptr other_dr = other_qp.data_reader_;
    const MetaStruct& other_meta = metaStructFor(other_dr);

    vector<OPENDDS_STRING> keys;
    for (; iter != range_end; ++iter) { // for each key in common w/ this topic
      keys.push_back(iter->second);
    }

    typename std::map<TopicSet, SampleVec>::iterator found =
      find_if(partial_results.begin(), partial_results.end(), Contains(other_topic));

    if (found == partial_results.end()) { // haven't seen this topic yet
      partial_results.erase(seen);
      TopicSet with_join(seen);
      with_join.insert(other_topic);
      SampleVec& join_result = partial_results[with_join];
      for (size_t i = 0; i < starting.size(); ++i) {
        GenericData other_keys(other_meta);
        for (size_t j = 0; j < keys.size(); ++j) {
          other_meta.assign(other_keys.ptr_, keys[j].c_str(),
                            &starting[i], keys[j].c_str(), resulting_meta);
        }
        if (!join(join_result, starting[i], keys, other_keys.ptr_, other_dr, other_meta)) {
          return DDS::RETCODE_ERROR;
        }
      }

      if (!join_result.empty() && !seen.count(other_topic)) {
        // Recursively join with topics that are adjacent to other_topic.
        const DDS::ReturnCode_t ret = process_joins(partial_results, join_result,
                                                    with_join, other_qp);
        if (ret != DDS::RETCODE_OK) {
          return ret;
        }
      }
    } else if (!found->first.count(this_topic) /*avoid looping back*/) {
      // We have partialResults for this topic, use them instead of recursing.
      // Combine the partial samples for the TopicSet seen and found->first.
      // Store the result into a new entry keyed with all topics in seen and found->first.
      // The existing two entries are removed since they are not needed anymore.
      combine(starting, found->second, keys, found->first);
      TopicSet new_topics(seen);
      for (set<OPENDDS_STRING>::const_iterator it = found->first.begin(); it != found->first.end(); ++it) {
        new_topics.insert(*it);
      }

      partial_results.erase(found);
      partial_results.erase(seen);
      partial_results[new_topics] = starting;
    }
  }
  return DDS::RETCODE_OK;
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::cross_join(
  std::map<TopicSet, SampleVec>& partial_results, const TopicSet& seen,
  const QueryPlan& qp)
{
  using namespace std;
  const MetaStruct& other_meta = metaStructFor(qp.data_reader_);
  vector<OPENDDS_STRING> no_keys;
  for (typename std::map<TopicSet, SampleVec>::iterator it_pr = partial_results.begin();
       it_pr != partial_results.end(); ++it_pr) {
    SampleVec resulting;
    for (typename SampleVec::iterator i = it_pr->second.begin(); i != it_pr->second.end(); ++i) {
      if (!join(resulting, *i, no_keys, 0, qp.data_reader_, other_meta)) {
        return DDS::RETCODE_ERROR;
      }
    }
    resulting.swap(it_pr->second);
  }

  TopicSet with_join(seen);
  with_join.insert(topicNameFor(qp.data_reader_));
  partial_results[with_join].swap(partial_results[seen]);
  partial_results.erase(seen);
  const DDS::ReturnCode_t ret = process_joins(partial_results, partial_results[with_join],
                                              with_join, qp);
  if (ret != DDS::RETCODE_OK) {
    partial_results.erase(with_join);
    return ret;
  }

  return DDS::RETCODE_OK;
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::incoming_sample(void* sample,
  const DDS::SampleInfo& info, const char* topic, const MetaStruct& meta)
{
  using namespace std;
  using namespace DDS;
  const QueryPlan& qp = query_plans_[topic];

  // Track results of joins along multiple paths through the MultiTopic keys.
  std::map<TopicSet, SampleVec> partial_results;
  TopicSet seen;
  seen.insert(topic);
  partial_results[seen].push_back(SampleWithInfo(topic, info));
  assign_fields(partial_results[seen].back().sample_, sample, qp, meta);

  DDS::ReturnCode_t ret = process_joins(partial_results, partial_results[seen], seen, qp);
  if (ret != DDS::RETCODE_OK) {
    return;
  }

  // Any topic we haven't seen needs to be cross-joined
  for (std::map<OPENDDS_STRING, QueryPlan>::iterator iter = query_plans_.begin();
       iter != query_plans_.end(); ++iter) {
    typename std::map<TopicSet, SampleVec>::iterator found =
      find_if(partial_results.begin(), partial_results.end(), Contains(iter->first));
    if (found == partial_results.end()) {
      ret = cross_join(partial_results, seen, iter->second);
      if (ret != DDS::RETCODE_OK) {
        return;
      }
    }
  }

  TypedDataReader* tdr = dynamic_cast<TypedDataReader*>(typed_reader_.in());
  if (!tdr) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReader_T::incoming_sample:")
               ACE_TEXT(" Failed to get TypedDataReader.\n")));
    return;
  }

  for (typename std::map<TopicSet, SampleVec>::iterator it_pr = partial_results.begin();
       it_pr != partial_results.end(); ++it_pr) {
    for (typename SampleVec::iterator i = it_pr->second.begin(); i != it_pr->second.end(); ++i) {
      InstanceHandle_t ih = tdr->store_synthetic_data(i->sample_, i->view_);
      if (ih != HANDLE_NIL) {
        typedef std::map<OPENDDS_STRING, InstanceHandle_t>::iterator mapiter_t;
        for (mapiter_t it_map = i->info_.begin(); it_map != i->info_.end(); ++it_map) {
          query_plans_[it_map->first].instances_.insert(make_pair(it_map->second, ih));
        }
      }
    }
  }
}

// The following methods implement the FooDataReader API by delegating
// to the typed_reader_.

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read(SampleSeq& received_data,
  DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->read(received_data, info_seq, max_samples,
    sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take(SampleSeq& received_data,
  DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->take(received_data, info_seq, max_samples,
    sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->read_w_condition(data_values, sample_infos,
    max_samples, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->take_w_condition(data_values, sample_infos,
    max_samples, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_next_sample(
  Sample& received_data, DDS::SampleInfo& sample_info)
{
  return typed_reader_->read_next_sample(received_data, sample_info);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_next_sample(
  Sample& received_data, DDS::SampleInfo& sample_info)
{
  return typed_reader_->take_next_sample(received_data, sample_info);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->read_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->take_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_instance_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::InstanceHandle_t handle,
  DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->read_instance_w_condition(data_values,
    sample_infos, max_samples, handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_instance_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::InstanceHandle_t handle,
  DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->take_instance_w_condition(data_values,
    sample_infos, max_samples, handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_next_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->read_next_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_next_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
{
  return typed_reader_->take_next_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_next_instance_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::InstanceHandle_t previous_handle,
  DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->read_next_instance_w_condition(data_values,
    sample_infos, max_samples, previous_handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_next_instance_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::InstanceHandle_t previous_handle,
  DDS::ReadCondition_ptr a_condition)
{
  return typed_reader_->take_next_instance_w_condition(data_values,
    sample_infos, max_samples, previous_handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::return_loan(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq)
{
  return typed_reader_->return_loan(received_data, info_seq);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::get_key_value(
  Sample& key_holder, DDS::InstanceHandle_t handle)
{
  return typed_reader_->get_key_value(key_holder, handle);
}

template<typename Sample, typename TypedDataReader>
DDS::InstanceHandle_t
MultiTopicDataReader_T<Sample, TypedDataReader>::lookup_instance(
  const Sample& instance_data)
{
  return typed_reader_->lookup_instance(instance_data);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
