#include "DcpsInfo_pch.h"
// -*- C++ -*-
//
// $Id$



#include /**/ "DCPSInfo_i.h"

#include "dds/DCPS/transport/simpleTCP/SimpleTcpFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpTransport.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration_rch.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/BuiltInTopicUtils.h"


#include /**/ "ace/Read_Buffer.h"
#include /**/ "ace/OS_NS_stdio.h"

#include /**/ "tao/debug.h"


// constructor
TAO_DDS_DCPSInfo_i::TAO_DDS_DCPSInfo_i (void)
{
}


//  destructor
TAO_DDS_DCPSInfo_i::~TAO_DDS_DCPSInfo_i (void)
{
}


TAO::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::assert_topic (
    TAO::DCPS::RepoId_out topicId,
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    const char * topicName,
    const char * dataTypeName,
    const ::DDS::TopicQos & qos
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* participantPtr;
  if (domainPtr->find_participant(participantId,participantPtr) != 0)
    {
      // bad participant id
      throw TAO::DCPS::Invalid_Participant();
    }

  TAO::DCPS::TopicStatus topicStatus = domainPtr->add_topic(topicId,
                                                         topicName,
                                                         dataTypeName,
                                                         qos,
                                                         participantPtr);

  return topicStatus;
}


TAO::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::find_topic (
    ::DDS::DomainId_t domainId,
    const char * topicName,
    CORBA::String_out dataTypeName,
    ::DDS::TopicQos_out qos,
    TAO::DCPS::RepoId_out topicId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw TAO::DCPS::Invalid_Domain();
    }

  TAO::DCPS::TopicStatus status = TAO::DCPS::NOT_FOUND;

  DCPS_IR_Topic* topic = 0;
  qos = new ::DDS::TopicQos;

  status = domainPtr->find_topic(topicName, topic);
  if (0 != topic)
  {
    status = TAO::DCPS::FOUND;
    const DCPS_IR_Topic_Description* desc = topic->get_topic_description();
    dataTypeName = desc->get_dataTypeName();
    *qos = *(topic->get_topic_qos());
    topicId = topic->get_id();
  }

  return status;
}


TAO::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::remove_topic (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    TAO::DCPS::RepoId topicId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw TAO::DCPS::Invalid_Topic();
    }

  TAO::DCPS::TopicStatus removedStatus = domainPtr->remove_topic(partPtr, topic);

  return removedStatus;
}


TAO::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::enable_topic (
  ::DDS::DomainId_t domainId,
  TAO::DCPS::RepoId participantId,
  TAO::DCPS::RepoId topicId
  ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
  CORBA::SystemException
  , TAO::DCPS::Invalid_Domain
  , TAO::DCPS::Invalid_Participant
  , TAO::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw TAO::DCPS::Invalid_Topic();
    }

  return TAO::DCPS::ENABLED;
}
  

TAO::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_publication (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    TAO::DCPS::RepoId topicId,
    TAO::DCPS::DataWriterRemote_ptr publication,
    const ::DDS::DataWriterQos & qos,
    const TAO::DCPS::TransportInterfaceInfo & transInfo,
    const ::DDS::PublisherQos & publisherQos
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw TAO::DCPS::Invalid_Topic();
    }

  TAO::DCPS::RepoId pubId = domainPtr->get_next_publication_id();

  DCPS_IR_Publication* pubPtr;
  ACE_NEW_RETURN(pubPtr,
                 DCPS_IR_Publication(
                   pubId,
                   partPtr,
                   topic,
                   publication,
                   qos,
                   transInfo,
                   publisherQos),
                 0);

  if (partPtr->add_publication(pubPtr) != 0)
    {
      // failed to add.  we are responsible for the memory.
      pubId = 0;
      delete pubPtr;
      pubPtr = 0;
    }
  else if (topic->add_publication_reference(pubPtr) != 0)
    {
      // Failed to add to the topic
      // so remove from participant and fail.
      partPtr->remove_publication(pubId);
      pubId = 0;
    }

  domainPtr->remove_dead_participants();

  return pubId;
}


