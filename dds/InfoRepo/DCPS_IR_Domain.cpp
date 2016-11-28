/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Domain.h"

#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include "DomainParticipantListener_i.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/InfoRepoDiscovery/InfoC.h"
#include "dds/DCPS/RepoIdConverter.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/Transient_Kludge.h"

#include /**/ "tao/debug.h"

#include <algorithm>
#include <sstream>

namespace { // Anonymous namespace for predicate functor definitions.

/// obj( description) == true if description.topic_ == topic;
class IsTheTopic {
public:
  IsTheTopic(const char* topic) : topic_(topic) { }

  bool operator()(DCPS_IR_Topic_Description* description) {
    return (0 == ACE_OS::strcmp(this->topic_, description->get_name()));
  }

private:
  const char* topic_;
};

} // End of anonymous namespace.

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

DCPS_IR_Domain::DCPS_IR_Domain(DDS::DomainId_t id, OpenDDS::DCPS::RepoIdGenerator& generator)
  : id_(id),
    participantIdGenerator_(generator),
    useBIT_(false)
{
}

DCPS_IR_Domain::~DCPS_IR_Domain()
{
}

const DCPS_IR_Participant_Map&
DCPS_IR_Domain::participants() const
{
  return this->participants_;
}

DCPS_IR_Participant*
DCPS_IR_Domain::participant(const OpenDDS::DCPS::RepoId& id) const
{
  DCPS_IR_Participant_Map::const_iterator where
  = this->participants_.find(id);

  if (where != this->participants_.end()) {
    return where->second;

  } else {
    return 0;
  }
}

int DCPS_IR_Domain::add_participant(DCPS_IR_Participant* participant)
{
  OpenDDS::DCPS::RepoId participantId = participant->get_id();
  OpenDDS::DCPS::RepoIdConverter converter(participantId);

  DCPS_IR_Participant_Map::iterator where
  = this->participants_.find(participantId);

  if (where == this->participants_.end()) {
    this->participants_.insert(
      DCPS_IR_Participant_Map::value_type(participantId, participant));

    // Publish the BIT information
    publish_participant_bit(participant);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Domain::add_participant: ")
                 ACE_TEXT("added participant %C in domain %d ")
                 ACE_TEXT("at 0x%x.\n"),
                 std::string(converter).c_str(),
                 id_,
                 participant));
    }

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Domain::add_participant: ")
                 ACE_TEXT("attempt to add already existing participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 id_));
    }

    return 1;
  }

  return 0;
}

int DCPS_IR_Domain::remove_participant(const OpenDDS::DCPS::RepoId& participantId,
                                       CORBA::Boolean notify_lost)
{
  DCPS_IR_Participant_Map::iterator where
  = this->participants_.find(participantId);

  if (where != this->participants_.end()) {
    // Extract the participant from the map.
    DCPS_IR_Participant* participant = where->second;

    // make sure the participant has cleaned up all publications,
    // subscriptions, and any topic references
    participant->remove_all_dependents(notify_lost);

    // Then remove it from the map.
    this->participants_.erase(where);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Domain::remove_participant: ")
                 ACE_TEXT("removed participant %C at 0x%x from domain %d.\n"),
                 std::string(converter).c_str(),
                 participant,
                 id_));
    }

    dispose_participant_bit(participant);
    delete participant;
    return 0;

  } else {
    OpenDDS::DCPS::RepoIdConverter converter(participantId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::remove_participant: ")
               ACE_TEXT("unable to find participant %C in domain %d.\n"),
               std::string(converter).c_str(),
               id_));
    return 1;
  }
}

OpenDDS::DCPS::TopicStatus DCPS_IR_Domain::add_topic(OpenDDS::DCPS::RepoId_out topicId,
                                                     const char * topicName,
                                                     const char * dataTypeName,
                                                     const DDS::TopicQos & qos,
                                                     DCPS_IR_Participant* participantPtr)
{
  topicId = OpenDDS::DCPS::GUID_UNKNOWN;

  OpenDDS::DCPS::RepoId topic_id = participantPtr->get_next_topic_id();
  OpenDDS::DCPS::TopicStatus status = add_topic_i(topic_id, topicName
                                                  , dataTypeName
                                                  , qos, participantPtr);

  if (status == OpenDDS::DCPS::CREATED) {

    topicId = topic_id;
  }

  return status;
}

OpenDDS::DCPS::TopicStatus
DCPS_IR_Domain::force_add_topic(const OpenDDS::DCPS::RepoId& topicId,
                                const char* topicName,
                                const char* dataTypeName,
                                const DDS::TopicQos & qos,
                                DCPS_IR_Participant* participantPtr)
{
  OpenDDS::DCPS::RepoId topic_id = topicId;
  OpenDDS::DCPS::TopicStatus status = add_topic_i(topic_id, topicName
                                                  , dataTypeName
                                                  , qos, participantPtr);

  return status;
}

