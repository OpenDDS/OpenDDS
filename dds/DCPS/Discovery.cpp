/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "Discovery.h"

#include "BuiltInTopicUtils.h"
#include "DCPS_Utils.h"
#include "Definitions.h"
#include "Marked_Default_Qos.h"
#include "Registered_Data_Types.h"
#include "SafetyProfileStreams.h"
#include "Service_Participant.h"

#include <dds/DdsDcpsCoreC.h>
#include <dds/OpenddsDcpsExtC.h>
#if OPENDDS_CONFIG_BUILT_IN_TOPICS
#  include <dds/DdsDcpsCoreTypeSupportImpl.h>
#  include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DDS::ReturnCode_t TypeObjReqCond::wait()
{
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  while (waiting) {
    cond.wait(thread_status_manager);
  }
  return rc;
}

void TypeObjReqCond::done(DDS::ReturnCode_t retcode)
{
  ACE_GUARD(LockType, g, lock);
  waiting = false;
  rc = retcode;
  cond.notify_all();
}

TypeInformation::TypeInformation()
  : flags_(Flags_None)
{}

TypeInformation::TypeInformation(const XTypes::TypeInformation& typeinfo)
  : xtypes_type_info_(typeinfo)
  , flags_(Flags_None)
{}

const char* Discovery::DEFAULT_REPO = "DEFAULT_REPO";
const char* Discovery::DEFAULT_RTPS = "DEFAULT_RTPS";
const char* Discovery::DEFAULT_STATIC = "DEFAULT_STATIC";

DDS::ReturnCode_t
Discovery::create_bit_topics(DomainParticipantImpl* participant)
{
#if OPENDDS_CONFIG_BUILT_IN_TOPICS

  TypeSupport_var type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_PARTICIPANT_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    // Participant topic
    DDS::ParticipantBuiltinTopicDataTypeSupport_var ts =
      new DDS::ParticipantBuiltinTopicDataTypeSupportImpl;

    const DDS::ReturnCode_t ret = ts->register_type(participant,
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

    const DDS::ReturnCode_t ret = ts->register_type(participant,
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

  // Internal thread status topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    OpenDDS::DCPS::InternalThreadBuiltinTopicDataTypeSupport_var ts =
      new OpenDDS::DCPS::InternalThreadBuiltinTopicDataTypeSupportImpl;

    const DDS::ReturnCode_t ret = ts->register_type(participant,
                                                    BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ")
        ACE_TEXT("Discovery::create_bit_topics, ")
        ACE_TEXT("register BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE returned %C.\n"),
        retcode_to_string(ret)),
        ret);
    }
  }

  DDS::Topic_var bit_internal_thread_topic =
    participant->create_topic(BUILT_IN_INTERNAL_THREAD_TOPIC,
      BUILT_IN_INTERNAL_THREAD_TOPIC_TYPE,
      TOPIC_QOS_DEFAULT,
      DDS::TopicListener::_nil(),
      DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_internal_thread_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) ")
      ACE_TEXT("Discovery::create_bit_topics, ")
      ACE_TEXT("Nil %C Topic\n"),
      BUILT_IN_INTERNAL_THREAD_TOPIC),
      DDS::RETCODE_ERROR);
  }

    // Connection Record Topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    OpenDDS::DCPS::ConnectionRecordTypeSupport_var ts =
      new OpenDDS::DCPS::ConnectionRecordTypeSupportImpl;

    const DDS::ReturnCode_t ret = ts->register_type(participant,
                                                    BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE);

    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ")
        ACE_TEXT("Discovery::create_bit_topics, ")
        ACE_TEXT("register BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE returned %C.\n"),
        retcode_to_string(ret)),
        ret);
    }
  }

  DDS::Topic_var bit_connection_record_topic =
    participant->create_topic(BUILT_IN_CONNECTION_RECORD_TOPIC,
      BUILT_IN_CONNECTION_RECORD_TOPIC_TYPE,
      TOPIC_QOS_DEFAULT,
      DDS::TopicListener::_nil(),
      DEFAULT_STATUS_MASK);

  if (CORBA::is_nil(bit_connection_record_topic)) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) ")
      ACE_TEXT("Discovery::create_bit_topics, ")
      ACE_TEXT("Nil %C Topic\n"),
      BUILT_IN_CONNECTION_RECORD_TOPIC),
      DDS::RETCODE_ERROR);
  }

  // Topic topic
  type_support =
    Registered_Data_Types->lookup(participant, BUILT_IN_TOPIC_TOPIC_TYPE);

  if (CORBA::is_nil(type_support)) {
    DDS::TopicBuiltinTopicDataTypeSupport_var ts =
      new DDS::TopicBuiltinTopicDataTypeSupportImpl;

    const DDS::ReturnCode_t ret = ts->register_type(participant,
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

    const DDS::ReturnCode_t ret = ts->register_type(participant,
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

    const DDS::ReturnCode_t ret = ts->register_type(participant,
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
  bit_connection_record_topic->enable();
  bit_internal_thread_topic->enable();

#else
  ACE_UNUSED_ARG(participant);
#endif

  return DDS::RETCODE_OK;
}

Discovery::Config::~Config()
{
}

void Discovery::update_publication_locators(DDS::DomainId_t,
                                            const GUID_t&,
                                            const GUID_t&,
                                            const TransportLocatorSeq&)
{}

void Discovery::update_subscription_locators(DDS::DomainId_t,
                                             const GUID_t&,
                                             const GUID_t&,
                                             const TransportLocatorSeq&)
{}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
