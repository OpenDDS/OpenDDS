#ifndef DCPSSUBSCRIBERI_H_
#define DCPSSUBSCRIBERI_H_

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class TAO_DDS_DCPSSubscriber_i
  : public virtual OpenDDS::DCPS::LocalObject< ::DDS::Subscriber>
{
public:
  TAO_DDS_DCPSSubscriber_i() {}

  virtual ~TAO_DDS_DCPSSubscriber_i() {}

  virtual DDS::InstanceHandle_t get_instance_handle() { return 0; }

  virtual DDS::DataReader_ptr create_datareader(
    DDS::TopicDescription_ptr /* a_topic_desc */,
    const DDS::DataReaderQos& /* qos */,
    DDS::DataReaderListener_ptr /* a_listener */,
    DDS::StatusMask /* mask */) { return 0; }

  virtual DDS::ReturnCode_t delete_datareader(
    DDS::DataReader_ptr /* a_datareader */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t delete_contained_entities() { return ::DDS::RETCODE_OK; }

  virtual DDS::DataReader_ptr lookup_datareader(
    const char* /* topic_name */)
  {
    return 0;
  }

  virtual DDS::ReturnCode_t get_datareaders(
    DDS::DataReaderSeq& /* readers */,
    DDS::SampleStateMask /* sample_states */,
    DDS::ViewStateMask /* view_states */,
    DDS::InstanceStateMask /* instance_states */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t notify_datareaders() { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t set_qos(
    const DDS::SubscriberQos& /* qos */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t get_qos(
    DDS::SubscriberQos& /* qos */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t set_listener(
    DDS::SubscriberListener_ptr /* a_listener */,
    DDS::StatusMask /* mask */) { return ::DDS::RETCODE_OK; }

  virtual DDS::SubscriberListener_ptr get_listener() { return 0; }

  virtual DDS::ReturnCode_t begin_access() { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t end_access() { return ::DDS::RETCODE_OK; }

  virtual DDS::DomainParticipant_ptr get_participant() { return 0; }

  virtual DDS::ReturnCode_t set_default_datareader_qos(
    const DDS::DataReaderQos& /* qos */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t get_default_datareader_qos(
    DDS::DataReaderQos& /* qos */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t copy_from_topic_qos(
    DDS::DataReaderQos& /* a_datareader_qos */,
    const DDS::TopicQos& /* a_topic_qos */) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t enable() { return ::DDS::RETCODE_OK; }

  virtual DDS::StatusCondition_ptr get_statuscondition() { return 0; }

  virtual DDS::StatusMask get_status_changes() { return 0; }

};


#endif /* DCPSSUBSCRIBERI_H_  */
