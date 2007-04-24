#include "DcpsInfo_pch.h"
#include /**/ "DCPS_IR_Domain.h"

#include /**/ "DCPS_IR_Participant.h"
#include /**/ "DCPS_IR_Topic_Description.h"
#include "DomainParticipantListener_i.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/PublisherImpl.h"


#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/TopicBuiltinTopicDataTypeSupportImpl.h"
#include "dds/ParticipantBuiltinTopicDataTypeSupportImpl.h"
#include "dds/SubscriptionBuiltinTopicDataTypeSupportImpl.h"
#include "dds/PublicationBuiltinTopicDataTypeSupportImpl.h"

#include "dds/DCPS/BuiltInTopicUtils.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/Transient_Kludge.h"

#include /**/ "tao/debug.h"


DCPS_IR_Domain::DCPS_IR_Domain (::DDS::DomainId_t id)
:
 id_(id),
 useBIT_(false)
{
}



DCPS_IR_Domain::~DCPS_IR_Domain()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (0 != cleanup_built_in_topics() )
    {
      ACE_ERROR((LM_ERROR, "ERROR: Failed to clean up the Built-In Topics!\n"));
    }
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



int DCPS_IR_Domain::add_participant(DCPS_IR_Participant* participant)
{
  TAO::DCPS::RepoId participantId = participant->get_id ();

  int status = participants_.bind(participantId, participant);

  switch (status)
    {
    case 0:
      // Publish the BIT information
      publish_participant_bit(participant);

      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::add_participant ")
            ACE_TEXT("Domain %d successfully added participant %X id: %d\n"),
            id_, participant, participantId));
        }
      break;
    case 1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_participant ")
        ACE_TEXT("Domain %d failed to add already existing participant id %d\n"),
        id_, participantId));
      break;
    case -1:
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_participant ")
        ACE_TEXT("Domain %d Unknown error while adding participant id %d\n"),
        id_, participantId));
    };

  return status;
}



int DCPS_IR_Domain::remove_participant(TAO::DCPS::RepoId participantId,
                                       CORBA::Boolean notify_lost)
{
  DCPS_IR_Participant* participant;

  int status = participants_.find (participantId, participant);

  if (0 == status)
    {
      // make sure the participant has cleaned up all publications,
      // subscriptions, and any topic references
      participant->remove_all_dependents(notify_lost);

      status = participants_.unbind (participantId, participant);

      if (0 == status)
        {
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::remove_participant ")
                ACE_TEXT("Domain %d removed participant %X id: %d\n"),
                id_, participant, participantId));
            }

          // Dispose the BIT information
          dispose_participant_bit(participant);

          delete participant;
        }
      else
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::remove_participant ")
            ACE_TEXT("Domain %d Error removing participant %X id: %d\n"),
            id_, participant, participantId));
        } // if (0 == status)
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::remove_participant ")
        ACE_TEXT("Domain %d Unable to find participant id: %d\n"),
        id_, participantId));
    } // if (0 == status)

  return status;
}



int DCPS_IR_Domain::find_participant(TAO::DCPS::RepoId participantId,
                                     DCPS_IR_Participant*& participant)
{
  int status = participants_.find(participantId, participant);
  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::find_participant ")
            ACE_TEXT("Domain %d located participant %X id: %d\n"),
            id_, participant, participantId));
        }
    }
  else
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::find_participant ")
            ACE_TEXT("Domain %d Unable to locate participant with id %d\n"),
            id_, participantId));
        }
    } // if (0 == status)

  return status;
}



TAO::DCPS::TopicStatus DCPS_IR_Domain::add_topic(TAO::DCPS::RepoId_out topicId,
                                                 const char * topicName,
                                                 const char * dataTypeName,
                                                 const ::DDS::TopicQos & qos,
                                                 DCPS_IR_Participant* participantPtr)
{
  topicId = 0;

  TAO::DCPS::RepoId topic_id = get_next_topic_id();
  TAO::DCPS::TopicStatus status = add_topic_i (topic_id, topicName
                                               , dataTypeName
                                               , qos, participantPtr);

  if (status == TAO::DCPS::CREATED) {
    topicId = topic_id;
  }

  return status;
}

TAO::DCPS::TopicStatus
DCPS_IR_Domain::force_add_topic(TAO::DCPS::RepoId topicId,
                                const char* topicName,
                                const char* dataTypeName,
                                const ::DDS::TopicQos & qos,
                                DCPS_IR_Participant* participantPtr)
{
  this->set_base_topic_id (topicId + 1);
  TAO::DCPS::TopicStatus status = add_topic_i (topicId, topicName
                                               , dataTypeName
                                               , qos, participantPtr);

  return status;
}

