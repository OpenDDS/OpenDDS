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
  typed_reader_ = TypedDataReader::_narrow(dr);
}

template<typename Sample, typename TypedDataReader>
const MetaStruct&
MultiTopicDataReader_T<Sample, TypedDataReader>::getResultingMeta()
{
  return getMetaStruct<Sample>();
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
