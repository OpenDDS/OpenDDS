/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADER_T_H
#define OPENDDS_DCPS_MULTITOPICDATAREADER_T_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DCPS/MultiTopicDataReaderBase.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

template<typename Sample, typename TypedDataReader>
class MultiTopicDataReader_T
  : public virtual LocalObject<typename TypedDataReader::Interface>
  , public virtual MultiTopicDataReaderBase {
public:
  typedef TAO::DCPS::ZeroCopyDataSeq<Sample> SampleSeq;

  void init_typed(DataReaderEx* dr);
  const MetaStruct& getResultingMeta();
  void incoming_sample(void* sample, const DDS::SampleInfo& info,
                       const char* topic, const MetaStruct& meta);

  DDS::ReturnCode_t read(SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
    CORBA::Long max_samples, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take(SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
    CORBA::Long max_samples, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t read_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t read_next_sample(Sample& received_data,
    DDS::SampleInfo& sample_info)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take_next_sample(Sample& received_data,
    DDS::SampleInfo& sample_info)
    ACE_THROW_SPEC((CORBA::SystemException));
  
  DDS::ReturnCode_t read_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq & info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq & info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t read_next_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take_next_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t read_next_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t previous_handle, DDS::ReadCondition_ptr a_condition)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t take_next_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq & sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t previous_handle, DDS::ReadCondition_ptr a_condition)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t return_loan(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_key_value(Sample& key_holder,
    DDS::InstanceHandle_t handle)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::InstanceHandle_t lookup_instance(const Sample& instance_data)
    ACE_THROW_SPEC((CORBA::SystemException));

private:
  typedef std::vector<Sample> SampleVec;

  void assign_fields(void* incoming, Sample& resulting, const QueryPlan& qp,
                     const MetaStruct& meta);

  void join(SampleVec& resulting, const Sample& prototype,
            const std::vector<std::string>& key_names, const void* key_data,
            DDS::DataReader_ptr other_dr, const MetaStruct& other_meta);

  struct GenericData {
    explicit GenericData(const MetaStruct& meta, bool doAlloc = true)
      : meta_(meta), ptr_(doAlloc ? meta.allocate() : NULL) {}
    ~GenericData() { meta_.deallocate(ptr_); }
    const MetaStruct& meta_;
    void* ptr_;
  };

  typename TypedDataReader::Interface::_var_type typed_reader_;
};


}
}

#ifdef ACE_TEMPLATES_REQUIRE_SOURCE
#include "dds/DCPS/MultiTopicDataReader_T.cpp"
#endif

#endif
#endif
