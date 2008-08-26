#include "DcpsInfo_pch.h"
// -*- C++ -*-
//
// $Id$



#include /**/ "DCPSInfo_i.h"

#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "UpdateManager.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

#include /**/ "tao/debug.h"

#include /**/ "ace/Read_Buffer.h"
#include /**/ "ace/OS_NS_stdio.h"
#include "ace/Dynamic_Service.h"

// constructor
TAO_DDS_DCPSInfo_i::TAO_DDS_DCPSInfo_i (CORBA::ORB_ptr orb
                                        , bool reincarnate
                                        , long federation)
  : orb_ (CORBA::ORB::_duplicate (orb))
    , federation_(federation)
    , participantIdGenerator_( federation)
    , um_ (0)
    , reincarnate_ (reincarnate)
{
}


//  destructor
TAO_DDS_DCPSInfo_i::~TAO_DDS_DCPSInfo_i (void)
{
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::attach_participant (
  ::DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& participantId
)
ACE_THROW_SPEC ((
  CORBA::SystemException
  , OpenDDS::DCPS::Invalid_Domain
  , OpenDDS::DCPS::Invalid_Participant
))
{
  // Grab the domain.
  DCPS_IR_Domain* domain;
  if( 0 != this->domains_.find( domainId, domain)) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* participant;
  if( 0 != domain->find_participant( participantId, participant)) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Establish ownership within the local repository.
  participant->takeOwnership();

  return false;
}

void
TAO_DDS_DCPSInfo_i::changeOwnership(
  ::DDS::DomainId_t              domainId,
  const ::OpenDDS::DCPS::RepoId& participantId,
  long                           sender,
  long                           owner
)
{
  // Grab the domain.
  DCPS_IR_Domain* domain;
  if( 0 != this->domains_.find( domainId, domain)) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* participant;
  if( 0 != domain->find_participant( participantId, participant)) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Establish the ownership.
  participant->changeOwner( sender, owner);
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::assert_topic (
    OpenDDS::DCPS::RepoId_out topicId,
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const char * topicName,
    const char * dataTypeName,
    const ::DDS::TopicQos & qos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* participantPtr;
  if (domainPtr->find_participant(participantId,participantPtr) != 0)
    {
      // bad participant id
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  OpenDDS::DCPS::TopicStatus topicStatus = domainPtr->add_topic(topicId,
                                                         topicName,
                                                         dataTypeName,
                                                         qos,
                                                         participantPtr);

  if( this->um_) {
    Update::UTopic topic (domainId, topicId, participantId
                                 , topicName, dataTypeName
                                 , const_cast< ::DDS::TopicQos &>(qos));
    this->um_->create (topic);
  }

  return topicStatus;
}

bool
TAO_DDS_DCPSInfo_i::add_topic (const OpenDDS::DCPS::RepoId& topicId,
                               ::DDS::DomainId_t domainId,
                               const OpenDDS::DCPS::RepoId& participantId,
                               const char* topicName,
                               const char* dataTypeName,
                               const ::DDS::TopicQos& qos)
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid domain Id: %d\n"), domainId));
      return false;
    }

  DCPS_IR_Participant* participantPtr;
  if (domainPtr->find_participant(participantId,participantPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid participant Id: %d\n"), participantId));
      return false;
    }

  OpenDDS::DCPS::TopicStatus topicStatus
    = domainPtr->force_add_topic (topicId, topicName, dataTypeName,
                                  qos, participantPtr);

  if (topicStatus != OpenDDS::DCPS::CREATED) {
    return false;
  }

  OpenDDS::DCPS::GuidConverter converter(
    const_cast< OpenDDS::DCPS::RepoId*>( &topicId)
  );
  // See if we are adding a topic that was created within this
  // repository or a different repository.
  if( converter.federationId() == this->federation_) {
    // Ensure the topic GUID values do not conflict.
    participantPtr->last_topic_key( converter.value());
  }

  return true;
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::find_topic (
    ::DDS::DomainId_t domainId,
    const char * topicName,
    CORBA::String_out dataTypeName,
    ::DDS::TopicQos_out qos,
    OpenDDS::DCPS::RepoId_out topicId
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  OpenDDS::DCPS::TopicStatus status = OpenDDS::DCPS::NOT_FOUND;

  DCPS_IR_Topic* topic = 0;
  qos = new ::DDS::TopicQos;

  status = domainPtr->find_topic(topicName, topic);
  if (0 != topic)
  {
    status = OpenDDS::DCPS::FOUND;
    const DCPS_IR_Topic_Description* desc = topic->get_topic_description();
    dataTypeName = desc->get_dataTypeName();
    *qos = *(topic->get_topic_qos());
    topicId = topic->get_id();
  }

  return status;
}


OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::remove_topic (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  OpenDDS::DCPS::TopicStatus removedStatus = domainPtr->remove_topic(partPtr, topic);

  if( this->um_ && partPtr->owner()) {
    Update::IdPath path( domainId, participantId, topicId);
    this->um_->destroy( path, Update::Topic);
  }

  return removedStatus;
}


OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::enable_topic (
  ::DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId
  )
  ACE_THROW_SPEC ((
  CORBA::SystemException
  , OpenDDS::DCPS::Invalid_Domain
  , OpenDDS::DCPS::Invalid_Participant
  , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  return OpenDDS::DCPS::ENABLED;
}


OpenDDS::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_publication (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataWriterRemote_ptr publication,
    const ::DDS::DataWriterQos & qos,
    const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
    const ::DDS::PublisherQos & publisherQos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic = domainPtr->find_topic( topicId);
  if( topic == 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  OpenDDS::DCPS::RepoId pubId = partPtr->get_next_publication_id();

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
                 OpenDDS::DCPS::GUID_UNKNOWN);

  if (partPtr->add_publication(pubPtr) != 0)
    {
      // failed to add.  we are responsible for the memory.
      pubId = OpenDDS::DCPS::GUID_UNKNOWN;
      delete pubPtr;
      pubPtr = 0;
    }
  else if (topic->add_publication_reference(pubPtr) != 0)
    {
      // Failed to add to the topic
      // so remove from participant and fail.
      partPtr->remove_publication(pubId);
      pubId = OpenDDS::DCPS::GUID_UNKNOWN;
    }

  if( this->um_) {
    CORBA::String_var callback = orb_->object_to_string (publication);

    Update::UWActor actor (domainId, pubId, topicId, participantId, Update::DataWriter
                                  , callback.in()
                                  , const_cast< ::DDS::PublisherQos &>(publisherQos)
                                  , const_cast< ::DDS::DataWriterQos &>(qos)
                                  , const_cast< OpenDDS::DCPS::TransportInterfaceInfo &>
                                  (transInfo));
    this->um_->create( actor);
  }

  domainPtr->remove_dead_participants();
  return pubId;
}

bool
TAO_DDS_DCPSInfo_i::add_publication (::DDS::DomainId_t domainId,
                                     const OpenDDS::DCPS::RepoId& participantId,
                                     const OpenDDS::DCPS::RepoId& topicId,
                                     const OpenDDS::DCPS::RepoId& pubId,
                                     const char* pub_str,
                                     const ::DDS::DataWriterQos & qos,
                                     const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
                                     const ::DDS::PublisherQos & publisherQos)
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid domain Id: %d\n"), domainId));
      return false;
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid participant Id: %d\n")
                  , participantId));
      return false;
    }

  DCPS_IR_Topic* topic = domainPtr->find_topic( topicId);
  if( topic == 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  /// @TODO: Check if this is already stored.  If so, just clear the callback IOR.

  CORBA::Object_var obj = orb_->string_to_object (pub_str);
  OpenDDS::DCPS::DataWriterRemote_var publication
    = OpenDDS::DCPS::DataWriterRemote::_unchecked_narrow (obj.in());

  DCPS_IR_Publication* pubPtr;
  ACE_NEW_RETURN(pubPtr,
                 DCPS_IR_Publication(
                   pubId,
                   partPtr,
                   topic,
                   publication.in(),
                   qos,
                   transInfo,
                   publisherQos),
                 0);

  if (partPtr->add_publication(pubPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add publisher to ")
                  "participant list.\n"));

      // failed to add.  we are responsible for the memory.
      delete pubPtr;
      return false;
    }
  else if (topic->add_publication_reference(pubPtr, false) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add publisher to ")
                  "topic list.\n"));

      // Failed to add to the topic
      // so remove from participant and fail.
      partPtr->remove_publication(pubId);
      return false;
    }

  OpenDDS::DCPS::GuidConverter converter(
    const_cast< OpenDDS::DCPS::RepoId*>( &pubId)
  );
  // See if we are adding a publication that was created within this
  // repository or a different repository.
  if( converter.federationId() == this->federation_) {
    // Ensure the publication GUID values do not conflict.
    partPtr->last_publication_key( converter.value());
  }

  return true;
}


