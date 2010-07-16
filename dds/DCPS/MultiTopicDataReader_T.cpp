/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADER_T_CPP
#define OPENDDS_DCPS_MULTITOPICDATAREADER_T_CPP

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE


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
  typedef vector<SubjectFieldSpec>::const_iterator iter_t;
  for (iter_t iter = proj.begin(); iter != proj.end(); ++iter) {
    const SubjectFieldSpec& sfs = *iter;
    getResultingMeta().assign(&resulting, sfs.resulting_name_.c_str(),
                              incoming, sfs.incoming_name_.c_str(), meta);
  }

  const vector<string>& proj_out = qp.keys_projected_out_;
  for (vector<string>::const_iterator iter = proj_out.begin();
       iter != proj_out.end(); ++iter) {
    getResultingMeta().assign(&resulting, iter->c_str(),
                              incoming, iter->c_str(), meta);
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::join(
  std::vector<Sample>& resulting, const Sample& prototype,
  const std::vector<std::string>& key_names, const void* key_data,
  DDS::DataReader_ptr other_dr, const MetaStruct& other_meta)
{
  using namespace DDS;
  DataReaderImpl* other_dri = dynamic_cast<DataReaderImpl*>(other_dr);
  TopicDescription_var other_td = other_dri->get_topicdescription();
  CORBA::String_var other_topic = other_td->get_name();
  const QueryPlan& other_qp = query_plans_[other_topic.in()];

  if (other_meta.numDcpsKeys() == key_names.size()) { // complete key
    InstanceHandle_t ih = other_dri->lookup_instance_generic(key_data);
    if (ih == HANDLE_NIL) {
      // no match for join, no results
      resulting.clear();
    } else {
      GenericData other_data(other_meta, false);
      SampleInfo info;
      ReturnCode_t ret = other_dri->read_instance_generic(other_data.ptr_,
        info, ih, READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
        //TODO: log & throw
      }
      resulting.push_back(Sample(prototype));
      assign_fields(other_data.ptr_, resulting.back(), other_qp, other_meta);
    }
  } else { // incomplete key
    SampleVec new_resulting;
    ReturnCode_t ret = RETCODE_OK;
    for (InstanceHandle_t ih = HANDLE_NIL; ret != RETCODE_NO_DATA;) {
      GenericData other_data(other_meta, false);
      SampleInfo info;
      ret = other_dri->read_next_instance_generic(other_data.ptr_, info, ih,
        READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
      if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
        //TODO: log & throw
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
        resulting.push_back(Sample(prototype));
        assign_fields(other_data.ptr_, resulting.back(), other_qp, other_meta);
      }
    }
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::incoming_sample(void* sample,
  const DDS::SampleInfo& /*info*/, const char* topic, const MetaStruct& meta)
{
  using namespace std;
  using namespace DDS;

  const QueryPlan& qp = query_plans_[topic];
  SampleVec resulting;
  resulting.push_back(Sample());
  assign_fields(sample, resulting[0], qp, meta);

  typedef multimap<string, string>::const_iterator iter_t;
  for (iter_t iter = qp.adjacent_joins_.begin();
       iter != qp.adjacent_joins_.end();) { // for each topic we're joining
    const string& other_topic = iter->first;
    iter_t range_end = qp.adjacent_joins_.upper_bound(other_topic);
    DataReader_ptr other_dr = query_plans_[other_topic].data_reader_;
    const MetaStruct& other_meta = metaStructFor(other_dr);

    vector<string> keys;
    for (; iter != range_end; ++iter) { // for each key in common w/ this topic
      keys.push_back(iter->second);
    }

    SampleVec join_result;
    for (size_t i = 0; i < resulting.size(); ++i) {
      GenericData other_key(other_meta);
      for (size_t j = 0; j < keys.size(); ++j) {
        other_meta.assign(other_key.ptr_, keys[j].c_str(),
                          sample, keys[j].c_str(), meta); 
      }
      join(join_result, resulting[i], keys,
           other_key.ptr_, other_dr, other_meta);
    }
    resulting.swap(join_result);
  }

  TypedDataReader* tdr = dynamic_cast<TypedDataReader*>(typed_reader_.in());
  for (typename SampleVec::iterator i = resulting.begin();
       i != resulting.end(); ++i) {
    tdr->store_synthetic_data(*i);
  }
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read(SampleSeq& received_data,
  DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException))
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
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->take(received_data, info_seq, max_samples,
    sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->read_w_condition(data_values, sample_infos,
    max_samples, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq& sample_infos,
  CORBA::Long max_samples, DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->take_w_condition(data_values, sample_infos,
    max_samples, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_next_sample(
  Sample& received_data, DDS::SampleInfo& sample_info)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->read_next_sample(received_data, sample_info);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_next_sample(
  Sample& received_data, DDS::SampleInfo& sample_info)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->take_next_sample(received_data, sample_info);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq & info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->read_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_instance(
  SampleSeq& received_data, DDS::SampleInfoSeq & info_seq,
  CORBA::Long max_samples, DDS::InstanceHandle_t a_handle,
  DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
  DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->take_instance(received_data, info_seq, max_samples,
    a_handle, sample_states, view_states, instance_states);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::read_next_instance(
  SampleSeq& received_data,
  DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
  DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
  DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException))
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
  ACE_THROW_SPEC((CORBA::SystemException))
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
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->read_next_instance_w_condition(data_values,
    sample_infos, max_samples, previous_handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::take_next_instance_w_condition(
  SampleSeq& data_values, DDS::SampleInfoSeq & sample_infos,
  CORBA::Long max_samples, DDS::InstanceHandle_t previous_handle,
  DDS::ReadCondition_ptr a_condition)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->take_next_instance_w_condition(data_values,
    sample_infos, max_samples, previous_handle, a_condition);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::return_loan(
  SampleSeq& received_data, DDS::SampleInfoSeq& info_seq)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->return_loan(received_data, info_seq);
}

template<typename Sample, typename TypedDataReader>
DDS::ReturnCode_t
MultiTopicDataReader_T<Sample, TypedDataReader>::get_key_value(
  Sample& key_holder, DDS::InstanceHandle_t handle)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->get_key_value(key_holder, handle);
}

template<typename Sample, typename TypedDataReader>
DDS::InstanceHandle_t
MultiTopicDataReader_T<Sample, TypedDataReader>::lookup_instance(
  const Sample& instance_data)
  ACE_THROW_SPEC((CORBA::SystemException))
{
  return typed_reader_->lookup_instance(instance_data);
}

}
}

#endif
#endif