void TAO_DDS_DCPSInfo_i::remove_publication (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    TAO::DCPS::RepoId publicationId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Publication
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  if (partPtr->remove_publication(publicationId) != 0)
    {
      domainPtr->remove_dead_participants();

      // throw exception because the publication was not removed!
      throw TAO::DCPS::Invalid_Publication();
    }

  domainPtr->remove_dead_participants();
}


TAO::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_subscription (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    TAO::DCPS::RepoId topicId,
    TAO::DCPS::DataReaderRemote_ptr subscription,
    const ::DDS::DataReaderQos & qos,
    const TAO::DCPS::TransportInterfaceInfo & transInfo,
    const ::DDS::SubscriberQos & subscriberQos
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw TAO::DCPS::Invalid_Topic();
    }

  TAO::DCPS::RepoId subId = domainPtr->get_next_subscription_id ();

  DCPS_IR_Subscription* subPtr;
  ACE_NEW_RETURN(subPtr,
                 DCPS_IR_Subscription(
                   subId,
                   partPtr,
                   topic,
                   subscription,
                   qos,
                   transInfo,
                   subscriberQos),
                 0);

  DCPS_IR_Topic_Description* description = subPtr->get_topic_description ();

  if (partPtr->add_subscription(subPtr) != 0)
    {
      // failed to add.  we are responsible for the memory.
      subId = 0;
      delete subPtr;
      subPtr = 0;
    }
  else if (description->add_subscription_reference(subPtr) != 0)
    {
      // No associations were made so remove and fail.
      partPtr->remove_subscription(subId);
      subId = 0;
    }

  domainPtr->remove_dead_participants();

  return subId;
}


void TAO_DDS_DCPSInfo_i::remove_subscription (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId,
    TAO::DCPS::RepoId subscriptionId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Subscription
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  if (partPtr->remove_subscription(subscriptionId) != 0)
    {
      // throw exception because the subscription was not removed!
      throw TAO::DCPS::Invalid_Subscription();
    }

  domainPtr->remove_dead_participants();
}


TAO::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_domain_participant (
    ::DDS::DomainId_t domain,
    const ::DDS::DomainParticipantQos & qos
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
  ))
{
  DCPS_IR_Domain* domainPtr;

  if (domains_.find(domain, domainPtr) != 0)
    {
      // throw exception
      throw TAO::DCPS::Invalid_Domain();
    }

  TAO::DCPS::RepoId participantId = domainPtr->get_next_participant_id ();

  DCPS_IR_Participant* participant;

  ACE_NEW_RETURN(participant,
                 DCPS_IR_Participant(
                   participantId,
                   domainPtr,
                   qos),
                 0);

  int status = domainPtr->add_participant (participant);

  if (0 != status)
    {
      // Adding the participant failed return the invalid
      // pariticipant Id number.
      participantId = 0;
      delete participant;
      participant = 0;
    }

  return participantId;
}


void TAO_DDS_DCPSInfo_i::remove_domain_participant (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId participantId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;

  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  int status = domainPtr->remove_participant (participantId);

  if (0 != status)
    {
      // Removing the participant failed
      throw TAO::DCPS::Invalid_Participant();
    }

}


void TAO_DDS_DCPSInfo_i::ignore_domain_participant (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId myParticipantId,
    TAO::DCPS::RepoId otherParticipantId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  partPtr->ignore_participant(otherParticipantId);

  domainPtr->remove_dead_participants();
}
  

void TAO_DDS_DCPSInfo_i::ignore_topic (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId myParticipantId,
    TAO::DCPS::RepoId topicId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  partPtr->ignore_topic(topicId);

  domainPtr->remove_dead_participants();
}
  

void TAO_DDS_DCPSInfo_i::ignore_subscription (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId myParticipantId,
    TAO::DCPS::RepoId subscriptionId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Subscription
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  partPtr->ignore_subscription(subscriptionId);

  domainPtr->remove_dead_participants();
}


void TAO_DDS_DCPSInfo_i::ignore_publication (
    ::DDS::DomainId_t domainId,
    TAO::DCPS::RepoId myParticipantId,
    TAO::DCPS::RepoId publicationId
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , TAO::DCPS::Invalid_Domain
    , TAO::DCPS::Invalid_Participant
    , TAO::DCPS::Invalid_Publication
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw TAO::DCPS::Invalid_Participant();
    }

  partPtr->ignore_publication(publicationId);

  domainPtr->remove_dead_participants();
}