void TAO_DDS_DCPSInfo_i::remove_publication (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& publicationId
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Publication
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  if (partPtr->remove_publication(publicationId) != 0)
    {
      domainPtr->remove_dead_participants();

      // throw exception because the publication was not removed!
      throw OpenDDS::DCPS::Invalid_Publication();
    }

  domainPtr->remove_dead_participants();

  if( this->um_ && partPtr->owner()) {
    Update::IdPath path( domainId, participantId, publicationId);
    this->um_->destroy( path, Update::Actor, Update::DataWriter);
  }
}


OpenDDS::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_subscription (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    OpenDDS::DCPS::DataReaderRemote_ptr subscription,
    const ::DDS::DataReaderQos & qos,
    const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
    const ::DDS::SubscriberQos & subscriberQos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic = domainPtr->find_topic( topicId);
  if( topic == 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  OpenDDS::DCPS::RepoId subId = partPtr->get_next_subscription_id();

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
                 OpenDDS::DCPS::GUID_UNKNOWN);

  if (partPtr->add_subscription(subPtr) != 0)
    {
      // failed to add.  we are responsible for the memory.
      subId = OpenDDS::DCPS::GUID_UNKNOWN;
      delete subPtr;
      subPtr = 0;
    }
  else if (topic->add_subscription_reference(subPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add subscription to ")
                  "topic list.\n"));
      // No associations were made so remove and fail.
      partPtr->remove_subscription(subId);
      subId = OpenDDS::DCPS::GUID_UNKNOWN;
    }

  if( this->um_) {
    CORBA::String_var callback = orb_->object_to_string (subscription);

    Update::URActor actor (domainId, subId, topicId, participantId, Update::DataReader
                                  , callback.in()
                                  , const_cast< ::DDS::SubscriberQos &>(subscriberQos)
                                  , const_cast< ::DDS::DataReaderQos &>(qos)
                                  , const_cast< OpenDDS::DCPS::TransportInterfaceInfo &>
                                  (transInfo));

    this->um_->create( actor);
  }

  domainPtr->remove_dead_participants();

  return subId;
}

bool
TAO_DDS_DCPSInfo_i::add_subscription (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& topicId,
    const OpenDDS::DCPS::RepoId& subId,
    const char* sub_str,
    const ::DDS::DataReaderQos & qos,
    const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
    const ::DDS::SubscriberQos & subscriberQos
  )
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid domain Id: %d\n"), domainId));
      return false;
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Invalid participant Id: %d\n")
                  , participantId));
      return false;
    }

  DCPS_IR_Topic* topic = domainPtr->find_topic( topicId);
  if( topic == 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  CORBA::Object_var obj = orb_->string_to_object (sub_str);
  OpenDDS::DCPS::DataReaderRemote_var subscription
    = OpenDDS::DCPS::DataReaderRemote::_unchecked_narrow (obj.in());

  DCPS_IR_Subscription* subPtr;
  ACE_NEW_RETURN(subPtr,
                 DCPS_IR_Subscription(
                   subId,
                   partPtr,
                   topic,
                   subscription.in(),
                   qos,
                   transInfo,
                   subscriberQos),
                 0);

  if (partPtr->add_subscription(subPtr) != 0)
    {
      ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add publisher to ")
                  "participant list.\n"));

      // failed to add.  we are responsible for the memory.
      delete subPtr;
      return false;
    }
  else if (topic->add_subscription_reference (subPtr, false))
  {
    ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add subscription to ")
      "topic list.\n"));

    // No associations were made so remove and fail.
    partPtr->remove_subscription(subId);
    return false;
  }

  OpenDDS::DCPS::GuidConverter converter(
    const_cast< OpenDDS::DCPS::RepoId*>( &subId)
  );
  // See if we are adding a subscription that was created within this
  // repository or a different repository.
  if( converter.federationId() == this->federation_) {
    // Ensure the subscription GUID values do not conflict.
    partPtr->last_subscription_key( converter.value());
  }

  return true;
}


void TAO_DDS_DCPSInfo_i::remove_subscription (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId,
    const OpenDDS::DCPS::RepoId& subscriptionId
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Subscription
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  if (partPtr->remove_subscription(subscriptionId) != 0)
    {
      // throw exception because the subscription was not removed!
      throw OpenDDS::DCPS::Invalid_Subscription();
    }

  domainPtr->remove_dead_participants();

  if( this->um_ && partPtr->owner()) {
    Update::IdPath path( domainId, participantId, subscriptionId);
    this->um_->destroy( path, Update::Actor, Update::DataReader);
  }
}


OpenDDS::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_domain_participant (
    ::DDS::DomainId_t domain,
    const ::DDS::DomainParticipantQos & qos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
  ))
{
  DCPS_IR_Domain* domainPtr;

  if (domains_.find(domain, domainPtr) != 0)
    {
      // throw exception
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  OpenDDS::DCPS::RepoId participantId = domainPtr->get_next_participant_id();

  DCPS_IR_Participant* participant;

  ACE_NEW_RETURN(participant,
                 DCPS_IR_Participant(
                   this->federation_,
                   participantId,
                   domainPtr,
                   qos, um_),
                 OpenDDS::DCPS::GUID_UNKNOWN);

  participant->takeOwnership();
  int status = domainPtr->add_participant (participant);

  if( 0 != status) {
    // Adding the participant failed return the invalid
    // pariticipant Id number.
    participantId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete participant;
    participant = 0;

  } else if( this->um_) {
    Update::UParticipant participant
      (domain, participantId, const_cast< ::DDS::DomainParticipantQos &>(qos));
    this->um_->create (participant);
  }

  if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
    std::stringstream buffer;
    long key = ::OpenDDS::DCPS::GuidConverter( participantId);
    buffer << participantId << "(" << std::hex << key << ")";
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
      ACE_TEXT("domain %d loaded participant %s at 0x%x.\n"),
      domain,
      buffer.str().c_str(),
      participant
    ));
  }

  return participantId;
}

bool
TAO_DDS_DCPSInfo_i::add_domain_participant (::DDS::DomainId_t domainId
                                            , const OpenDDS::DCPS::RepoId& participantId
                                            , const ::DDS::DomainParticipantQos & qos)
{
  DCPS_IR_Domain* domainPtr;
  if( domains_.find(domainId, domainPtr) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
      ACE_TEXT("invalid domain Id: %d\n"),
      domainId
    ));
    return false;
  }

  DCPS_IR_Participant* partPtr;
  if( domainPtr->find_participant(participantId, partPtr) == 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
      ACE_TEXT("participant id %s already exists.\n"),
      participantId
    ));
    return false;
  }

  // Prepare to manipulate the participant's Id value.
  OpenDDS::DCPS::GuidConverter converter(
    const_cast< OpenDDS::DCPS::RepoId*>( &participantId)
  );

  DCPS_IR_Participant* participant;
  ACE_NEW_RETURN (participant,
                 DCPS_IR_Participant( this->federation_,
                                      participantId,
                                      domainPtr,
                                      qos, um_), 0);

  int status = domainPtr->add_participant (participant);
  if( 0 != status) {
    std::stringstream buffer;
    buffer << participantId << "(" << std::hex << long(converter) << ")";
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
      ACE_TEXT("domain %d failed to load participant %s at 0x%x.\n"),
      domainId,
      buffer.str().c_str(),
      participant
    ));

    delete participant;
    return false;
  }

  // See if we are adding a participant that was created within this
  // repository or a different repository.
  if( converter.federationId() == this->federation_) {
    // Ensure the participant GUID values do not conflict.
    domainPtr->last_participant_key( converter.participantId());
    if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
        ACE_TEXT("Adjusting highest participant Id value to at least %d.\n"),
        converter.participantId()
      ));
    }
  }

  if( ::OpenDDS::DCPS::DCPS_debug_level > 4) {
    std::stringstream buffer;
    buffer << participantId << "(" << std::hex << long(converter) << ")";
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
      ACE_TEXT("domain %d loaded participant %s at 0x%x.\n"),
      domainId,
      buffer.str().c_str(),
      participant
    ));
  }

  return true;
}

