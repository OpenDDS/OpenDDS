#ifndef TEST_DDS_DCPS_MOCK_PUBLISHER_H
#define TEST_DDS_DCPS_MOCK_PUBLISHER_H

#include <gmock/gmock.h>

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/EntityImpl.h"
#include "dds/DCPS/Qos_Helper.h"

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

class MockPublisher
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Publisher>
  , public virtual OpenDDS::DCPS::EntityImpl {
public:
  MOCK_METHOD0(enable, DDS::ReturnCode_t(void));
  MOCK_METHOD0(get_instance_handle, DDS::InstanceHandle_t(void));
  MOCK_METHOD4(create_datawriter, DDS::DataWriter_ptr(DDS::Topic_ptr a_topic,
                                                      const DDS::DataWriterQos & qos,
                                                      DDS::DataWriterListener_ptr a_listener,
                                                      DDS::StatusMask mask));
  MOCK_METHOD1(delete_datawriter, DDS::ReturnCode_t(DDS::DataWriter_ptr a_datawriter));
  MOCK_METHOD1(lookup_datawriter, DDS::DataWriter_ptr(const char * topic_name));
  MOCK_METHOD0(delete_contained_entities, DDS::ReturnCode_t(void));
  MOCK_METHOD1(set_qos, DDS::ReturnCode_t(const DDS::PublisherQos & qos));
  MOCK_METHOD1(get_qos, DDS::ReturnCode_t(DDS::PublisherQos & qos));
  MOCK_METHOD2(set_listener, DDS::ReturnCode_t(DDS::PublisherListener_ptr a_listener,
                                               DDS::StatusMask mask));
  MOCK_METHOD0(get_listener, DDS::PublisherListener_ptr(void));
  MOCK_METHOD0(suspend_publications, DDS::ReturnCode_t(void));
  MOCK_METHOD0(resume_publications, DDS::ReturnCode_t(void));
  MOCK_METHOD0(begin_coherent_changes, DDS::ReturnCode_t(void));
  MOCK_METHOD0(end_coherent_changes, DDS::ReturnCode_t(void));
  MOCK_METHOD1(wait_for_acknowledgments, DDS::ReturnCode_t(const DDS::Duration_t & max_wait));
  MOCK_METHOD0(get_participant, DDS::DomainParticipant_ptr(void));
  MOCK_METHOD1(set_default_datawriter_qos, DDS::ReturnCode_t(const DDS::DataWriterQos & qos));
  MOCK_METHOD1(get_default_datawriter_qos, DDS::ReturnCode_t(DDS::DataWriterQos & qos));
  MOCK_METHOD2(copy_from_topic_qos, DDS::ReturnCode_t(DDS::DataWriterQos & a_datawriter_qos,
                                                      const DDS::TopicQos & a_topic_qos));
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // TEST_DDS_DCPS_MOCK_PUBLISHER_H
