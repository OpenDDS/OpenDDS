#ifndef TEST_DDS_DCPS_MOCK_TOPIC_H
#define TEST_DDS_DCPS_MOCK_TOPIC_H

#include <gmock/gmock.h>

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/EntityImpl.h"
#include "dds/DCPS/Qos_Helper.h"

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

class MockTopic
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Topic>
  , public virtual OpenDDS::DCPS::EntityImpl {
public:
  MOCK_METHOD0(enable, DDS::ReturnCode_t(void));
  MOCK_METHOD0(get_instance_handle, DDS::InstanceHandle_t(void));
  MOCK_METHOD0(get_type_name, char*(void));
  MOCK_METHOD0(get_name, char*(void));
  MOCK_METHOD0(get_participant, DDS::DomainParticipant_ptr(void));
  MOCK_METHOD1(set_qos, DDS::ReturnCode_t(const DDS::TopicQos & qos));
  MOCK_METHOD1(get_qos, DDS::ReturnCode_t(DDS::TopicQos & qos));
  MOCK_METHOD2(set_listener, DDS::ReturnCode_t(DDS::TopicListener_ptr a_listener,
                                               DDS::StatusMask mask));
  MOCK_METHOD0(get_listener, DDS::TopicListener_ptr(void));
  MOCK_METHOD1(get_inconsistent_topic_status, DDS::ReturnCode_t(DDS::InconsistentTopicStatus & a_status));
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // TEST_DDS_DCPS_MOCK_PUBLISHER_H