TAO::DCPS::TopicStatus DCPS_IR_Domain::add_topic_i (TAO::DCPS::RepoId topicId,
                                                    const char * topicName,
                                                    const char * dataTypeName,
                                                    const ::DDS::TopicQos & qos,
                                                    DCPS_IR_Participant* participantPtr)
{
  DCPS_IR_Topic_Description* description;
  int descriptionLookup = find_topic_description(topicName, dataTypeName, description);
  if (1 == descriptionLookup)
    {
      topicId = 0;
      return TAO::DCPS::CONFLICTING_TYPENAME;
    }
  else if (-1 == descriptionLookup)
    {
      ACE_NEW_RETURN(description,
                     DCPS_IR_Topic_Description(
                       this,
                       topicName,
                       dataTypeName),
                       TAO::DCPS::NOT_FOUND);

      int descriptionAddition = add_topic_description (description);
      if (0 != descriptionAddition)
        {
          // unable to add the topic
          delete description;
          description = 0;
          topicId = 0;
          if (2 == descriptionAddition)
            {
              return TAO::DCPS::CONFLICTING_TYPENAME;
            }
          else
            {
              return TAO::DCPS::NOT_FOUND;
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
                 TAO::DCPS::NOT_FOUND);

  TAO::DCPS::TopicStatus topicStatus = TAO::DCPS::NOT_FOUND;

  int bindStatus = description->add_topic(topic);

  if (0 == bindStatus)
    {
      bindStatus = participantPtr->add_topic_reference(topic);

      if (0 == bindStatus)
        {
          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::add_topic ")
                ACE_TEXT("Successfully added topic %X id: %d\n"),
                topic, topicId));
            }
          topicStatus = TAO::DCPS::CREATED;

          // Publish the BIT information
          publish_topic_bit(topic);
        }
      else
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_topic ")
            ACE_TEXT("Unable to add topic %X id %d to Domain Participant\n"),
            topic, topicId));
          topicStatus = TAO::DCPS::NOT_FOUND;
          topicId = 0;
          description->remove_topic(topic);
          delete topic;
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_topic ")
        ACE_TEXT("Unable to add topic %X id %d to Topic Description\n"),
        topic, topicId));
      topicStatus = TAO::DCPS::NOT_FOUND;
      topicId = 0;
      delete topic;
    }

    return topicStatus;
}



TAO::DCPS::TopicStatus DCPS_IR_Domain::find_topic(const char * topicName,
                                                  DCPS_IR_Topic*& topic)

{
  TAO::DCPS::TopicStatus topicStatus = TAO::DCPS::NOT_FOUND;
  DCPS_IR_Topic_Description* temp = 0;

  DCPS_IR_Topic_Description_Set::ITERATOR iter = topicDescriptions_.begin ();
  DCPS_IR_Topic_Description_Set::ITERATOR end = topicDescriptions_.end();

  while (iter != end && topicStatus == TAO::DCPS::NOT_FOUND)
    {
      temp = *iter;
      if ( ACE_OS::strcmp(topicName, temp->get_name()) == 0 )
        {
          temp = *iter;
          topic = temp->get_first_topic();
          topicStatus = TAO::DCPS::FOUND;

          if (TAO_debug_level > 0)
            {
              ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::find_topic ")
                ACE_TEXT("Domain %d Located topic %d\n"),
                id_, topic->get_id() ));
            }
        }

      ++iter;
    }

  return topicStatus;
}



TAO::DCPS::TopicStatus DCPS_IR_Domain::remove_topic(DCPS_IR_Participant* part,
                                                    DCPS_IR_Topic*& topic)
{
  DCPS_IR_Topic_Description* description = topic->get_topic_description();

  if (description->remove_topic(topic) != 0)
    {
      // An unknown error means that the description may still
      // have the topic in its topic set.  We can't remove it.
      // The system is an inconsistent state!
      throw TAO::DCPS::Invalid_Topic();
    }

  if (description->get_number_topics() == 0)
    {
      // Remove the topic description
      if (remove_topic_description(description) != 0)
        {
          // An unknown error means that the description may still
          // have the topic in its topic set.
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: Topic Description %s %s  ")
                     ACE_TEXT("was not correctly removed from Domain %d"),
                     description->get_name(),
                     description->get_dataTypeName(),
                     id_
                     ));
        }
      else
        {
          delete description;
          description = 0;
        }
    }
  // description variable is invalid after this point

  if (part->remove_topic_reference(topic->get_id(), topic) != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: Domain %d Topic %d ")
        ACE_TEXT("was not correctly removed from Participant %d"),
        id_, topic->get_id(), part->get_id() ));

    }

  // Dispose the BIT information
  dispose_topic_bit(topic);

  delete topic;
  topic = 0;

  return TAO::DCPS::REMOVED;
}



