/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Discovery.h"
#include "Service_Participant.h"
#include "BuiltInTopicUtils.h"
#include "Registered_Data_Types.h"
#include "DdsDcpsCoreC.h"
#include "Marked_Default_Qos.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#ifndef DDS_HAS_MINIMUM_BIT
#include "DdsDcpsCoreTypeSupportImpl.h"
#endif /* DDS_HAS_MINIMUM_BIT */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char* Discovery::DEFAULT_REPO = "DEFAULT_REPO";
const char* Discovery::DEFAULT_RTPS = "DEFAULT_RTPS";
const char* Discovery::DEFAULT_STATIC = "DEFAULT_STATIC";

DDS::ReturnCode_t
Discovery::create_bit_topics(DomainParticipantImpl* participant)
{
#ifndef DDS_HAS_MINIMUM_BIT

  TypeSupport_var type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_PARTICIPANT_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    // Participant topic
    DDS::ParticipantBuiltinTopicDataTypeSupport_var ts =
      new DDS::ParticipantBuiltinTopicDataTypeSupportImpl;

    DDS::ReturnCode_t ret = ts->register_type(participant,
                                              BUILT_IN_PARTICIPANT_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("Discovery::create_bit_topics, ")
                        ACE_TEXT("register BUILT_IN_PARTICIPANT_TOPIC_TYPE returned %d.\n"),
                        ret),
                       ret);
    }
  }

  DDS::Topic_var bit_part_topic =
    participant->create_topic(BUILT_IN_PARTICIPANT_TOPIC,
                              BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_part_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("Discovery::create_bit_topics, ")
                      ACE_TEXT("Nil %C Topic\n"),
                      BUILT_IN_PARTICIPANT_TOPIC),
                     DDS::RETCODE_ERROR);
  }

  // Participant location topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    OpenDDS::DCPS::ParticipantLocationBuiltinTopicDataTypeSupport_var ts =
      new OpenDDS::DCPS::ParticipantLocationBuiltinTopicDataTypeSupportImpl;

    DDS::ReturnCode_t ret = ts->register_type(participant,
      BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ")
        ACE_TEXT("Discovery::create_bit_topics, ")
        ACE_TEXT("register BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE returned %C.\n"),
        retcode_to_string(ret)),
        ret);
    }
  }

  DDS::Topic_var bit_part_loc_topic =
    participant->create_topic(BUILT_IN_PARTICIPANT_LOCATION_TOPIC,
      BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE,
      TOPIC_QOS_DEFAULT,
      DDS::TopicListener::_nil(),
      DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_part_loc_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) ")
      ACE_TEXT("Discovery::create_bit_topics, ")
      ACE_TEXT("Nil %C Topic\n"),
      BUILT_IN_PARTICIPANT_LOCATION_TOPIC),
      DDS::RETCODE_ERROR);
  }

  // Topic topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_TOPIC_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    DDS::TopicBuiltinTopicDataTypeSupport_var ts =
      new DDS::TopicBuiltinTopicDataTypeSupportImpl;

    DDS::ReturnCode_t ret = ts->register_type(participant,
                                              BUILT_IN_TOPIC_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("Discovery::create_bit_topics, ")
                        ACE_TEXT("register BUILT_IN_TOPIC_TOPIC_TYPE returned %d.\n"),
                        ret),
                       ret);
    }
  }

  DDS::Topic_var bit_topic_topic =
    participant->create_topic(BUILT_IN_TOPIC_TOPIC,
                              BUILT_IN_TOPIC_TOPIC_TYPE,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_topic_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("Discovery::create_bit_topics, ")
                      ACE_TEXT("Nil %C Topic\n"),
                      BUILT_IN_TOPIC_TOPIC),
                     DDS::RETCODE_ERROR);
  }

  // Subscription topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    DDS::SubscriptionBuiltinTopicDataTypeSupport_var ts =
      new DDS::SubscriptionBuiltinTopicDataTypeSupportImpl;

    DDS::ReturnCode_t ret = ts->register_type(participant,
                                              BUILT_IN_SUBSCRIPTION_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("Discovery::create_bit_topics, ")
                        ACE_TEXT("register BUILT_IN_SUBSCRIPTION_TOPIC_TYPE returned %d.\n"),
                        ret),
                       ret);
    }
  }

  DDS::Topic_var bit_sub_topic =
    participant->create_topic(BUILT_IN_SUBSCRIPTION_TOPIC,
                              BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_sub_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("Discovery::create_bit_topics, ")
                      ACE_TEXT("Nil %C Topic\n"),
                      BUILT_IN_SUBSCRIPTION_TOPIC),
                     DDS::RETCODE_ERROR);
  }

  // Publication topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_PUBLICATION_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    DDS::PublicationBuiltinTopicDataTypeSupport_var ts =
      new DDS::PublicationBuiltinTopicDataTypeSupportImpl;

    DDS::ReturnCode_t ret = ts->register_type(participant,
                                              BUILT_IN_PUBLICATION_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("Discovery::create_bit_topics, ")
                        ACE_TEXT("register BUILT_IN_PUBLICATION_TOPIC_TYPE returned %d.\n"),
                        ret),
                       ret);
    }
  }

  DDS::Topic_var bit_pub_topic =
    participant->create_topic(BUILT_IN_PUBLICATION_TOPIC,
                              BUILT_IN_PUBLICATION_TOPIC_TYPE,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_pub_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: Discovery::create_bit_topics, ")
                      ACE_TEXT("Nil %C Topic\n"),
                      BUILT_IN_PUBLICATION_TOPIC),
                     DDS::RETCODE_ERROR);
  }


  bit_part_topic->enable();
  bit_topic_topic->enable();
  bit_sub_topic->enable();
  bit_pub_topic->enable();

  bit_part_loc_topic->enable();

#else
  ACE_UNUSED_ARG(participant);
#endif /* DDS_HAS_MINIMUM_BIT */

  return DDS::RETCODE_OK;
}

Discovery::Config::~Config()
{
}

void Discovery::update_publication_locators(DDS::DomainId_t,
                                            const RepoId&,
                                            const RepoId&,
                                            const TransportLocatorSeq&)
{}

void Discovery::update_subscription_locators(DDS::DomainId_t,
                                             const RepoId&,
                                             const RepoId&,
                                             const TransportLocatorSeq&)
{}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