void TAO_DDS_DCPSInfo_i::remove_domain_participant (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& participantId
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;

  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  CORBA::Boolean dont_notify_lost = 0;
  int status = domainPtr->remove_participant (participantId, dont_notify_lost);

  if (0 != status)
    {
      // Removing the participant failed
      throw OpenDDS::DCPS::Invalid_Participant();
    }
}


void TAO_DDS_DCPSInfo_i::ignore_domain_participant (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    CORBA::Long ignoreKey
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  OpenDDS::DCPS::RepoId ignoreId = domainPtr->participant( ignoreKey);
  partPtr->ignore_participant( ignoreId);

  domainPtr->remove_dead_participants();
}


void TAO_DDS_DCPSInfo_i::ignore_topic (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    CORBA::Long ignoreKey
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  OpenDDS::DCPS::RepoId ignoreId = domainPtr->topic( ignoreKey);
  partPtr->ignore_topic( ignoreId);

  domainPtr->remove_dead_participants();
}


void TAO_DDS_DCPSInfo_i::ignore_subscription (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    CORBA::Long ignoreKey
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Subscription
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  OpenDDS::DCPS::RepoId ignoreId = domainPtr->subscription( ignoreKey);
  partPtr->ignore_subscription( ignoreId);

  domainPtr->remove_dead_participants();
}