int DCPS_IR_Domain::find_topic_description(const char* name,
                                           const char* dataTypeName,
                                           DCPS_IR_Topic_Description*& desc)
{
  // set the status to not found
  int status = -1;
  DCPS_IR_Topic_Description* temp = 0;

  DCPS_IR_Topic_Description_Set::ITERATOR iter = topicDescriptions_.begin ();
  DCPS_IR_Topic_Description_Set::ITERATOR end = topicDescriptions_.end();

  while (iter != end && status == -1)
    {
      temp = *iter;
      if ( ACE_OS::strcmp(name, temp->get_name()) == 0 )
        {
          if ( ACE_OS::strcmp(dataTypeName, temp->get_dataTypeName()) == 0 )
            {
              desc = *iter;
              status = 0;

              if (TAO_debug_level > 0)
                {
                  ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::find_topic_description ")
                    ACE_TEXT("Domain %d Located topic description %s %s %X\n"),
                    id_, name, dataTypeName, desc));
                }
            }
          else //if ( ACE_OS::strcmp(dataTypeName, temp->get_dataTypeName()) == 0 )
            {
              status = 1;
              if (TAO_debug_level > 0)
                {
                  ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::find_topic_description ")
                    ACE_TEXT("Domain %d Located inconsistent topic description existing: %s %s  ")
                    ACE_TEXT(" attempted: %s %s\n"),
                    id_,
                    temp->get_name(), temp->get_dataTypeName(),
                    name, dataTypeName));
                }
            }
        }

      ++iter;
    }

  return status;
}



int DCPS_IR_Domain::init_built_in_topics()
{

#if !defined (DDS_HAS_MINIMUM_BIT)

  // Tell the DCPS framework to use a limited DURABILITY.kind=TRANSIENT
  // implementation and also indicates that DCPS framework BIT subscriber
  // and datareaders should not be created.
  TheTransientKludge->enable (); 

  if (TAO_debug_level > 0)
    {
      ACE_DEBUG((LM_DEBUG,
                ACE_TEXT("DCPS_IR_Domain::init_built_in_topics() ")
                ACE_TEXT(" Initializing Built In Topics for domain %d\n"),
                id_ ));
    }

  try
    {
      bitParticipantFactory_ = TheParticipantFactory;

      TAO_DCPS_DomainParticipantListener_i* listenerImpl =
        new TAO_DCPS_DomainParticipantListener_i;

      bitParticipantListener_ = ::TAO::DCPS::servant_to_reference (listenerImpl);

      bitParticipant_ =
        bitParticipantFactory_->create_participant(id_,
                                                   PARTICIPANT_QOS_DEFAULT,
                                                   bitParticipantListener_.in());
      if (CORBA::is_nil (bitParticipant_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil DomainParticipant in ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

      int transportResult = init_built_in_topics_transport();
      if (0 != transportResult)
        {
          return transportResult;
        }

      int topicsResult = init_built_in_topics_topics();
      if (0 != topicsResult)
        {
          return topicsResult;
        }

      int datawritersResult = init_built_in_topics_datawriters();
      if (0 != datawritersResult)
        {
          return datawritersResult;
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("ERROR: Exception caught in main.cpp:");
      return 1;
    }

  // enable the Built-In Topics
  useBIT_ = true;

  return 0;
#else
  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



int DCPS_IR_Domain::init_built_in_topics_topics()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  try
    {
      ::DDS::TopicQos topic_qos;
      bitParticipant_->get_default_topic_qos(topic_qos);


      // Participant topic
      ::DDS::ParticipantBuiltinTopicDataTypeSupportImpl* participantTypeSupport_servant =
        new ::DDS::ParticipantBuiltinTopicDataTypeSupportImpl();
      ::DDS::ParticipantBuiltinTopicDataTypeSupport_var participantTypeSupport =
        TAO::DCPS::servant_to_reference (participantTypeSupport_servant);

      if (::DDS::RETCODE_OK !=
          participantTypeSupport->register_type(bitParticipant_.in (),
                                                ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("ERROR: Failed to register the ParticipantBuiltinTopicDataTypeSupport.")));
          return 1;
        }

      bitParticipantTopic_ =
        bitParticipant_->create_topic (::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                       ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (bitParticipantTopic_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil %s Topic from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                             ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC),
                            1);
        }

      // Topic topic
      ::DDS::TopicBuiltinTopicDataTypeSupportImpl* topicTypeSupport_servant =
        new ::DDS::TopicBuiltinTopicDataTypeSupportImpl();
      ::DDS::TopicBuiltinTopicDataTypeSupport_var topicTypeSupport =
        TAO::DCPS::servant_to_reference (topicTypeSupport_servant);

      if (::DDS::RETCODE_OK !=
          topicTypeSupport->register_type(bitParticipant_.in (),
                                          ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("ERROR: Failed to register the TopicBuiltinTopicDataTypeSupport.")));
          return 1;
        }

      bitTopicTopic_ =
        bitParticipant_->create_topic (::TAO::DCPS::BUILT_IN_TOPIC_TOPIC,
                                       ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (bitTopicTopic_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil %s Topic from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                             ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC),
                            1);
        }

      // Subscription topic
      ::DDS::SubscriptionBuiltinTopicDataTypeSupportImpl* subscriptionTypeSupport_servant =
        new ::DDS::SubscriptionBuiltinTopicDataTypeSupportImpl();
      ::DDS::SubscriptionBuiltinTopicDataTypeSupport_var subscriptionTypeSupport =
        TAO::DCPS::servant_to_reference (subscriptionTypeSupport_servant);

      if (::DDS::RETCODE_OK !=
          subscriptionTypeSupport->register_type(bitParticipant_.in (),
                                                 ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("ERROR: Failed to register the SubscriptionBuiltinTopicDataTypeSupport.")));
          return 1;
        }

      bitSubscriptionTopic_ =
        bitParticipant_->create_topic (::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC,
                                       ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (bitSubscriptionTopic_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil %s Topic from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                             ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC),
                            1);
        }

      // Publication topic
      ::DDS::PublicationBuiltinTopicDataTypeSupportImpl* publicationTypeSupport_servant =
        new ::DDS::PublicationBuiltinTopicDataTypeSupportImpl();
      ::DDS::PublicationBuiltinTopicDataTypeSupport_var publicationTypeSupport =
        TAO::DCPS::servant_to_reference (publicationTypeSupport_servant);

      if (::DDS::RETCODE_OK !=
          publicationTypeSupport->register_type(bitParticipant_.in (),
                                                ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("ERROR: Failed to register the PublicationBuiltinTopicDataTypeSupport.")));
          return 1;
        }

      bitPublicationTopic_ =
        bitParticipant_->create_topic (::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC,
                                       ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE,
                                       topic_qos,
                                       ::DDS::TopicListener::_nil());
      if (CORBA::is_nil (bitPublicationTopic_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil %s Topic from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n"),
                             ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC),
                            1);
        }

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: Exception caught in DCPS_IR_Domain::init_built_in_topics_topics:");
      return 1;
    }

  return 0;

