/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADERBASE_H
#define OPENDDS_DCPS_MULTITOPICDATAREADERBASE_H

#ifndef OPENDDS_NO_MULTI_TOPIC

#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DCPS/ZeroCopySeq_T.h"
#include "dds/DCPS/MultiTopicImpl.h"
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class SubscriberImpl;

class OpenDDS_Dcps_Export MultiTopicDataReaderBase
  : public virtual LocalObject<DataReaderEx> {
public:
  MultiTopicDataReaderBase() {}

  void init(const DDS::DataReaderQos& dr_qos,
    DDS::DataReaderListener_ptr a_listener, DDS::StatusMask mask,
    SubscriberImpl* parent, MultiTopicImpl* multitopic);

  void data_available(DDS::DataReader_ptr reader);

  // used by the SubscriberImpl

  void set_status_changed_flag(DDS::StatusKind status, bool flag);
  bool have_sample_states(DDS::SampleStateMask sample_states) const;
  void cleanup();

  // DDS::Entity interface

  DDS::InstanceHandle_t get_instance_handle();

  DDS::ReturnCode_t enable();

  DDS::StatusCondition_ptr get_statuscondition();

  DDS::StatusMask get_status_changes();

  // DDS::DataReader interface

  DDS::ReadCondition_ptr create_readcondition(
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

#ifndef OPENDDS_NO_QUERY_CONDITION
  DDS::QueryCondition_ptr create_querycondition(
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states, const char* query_expression,
    const DDS::StringSeq& query_parameters);
#endif

  DDS::ReturnCode_t delete_readcondition(DDS::ReadCondition_ptr a_condition);

  DDS::ReturnCode_t delete_contained_entities();

  DDS::ReturnCode_t set_qos(const DDS::DataReaderQos& qos);

  DDS::ReturnCode_t get_qos(DDS::DataReaderQos& qos);

  DDS::ReturnCode_t set_listener(DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask);

  DDS::DataReaderListener_ptr get_listener();

  DDS::TopicDescription_ptr get_topicdescription();

  DDS::Subscriber_ptr get_subscriber();

  DDS::ReturnCode_t get_sample_rejected_status(
    DDS::SampleRejectedStatus& status);

  DDS::ReturnCode_t get_liveliness_changed_status(
    DDS::LivelinessChangedStatus& status);

  DDS::ReturnCode_t get_requested_deadline_missed_status(
    DDS::RequestedDeadlineMissedStatus& status);

  DDS::ReturnCode_t get_requested_incompatible_qos_status(
    DDS::RequestedIncompatibleQosStatus& status);

  DDS::ReturnCode_t get_subscription_matched_status(
    DDS::SubscriptionMatchedStatus& status);

  DDS::ReturnCode_t get_sample_lost_status(DDS::SampleLostStatus& status);

  DDS::ReturnCode_t wait_for_historical_data(const DDS::Duration_t& max_wait);

  DDS::ReturnCode_t get_matched_publications(
    DDS::InstanceHandleSeq& publication_handles);

#ifndef DDS_HAS_MINIMUM_BIT
  DDS::ReturnCode_t get_matched_publication_data(
    DDS::PublicationBuiltinTopicData& publication_data,
    DDS::InstanceHandle_t publication_handle);
#endif

  // OpenDDS::DCPS::DataReaderEx interface

  void get_latency_stats(LatencyStatisticsSeq& stats);

  void reset_latency_stats();

  CORBA::Boolean statistics_enabled();

  void statistics_enabled(CORBA::Boolean statistics_enabled);

private:
  virtual void init_typed(DataReaderEx* dr) = 0;
  virtual const MetaStruct& getResultingMeta() = 0;
  virtual void incoming_sample(void* sample, const DDS::SampleInfo& info,
                               const char* topic, const MetaStruct& meta) = 0;

  class Listener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
  public:
    explicit Listener(MultiTopicDataReaderBase* outer)
      : outer_(outer)
    {}

    void on_requested_deadline_missed(DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus& status);

    void on_requested_incompatible_qos(DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus& status);

    void on_sample_rejected(DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status);

    void on_liveliness_changed(DDS::DataReader_ptr reader,
      const DDS::LivelinessChangedStatus& status);

    void on_data_available(DDS::DataReader_ptr reader);

    void on_subscription_matched(DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchedStatus& status);

    void on_sample_lost(DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status);

  private:
    MultiTopicDataReaderBase* outer_;
  };

  DDS::DataReaderListener_var listener_;
  DataReaderEx_var resulting_reader_;

protected:

  OPENDDS_STRING topicNameFor(DDS::DataReader_ptr dr);
  const MetaStruct& metaStructFor(DDS::DataReader_ptr dr);

  typedef MultiTopicImpl::SubjectFieldSpec SubjectFieldSpec;

  struct QueryPlan {
    DDS::DataReader_var data_reader_;
    std::vector<SubjectFieldSpec> projection_;
    std::vector<OPENDDS_STRING> keys_projected_out_;
    std::multimap<OPENDDS_STRING, OPENDDS_STRING> adjacent_joins_; // topic -> key
    std::set<std::pair<DDS::InstanceHandle_t /*of this data_reader_*/,
      DDS::InstanceHandle_t /*of the resulting DR*/> > instances_;
  };

  // key: topicName for this reader
  OPENDDS_MAP(OPENDDS_STRING, QueryPlan) query_plans_;

  OPENDDS_DELETED_COPY_MOVE_CTOR_ASSIGN(MultiTopicDataReaderBase)
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
#endif
