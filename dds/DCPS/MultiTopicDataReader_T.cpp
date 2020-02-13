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
MultiTopicDataReader_T<Sample, TypedDataReader>::assign_fields(void* incoming,
  Sample& resulting, const MultiTopicDataReaderBase::QueryPlan& qp,
  const MetaStruct& meta)
{
  using namespace std;
  const vector<SubjectFieldSpec>& proj = qp.projection_;
  const MetaStruct& resulting_meta = getResultingMeta();
  typedef vector<SubjectFieldSpec>::const_iterator iter_t;
  for (iter_t iter = proj.begin(); iter != proj.end(); ++iter) {
    const SubjectFieldSpec& sfs = *iter;
    resulting_meta.assign(&resulting, sfs.resulting_name_.c_str(),
                          incoming, sfs.incoming_name_.c_str(), meta);
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
  const MetaStruct& meta = getResultingMeta();
  for (TopicSet::const_iterator iterTopic = other_topics.begin();
       iterTopic != other_topics.end(); ++iterTopic) {
    const QueryPlan& qp = query_plans_[*iterTopic];
    const vector<SubjectFieldSpec>& proj = qp.projection_;
    typedef vector<SubjectFieldSpec>::const_iterator iter_t;
    for (iter_t iter = proj.begin(); iter != proj.end(); ++iter) {
      const SubjectFieldSpec& sfs = *iter;
      meta.assign(&target, sfs.resulting_name_.c_str(),
                  &source, sfs.resulting_name_.c_str(), meta);
    }
  }
}

template<typename Sample, typename TypedDataReader>
void
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
    return;
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
      ReturnCode_t ret = other_dri->read_instance_generic(other_data.ptr_,
        info, ih, READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
        throw std::runtime_error(
         OPENDDS_STRING("In join(), incoming DataReader for ") + OPENDDS_STRING(other_topic) +
         " read_instance_generic: " + retcode_to_string(ret));
      } else if (ret == DDS::RETCODE_OK) {
        resulting.push_back(prototype);
        resulting.back().combine(SampleWithInfo(other_topic.in(), info));
        assign_fields(other_data.ptr_, resulting.back().sample_,
                      other_qp, other_meta);
      }
    }
  } else { // incomplete key or cross-join (0 key fields)
    SampleVec new_resulting;
    ReturnCode_t ret = RETCODE_OK;
    for (InstanceHandle_t ih = HANDLE_NIL; ret != RETCODE_NO_DATA;) {
      GenericData other_data(other_meta, false);
      SampleInfo info;
      ret = other_dri->read_next_instance_generic(other_data.ptr_, info, ih,
        READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
        std::ostringstream ss;
        ss
          << "In join(), incoming DataReader for " << OPENDDS_STRING(other_topic)
          << " read_next_instance_generic: " << retcode_to_string(ret);
        throw std::runtime_error(ss.str());
      } else if (ret == RETCODE_NO_DATA) {
        break;
      }
      ih = info.instance_handle;

      bool match = true;
      for (size_t i = 0; match && i < key_names.size(); ++i) {
        if (!other_meta.compare(key_data, other_data.ptr_,
                                key_names[i].c_str())) {
          match = false;
        }
      }

      if (match) {
        resulting.push_back(prototype);
        resulting.back().combine(SampleWithInfo(other_topic.in(), info));
        assign_fields(other_data.ptr_, resulting.back().sample_,
                      other_qp, other_meta);
      }
    }
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::combine(
  SampleVec& resulting, const SampleVec& other,
  const std::vector<OPENDDS_STRING>& key_names, const TopicSet& other_topics)
{
  const MetaStruct& meta = getResultingMeta();
  SampleVec newData;
  for (typename SampleVec::iterator iterRes = resulting.begin();
       iterRes != resulting.end(); /*incremented in loop*/) {
    bool foundOneMatch = false;
    for (typename SampleVec::const_iterator iterOther = other.begin();
         iterOther != other.end(); ++iterOther) {
      bool match = true;
      for (size_t i = 0; match && i < key_names.size(); ++i) {
        if (!meta.compare(&*iterRes, &*iterOther, key_names[i].c_str())) {
          match = false;
        }
      }
      if (!match) {
        continue;
      }
      if (foundOneMatch) {
        newData.push_back(*iterRes);
        newData.back().combine(*iterOther);
        assign_resulting_fields(newData.back().sample_,
                                iterOther->sample_, other_topics);
      } else {
        foundOneMatch = true;
        iterRes->combine(*iterOther);
        assign_resulting_fields(iterRes->sample_,
                                iterOther->sample_, other_topics);
      }
    }
    if (foundOneMatch) {
      ++iterRes;
    } else {
      // no match found in 'other' so data must not appear in result set
      iterRes = resulting.erase(iterRes);
    }
  }
  resulting.insert(resulting.end(), newData.begin(), newData.end());
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::process_joins(
  std::map<TopicSet, SampleVec>& partialResults, SampleVec starting,
  const TopicSet& seen, const QueryPlan& qp)
{
  using namespace std;
  const MetaStruct& resulting_meta = getResultingMeta();
  const OPENDDS_STRING this_topic = topicNameFor(qp.data_reader_);
  typedef multimap<OPENDDS_STRING, OPENDDS_STRING>::const_iterator iter_t;
  for (iter_t iter = qp.adjacent_joins_.begin();
       iter != qp.adjacent_joins_.end();) { // for each topic we're joining
    const OPENDDS_STRING& other_topic = iter->first;
    iter_t range_end = qp.adjacent_joins_.upper_bound(other_topic);
    const QueryPlan& other_qp = query_plans_[other_topic];
    DDS::DataReader_ptr other_dr = other_qp.data_reader_;

    try {
      const MetaStruct& other_meta = metaStructFor(other_dr);

      vector<OPENDDS_STRING> keys;
      for (; iter != range_end; ++iter) { // for each key in common w/ this topic
        keys.push_back(iter->second);
      }

      typename std::map<TopicSet, SampleVec>::iterator found =
        find_if(partialResults.begin(), partialResults.end(),
          Contains(other_topic));

      if (found == partialResults.end()) { // haven't seen this topic yet

        partialResults.erase(seen);
        TopicSet withJoin(seen);
        withJoin.insert(other_topic);
        SampleVec& join_result = partialResults[withJoin];
        for (size_t i = 0; i < starting.size(); ++i) {
          GenericData other_key(other_meta);
          for (size_t j = 0; j < keys.size(); ++j) {
            other_meta.assign(other_key.ptr_, keys[j].c_str(),
              &starting[i], keys[j].c_str(), resulting_meta);
          }
          join(join_result, starting[i], keys,
            other_key.ptr_, other_dr, other_meta);
        }

        if (!join_result.empty() && !seen.count(other_topic)) {
          // recurse
          process_joins(partialResults, join_result, withJoin, other_qp);
        }

      } else if (!found->first.count(this_topic) /*avoid looping back*/) {
        // we have partialResults for this topic, use them instead of recursing

        combine(starting, found->second, keys, found->first);
        TopicSet newKey(found->first);
        for (set<OPENDDS_STRING>::const_iterator i3 = found->first.begin();
          i3 != found->first.end(); ++i3) {
          newKey.insert(*i3);
        }
        partialResults.erase(found);
        partialResults[newKey] = starting;

      }
    } catch (const std::runtime_error& e) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReader_T::process_joins: %C\n"), e.what()));
    }
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::cross_join(
  std::map<TopicSet, SampleVec>& partialResults, const TopicSet& seen,
  const QueryPlan& qp)
{
  using namespace std;
  try {
    const MetaStruct& other_meta = metaStructFor(qp.data_reader_);
    vector<OPENDDS_STRING> no_keys;
    for (typename std::map<TopicSet, SampleVec>::iterator iterPR =
      partialResults.begin(); iterPR != partialResults.end(); ++iterPR) {
      SampleVec resulting;
      for (typename SampleVec::iterator i = iterPR->second.begin();
        i != iterPR->second.end(); ++i) {
        join(resulting, *i, no_keys, NULL, qp.data_reader_, other_meta);
      }
      resulting.swap(iterPR->second);
    }
    TopicSet withJoin(seen);
    withJoin.insert(topicNameFor(qp.data_reader_));
    partialResults[withJoin].swap(partialResults[seen]);
    partialResults.erase(seen);
    process_joins(partialResults, partialResults[withJoin], withJoin, qp);
  } catch (const std::runtime_error& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: MultiTopicDataReader_T::cross_join: %C\n"), e.what()));
  }
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
  std::map<TopicSet, SampleVec> partialResults;
  TopicSet seen;
  seen.insert(topic);
  partialResults[seen].push_back(SampleWithInfo(topic, info));
  assign_fields(sample, partialResults[seen].back().sample_, qp, meta);

  process_joins(partialResults, partialResults[seen], seen, qp);

  // Any topic we haven't seen needs to be cross-joined
  for (std::map<OPENDDS_STRING, QueryPlan>::iterator iter = query_plans_.begin();
       iter != query_plans_.end(); ++iter) {
    typename std::map<TopicSet, SampleVec>::iterator found =
      find_if(partialResults.begin(), partialResults.end(),
              Contains(iter->first));
    if (found == partialResults.end()) {
      cross_join(partialResults, seen, iter->second);
    }
  }

  TypedDataReader* tdr = dynamic_cast<TypedDataReader*>(typed_reader_.in());

  if (!tdr) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("MultiTopicDataReader_T::incoming_sample, ")
      ACE_TEXT("Failed to get TypedDataReader.\n")));
    return;
  }

  for (typename std::map<TopicSet, SampleVec>::iterator iterPR =
       partialResults.begin(); iterPR != partialResults.end(); ++iterPR) {
    for (typename SampleVec::iterator i = iterPR->second.begin();
         i != iterPR->second.end(); ++i) {
      InstanceHandle_t ih = tdr->store_synthetic_data(i->sample_, i->view_);
      if (ih != HANDLE_NIL) {
        typedef std::map<OPENDDS_STRING, InstanceHandle_t>::iterator mapiter_t;
        for (mapiter_t iterMap = i->info_.begin(); iterMap != i->info_.end();
             ++iterMap) {
          query_plans_[iterMap->first].instances_.insert(
            make_pair(iterMap->second, ih));
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