#else

  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



int DCPS_IR_Domain::init_built_in_topics_datawriters()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  try
    {
      ::DDS::DataWriter_var datawriter;
      ::DDS::DataWriterQos dw_qos;
      bitPublisher_->get_default_datawriter_qos (dw_qos);
      dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

      // Participant DataWriter
      datawriter =
        bitPublisher_->create_datawriter(bitParticipantTopic_.in (),
                                         dw_qos,
                                         ::DDS::DataWriterListener::_nil());

      bitParticipantDataWriter_ =
        ::DDS::ParticipantBuiltinTopicDataDataWriter::_narrow(datawriter.in ());
      if (CORBA::is_nil (bitParticipantDataWriter_.in()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil DomainParticipant DataWriter from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

      // Topic DataWriter
      datawriter =
        bitPublisher_->create_datawriter(bitTopicTopic_.in (),
                                          dw_qos,
                                          ::DDS::DataWriterListener::_nil());

      bitTopicDataWriter_ =
        ::DDS::TopicBuiltinTopicDataDataWriter::_narrow(datawriter.in ());
      if (CORBA::is_nil (bitTopicDataWriter_.in()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil Topic DataWriter from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

      // Subscription DataWriter
      datawriter =
        bitPublisher_->create_datawriter(bitSubscriptionTopic_.in (),
                                          dw_qos,
                                          ::DDS::DataWriterListener::_nil());

      bitSubscriptionDataWriter_ =
        ::DDS::SubscriptionBuiltinTopicDataDataWriter::_narrow(
                        datawriter.in ());
      if (CORBA::is_nil (bitSubscriptionDataWriter_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil Subscription DataWriter from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

      // Publication DataWriter
      datawriter =
        bitPublisher_->create_datawriter(bitPublicationTopic_.in (),
                                         dw_qos,
                                         ::DDS::DataWriterListener::_nil());

      bitPublicationDataWriter_ =
        ::DDS::PublicationBuiltinTopicDataDataWriter::_narrow(datawriter.in ());
      if (CORBA::is_nil (bitPublicationDataWriter_.in()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil Publication DataWriter from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
        "ERROR: Exception caught in DCPS_IR_Domain::init_built_in_topics_datawriters:");
      return 1;
    }

  return 0;
#else

  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



int DCPS_IR_Domain::init_built_in_topics_transport ()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  try
    {
      transportImpl_ =
        TheTransportFactory->obtain(TAO::DCPS::BIT_ALL_TRAFFIC);

      // Create the Publisher
      bitPublisher_ =
        bitParticipant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                          ::DDS::PublisherListener::_nil());
      if (CORBA::is_nil (bitPublisher_.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: ")
                            ACE_TEXT("Nil Publisher from ")
                            ACE_TEXT("DCPS_IR_Domain::init_built_in_topics.\n")),
                            1);
        }

      // Attach the Publisher with the TransportImpl.
      ::TAO::DCPS::PublisherImpl* pubServant
        = ::TAO::DCPS::reference_to_servant < ::TAO::DCPS::PublisherImpl, ::DDS::Publisher_ptr>
        (bitPublisher_.in ());

      TAO::DCPS::AttachStatus status
        = pubServant->attach_transport(transportImpl_.in());

      if (status != TAO::DCPS::ATTACH_OK)
        {
          // We failed to attach to the transport for some reason.
          const char* status_str;

          switch (status)
            {
              case TAO::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = "ATTACH_BAD_TRANSPORT";
                break;
              case TAO::DCPS::ATTACH_ERROR:
                status_str = "ATTACH_ERROR";
                break;
              case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = "ATTACH_INCOMPATIBLE_QOS";
                break;
              default:
                status_str = "Unknown Status";
                break;
            }

          ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Failed to attach to the transport. ")
                            ACE_TEXT("AttachStatus == %s\n"), status_str),
                            1);
        }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception (
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
  if (useBIT_)
    {
      // clean up the Built-in Topic objects

      ::DDS::ReturnCode_t delDataWriterRetCode;
      if (! CORBA::is_nil (bitPublisher_.in ()))
        {
          delDataWriterRetCode =
            bitPublisher_->delete_datawriter(
                bitParticipantDataWriter_.in ());
          delDataWriterRetCode =
            bitPublisher_->delete_datawriter(
                bitTopicDataWriter_.in ());
          delDataWriterRetCode =
            bitPublisher_->delete_datawriter(
                bitSubscriptionDataWriter_.in ());
          delDataWriterRetCode =
            bitPublisher_->delete_datawriter(
                bitPublicationDataWriter_.in ());

          ::DDS::ReturnCode_t delPublisherRetCode =
            bitParticipant_->delete_publisher(
                bitPublisher_.in ());
          ACE_UNUSED_ARG (delPublisherRetCode);

        } // if (0 != bitPublisher_)


      if (! CORBA::is_nil (bitParticipant_.in ()))
        {
          ::DDS::ReturnCode_t delTopicRetCode;
          delTopicRetCode =
            bitParticipant_->delete_topic(bitParticipantTopic_.in ());
          delTopicRetCode =
            bitParticipant_->delete_topic(bitTopicTopic_.in ());
          delTopicRetCode =
            bitParticipant_->delete_topic(bitSubscriptionTopic_.in ());
          delTopicRetCode =
            bitParticipant_->delete_topic(bitPublicationTopic_.in ());

          ::DDS::ReturnCode_t delParticipantRetCode =
            bitParticipantFactory_->delete_participant(bitParticipant_.in ());
          ACE_UNUSED_ARG (delParticipantRetCode);
        }
    }

  return 0;

#else
  return 1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



int DCPS_IR_Domain::add_topic_description(DCPS_IR_Topic_Description*& desc)
{

  // set the status to not found
  int foundConflicting = 0;
  DCPS_IR_Topic_Description* temp = 0;

  DCPS_IR_Topic_Description_Set::ITERATOR iter = topicDescriptions_.begin ();
  DCPS_IR_Topic_Description_Set::ITERATOR end = topicDescriptions_.end();

  while (iter != end && 0 == foundConflicting)
    {
      temp = *iter;
      if ( (ACE_OS::strcmp(desc->get_name(), temp->get_name()) == 0) )
        {
          if ( (ACE_OS::strcmp(desc->get_dataTypeName(), temp->get_dataTypeName()) == 0) )
            {
              foundConflicting = 1;
            }
          else
            {
              foundConflicting = 2;
              if (TAO_debug_level > 0)
                {
                  ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_topic_description ")
                    ACE_TEXT("Domain %d Located inconsistent topic description existing: %s %s  ")
                    ACE_TEXT(" attempted: %s %s\n"),
                    id_,
                    temp->get_name(), temp->get_dataTypeName(),
                    desc->get_name(), desc->get_dataTypeName()
                    ));
                }
            }  /* if ( (ACE_OS::strcmp(desc->get_dataTypeName(), temp->get_dataTypeName()) == 0) ) */
        } /* if ( (ACE_OS::strcmp(desc->get_name(), temp->get_name()) == 0) ) */
      ++iter;
    }

  int status;

  if (0 == foundConflicting)
  {
    status = topicDescriptions_.insert(desc);

    switch (status)
      {
      case 0:
        if (TAO_debug_level > 0)
          {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::add_topic_description ")
              ACE_TEXT("Domain %d Successfully added Topic Description %X\n"),
              id_, desc));
          }
        break;
      case 1:
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_topic_description ")
          ACE_TEXT("Domain %d failed to add already existing Topic Description %X\n"),
          id_, desc));
        break;
      case -1:
        ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::add_topic_description ")
          ACE_TEXT("Domain %d Unknown error while adding Topic Description %X\n"),
          id_, desc));
      };
    }
  else  //if (0 == foundConflicting)
    {
      status = foundConflicting;
    }

  return status;
}



