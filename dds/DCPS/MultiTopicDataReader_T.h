/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADER_T_H
#define OPENDDS_DCPS_MULTITOPICDATAREADER_T_H

#ifndef OPENDDS_NO_MULTI_TOPIC

#include "dds/DCPS/MultiTopicDataReaderBase.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template<typename Sample, typename TypedDataReader>
class MultiTopicDataReader_T
  : public virtual LocalObject<typename TypedDataReader::Interface>
  , public virtual MultiTopicDataReaderBase {
public:
  typedef TAO::DCPS::ZeroCopyDataSeq<Sample> SampleSeq;

  MultiTopicDataReader_T() {}

  void init_typed(DataReaderEx* dr);
  const MetaStruct& getResultingMeta();
  void incoming_sample(void* sample, const DDS::SampleInfo& info,
                       const char* topic, const MetaStruct& meta);

  DDS::ReturnCode_t read(SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
    CORBA::Long max_samples, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t take(SampleSeq& received_data, DDS::SampleInfoSeq& info_seq,
    CORBA::Long max_samples, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t read_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t take_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t read_next_sample(Sample& received_data,
    DDS::SampleInfo& sample_info);

  DDS::ReturnCode_t take_next_sample(Sample& received_data,
    DDS::SampleInfo& sample_info);

  DDS::ReturnCode_t read_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq & info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t take_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq & info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t read_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t handle, DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t take_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq & sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t handle, DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t read_next_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t take_next_instance(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq, CORBA::Long max_samples,
    DDS::InstanceHandle_t a_handle, DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t read_next_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq& sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t previous_handle, DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t take_next_instance_w_condition(SampleSeq& data_values,
    DDS::SampleInfoSeq & sample_infos, CORBA::Long max_samples,
    DDS::InstanceHandle_t previous_handle, DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t return_loan(SampleSeq& received_data,
    DDS::SampleInfoSeq& info_seq);

  DDS::ReturnCode_t get_key_value(Sample& key_holder,
    DDS::InstanceHandle_t handle);

  DDS::InstanceHandle_t lookup_instance(const Sample& instance_data);

private:

  struct SampleWithInfo {
    SampleWithInfo(const OPENDDS_STRING& topic, const DDS::SampleInfo& sampinfo)
      : sample_(),
        view_(sampinfo.view_state) {
      info_[topic] = sampinfo.instance_handle;
    }
    void combine(const SampleWithInfo& other) {
      info_.insert(other.info_.begin(), other.info_.end());
      if (other.view_ == DDS::NEW_VIEW_STATE) view_ = DDS::NEW_VIEW_STATE;
    }
    Sample sample_;
    DDS::ViewStateKind view_;
    OPENDDS_MAP(OPENDDS_STRING/*topicName*/, DDS::InstanceHandle_t) info_;
  };

  typedef std::vector<SampleWithInfo> SampleVec;
  typedef std::set<OPENDDS_STRING> TopicSet;

  // Given a QueryPlan that describes how to treat 'incoming' data from a
  // certain topic (with MetaStruct 'meta'), assign its relevant fields to
  // the corresponding fields of 'resulting'.
  void assign_fields(void* incoming, Sample& resulting, const QueryPlan& qp,
                     const MetaStruct& meta);

  // Process all joins (recursively) in the QueryPlan 'qp'.
  void process_joins(OPENDDS_MAP(TopicSet, SampleVec)& partialResults,
                     SampleVec starting, const TopicSet& seen,
                     const QueryPlan& qp);

  // Starting with a 'prototype' sample, fill a 'resulting' vector with all
  // data from 'other_dr' (with MetaStruct 'other_meta') such that all key
  // fields named in 'key_names' match the values in 'key_data'.  The struct
  // pointed-to by 'key_data' is of the type used by the 'other_dr'.
  void join(SampleVec& resulting, const SampleWithInfo& prototype,
            const std::vector<OPENDDS_STRING>& key_names, const void* key_data,
            DDS::DataReader_ptr other_dr, const MetaStruct& other_meta);

  // When no common keys are found, natural join devolves to a cross-join where
  // each instance in the joined-to-topic (qp) is combined with the results so
  // far (partialResults).
  void cross_join(OPENDDS_MAP(TopicSet, SampleVec)& partialResults,
                  const TopicSet& seen, const QueryPlan& qp);

  // Combine two vectors of data, 'resulting' and 'other', with the results of
  // the combination going into 'resulting'.  Use the keys in 'key_names' to
  // determine which elements to combine, and the topic names in 'other_topics'
  // to determine which fields from 'other' should be assigned to 'resulting'.
  void combine(SampleVec& resulting, const SampleVec& other,
               const std::vector<OPENDDS_STRING>& key_names,
               const TopicSet& other_topics);

  // Helper for combine(), similar to assign_fields but instead of coming from
  // a differently-typed struct in a void*, the data comes from an existing
  // Sample, 'source'.  Each field projeted from any of the topics in
  // 'other_topics' is copied from 'source' to 'target'.
  void assign_resulting_fields(Sample& target, const Sample& source,
                               const TopicSet& other_topics);

  struct GenericData {
    explicit GenericData(const MetaStruct& meta, bool doAlloc = true)
      : meta_(meta), ptr_(doAlloc ? meta.allocate() : NULL) {}
    ~GenericData() { meta_.deallocate(ptr_); }
    const MetaStruct& meta_;
    void* ptr_;
  };

  struct Contains { // predicate for std::find_if()
    const OPENDDS_STRING& look_for_;
    explicit Contains(const OPENDDS_STRING& s) : look_for_(s) {}
    bool operator()(const std::pair<const std::set<OPENDDS_STRING>, SampleVec>& e)
      const {
      return e.first.count(look_for_);
    }
  };

  typename TypedDataReader::Interface::_var_type typed_reader_;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef ACE_TEMPLATES_REQUIRE_SOURCE
#include "dds/DCPS/MultiTopicDataReader_T.cpp"
#endif

#endif
#endif