int TAO_DDS_DCPSInfo_i::load_domains (const char* filename,
                                      bool use_bit)
{
  int status;

  FILE* file = 0;
  file = ACE_OS::fopen (filename, "r");
  if (file == 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR, "ERROR: cannot open domain id file <%s>\n",
                         filename), -1);
    }

  ACE_Read_Buffer tmp(file);
  char* buf = tmp.read ('\n', '\n', '\0');
  int replaced = tmp.replaced();
  while (0 != replaced)
    {

      ::DDS::DomainId_t domainId = ACE_OS::strtol(buf, 0, 10);
      tmp.alloc ()->free (buf);
      if ( 0 == domainId)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             "ERROR: reading domain id file <%s>\n",
                              filename), -1);
        }

      DCPS_IR_Domain* domainPtr;
      ACE_NEW_RETURN(domainPtr,
                     DCPS_IR_Domain(domainId),
                     -1);

      status = domains_.bind(domainId, domainPtr);

      switch (status)
        {
        case 0:
          {
            int bit_status = 0;

            if (use_bit)
              {
#if !defined (DDS_HAS_MINIMUM_BIT)
                bit_status = domainPtr->init_built_in_topics();
#endif // !defined (DDS_HAS_MINIMUM_BIT)
              }

            if (0 == bit_status)
              {
                if (TAO_debug_level > 0)
                  {
                    ACE_DEBUG((LM_DEBUG, ACE_TEXT("TAO_DDS_DCPSInfo_i::load_domains ")
                      ACE_TEXT("Successfully loaded domain %X id: %d\n"),
                      domainPtr, domainId));
                  }
              }
            else
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("ERROR: Failed to initialize the Built-In Topics ")
                          ACE_TEXT("when loading domain id = %d\n"),
                          domainId));
                domains_.unbind(domainId);
              }

          }
          break;
        case 1:
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: TAO_DDS_DCPSInfo_i::load_domains ")
            ACE_TEXT("Attempted to load existing domain id %d\n"),
            domainId));
          break;
        case -1:
          ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: TAO_DDS_DCPSInfo_i::load_domains ")
            ACE_TEXT("Unknown error while loading domain id %d\n"),
            domainId));
        };

      buf = tmp.read ('\n', '\n', '\0');
      replaced = tmp.replaced();
    }

  ACE_OS::fclose (file);
  return domains_.current_size();

}


int TAO_DDS_DCPSInfo_i::init_transport (int listen_address_given, 
                                        const ACE_INET_Addr listen)
{
  int status = 0;

  TheTransportFactory->register_type(TAO::DCPS::BIT_SIMPLE_TCP,
                                     new TAO::DCPS::SimpleTcpFactory());

  TAO::DCPS::SimpleTcpConfiguration_rch config =
      new TAO::DCPS::SimpleTcpConfiguration();

  if (listen_address_given)
    config->local_address_ = listen;
  //else
    // defaults to framework/OS assigned address

  TAO::DCPS::TransportImpl_rch transport_impl =
      TheTransportFactory->create(TAO::DCPS::BIT_ALL_TRAFFIC,
                                  TAO::DCPS::BIT_SIMPLE_TCP);

  if (transport_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::init_transport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  return status;
}


#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)

template class ACE_Map_Entry<::DDS::DomainId_t,DCPS_IR_Domain*>;
template class ACE_Map_Manager<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>;
template class ACE_Map_Iterator_Base<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>;
template class ACE_Map_Iterator<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>;
template class ACE_Map_Reverse_Iterator<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>;

#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)

#pragma instantiate ACE_Map_Entry<::DDS::DomainId_t,DCPS_IR_Domain*>
#pragma instantiate ACE_Map_Manager<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator_Base<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Iterator<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>
#pragma instantiate ACE_Map_Reverse_Iterator<::DDS::DomainId_t,DCPS_IR_Domain*,ACE_Null_Mutex>

#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */
