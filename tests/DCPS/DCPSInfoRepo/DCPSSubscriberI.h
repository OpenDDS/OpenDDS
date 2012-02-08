#ifndef DCPSSUBSCRIBERI_H_
#define DCPSSUBSCRIBERI_H_

#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DdsDcpsInfrastructureTypeSupportImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

//Class TAO_DDS_DCPSSubscriber_i
class TAO_DDS_DCPSSubscriber_i
  : public virtual OpenDDS::DCPS::LocalObject< ::DDS::Subscriber>
{
public:
  TAO_DDS_DCPSSubscriber_i() {}

  virtual ~TAO_DDS_DCPSSubscriber_i() {}

  virtual DDS::InstanceHandle_t get_instance_handle()
    ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::DataReader_ptr create_datareader(
    DDS::TopicDescription_ptr a_topic_desc,
    const DDS::DataReaderQos& qos,
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask)
    ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::DataReader_ptr create_opendds_datareader(
    DDS::TopicDescription_ptr a_topic_desc,
    const DDS::DataReaderQos& qos,
    const OpenDDS::DCPS::DataReaderQosExt& ext_qos,
    DDS::DataReaderListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::ReturnCode_t delete_datareader(
    DDS::DataReader_ptr a_datareader)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t delete_contained_entities()
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::DataReader_ptr lookup_datareader(
    const char* topic_name)
    ACE_THROW_SPEC((CORBA::SystemException))
  {
/*    if (ACE_OS::strcmp(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC, topic_name) == 0)
      {
        return DDS::DataReader::_duplicate(&subBIT_);
      }
    else if (ACE_OS::strcmp(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC, topic_name) == 0)
      {
        return DDS::DataReader::_duplicate(&pubBIT_);
      }
*/
    return 0;
  }

  virtual DDS::ReturnCode_t get_datareaders(
    DDS::DataReaderSeq& readers,
    DDS::SampleStateMask sample_states,
    DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t notify_datareaders()
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t set_qos(
    const DDS::SubscriberQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t get_qos(
    DDS::SubscriberQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t set_listener(
    DDS::SubscriberListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::SubscriberListener_ptr get_listener()
  ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::ReturnCode_t begin_access()
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t end_access()
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::DomainParticipant_ptr get_participant()
  ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::ReturnCode_t set_default_datareader_qos(
    const DDS::DataReaderQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t get_default_datareader_qos(
    DDS::DataReaderQos& qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual void get_default_datareader_qos_ext(
    OpenDDS::DCPS::DataReaderQosExt& qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { }

  virtual DDS::ReturnCode_t copy_from_topic_qos(
    DDS::DataReaderQos& a_datareader_qos,
    const DDS::TopicQos& a_topic_qos)
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::ReturnCode_t enable()
  ACE_THROW_SPEC((CORBA::SystemException)) { return ::DDS::RETCODE_OK; }

  virtual DDS::StatusCondition_ptr get_statuscondition()
    ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

  virtual DDS::StatusMask get_status_changes()
    ACE_THROW_SPEC((CORBA::SystemException)) { return 0; }

/*  DDS::SubscriptionBuiltinTopicDataDataReaderImpl subBIT_;
  DDS::PublicationBuiltinTopicDataDataReaderImpl pubBIT_;*/
};


#endif /* DCPSSUBSCRIBERI_H_  */