void TAO_DDS_DCPSInfo_i::ignore_publication (
    ::DDS::DomainId_t domainId,
    const OpenDDS::DCPS::RepoId& myParticipantId,
    CORBA::Long ignoreKey
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Publication
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(myParticipantId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  OpenDDS::DCPS::RepoId ignoreId = domainPtr->publication( ignoreKey);
  partPtr->ignore_publication( ignoreId);

  domainPtr->remove_dead_participants();
}


CORBA::Boolean TAO_DDS_DCPSInfo_i::update_publication_qos (
  ::DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const ::DDS::DataWriterQos & qos,
  const ::DDS::PublisherQos & publisherQos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Publication
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(partId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Publication* pub;
  if (partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0)
    {
      throw OpenDDS::DCPS::Invalid_Publication();
    }

  Update::SpecificQos qosType; 
  if (pub->set_qos (qos, publisherQos, qosType) == false) // not compatible
    return 0;

  if( this->um_) {
    Update::IdPath path( domainId, partId, dwId);
    switch( qosType) {
      case Update::DataWriterQos:
        this->um_->update( path, qos);
        break;

      case Update::PublisherQos:
        this->um_->update( path, publisherQos);
        break;

      case Update::NoQos:
      default:
        break;
    }
  }

  return 1;
}

void
TAO_DDS_DCPSInfo_i::update_publication_qos (
  ::DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const ::DDS::DataWriterQos&  qos
)
{
  DCPS_IR_Domain* domainPtr;
  if( domains_.find(domainId, domainPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* partPtr;
  if( domainPtr->find_participant(partId, partPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* pub;
  if( partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  pub->set_qos( qos);
}

void
TAO_DDS_DCPSInfo_i::update_publication_qos (
  ::DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const ::DDS::PublisherQos&   qos
)
{
  DCPS_IR_Domain* domainPtr;
  if( domains_.find(domainId, domainPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* partPtr;
  if( domainPtr->find_participant(partId, partPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* pub;
  if( partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  pub->set_qos( qos);
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_subscription_qos (
  ::DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const ::DDS::DataReaderQos & qos,
  const ::DDS::SubscriberQos & subscriberQos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Subscription
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(partId, partPtr) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Subscription* sub;
  if (partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0)
    {
      throw OpenDDS::DCPS::Invalid_Subscription();
    }

  Update::SpecificQos qosType;
  if (sub->set_qos (qos, subscriberQos, qosType) == false)
    return 0;

  if( this->um_) {
    Update::IdPath path( domainId, partId, drId);
    switch( qosType) {
      case Update::DataReaderQos:
        this->um_->update( path, qos);
        break;

      case Update::SubscriberQos:
        this->um_->update( path, subscriberQos);
        break;

      case Update::NoQos:
      default:
        break;
    }
  }
  return 1;
}

void
TAO_DDS_DCPSInfo_i::update_subscription_qos (
  ::DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const ::DDS::DataReaderQos&  qos
)
{
  DCPS_IR_Domain* domainPtr;
  if( domains_.find(domainId, domainPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* partPtr;
  if( domainPtr->find_participant(partId, partPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* sub;
  if( partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  sub->set_qos( qos);
}

void
TAO_DDS_DCPSInfo_i::update_subscription_qos (
  ::DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const ::DDS::SubscriberQos&  qos
)
{
  DCPS_IR_Domain* domainPtr;
  if( domains_.find(domainId, domainPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* partPtr;
  if( domainPtr->find_participant(partId, partPtr) != 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* sub;
  if( partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  sub->set_qos( qos);
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_topic_qos (
  const OpenDDS::DCPS::RepoId& topicId,
  ::DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const ::DDS::TopicQos & qos
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
    , OpenDDS::DCPS::Invalid_Domain
    , OpenDDS::DCPS::Invalid_Participant
    , OpenDDS::DCPS::Invalid_Topic
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId,partPtr) != 0)
    {
      // bad participant id
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  DCPS_IR_Topic* topic;
  if (partPtr->find_topic_reference(topicId, topic) != 0)
    {
      throw OpenDDS::DCPS::Invalid_Topic();
    }

  if (topic->set_topic_qos (qos) == false)
    return 0;

  if( this->um_ && partPtr->owner()) {
    Update::IdPath path( domainId, participantId, topicId);
    this->um_->update( path, qos);
  }
  return 1;
}


CORBA::Boolean TAO_DDS_DCPSInfo_i::update_domain_participant_qos (
    ::DDS::DomainId_t domainId,
    const ::OpenDDS::DCPS::RepoId& participantId,
    const ::DDS::DomainParticipantQos & qos
  )
  ACE_THROW_SPEC ((
    ::CORBA::SystemException,
    ::OpenDDS::DCPS::Invalid_Domain,
    ::OpenDDS::DCPS::Invalid_Participant
  ))
{
  DCPS_IR_Domain* domainPtr;
  if (domains_.find(domainId, domainPtr) != 0)
    {
      // bad domain id
      throw OpenDDS::DCPS::Invalid_Domain();
    }

  DCPS_IR_Participant* partPtr;
  if (domainPtr->find_participant(participantId,partPtr) != 0)
    {
      // bad participant id
      throw OpenDDS::DCPS::Invalid_Participant();
    }

  if (partPtr->set_qos (qos) == false)
    return 0;

  if( this->um_ && partPtr->owner()) {
    Update::IdPath path( domainId, participantId, participantId);
    this->um_->update( path, qos);
  }

  return 1;
}


int TAO_DDS_DCPSInfo_i::load_domains (const ACE_TCHAR* filename,
                                      bool use_bit)
{
  int status;

  FILE* file = 0;
  file = ACE_OS::fopen (filename, ACE_TEXT("r"));
  if (file == 0)
    {
      ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT("ERROR: cannot open domain id file <%s>\n"),
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
                     DCPS_IR_Domain(domainId, this->participantIdGenerator_),
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
                if (::OpenDDS::DCPS::DCPS_debug_level > 0)
                  {
                    ACE_DEBUG((LM_DEBUG,
                      ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::load_domains: ")
                      ACE_TEXT("successfully loaded domain %d at %x.\n"),
                      domainId,
                      domainPtr
                    ));
                  }
              }
            else
              {
                ACE_ERROR((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: Failed to initialize the Built-In Topics ")
                          ACE_TEXT("when loading domain id = %d\n"),
                          domainId));
                domains_.unbind(domainId);
              }

          }
          break;
        case 1:
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::load_domains ")
            ACE_TEXT("Attempted to load existing domain id %d\n"),
            domainId));
          break;
        case -1:
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::load_domains ")
            ACE_TEXT("Unknown error while loading domain id %d\n"),
            domainId));
        };

      buf = tmp.read ('\n', '\n', '\0');
      replaced = tmp.replaced();
    }

  ACE_OS::fclose (file);

  // Initialize persistence
  if (!this->init_persistence ()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::load_domains ")
               ACE_TEXT("Unable to initialize persistence.\n")));
  }

  return domains_.current_size();
}


int TAO_DDS_DCPSInfo_i::init_transport (int listen_address_given,
                                        const ACE_INET_Addr listen)
{
  int status = 0;

  OpenDDS::DCPS::TransportImpl_rch trans_impl
    = TheTransportFactory->create_transport_impl (OpenDDS::DCPS::BIT_ALL_TRAFFIC,
                                                  ACE_TEXT("SimpleTcp"),
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->get_or_create_configuration (OpenDDS::DCPS::BIT_ALL_TRAFFIC,
                                                        ACE_TEXT("SimpleTcp"));
  OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (config.in ());

  if (listen_address_given)
    tcp_config->local_address_ = listen;

  if (trans_impl->configure(config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::init_transport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }
  return status;
}

bool
TAO_DDS_DCPSInfo_i::receive_image (const Update::UImage& image)
{
  for (Update::UImage::ParticipantSeq::const_iterator
         iter = image.participants.begin();
       iter != image.participants.end(); iter++)
    {
      const Update::UParticipant* part = *iter;

      if (!this->add_domain_participant (part->domainId, part->participantId
                                         , part->participantQos)) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add Domain Participant.\n")));
        return false;
      }
    }

  for (Update::UImage::TopicSeq::const_iterator iter = image.topics.begin();
       iter != image.topics.end(); iter++)
    {
      const Update::UTopic* topic = *iter;

      if (!this->add_topic (topic->topicId, topic->domainId
                            , topic->participantId, topic->name.c_str()
                            , topic->dataType.c_str(), topic->topicQos)) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add Domain Topic.\n")));
        return false;
      }
    }

  for (Update::UImage::ReaderSeq::const_iterator iter = image.actors.begin();
       iter != image.actors.end(); iter++)
    {
      const Update::URActor* sub = *iter;

      if (!this->add_subscription (sub->domainId, sub->participantId
                                   , sub->topicId, sub->actorId
                                   , sub->callback.c_str(), sub->drdwQos
                                   , sub->transportInterfaceInfo
                                   , sub->pubsubQos)) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add Subscriber.\n")));
        return false;
      }
    }

  for (Update::UImage::WriterSeq::const_iterator iter = image.wActors.begin();
       iter != image.wActors.end(); iter++)
    {
      const Update::UWActor* pub = *iter;

      if (!this->add_publication (pub->domainId, pub->participantId
                                  , pub->topicId, pub->actorId
                                  , pub->callback.c_str() , pub->drdwQos
                                  , pub->transportInterfaceInfo
                                  , pub->pubsubQos)) {
        ACE_ERROR ((LM_ERROR, ACE_TEXT("Failed to add Publisher.\n")));
        return false;
      }
    }

  return true;
}

void
TAO_DDS_DCPSInfo_i::add( Update::Updater* updater)
{
  if( this->um_) {
    this->um_->add( updater);
  }
}

bool
TAO_DDS_DCPSInfo_i::init_persistence (void)
{
  um_ = ACE_Dynamic_Service<UpdateManagerSvc>::instance
    ("UpdateManagerSvc");

  if (um_ != 0)
    {
      um_->add (this);

      // Request persistent image.
      if (reincarnate_) {
        um_->requestImage ();
      }
    }
  else {
    ACE_ERROR_RETURN ((LM_ERROR, ACE_TEXT("TAO_DDS_DCPSInfo_i> Failed to discover ")
                       "UpdateManagerSvc.\n"), false);
  }

  return true;
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