int DCPS_IR_Domain::remove_topic_description(DCPS_IR_Topic_Description*& desc)
{
  int status = topicDescriptions_.remove(desc);
  if (0 == status)
    {
      if (TAO_debug_level > 0)
        {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("DCPS_IR_Domain::remove_topic_description ")
            ACE_TEXT("Domain %d Removed Topic Description %X\n"),
            id_, desc));
        }
    }
  else
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DCPS_IR_Domain::remove_topic_description ")
        ACE_TEXT("Domain %d Unable to remove Topic Description %X\n"),
        id_, desc));
    } // if (0 == status)

  return status;
}



void DCPS_IR_Domain::add_dead_participant(DCPS_IR_Participant* participant)
{
  deadParticipants_.insert(participant);
}



void DCPS_IR_Domain::remove_dead_participants ()
{
  if (0 < deadParticipants_.size() )
    {
      DCPS_IR_Participant* dead = 0;
      DCPS_IR_Participant_Set::ITERATOR iter = deadParticipants_.begin ();
      DCPS_IR_Participant_Set::ITERATOR end = deadParticipants_.end();

      while (iter != end)
        {
          dead = *iter;
          ++iter;
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("ERROR: DCPS_IR_Domain::remove_dead_participants () ")
            ACE_TEXT("Removing dead participant %X id %d\n"),
            dead,
            dead->get_id()
            ));
          deadParticipants_.remove(dead);

          dead->set_alive(0);

          CORBA::Boolean notify_lost = 1;
          dead->remove_all_dependents(notify_lost);
          remove_participant(dead->get_id(), notify_lost);
        }
    }
}



