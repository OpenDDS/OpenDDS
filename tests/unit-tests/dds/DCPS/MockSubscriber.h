#ifndef TEST_DDS_DCPS_MOCK_SUBSCRIBER_H
#define TEST_DDS_DCPS_MOCK_SUBSCRIBER_H

#include <gmock/gmock.h>

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/EntityImpl.h"
#include "dds/DCPS/Qos_Helper.h"

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

class MockSubscriber
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Subscriber>
  , public virtual OpenDDS::DCPS::EntityImpl {
public:
  MOCK_METHOD0(enable, DDS::ReturnCode_t(void));
  MOCK_METHOD0(get_instance_handle, DDS::InstanceHandle_t(void));
  MOCK_METHOD4(create_datareader, DDS::DataReader_ptr(DDS::TopicDescription_ptr a_topic,
                                                      const DDS::DataReaderQos & qos,
                                                      DDS::DataReaderListener_ptr a_listener,
                                                      DDS::StatusMask mask));
  MOCK_METHOD1(delete_datareader, DDS::ReturnCode_t(DDS::DataReader_ptr a_datareader));
  MOCK_METHOD0(delete_contained_entities, DDS::ReturnCode_t(void));
  MOCK_METHOD1(lookup_datareader, DDS::DataReader_ptr(const char * topic_name));
  MOCK_METHOD4(get_datareaders, DDS::ReturnCode_t(DDS::DataReaderSeq & readers,
                                                  DDS::SampleStateMask sample_states,
                                                  DDS::ViewStateMask view_states,
                                                  DDS::InstanceStateMask instance_states));
  MOCK_METHOD0(notify_datareaders, DDS::ReturnCode_t(void));
  MOCK_METHOD1(set_qos, DDS::ReturnCode_t(const DDS::SubscriberQos & qos));
  MOCK_METHOD1(get_qos, DDS::ReturnCode_t(DDS::SubscriberQos & qos));
  MOCK_METHOD2(set_listener, DDS::ReturnCode_t(DDS::SubscriberListener_ptr a_listener,
                                               DDS::StatusMask mask));
  MOCK_METHOD0(get_listener, DDS::SubscriberListener_ptr(void));
  MOCK_METHOD0(begin_access, DDS::ReturnCode_t(void));
  MOCK_METHOD0(end_access, DDS::ReturnCode_t(void));
  MOCK_METHOD0(get_participant, DDS::DomainParticipant_ptr(void));
  MOCK_METHOD1(set_default_datareader_qos, DDS::ReturnCode_t(const DDS::DataReaderQos & qos));
  MOCK_METHOD1(get_default_datareader_qos, DDS::ReturnCode_t(DDS::DataReaderQos & qos));
  MOCK_METHOD2(copy_from_topic_qos, DDS::ReturnCode_t(DDS::DataReaderQos & a_datareader_qos,
                                                      const DDS::TopicQos & a_topic_qos));
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // TEST_DDS_DCPS_MOCK_SUBSCRIBER_H
