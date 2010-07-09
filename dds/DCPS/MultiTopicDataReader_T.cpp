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
  Sample& resulting, const char* topic, const MetaStruct& meta)
{
  using namespace std;
  typedef multimap<string, SubjectFieldSpec>::iterator iter_t;
  typedef pair<iter_t, iter_t> iterpair_t;
  for (iterpair_t iters = field_map_.equal_range(topic);
      iters.first != iters.second; ++iters.first) {
    const SubjectFieldSpec& sfs = iters.first->second;
    const string& resulting_name = sfs.resulting_name_.empty()
      ? sfs.incoming_name_ : sfs.resulting_name_;
    getResultingMeta().assign(&resulting, resulting_name.c_str(),
      incoming, sfs.incoming_name_.c_str(), meta);
  }
}

template<typename Sample, typename TypedDataReader>
void
MultiTopicDataReader_T<Sample, TypedDataReader>::incoming_sample(void* sample,
  const DDS::SampleInfo& info, const char* topic, const MetaStruct& meta)
{
  using namespace std;
  using namespace DDS;
  Sample resulting;
  assign_fields(sample, resulting, topic, meta);
  TypedDataReader* tdr = dynamic_cast<TypedDataReader*>(typed_reader_.in());

  //TODO: process the "joins" to complete the fields of <resulting>
  //currently the simple case of joining two topics on one key is handled
  //but all of the other cases still need to be written
  typedef map<string, set<string> >::iterator iter2_t;
  for (iter2_t iter = join_keys_.begin(); iter != join_keys_.end(); ++iter) {
    const string& key = iter->first;
    const set<string>& topics = iter->second;
    if (topics.count(topic)) {
      getResultingMeta().assign(&resulting, key.c_str(), sample,
        key.c_str(), meta);
      for (set<string>::const_iterator i = topics.begin();
          i != topics.end(); ++i) {
        if (*i != topic) {
          DataReader_var other_dr = incoming_readers_[*i];
          const MetaStruct& other_meta = metaStructFor(other_dr);
          void* other_key_data = other_meta.allocate();
          other_meta.assign(other_key_data, key.c_str(),
            sample, key.c_str(), meta);
          DataReaderImpl* other_dri =
            dynamic_cast<DataReaderImpl*>(other_dr.in());
          InstanceHandle_t ih =
            other_dri->lookup_instance_generic(other_key_data);
          other_meta.deallocate(other_key_data);
          if (ih != DDS::HANDLE_NIL) {
            void* other_data;
            SampleInfo info;
            other_dri->read_instance_generic(other_data, info, ih,
              READ_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
            assign_fields(other_data, resulting, i->c_str(), other_meta);
            other_meta.deallocate(other_data);
            tdr->store_synthetic_data(resulting);
          }
        }
      }
    } else {
      //TODO: handle case where key is not in this topic
    }
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