OpenDDS::DCPS::TopicStatus DCPS_IR_Domain::add_topic_i(OpenDDS::DCPS::RepoId& topicId,
                                                       const char * topicName,
                                                       const char * dataTypeName,
                                                       const DDS::TopicQos & qos,
                                                       DCPS_IR_Participant* participantPtr)
{
  DCPS_IR_Topic_Description* description;
  int descriptionLookup = find_topic_description(topicName, dataTypeName, description);

  if (1 == descriptionLookup) {
    topicId = OpenDDS::DCPS::GUID_UNKNOWN;
    return OpenDDS::DCPS::CONFLICTING_TYPENAME;

  } else if (-1 == descriptionLookup) {
    ACE_NEW_RETURN(description,
                   DCPS_IR_Topic_Description(
                     this,
                     topicName,
                     dataTypeName),
                   OpenDDS::DCPS::NOT_FOUND);

    int descriptionAddition = add_topic_description(description);

    if (0 != descriptionAddition) {
      // unable to add the topic
      delete description;
      description = 0;
      topicId = OpenDDS::DCPS::GUID_UNKNOWN;

      if (2 == descriptionAddition) {
        return OpenDDS::DCPS::CONFLICTING_TYPENAME;

      } else {
        return OpenDDS::DCPS::NOT_FOUND;
      }
    }
  }

  DCPS_IR_Topic* topic;
  ACE_NEW_RETURN(topic,
                 DCPS_IR_Topic(
                   topicId,
                   qos,
                   this,
                   participantPtr,
                   description),
                 OpenDDS::DCPS::NOT_FOUND);

  OpenDDS::DCPS::TopicStatus topicStatus = OpenDDS::DCPS::NOT_FOUND;

  switch (description->add_topic(topic)) {
  case 0: {
    switch (participantPtr->add_topic_reference(topic)) {
    case 0: {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter converter(topicId);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Domain::add_topic_i: ")
                   ACE_TEXT("Domain %d successfully added topic %C ")
                   ACE_TEXT("at 0x%x.\n"),
                   this->id_,
                   std::string(converter).c_str(),
                   topic));
      }

      topicStatus = OpenDDS::DCPS::CREATED;

      // Keep a reference to easily locate the topic by id.
      this->idToTopicMap_[ topicId] = topic;

      // Publish the BIT information
      publish_topic_bit(topic);
    }
    break;

    case 1:

      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        OpenDDS::DCPS::RepoIdConverter converter(topicId);
        ACE_DEBUG((LM_NOTICE,
                   ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Domain::add_topic_i: ")
                   ACE_TEXT("Domain %d declined to add duplicate topic %C at 0x%x.\n"),
                   this->id_,
                   std::string(converter).c_str(),
                   topic));
      }

      topicStatus = OpenDDS::DCPS::NOT_FOUND;
      topicId = OpenDDS::DCPS::GUID_UNKNOWN;
      description->remove_topic(topic);
      delete topic;
      break;

    case -1: {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::add_topic_i: ")
                 ACE_TEXT("Domain %d failed to add topic %C at 0x%x.\n"),
                 this->id_,
                 std::string(converter).c_str(),
                 topic));
      topicStatus = OpenDDS::DCPS::NOT_FOUND;
      topicId = OpenDDS::DCPS::GUID_UNKNOWN;
      description->remove_topic(topic);
      delete topic;
    }
    break;
    }
  }
  break;

  case 1:

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) WARNING: DCPS_IR_Domain::add_topic ")
                 ACE_TEXT("Unable to add topic 0x%x id %C to Topic Description\n"),
                 topic,
                 std::string(converter).c_str()));
    }

    topicStatus = OpenDDS::DCPS::NOT_FOUND;
    topicId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete topic;
    break;

  case -1: {
    OpenDDS::DCPS::RepoIdConverter converter(topicId);
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::add_topic ")
               ACE_TEXT("Unable to add topic 0x%x id %C to Topic Description\n"),
               topic,
               std::string(converter).c_str()));
    topicStatus = OpenDDS::DCPS::NOT_FOUND;
    topicId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete topic;
  }
  break;
  }

  return topicStatus;
}

OpenDDS::DCPS::TopicStatus
DCPS_IR_Domain::find_topic(const char* topicName, DCPS_IR_Topic*& topic)
{
  IsTheTopic isTheTopic(topicName);
  DCPS_IR_Topic_Description_Set::iterator which
  = std::find_if(
      this->topicDescriptions_.begin(),
      this->topicDescriptions_.end(),
      isTheTopic);

  if (which != this->topicDescriptions_.end()) {
    // Extract the topic from the description.
    topic = (*which)->get_first_topic();

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoId topicId = topic->get_id();
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Domain::find_topic: ")
                 ACE_TEXT("located topic %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 id_));
    }

    return OpenDDS::DCPS::FOUND;

  } else {
    // topic = 0;
    return OpenDDS::DCPS::NOT_FOUND;
  }
}