::DDS::DomainId_t DCPS_IR_Domain::get_id ()
{
  return id_;
}



TAO::DCPS::RepoId DCPS_IR_Domain::get_next_participant_id ()
{
  return participantIdGenerator_.get_next_id ();
}

bool
DCPS_IR_Domain::set_base_participant_id (TAO::DCPS::RepoId id)
{
  return participantIdGenerator_.set_base_id (id);
}


TAO::DCPS::RepoId DCPS_IR_Domain::get_next_topic_id ()
{
  return topicIdGenerator_.get_next_id ();
}

bool
DCPS_IR_Domain::set_base_topic_id (TAO::DCPS::RepoId id)
{
  return topicIdGenerator_.set_base_id (id);
}


TAO::DCPS::RepoId DCPS_IR_Domain::get_next_publication_id ()
{
  return pubsubIdGenerator_.get_next_id ();
}

bool
DCPS_IR_Domain::set_base_publication_id (TAO::DCPS::RepoId id)
{
  return pubsubIdGenerator_.set_base_id (id);
}


TAO::DCPS::RepoId DCPS_IR_Domain::get_next_subscription_id ()
{
  return pubsubIdGenerator_.get_next_id ();
}

bool
DCPS_IR_Domain::set_base_subscription_id (TAO::DCPS::RepoId id)
{
  return pubsubIdGenerator_.set_base_id (id);
}


void DCPS_IR_Domain::publish_participant_bit (DCPS_IR_Participant* participant)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      if (1 != participant->get_id())
        {
          try
          {
            const ::DDS::DomainParticipantQos* participantQos = participant->get_qos ();

            ::DDS::ParticipantBuiltinTopicData data;
            data.key[0] = id_;
            data.key[1] = participant->get_id();
            data.key[2]= 0;
            data.user_data = participantQos->user_data;
            data.hostname = participant->get_hostname();
            data.process_id = participant->get_process_id();
            ::DDS::InstanceHandle_t handle
              = bitParticipantDataWriter_->_cxx_register (data);

            participant->set_handle(handle);

            if (TAO_debug_level > 0)
            {
              ACE_DEBUG ((LM_DEBUG, 
                "(%P|%t)DCPS_IR_Domain::publish_participant_bit: %d %d %d \n", 
                data.key[0], data.key[1], data.key[2]));
            }

            bitParticipantDataWriter_->write(data,
                                            handle);
          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::publish_topic_bit:");
          }
        }
      else
        {
          participant->set_bit_status(1);
        }

    }
#else
  ACE_UNUSED_ARG(participant);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::publish_topic_bit (DCPS_IR_Topic* topic)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      DCPS_IR_Topic_Description* desc =
        topic->get_topic_description();
      const char* dataTypeName = desc->get_dataTypeName ();

      bool isNotBIT =
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

      if ( isNotBIT )
        {
          try
          {
            const ::DDS::TopicQos* topicQos = topic->get_topic_qos ();

            ::DDS::TopicBuiltinTopicData data;
            data.key[0] = id_;
            data.key[1] = topic->get_participant_id();
            data.key[2]= topic->get_id();
            data.name = desc->get_name();
            data.type_name = desc->get_dataTypeName();
            data.durability = topicQos->durability;
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

            ::DDS::InstanceHandle_t handle
              = bitTopicDataWriter_->_cxx_register (data);

            topic->set_handle(handle);

            if (TAO_debug_level > 0)
            {
              ACE_DEBUG ((LM_DEBUG, 
                "(%P|%t)DCPS_IR_Domain::publish_topic_bit: %d %d %d \n", 
                data.key[0], data.key[1], data.key[2]));
            }

            bitTopicDataWriter_->write(data,
                                      handle);
          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::publish_topic_bit:");
          }
        }
      else
        {
          topic->set_bit_status(1);
        }
    }
#else
  ACE_UNUSED_ARG(topic);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::publish_subscription_bit (DCPS_IR_Subscription* subscription)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      DCPS_IR_Topic_Description* desc =
        subscription->get_topic_description();

      const char* dataTypeName = desc->get_dataTypeName ();

      bool isNotBIT =
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

      if ( isNotBIT )
        {
          try
          {
            const ::DDS::DataReaderQos* readerQos = subscription->get_datareader_qos();
            const ::DDS::SubscriberQos* publisherQos = subscription->get_subscriber_qos ();

            DCPS_IR_Topic* topic = subscription->get_topic ();
            const ::DDS::TopicQos* topicQos = topic->get_topic_qos();

            ::DDS::SubscriptionBuiltinTopicData data;
            data.key[0] = id_;
            data.key[1] = subscription->get_participant_id();
            data.key[2]= subscription->get_id();
            data.participant_key[0] = id_;
            data.participant_key[1] = subscription->get_participant_id();
            data.participant_key[2] = 0;
            data.topic_name = desc->get_name();
            data.type_name = desc->get_dataTypeName();
            data.durability = readerQos->durability;
            data.deadline = readerQos->deadline;
            data.latency_budget = readerQos->latency_budget;
            data.liveliness = readerQos->liveliness;
            data.reliability = readerQos->reliability;
            data.destination_order = readerQos->destination_order;
            data.user_data = readerQos->user_data;
            data.time_based_filter = readerQos->time_based_filter;
            data.presentation = publisherQos->presentation;
            data.partition = publisherQos->partition;
            data.topic_data = topicQos->topic_data;
            data.group_data = publisherQos->group_data;

            ::DDS::InstanceHandle_t handle
              = bitSubscriptionDataWriter_->_cxx_register (data);

            subscription->set_handle(handle);

            if (TAO_debug_level > 0)
            {
              ACE_DEBUG ((LM_DEBUG, 
                "(%P|%t)DCPS_IR_Domain::publish_subscription_bit: %d %d %d \n", 
                data.key[0], data.key[1], data.key[2]));
            }

            bitSubscriptionDataWriter_->write(data,
                                              handle);
          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::publish_subscription_bit:");
          }
        }
      else
        {
          subscription->set_bit_status(1);
        }
    }
#else
  ACE_UNUSED_ARG(subscription);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::publish_publication_bit (DCPS_IR_Publication* publication)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      DCPS_IR_Topic_Description* desc =
        publication->get_topic_description();

      const char* dataTypeName = desc->get_dataTypeName ();

      bool isNotBIT =
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC_TYPE) &&
        ACE_OS::strcmp(dataTypeName, ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC_TYPE);

      if ( isNotBIT )
        {
          try
          {
            const ::DDS::DataWriterQos* writerQos = publication->get_datawriter_qos();
            const ::DDS::PublisherQos* publisherQos = publication->get_publisher_qos ();

            DCPS_IR_Topic* topic = publication->get_topic ();
            const ::DDS::TopicQos* topicQos = topic->get_topic_qos();

            ::DDS::PublicationBuiltinTopicData data;
            data.key[0] = id_;
            data.key[1] = publication->get_participant_id();
            data.key[2]= publication->get_id();
            data.participant_key[0] = id_;
            data.participant_key[1] = publication->get_participant_id();
            data.participant_key[2] = 0;
            data.topic_name = desc->get_name();
            data.type_name = desc->get_dataTypeName();
            data.durability = writerQos->durability;
            data.deadline = writerQos->deadline;
            data.latency_budget = writerQos->latency_budget;
            data.liveliness = writerQos->liveliness;
            data.reliability = writerQos->reliability;
            data.user_data = writerQos->user_data;
            data.ownership_strength = writerQos->ownership_strength;
            data.presentation = publisherQos->presentation;
            data.partition = publisherQos->partition;
            data.topic_data = topicQos->topic_data;
            data.group_data = publisherQos->group_data;

            ::DDS::InstanceHandle_t handle
              = bitPublicationDataWriter_->_cxx_register (data);

            publication->set_handle(handle);

            if (TAO_debug_level > 0)
            {
              ACE_DEBUG ((LM_DEBUG, 
                "(%P|%t)DCPS_IR_Domain::publish_publication_bit: %d %d %d \n", 
                data.key[0], data.key[1], data.key[2]));
            }

            bitPublicationDataWriter_->write(data,
                                            handle);
          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::publish_publication_bit:");
          }
        }
      else
        {
          publication->set_bit_status(1);
        }
    }
#else
  ACE_UNUSED_ARG(publication);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::dispose_participant_bit (DCPS_IR_Participant* participant)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      if ( ! participant->is_bit())
        {
          try
          {
            ::DDS::ParticipantBuiltinTopicData key_data;
            ::DDS::InstanceHandle_t handle = participant->get_handle();

            ::DDS::ReturnCode_t retGetKey
              = bitParticipantDataWriter_->get_key_value(key_data,
                                                        handle);

            if (::DDS::RETCODE_OK != retGetKey)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_participant_bit ")
                          ACE_TEXT("Unable to get_key_value for participant ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          participant,
                          handle,
                          retGetKey
                          ));
              }

            ::DDS::ReturnCode_t retDispose =
              bitParticipantDataWriter_->dispose(key_data,
                                                handle);
            if (::DDS::RETCODE_OK != retDispose)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_participant_bit ")
                          ACE_TEXT("Unable to dispose for participant ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          participant,
                          handle,
                          retDispose
                          ));
              }

          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::dispose_participant_bit:");
          }
        }
    }
#else
  ACE_UNUSED_ARG(participant);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::dispose_topic_bit (DCPS_IR_Topic* topic)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      if ( ! topic->is_bit())
        {
          try
          {
            ::DDS::TopicBuiltinTopicData key_data;
            ::DDS::InstanceHandle_t handle = topic->get_handle();

            ::DDS::ReturnCode_t retGetKey
              = bitTopicDataWriter_->get_key_value(key_data,
                                                  handle);

            if (::DDS::RETCODE_OK != retGetKey)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_topic_bit ")
                          ACE_TEXT("Unable to get_key_value for topic ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          topic,
                          handle,
                          retGetKey
                          ));
              }

            ::DDS::ReturnCode_t retDispose =
              bitTopicDataWriter_->dispose(key_data,
                                          handle);
            if (::DDS::RETCODE_OK != retDispose)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_topic_bit ")
                          ACE_TEXT("Unable to dispose for topic ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          topic,
                          handle,
                          retDispose
                          ));
              }
          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::dispose_topic_bit:");
          }
        }
    }
#else
  ACE_UNUSED_ARG(topic);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::dispose_subscription_bit (DCPS_IR_Subscription* subscription)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      if ( ! subscription->is_bit() )
        {
          try
          {
            ::DDS::SubscriptionBuiltinTopicData key_data;
            ::DDS::InstanceHandle_t handle = subscription->get_handle();

            ::DDS::ReturnCode_t retGetKey
              = bitSubscriptionDataWriter_->get_key_value(key_data,
                                                          handle);

            if (::DDS::RETCODE_OK != retGetKey)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_subscription_bit ")
                          ACE_TEXT("Unable to get_key_value for subscription ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          subscription,
                          handle,
                          retGetKey
                          ));
              }

            ::DDS::ReturnCode_t retDispose =
              bitSubscriptionDataWriter_->dispose(key_data,
                                                  handle);
            if (::DDS::RETCODE_OK != retDispose)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_subscription_bit ")
                          ACE_TEXT("Unable to dispose for subscription ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          subscription,
                          handle,
                          retDispose
                          ));
              }

          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::dispose_subscription_bit:");
          }
        }
    }
#else
  ACE_UNUSED_ARG(subscription);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}



void DCPS_IR_Domain::dispose_publication_bit (DCPS_IR_Publication* publication)
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  if (useBIT_)
    {
      if ( ! publication->is_bit())
        {
          try
          {
            ::DDS::PublicationBuiltinTopicData key_data;
            ::DDS::InstanceHandle_t handle = publication->get_handle();

            ::DDS::ReturnCode_t retGetKey
              = bitPublicationDataWriter_->get_key_value(key_data, handle);

            if (::DDS::RETCODE_OK != retGetKey)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_publication_bit ")
                          ACE_TEXT("Unable to get_key_value for publication ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          publication,
                          handle,
                          retGetKey
                          ));
              }

            ::DDS::ReturnCode_t retDispose =
              bitPublicationDataWriter_->dispose(key_data,
                                                handle);
            if (::DDS::RETCODE_OK != retDispose)
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: DCPS_IR_Domain::dispose_publication_bit ")
                          ACE_TEXT("Unable to dispose for publication ptr %X handle %d.  ")
                          ACE_TEXT("Call returned %d.\n"),
                          publication,
                          handle,
                          retDispose
                          ));
              }

          }
          catch (const CORBA::Exception& ex)
          {
            ex._tao_print_exception (
              "ERROR: Exception caught in DCPS_IR_Domain::dispose_publication_bit:");
          }
        }
    }
#else
  ACE_UNUSED_ARG(publication);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}





#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Node<DCPS_IR_Topic_Description*>;
template class ACE_Unbounded_Set<DCPS_IR_Topic_Description*>;
template class ACE_Unbounded_Set_Iterator<DCPS_IR_Topic_Description*>;

template class ACE_Node<DCPS_IR_Participant*>;
template class ACE_Unbounded_Set<DCPS_IR_Participant*>;
template class ACE_Unbounded_Set_Iterator<DCPS_IR_Participant*>;

template class ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Participant*>;
template class ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Node<DCPS_IR_Topic_Description*>
#pragma instantiate ACE_Unbounded_Set<DCPS_IR_Topic_Description*>
#pragma instantiate ACE_Unbounded_Set_Iterator<DCPS_IR_Topic_Description*>

#pragma instantiate ACE_Node<DCPS_IR_Participant*>
#pragma instantiate ACE_Unbounded_Set<DCPS_IR_Participant*>
#pragma instantiate ACE_Unbounded_Set_Iterator<DCPS_IR_Participant*>

#pragma instantiate ACE_Map_Entry<TAO::DCPS::RepoId,DCPS_IR_Participant*>
#pragma instantiate ACE_Map_Manager<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<TAO::DCPS::RepoId,DCPS_IR_Participant*,ACE_Null_Mutex>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
