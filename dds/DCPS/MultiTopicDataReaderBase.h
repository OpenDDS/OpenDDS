/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MULTITOPICDATAREADERBASE_H
#define OPENDDS_DCPS_MULTITOPICDATAREADERBASE_H

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/ZeroCopySeq_T.h"
#include "dds/DCPS/MultiTopicImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export MultiTopicDataReaderBase
  : public virtual LocalObject<DataReaderEx>
  , public virtual EntityImpl {
public:
  void init(const DDS::DataReaderQos& dr_qos, const DataReaderQosExt& ext_qos,
    DDS::DataReaderListener_ptr a_listener, DDS::StatusMask mask,
    SubscriberImpl* parent, MultiTopicImpl* multitopic);

  void data_available(DDS::DataReader_ptr reader);

  DDS::InstanceHandle_t get_instance_handle()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t enable()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReadCondition_ptr create_readcondition(DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states, DDS::InstanceStateMask instance_states)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::QueryCondition_ptr create_querycondition(
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states, const char* query_expression,
    const DDS::StringSeq& query_parameters)
    ACE_THROW_SPEC((CORBA::SystemException));
    
  DDS::ReturnCode_t delete_readcondition(DDS::ReadCondition_ptr a_condition)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t delete_contained_entities()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t set_qos(const DDS::DataReaderQos& qos)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_qos(DDS::DataReaderQos& qos)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t set_listener(DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::DataReaderListener_ptr get_listener()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::TopicDescription_ptr get_topicdescription()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::Subscriber_ptr get_subscriber()
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_sample_rejected_status(
    DDS::SampleRejectedStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_liveliness_changed_status(
    DDS::LivelinessChangedStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_requested_deadline_missed_status(
    DDS::RequestedDeadlineMissedStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_requested_incompatible_qos_status(
    DDS::RequestedIncompatibleQosStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_subscription_matched_status(
    DDS::SubscriptionMatchedStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_sample_lost_status(DDS::SampleLostStatus& status)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t wait_for_historical_data(const DDS::Duration_t& max_wait)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_matched_publications(
    DDS::InstanceHandleSeq& publication_handles)
    ACE_THROW_SPEC((CORBA::SystemException));

  DDS::ReturnCode_t get_matched_publication_data(
    DDS::PublicationBuiltinTopicData& publication_data,
    DDS::InstanceHandle_t publication_handle)
    ACE_THROW_SPEC((CORBA::SystemException));

  void get_latency_stats(LatencyStatisticsSeq& stats)
    ACE_THROW_SPEC((CORBA::SystemException));

  void reset_latency_stats()
    ACE_THROW_SPEC((CORBA::SystemException));

  CORBA::Boolean statistics_enabled()
    ACE_THROW_SPEC((CORBA::SystemException));

  void statistics_enabled(CORBA::Boolean statistics_enabled)
    ACE_THROW_SPEC((CORBA::SystemException));

private:
  virtual void init_typed(DataReaderEx* dr) = 0;
  virtual const MetaStruct& getResultingMeta() = 0;

  class Listener
    : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
  public:
    explicit Listener(MultiTopicDataReaderBase* outer)
      : outer_(outer)
    {}

    void on_requested_deadline_missed(DDS::DataReader_ptr reader,
      const DDS::RequestedDeadlineMissedStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_requested_incompatible_qos(DDS::DataReader_ptr reader,
      const DDS::RequestedIncompatibleQosStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_sample_rejected(DDS::DataReader_ptr reader,
      const DDS::SampleRejectedStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_liveliness_changed(DDS::DataReader_ptr reader,
      const DDS::LivelinessChangedStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_data_available(DDS::DataReader_ptr reader)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_subscription_matched(DDS::DataReader_ptr reader,
      const DDS::SubscriptionMatchedStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

    void on_sample_lost(DDS::DataReader_ptr reader,
      const DDS::SampleLostStatus& status)
      ACE_THROW_SPEC((CORBA::SystemException));

  private:
    MultiTopicDataReaderBase* outer_;
  };

  DDS::DataReaderListener_var listener_;
  DataReaderEx_var resulting_reader_;
  std::vector<DDS::DataReader_var> incoming_readers_;

  // key: topicName of incoming datareader
  typedef MultiTopicImpl::SubjectFieldSpec SubjectFieldSpec;
  std::multimap<std::string, SubjectFieldSpec> field_map_;
};

}
}

#endif
#endif