DCPS_IR_Topic*
DCPS_IR_Domain::find_topic(const OpenDDS::DCPS::RepoId& id)
{
  IdToTopicMap::const_iterator location = this->idToTopicMap_.find(id);

  if (location == this->idToTopicMap_.end()) {
    return 0;
  }

  return location->second;
}

OpenDDS::DCPS::TopicStatus DCPS_IR_Domain::remove_topic(DCPS_IR_Participant* part,
                                                        DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Description* description = topic->get_topic_description();

  if (description->remove_topic(topic) != 0) {
    // An unknown error means that the description may still
    // have the topic in its topic set.  We can't remove it.
    // The system is an inconsistent state!
    throw OpenDDS::DCPS::Invalid_Topic();
  }

  if (description->get_number_topics() == 0) {
    // Remove the topic description
    if (remove_topic_description(description) != 0) {
      // An unknown error means that the description may still
      // have the topic in its topic set.
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Topic Description %C %C  ")
                 ACE_TEXT("was not correctly removed from Domain %d"),
                 description->get_name(),
                 description->get_dataTypeName(),
                 id_));

    } else {
      delete description;
      description = 0;
    }
  }

  // description variable is invalid after this point

  if (part->remove_topic_reference(topic->get_id(), topic) != 0) {
    OpenDDS::DCPS::RepoIdConverter part_converter(part->get_id());
    OpenDDS::DCPS::RepoIdConverter topic_converter(topic->get_id());
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Domain %d Topic %C ")
               ACE_TEXT("was not correctly removed from Participant %C"),
               id_,
               std::string(topic_converter).c_str(),
               std::string(part_converter).c_str()));
  }

  // Dispose the BIT information
  dispose_topic_bit(topic);

  topic->release(true);
  topic = 0;
  return OpenDDS::DCPS::REMOVED;
}

int
DCPS_IR_Domain::find_topic_description(
  const char* name,
  const char* dataTypeName,
  DCPS_IR_Topic_Description*& desc)
{
  IsTheTopic isTheTopic(name);
  DCPS_IR_Topic_Description_Set::iterator which
  = std::find_if(
      this->topicDescriptions_.begin(),
      this->topicDescriptions_.end(),
      isTheTopic);

  if (which != this->topicDescriptions_.end()) {
    if (0 == ACE_OS::strcmp(dataTypeName, (*which)->get_dataTypeName())) {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DCPS_IR_Domain::find_topic_description: ")
                   ACE_TEXT("located topic description %C/%C in domain %d.\n"),
                   name,
                   dataTypeName,
                   id_));
      }

      desc = *which;
      return 0;

    } else {
      if (OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_NOTICE,
                   ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Domain::find_topic_description: ")
                   ACE_TEXT("searching for topic description %C/%C, ")
                   ACE_TEXT("located topic description %C/%C instead in domain %d.\n"),
                   name,
                   dataTypeName,
                   (*which)->get_name(),
                   (*which)->get_dataTypeName(),
                   id_));
      }

      // desc = 0;
      return 1;
    }

  } else {
    // desc = 0;
    return -1;
  }
}

#if defined (DDS_HAS_MINIMUM_BIT)
int DCPS_IR_Domain::init_built_in_topics(bool /* federated */)
{
  return 1;
}
#else
int DCPS_IR_Domain::init_built_in_topics(bool federated)
{
  // Indicates that BIT subscriber and datareaders should not be created.
  TheTransientKludge->enable();

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Domain::init_built_in_topics() ")
               ACE_TEXT(" Initializing Built In Topics for domain %d\n"),
               id_));
  }

  try {
    bitParticipantFactory_ = TheParticipantFactory;

    bitParticipantListener_ = new OPENDDS_DCPS_DomainParticipantListener_i;

    bitParticipant_ =
      bitParticipantFactory_->create_participant(id_,
                                                 PARTICIPANT_QOS_DEFAULT,
                                                 bitParticipantListener_.in(),
                                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitParticipant_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil DomainParticipant in ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }


    int transportResult = init_built_in_topics_transport();

    if (0 != transportResult) {
      return transportResult;
    }

    int topicsResult = init_built_in_topics_topics();

    if (0 != topicsResult) {
      return topicsResult;
    }

    int datawritersResult = init_built_in_topics_datawriters(federated);

    if (0 != datawritersResult) {
      return datawritersResult;
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("ERROR: Exception caught in main.cpp:");
    return 1;
  }

  // enable the Built-In Topics
  useBIT_ = true;
  return 0;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#if defined (DDS_HAS_MINIMUM_BIT)
int DCPS_IR_Domain::reassociate_built_in_topic_pubs()
{
  return 1;
}
#else
int DCPS_IR_Domain::reassociate_built_in_topic_pubs()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DCPS_IR_Domain::reassociate_built_in_topic_pubs() ")
               ACE_TEXT(" Re-associating Built In Topics for domain %d\n"),
               id_));
  }

  DCPS_IR_Participant_Map::iterator participantIter = participants_.begin();
  DCPS_IR_Participant_Map::iterator end = participants_.end();
  while (participantIter != end
         && !participantIter->second->isBitPublisher() ) {
    participantIter++;
  }

  if (participantIter != end) {
    for (DCPS_IR_Topic_Map::const_iterator topicIter
           = participantIter->second->topics().begin();
           topicIter != participantIter->second->topics().end();
           ++topicIter) {
      topicIter->second->reassociate_all_publications();
    }
  }

  return 0;
}
#endif // !defined (DDS_HAS_MINIMUM_BIT)

int DCPS_IR_Domain::init_built_in_topics_topics()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  try {
    DDS::TopicQos topic_qos;
    bitParticipant_->get_default_topic_qos(topic_qos);

    // Participant topic
    DDS::ParticipantBuiltinTopicDataTypeSupport_var
    participantTypeSupport(new DDS::ParticipantBuiltinTopicDataTypeSupportImpl());

    if (DDS::RETCODE_OK !=
        participantTypeSupport->register_type(bitParticipant_.in(),
                                              OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to register the ParticipantBuiltinTopicDataTypeSupport.")));
      return 1;
    }

    bitParticipantTopic_ =
      bitParticipant_->create_topic(OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                    OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                                    topic_qos,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitParticipantTopic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil %C Topic from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                        OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC),
                       1);
    }

    // Topic topic
    DDS::TopicBuiltinTopicDataTypeSupport_var
    topicTypeSupport(new DDS::TopicBuiltinTopicDataTypeSupportImpl());

    if (DDS::RETCODE_OK !=
        topicTypeSupport->register_type(bitParticipant_.in(),
                                        OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to register the TopicBuiltinTopicDataTypeSupport.")));
      return 1;
    }

    bitTopicTopic_ =
      bitParticipant_->create_topic(OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC,
                                    OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE,
                                    topic_qos,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitTopicTopic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil %C Topic from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                        OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC),
                       1);
    }

    // Subscription topic
    DDS::SubscriptionBuiltinTopicDataTypeSupport_var
    subscriptionTypeSupport(new DDS::SubscriptionBuiltinTopicDataTypeSupportImpl());

    if (DDS::RETCODE_OK !=
        subscriptionTypeSupport->register_type(bitParticipant_.in(),
                                               OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to register the SubscriptionBuiltinTopicDataTypeSupport.")));
      return 1;
    }

    bitSubscriptionTopic_ =
      bitParticipant_->create_topic(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC,
                                    OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                                    topic_qos,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitSubscriptionTopic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil %C Topic from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                        OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC),
                       1);
    }

    // Publication topic
    DDS::PublicationBuiltinTopicDataTypeSupport_var
    publicationTypeSupport(new DDS::PublicationBuiltinTopicDataTypeSupportImpl());

    if (DDS::RETCODE_OK !=
        publicationTypeSupport->register_type(bitParticipant_.in(),
                                              OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Failed to register the PublicationBuiltinTopicDataTypeSupport.")));
      return 1;
    }

    bitPublicationTopic_ =
      bitParticipant_->create_topic(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC,
                                    OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE,
                                    topic_qos,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitPublicationTopic_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil %C Topic from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                        OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC),
                       1);
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DCPS_IR_Domain::init_built_in_topics_topics:");
    return 1;
  }

  return 0;

#else

  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

#if defined (DDS_HAS_MINIMUM_BIT)
int DCPS_IR_Domain::init_built_in_topics_datawriters(bool /* federated */)
{
  return 1;
}
#else
int DCPS_IR_Domain::init_built_in_topics_datawriters(bool federated)
{

  try {
    DDS::DataWriter_var datawriter;

    DDS::DataWriterQos participantWriterQos;
    bitPublisher_->get_default_datawriter_qos(participantWriterQos);
    participantWriterQos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    if (federated) {
      participantWriterQos.liveliness.lease_duration.nanosec = 0;
      participantWriterQos.liveliness.lease_duration.sec
      = TheServiceParticipant->federation_liveliness();
    }

    // Participant DataWriter
    datawriter =
      bitPublisher_->create_datawriter(bitParticipantTopic_.in(),
                                       participantWriterQos,
                                       DDS::DataWriterListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bitParticipantDataWriter_ =
      DDS::ParticipantBuiltinTopicDataDataWriter::_narrow(datawriter.in());

    if (CORBA::is_nil(bitParticipantDataWriter_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil DomainParticipant DataWriter from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }

    DDS::DataWriterQos dw_qos;
    bitPublisher_->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    // Topic DataWriter
    datawriter =
      bitPublisher_->create_datawriter(bitTopicTopic_.in(),
                                       dw_qos,
                                       DDS::DataWriterListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bitTopicDataWriter_ =
      DDS::TopicBuiltinTopicDataDataWriter::_narrow(datawriter.in());

    if (CORBA::is_nil(bitTopicDataWriter_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil Topic DataWriter from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }

    // Subscription DataWriter
    datawriter =
      bitPublisher_->create_datawriter(bitSubscriptionTopic_.in(),
                                       dw_qos,
                                       DDS::DataWriterListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bitSubscriptionDataWriter_ =
      DDS::SubscriptionBuiltinTopicDataDataWriter::_narrow(
        datawriter.in());

    if (CORBA::is_nil(bitSubscriptionDataWriter_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil Subscription DataWriter from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }

    // Publication DataWriter
    datawriter =
      bitPublisher_->create_datawriter(bitPublicationTopic_.in(),
                                       dw_qos,
                                       DDS::DataWriterListener::_nil(),
                                       OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    bitPublicationDataWriter_ =
      DDS::PublicationBuiltinTopicDataDataWriter::_narrow(datawriter.in());

    if (CORBA::is_nil(bitPublicationDataWriter_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil Publication DataWriter from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DCPS_IR_Domain::init_built_in_topics_datawriters:");
    return 1;
  }
  return 0;
}
#endif // defined (DDS_HAS_MINIMUM_BIT)

int DCPS_IR_Domain::init_built_in_topics_transport()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  try {
    std::string config_name =
      OpenDDS::DCPS::TransportRegistry::DEFAULT_INST_PREFIX
      + std::string("InfoRepoBITTransportConfig");
    transportConfig_ =
      OpenDDS::DCPS::TransportRegistry::instance()->get_config(config_name);

    // Create the Publisher
    bitPublisher_ =
      bitParticipant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                        DDS::PublisherListener::_nil(),
                                        OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(bitPublisher_.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Nil Publisher from ")
                        ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                       1);
    }

    // Attach the Publisher with the TransportImpl.
    OpenDDS::DCPS::TransportRegistry::instance()->bind_config(transportConfig_,
                                                              bitPublisher_.in());

  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception(
      "ERROR: Exception caught in DCPS_IR_Domain::init_built_in_topics_transport:");
    return 1;
  }

  return 0;
#else

  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

int DCPS_IR_Domain::cleanup_built_in_topics()
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    // clean up the Built-in Topic objects
    bitParticipant_->delete_contained_entities();
    bitPublisher_ = 0;
    bitParticipantDataWriter_ = 0;
    bitTopicDataWriter_ = 0;
    bitSubscriptionDataWriter_ = 0;
    bitPublicationDataWriter_ = 0;
    bitParticipantTopic_ = 0;
    bitTopicTopic_ = 0;
    bitSubscriptionTopic_ = 0;
    bitPublicationTopic_ = 0;

    bitParticipantFactory_->delete_participant(bitParticipant_); // deletes this
  }

  return 0;

#else
  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

int DCPS_IR_Domain::add_topic_description(DCPS_IR_Topic_Description*& desc)
{
  DCPS_IR_Topic_Description* discard = 0;

  switch (this->find_topic_description(
            desc->get_name(),
            desc->get_dataTypeName(),
            discard)) {
  case -1:
    this->topicDescriptions_.insert(desc);

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DCPS_IR_Domain::add_topic_description: ")
                 ACE_TEXT("added Topic Description 0x%x in domain %d.\n"),
                 desc,
                 id_));
    }

    return 0;

  case 0:
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Domain::add_topic_description: ")
               ACE_TEXT("attempt to add existing Topic Description 0x%x to domain %d.\n"),
               desc,
               id_));
    return 1;

  case 1:
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE: DCPS_IR_Domain::add_topic_description: ")
               ACE_TEXT("attempt to add incompatible Topic Description 0x%x to domain %d.\n"),
               desc,
               id_));
    return 2;

  default:
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::add_topic_description: ")
               ACE_TEXT("unknown error adding Topic Description 0x%x to domain %d.\n"),
               desc,
               id_));
    return 2;
  }
}

int DCPS_IR_Domain::remove_topic_description(DCPS_IR_Topic_Description*& desc)
{
  DCPS_IR_Topic_Description_Set::iterator where
  = this->topicDescriptions_.find(desc);

  if (where != this->topicDescriptions_.end()) {
    /// @TODO: Is this a leak?  Who owns the contained description?
    // delete where->second;
    this->topicDescriptions_.erase(where);
    return 0;

  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::remove_topic_description: ")
               ACE_TEXT("unable to remove Topic Description 0x%x from domain %d.\n"),
               desc,
               id_));
    return -1;
  }
}

void DCPS_IR_Domain::add_dead_participant(DCPS_IR_Participant* participant)
{
  deadParticipants_.insert(participant);
}

void DCPS_IR_Domain::remove_dead_participants()
{
  if (0 < deadParticipants_.size()) {
    DCPS_IR_Participant* dead = 0;
    DCPS_IR_Participant_Set::ITERATOR iter = deadParticipants_.begin();

    // repeat end() due to the value possibly changing from additional dead
    // participants during notifications
    while (iter != deadParticipants_.end()) {
      dead = *iter;
      ++iter;

      OpenDDS::DCPS::RepoIdConverter converter(dead->get_id());
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::remove_dead_participants () ")
                 ACE_TEXT("Removing dead participant 0x%x id %C\n"),
                 dead,
                 std::string(converter).c_str()));
      deadParticipants_.remove(dead);

      dead->set_alive(0);

      CORBA::Boolean notify_lost = 1;
      remove_participant(dead->get_id(), notify_lost);
    }
  }
}

DDS::DomainId_t DCPS_IR_Domain::get_id()
{
  return id_;
}

OpenDDS::DCPS::RepoId
DCPS_IR_Domain::get_next_participant_id()
{
  return this->participantIdGenerator_.next();
}

void
DCPS_IR_Domain::last_participant_key(long key)
{
  this->participantIdGenerator_.last(key);
}

namespace {
  void get_BuiltinTopicKey(DDS::BuiltinTopicKey_t& key,
                           const OpenDDS::DCPS::RepoId& id)
  {
    OpenDDS::DCPS::RepoIdConverter c(id);
    key.value[0] = c.federationId();
    key.value[1] = c.participantId();
    key.value[2] = c.entityId();
  }
}

void DCPS_IR_Domain::publish_participant_bit(DCPS_IR_Participant* participant)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    if (!participant->is_bit()) {
      try {
        const DDS::DomainParticipantQos* participantQos = participant->get_qos();

        DDS::ParticipantBuiltinTopicData data;
        get_BuiltinTopicKey(data.key, participant->get_id());
        data.user_data = participantQos->user_data;

        DDS::InstanceHandle_t handle
        = bitParticipantDataWriter_->register_instance(data);

        participant->set_handle(handle);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DCPS_IR_Domain::publish_participant_bit: [ %d, 0x%x, 0x%x], handle %d.\n",
                     data.key.value[0], data.key.value[1], data.key.value[2], handle));
        }

        bitParticipantDataWriter_->write(data,
                                         handle);

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::publish_participant_bit:");
      }

    } else {
      participant->set_bit_status(1);
    }

  }

#else
  ACE_UNUSED_ARG(participant);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::publish_topic_bit(DCPS_IR_Topic* topic)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    DCPS_IR_Topic_Description* desc =
      topic->get_topic_description();
    const char* dataTypeName = desc->get_dataTypeName();

    bool isNotBIT =
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

    if (isNotBIT) {
      try {
        const DDS::TopicQos* topicQos = topic->get_topic_qos();

        DDS::TopicBuiltinTopicData data;
        get_BuiltinTopicKey(data.key, topic->get_id());
        data.name = desc->get_name();
        data.type_name = desc->get_dataTypeName();
        data.durability = topicQos->durability;
        data.durability_service = topicQos->durability_service;
        data.deadline = topicQos->deadline;
        data.latency_budget = topicQos->latency_budget;
        data.liveliness = topicQos->liveliness;
        data.reliability = topicQos->reliability;
        data.transport_priority = topicQos->transport_priority;
        data.lifespan = topicQos->lifespan;
        data.destination_order = topicQos->destination_order;
        data.history = topicQos->history;
        data.resource_limits = topicQos->resource_limits;
        data.ownership = topicQos->ownership;
        data.topic_data = topicQos->topic_data;

        DDS::InstanceHandle_t handle =
          bitTopicDataWriter_->register_instance(data);

        topic->set_handle(handle);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DCPS_IR_Domain::publish_topic_bit: [ %d, 0x%x, 0x%x], handle %d.\n",
                     data.key.value[0], data.key.value[1], data.key.value[2], handle));
        }

        bitTopicDataWriter_->write(data, handle);

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::publish_topic_bit:");
      }

    } else {
      topic->set_bit_status(1);
    }
  }

#else
  ACE_UNUSED_ARG(topic);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::publish_subscription_bit(DCPS_IR_Subscription* subscription)
{

#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    DCPS_IR_Topic_Description* desc =
      subscription->get_topic_description();

    const char* dataTypeName = desc->get_dataTypeName();

    bool isNotBIT =
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

    if (isNotBIT) {
      try {
        const DDS::DataReaderQos* readerQos = subscription->get_datareader_qos();
        const DDS::SubscriberQos* publisherQos = subscription->get_subscriber_qos();

        DCPS_IR_Topic* topic = subscription->get_topic();
        const DDS::TopicQos* topicQos = topic->get_topic_qos();

        DDS::SubscriptionBuiltinTopicData data;
        get_BuiltinTopicKey(data.key, subscription->get_id());
        get_BuiltinTopicKey(data.participant_key,
                            subscription->get_participant_id());
        data.topic_name = desc->get_name();
        data.type_name = desc->get_dataTypeName();
        data.durability = readerQos->durability;
        data.deadline = readerQos->deadline;
        data.latency_budget = readerQos->latency_budget;
        data.liveliness = readerQos->liveliness;
        data.reliability = readerQos->reliability;
        data.ownership = readerQos->ownership;
        data.destination_order = readerQos->destination_order;
        data.user_data = readerQos->user_data;
        data.time_based_filter = readerQos->time_based_filter;
        data.presentation = publisherQos->presentation;
        data.partition = publisherQos->partition;
        data.topic_data = topicQos->topic_data;
        data.group_data = publisherQos->group_data;

        DDS::InstanceHandle_t handle
        = bitSubscriptionDataWriter_->register_instance(data);

        subscription->set_handle(handle);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DCPS_IR_Domain::publish_subscription_bit: [ %d, 0x%x, 0x%x], handle %d.\n",
                     data.key.value[0], data.key.value[1], data.key.value[2], handle));
        }

        bitSubscriptionDataWriter_->write(data,
                                          handle);

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::publish_subscription_bit:");
      }

    } else {
      subscription->set_bit_status(1);
    }
  }

#else
  ACE_UNUSED_ARG(subscription);
#endif // !defined (DDS_HAS_MINIMUM_BIT)

}

void DCPS_IR_Domain::publish_publication_bit(DCPS_IR_Publication* publication)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {

    DCPS_IR_Topic_Description* desc =
      publication->get_topic_description();

    const char* dataTypeName = desc->get_dataTypeName();

    bool isNotBIT =
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
      ACE_OS::strcmp(dataTypeName, OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

    if (isNotBIT) {
      try {
        const DDS::DataWriterQos* writerQos = publication->get_datawriter_qos();
        const DDS::PublisherQos* publisherQos = publication->get_publisher_qos();

        DCPS_IR_Topic* topic = publication->get_topic();
        const DDS::TopicQos* topicQos = topic->get_topic_qos();

        DDS::PublicationBuiltinTopicData data;
        get_BuiltinTopicKey(data.key, publication->get_id());
        get_BuiltinTopicKey(data.participant_key,
                            publication->get_participant_id());
        data.topic_name = desc->get_name();
        data.type_name = desc->get_dataTypeName();
        data.durability = writerQos->durability;
        data.durability_service = writerQos->durability_service;
        data.deadline = writerQos->deadline;
        data.latency_budget = writerQos->latency_budget;
        data.liveliness = writerQos->liveliness;
        data.reliability = writerQos->reliability;
        data.lifespan = writerQos->lifespan;
        data.user_data = writerQos->user_data;
        data.ownership = writerQos->ownership;
        data.ownership_strength = writerQos->ownership_strength;
        data.destination_order = writerQos->destination_order;
        data.presentation = publisherQos->presentation;
        data.partition = publisherQos->partition;
        data.topic_data = topicQos->topic_data;
        data.group_data = publisherQos->group_data;

        DDS::InstanceHandle_t handle
        = bitPublicationDataWriter_->register_instance(data);

        publication->set_handle(handle);

        if (OpenDDS::DCPS::DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     "(%P|%t) DCPS_IR_Domain::publish_publication_bit: [ %d, 0x%x, 0x%x], handle %d.\n",
                     data.key.value[0], data.key.value[1], data.key.value[2], handle));
        }

        DDS::ReturnCode_t status = bitPublicationDataWriter_->write(data, handle);
        if (status != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                       "(%P|%t) DCPS_IR_Domain::publish_publication_bit: write() status of %d\n",
                       status));
        }

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::publish_publication_bit:");
      }

    } else {
      publication->set_bit_status(1);
    }
  }

#else
  ACE_UNUSED_ARG(publication);
#endif // !defined (DDS_HAS_MINIMUM_BIT)

}

void DCPS_IR_Domain::dispose_participant_bit(DCPS_IR_Participant* participant)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    if (!participant->isBitPublisher()) {
      try {
        DDS::ParticipantBuiltinTopicData key_data;
        DDS::InstanceHandle_t handle = participant->get_handle();

        DDS::ReturnCode_t retGetKey
        = bitParticipantDataWriter_->get_key_value(key_data,
                                                   handle);

        if (DDS::RETCODE_OK != retGetKey) {
          OpenDDS::DCPS::RepoIdConverter converter(participant->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_participant_bit ")
                     ACE_TEXT("Unable to get_key_value for participant %C handle %d.\n"),
                     std::string(converter).c_str(),
                     handle));
        }

        DDS::ReturnCode_t retDispose =
          bitParticipantDataWriter_->dispose(key_data,
                                             handle);

        if (DDS::RETCODE_OK != retDispose) {
          OpenDDS::DCPS::RepoIdConverter converter(participant->get_id());
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_participant_bit ")
                     ACE_TEXT("Unable to dispose for participant %C handle %d.\n"),
                     std::string(converter).c_str(),
                     handle));
        }

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "ERROR: Exception caught in DCPS_IR_Domain::dispose_participant_bit:");
      }
    }
  }

#else
  ACE_UNUSED_ARG(participant);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::dispose_topic_bit(DCPS_IR_Topic* topic)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    if (!topic->is_bit()) {
      try {
        DDS::TopicBuiltinTopicData key_data;
        DDS::InstanceHandle_t handle = topic->get_handle();

        DDS::ReturnCode_t retGetKey
        = bitTopicDataWriter_->get_key_value(key_data,
                                             handle);

        if (DDS::RETCODE_OK != retGetKey) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_topic_bit ")
                     ACE_TEXT("Unable to get_key_value for topic ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     topic,
                     handle,
                     retGetKey));
        }

        DDS::ReturnCode_t retDispose =
          bitTopicDataWriter_->dispose(key_data,
                                       handle);

        if (DDS::RETCODE_OK != retDispose) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_topic_bit ")
                     ACE_TEXT("Unable to dispose for topic ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     topic,
                     handle,
                     retDispose));
        }

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::dispose_topic_bit:");
      }
    }
  }

#else
  ACE_UNUSED_ARG(topic);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::dispose_subscription_bit(DCPS_IR_Subscription* subscription)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    if (!subscription->is_bit()) {
      try {
        DDS::SubscriptionBuiltinTopicData key_data;
        DDS::InstanceHandle_t handle = subscription->get_handle();

        DDS::ReturnCode_t retGetKey
        = bitSubscriptionDataWriter_->get_key_value(key_data,
                                                    handle);

        if (DDS::RETCODE_OK != retGetKey) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_subscription_bit ")
                     ACE_TEXT("Unable to get_key_value for subscription ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     subscription,
                     handle,
                     retGetKey));
        }

        DDS::ReturnCode_t retDispose =
          bitSubscriptionDataWriter_->dispose(key_data,
                                              handle);

        if (DDS::RETCODE_OK != retDispose) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_subscription_bit ")
                     ACE_TEXT("Unable to dispose for subscription ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     subscription,
                     handle,
                     retDispose));
        }

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::dispose_subscription_bit:");
      }
    }
  }

#else
  ACE_UNUSED_ARG(subscription);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::dispose_publication_bit(DCPS_IR_Publication* publication)
{
#if !defined (DDS_HAS_MINIMUM_BIT)

  if (useBIT_) {
    if (!publication->is_bit()) {
      try {
        DDS::PublicationBuiltinTopicData key_data;
        DDS::InstanceHandle_t handle = publication->get_handle();

        DDS::ReturnCode_t retGetKey
        = bitPublicationDataWriter_->get_key_value(key_data, handle);

        if (DDS::RETCODE_OK != retGetKey) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_publication_bit ")
                     ACE_TEXT("Unable to get_key_value for publication ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     publication,
                     handle,
                     retGetKey));
        }

        DDS::ReturnCode_t retDispose =
          bitPublicationDataWriter_->dispose(key_data,
                                             handle);

        if (DDS::RETCODE_OK != retDispose) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_publication_bit ")
                     ACE_TEXT("Unable to dispose for publication ptr 0x%x handle %d.  ")
                     ACE_TEXT("Call returned %d.\n"),
                     publication,
                     handle,
                     retDispose));
        }

      } catch (const CORBA::Exception& ex) {
        ex._tao_print_exception(
          "(%P|%t) ERROR: Exception caught in DCPS_IR_Domain::dispose_publication_bit:");
      }
    }
  }

#else
  ACE_UNUSED_ARG(publication);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

void DCPS_IR_Domain::remove_topic_id_mapping(const OpenDDS::DCPS::RepoId& topicId)
{
  IdToTopicMap::iterator map_entry = this->idToTopicMap_.find(topicId);
  if (map_entry != this->idToTopicMap_.end())
    idToTopicMap_.erase(map_entry);
}

std::string DCPS_IR_Domain::dump_to_string(const std::string& prefix, int depth) const
{
  std::string str;
#if !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  for (int i=0; i < depth; i++)
    str += prefix;
  std::string indent = str + prefix;
  std::ostringstream os;
  os << "DCPS_IR_Domain[" << id_ << "]";
  str += os.str();
  if (useBIT_)
    str += " BITS";
  str += "\n";

  str += indent + "Participants:\n";
  for (DCPS_IR_Participant_Map::const_iterator pm = participants_.begin();
       pm != participants_.end();
       pm++)
  {
    str += pm->second->dump_to_string(prefix, depth+1);
  }

  str += indent + "Dead Participants:\n";
  for (DCPS_IR_Participant_Set::const_iterator dp = deadParticipants_.begin();
       dp != deadParticipants_.end();
       dp++)
  {
    OpenDDS::DCPS::RepoIdConverter sub_converter((*dp)->get_id());
    str += indent + std::string(sub_converter);
    str += "\n";
  }

  str += indent + "Topic Descriptions:\n";
  for (DCPS_IR_Topic_Description_Set::const_iterator tdi = topicDescriptions_.begin();
       tdi != topicDescriptions_.end();
       tdi++)
  {
    str += (*tdi)->dump_to_string(prefix, depth+1);
  }
#endif // !defined (OPENDDS_INFOREPO_REDUCED_FOOTPRINT)
  return str;
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

