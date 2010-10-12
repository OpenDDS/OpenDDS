/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include /**/ "DCPSInfo_i.h"

#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "UpdateManager.h"
#include "ShutdownInterface.h"

#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/RepoIdConverter.h"

#include /**/ "tao/debug.h"

#include /**/ "ace/Read_Buffer.h"
#include /**/ "ace/OS_NS_stdio.h"
#include "ace/Dynamic_Service.h"
#include "ace/Reactor.h"

// constructor
TAO_DDS_DCPSInfo_i::TAO_DDS_DCPSInfo_i(CORBA::ORB_ptr orb
                                       , bool reincarnate
                                       , ShutdownInterface* shutdown
                                       , long federation)
  : orb_(CORBA::ORB::_duplicate(orb))
  , federation_(federation)
  , participantIdGenerator_(federation)
  , um_(0)
  , reincarnate_(reincarnate)
  , shutdown_(shutdown)
  , reassociate_timer_id_(-1)
{
}

//  destructor
TAO_DDS_DCPSInfo_i::~TAO_DDS_DCPSInfo_i()
{
}

int
TAO_DDS_DCPSInfo_i::handle_timeout(const ACE_Time_Value& /*now*/,
                                   const void* /*arg*/)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // NOTE: This is a purposefully naive approach to addressing defunct
  // associations.  In the future, it may be worthwhile to introduce a
  // callback model to fix the heinous runtime cost below:
  for (DCPS_IR_Domain_Map::const_iterator dom(this->domains_.begin());
       dom != this->domains_.end(); ++dom) {

    const DCPS_IR_Participant_Map& participants(dom->second->participants());
    for (DCPS_IR_Participant_Map::const_iterator part(participants.begin());
         part != participants.end(); ++part) {

      const DCPS_IR_Subscription_Map& subscriptions(part->second->subscriptions());
      for (DCPS_IR_Subscription_Map::const_iterator sub(subscriptions.begin());
           sub != subscriptions.end(); ++sub) {
        sub->second->reevaluate_defunct_associations();
      }

      const DCPS_IR_Publication_Map& publications(part->second->publications());
      for (DCPS_IR_Publication_Map::const_iterator pub(publications.begin());
           pub != publications.end(); ++pub) {
        pub->second->reevaluate_defunct_associations();
      }
    }
  }

  return 0;
}

void
TAO_DDS_DCPSInfo_i::shutdown() ACE_THROW_SPEC((CORBA::SystemException))
{
  this->shutdown_->shutdown();
}

CORBA::ORB_ptr
TAO_DDS_DCPSInfo_i::orb()
{
  return CORBA::ORB::_duplicate(this->orb_.in());
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::attach_participant(
  DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& participantId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* participant
  = where->second->participant(participantId);

  if (0 == participant) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Establish ownership within the local repository.
  participant->takeOwnership();

  return false;
}

bool
TAO_DDS_DCPSInfo_i::changeOwnership(
  DDS::DomainId_t              domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  long                           sender,
  long                           owner)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    return false;
  }

  // Grab the participant.
  DCPS_IR_Participant* participant
  = where->second->participant(participantId);

  if (0 == participant) {
    return false;
  }

  // Establish the ownership.
  participant->changeOwner(sender, owner);
  return true;
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::assert_topic(
  OpenDDS::DCPS::RepoId_out topicId,
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const char * topicName,
  const char * dataTypeName,
  const DDS::TopicQos & qos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::INTERNAL_ERROR);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* participantPtr
  = where->second->participant(participantId);

  if (0 == participantPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  OpenDDS::DCPS::TopicStatus topicStatus
  = where->second->add_topic(
      topicId,
      topicName,
      dataTypeName,
      qos,
      participantPtr);

  if (this->um_ && (participantPtr->isBitPublisher() == false)) {
    Update::UTopic topic(domainId, topicId, participantId
                         , topicName, dataTypeName
                         , const_cast<DDS::TopicQos &>(qos));
    this->um_->create(topic);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::assert_topic: ")
                 ACE_TEXT("pushing creation of topic %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return topicStatus;
}

bool
TAO_DDS_DCPSInfo_i::add_topic(const OpenDDS::DCPS::RepoId& topicId,
                              DDS::DomainId_t domainId,
                              const OpenDDS::DCPS::RepoId& participantId,
                              const char* topicName,
                              const char* dataTypeName,
                              const DDS::TopicQos& qos)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_topic: ")
                 ACE_TEXT("invalid domain %d.\n"),
                 domainId));
    }

    return false;
  }

  // Grab the participant.
  DCPS_IR_Participant* participantPtr
  = where->second->participant(participantId);

  if (0 == participantPtr) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_topic: ")
                 ACE_TEXT("invalid participant %C.\n"),
                 std::string(converter).c_str()));
    }

    return false;
  }

  OpenDDS::DCPS::TopicStatus topicStatus
  = where->second->force_add_topic(topicId, topicName, dataTypeName,
                                   qos, participantPtr);

  if (topicStatus != OpenDDS::DCPS::CREATED) {
    return false;
  }

  OpenDDS::DCPS::RepoIdConverter converter(topicId);

  // See if we are adding a topic that was created within this
  // repository or a different repository.
  if (converter.federationId() == federation_) {
    // Ensure the topic RepoId values do not conflict.
    participantPtr->last_topic_key(converter.entityKey());
  }

  return true;
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::find_topic(
  DDS::DomainId_t domainId,
  const char * topicName,
  CORBA::String_out dataTypeName,
  DDS::TopicQos_out qos,
  OpenDDS::DCPS::RepoId_out topicId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::INTERNAL_ERROR);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  OpenDDS::DCPS::TopicStatus status = OpenDDS::DCPS::NOT_FOUND;

  DCPS_IR_Topic* topic = 0;
  qos = new DDS::TopicQos;

  status = where->second->find_topic(topicName, topic);

  if (0 != topic) {
    status = OpenDDS::DCPS::FOUND;
    const DCPS_IR_Topic_Description* desc = topic->get_topic_description();
    dataTypeName = desc->get_dataTypeName();
    *qos = *(topic->get_topic_qos());
    topicId = topic->get_id();
  }

  return status;
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::remove_topic(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::INTERNAL_ERROR);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Topic* topic;

  if (partPtr->find_topic_reference(topicId, topic) != 0) {
    throw OpenDDS::DCPS::Invalid_Topic();
  }

  OpenDDS::DCPS::TopicStatus removedStatus = where->second->remove_topic(partPtr, topic);

  if (this->um_
      && (partPtr->isOwner() == true)
      && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, participantId, topicId);
    this->um_->destroy(path, Update::Topic);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::remove_topic: ")
                 ACE_TEXT("pushing deletion of topic %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return removedStatus;
}

OpenDDS::DCPS::TopicStatus TAO_DDS_DCPSInfo_i::enable_topic(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::INTERNAL_ERROR);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Topic* topic;

  if (partPtr->find_topic_reference(topicId, topic) != 0) {
    throw OpenDDS::DCPS::Invalid_Topic();
  }

  return OpenDDS::DCPS::ENABLED;
}

OpenDDS::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_publication(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId,
  OpenDDS::DCPS::DataWriterRemote_ptr publication,
  const DDS::DataWriterQos & qos,
  const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
  const DDS::PublisherQos & publisherQos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::GUID_UNKNOWN);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Topic* topic = where->second->find_topic(topicId);

  if (topic == 0) {
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

  if (partPtr->add_publication(pubPtr) != 0) {
    // failed to add.  we are responsible for the memory.
    pubId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete pubPtr;
    pubPtr = 0;

  } else if (topic->add_publication_reference(pubPtr) != 0) {
    // Failed to add to the topic
    // so remove from participant and fail.
    partPtr->remove_publication(pubId);
    pubId = OpenDDS::DCPS::GUID_UNKNOWN;
  }

  if (this->um_ && (partPtr->isBitPublisher() == false)) {
    CORBA::String_var callback = orb_->object_to_string(publication);

    Update::UWActor actor(domainId, pubId, topicId, participantId, Update::DataWriter
                          , callback.in()
                          , const_cast<DDS::PublisherQos &>(publisherQos)
                          , const_cast<DDS::DataWriterQos &>(qos)
                          , const_cast<OpenDDS::DCPS::TransportInterfaceInfo &>
                          (transInfo));
    this->um_->create(actor);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_publication: ")
                 ACE_TEXT("pushing creation of publication %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  where->second->remove_dead_participants();
  return pubId;
}

bool
TAO_DDS_DCPSInfo_i::add_publication(DDS::DomainId_t domainId,
                                    const OpenDDS::DCPS::RepoId& participantId,
                                    const OpenDDS::DCPS::RepoId& topicId,
                                    const OpenDDS::DCPS::RepoId& pubId,
                                    const char* pub_str,
                                    const DDS::DataWriterQos & qos,
                                    const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
                                    const DDS::PublisherQos & publisherQos,
                                    bool associate)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_publication: ")
                 ACE_TEXT("invalid domain %d.\n"),
                 domainId));
    }

    return false;
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(pubId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_publication: ")
                 ACE_TEXT("invalid participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }

    return false;
  }

  DCPS_IR_Topic* topic = where->second->find_topic(topicId);

  if (topic == 0) {
    OpenDDS::DCPS::RepoIdConverter converter(topicId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_publication: ")
               ACE_TEXT("invalid topic %C in domain %d.\n"),
               std::string(converter).c_str(),
               domainId));
    return false;
  }

  /// @TODO: Check if this is already stored.  If so, just clear the callback IOR.

  CORBA::Object_var obj = orb_->string_to_object(pub_str);
  OpenDDS::DCPS::DataWriterRemote_var publication
  = OpenDDS::DCPS::DataWriterRemote::_unchecked_narrow(obj.in());

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

  switch (partPtr->add_publication(pubPtr)) {
  case -1: {
    OpenDDS::DCPS::RepoIdConverter converter(pubId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::add_publication: ")
               ACE_TEXT("failed to add publication to participant %C.\n"),
               std::string(converter).c_str()));
  }
  // Deliberate fall through to next case.

  case 1:
    delete pubPtr;
    return false;

  case 0:
  default:
    break;
  }

  switch (topic->add_publication_reference(pubPtr, associate)) {
  case -1: {
    OpenDDS::DCPS::RepoIdConverter converter(pubId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::add_publication: ")
               ACE_TEXT("failed to add publication to participant %C topic list.\n"),
               std::string(converter).c_str()));

    // Remove the publication.
    partPtr->remove_publication(pubId);

  }
  return false;

  case 1: // This is actually a really really bad place to jump to.
    // This means that we successfully added the new publication
    // to the participant (it had not been inserted before) but
    // that we are adding a duplicate publication to the topic
    // list - which should never ever be able to happen.
    return false;

  case 0:
  default:
    break;
  }

  OpenDDS::DCPS::RepoIdConverter converter(pubId);

  // See if we are adding a publication that was created within this
  // repository or a different repository.
  if (converter.federationId() == federation_) {
    // Ensure the publication RepoId values do not conflict.
    partPtr->last_publication_key(converter.entityKey());
  }

  return true;
}

void TAO_DDS_DCPSInfo_i::remove_publication(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& publicationId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Publication))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  if (partPtr->remove_publication(publicationId) != 0) {
    where->second->remove_dead_participants();

    // throw exception because the publication was not removed!
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  where->second->remove_dead_participants();

  if (this->um_
      && (partPtr->isOwner() == true)
      && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, participantId, publicationId);
    this->um_->destroy(path, Update::Actor, Update::DataWriter);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(publicationId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::remove_publication: ")
                 ACE_TEXT("pushing deletion of publication %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }
}

OpenDDS::DCPS::RepoId TAO_DDS_DCPSInfo_i::add_subscription(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId,
  OpenDDS::DCPS::DataReaderRemote_ptr subscription,
  const DDS::DataReaderQos & qos,
  const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
  const DDS::SubscriberQos & subscriberQos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, OpenDDS::DCPS::GUID_UNKNOWN);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Topic* topic = where->second->find_topic(topicId);

  if (topic == 0) {
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

  if (partPtr->add_subscription(subPtr) != 0) {
    // failed to add.  we are responsible for the memory.
    subId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete subPtr;
    subPtr = 0;

  } else if (topic->add_subscription_reference(subPtr) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to add subscription to topic list.\n")));
    // No associations were made so remove and fail.
    partPtr->remove_subscription(subId);
    subId = OpenDDS::DCPS::GUID_UNKNOWN;
  }

  if (this->um_ && (partPtr->isBitPublisher() == false)) {
    CORBA::String_var callback = orb_->object_to_string(subscription);

    Update::URActor actor(domainId, subId, topicId, participantId, Update::DataReader
                          , callback.in()
                          , const_cast<DDS::SubscriberQos &>(subscriberQos)
                          , const_cast<DDS::DataReaderQos &>(qos)
                          , const_cast<OpenDDS::DCPS::TransportInterfaceInfo &>
                          (transInfo));

    this->um_->create(actor);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(subId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_subscription: ")
                 ACE_TEXT("pushing creation of subscription %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  where->second->remove_dead_participants();

  return subId;
}

bool
TAO_DDS_DCPSInfo_i::add_subscription(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& topicId,
  const OpenDDS::DCPS::RepoId& subId,
  const char* sub_str,
  const DDS::DataReaderQos & qos,
  const OpenDDS::DCPS::TransportInterfaceInfo & transInfo,
  const DDS::SubscriberQos & subscriberQos,
  bool associate)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_subscription: ")
                 ACE_TEXT("invalid domain %d.\n"),
                 domainId));
    }

    return false;
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_subscription: ")
                 ACE_TEXT("invalid participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }

    return false;
  }

  DCPS_IR_Topic* topic = where->second->find_topic(topicId);

  if (topic == 0) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: TAO_DDS_DCPSInfo_i:add_subscription: ")
                 ACE_TEXT("invalid topic %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }

    return false;
  }

  CORBA::Object_var obj = orb_->string_to_object(sub_str);
  OpenDDS::DCPS::DataReaderRemote_var subscription
  = OpenDDS::DCPS::DataReaderRemote::_unchecked_narrow(obj.in());

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

  switch (partPtr->add_subscription(subPtr)) {
  case -1: {
    OpenDDS::DCPS::RepoIdConverter converter(subId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::add_subscription: ")
               ACE_TEXT("failed to add subscription to participant %C.\n"),
               std::string(converter).c_str()));
  }
  // Deliberate fall through to next case.

  case 1:
    delete subPtr;
    return false;

  case 0:
  default:
    break;
  }

  switch (topic->add_subscription_reference(subPtr, associate)) {
  case -1: {
    OpenDDS::DCPS::RepoIdConverter converter(subId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::add_subscription: ")
               ACE_TEXT("failed to add subscription to participant %C topic list.\n"),
               std::string(converter).c_str()));

    // Remove the subscription.
    partPtr->remove_subscription(subId);

  }
  return false;

  case 1: // This is actually a really really bad place to jump to.
    // This means that we successfully added the new subscription
    // to the participant (it had not been inserted before) but
    // that we are adding a duplicate subscription to the topic
    // list - which should never ever be able to happen.
    return false;

  case 0:
  default:
    break;
  }

  OpenDDS::DCPS::RepoIdConverter converter(subId);

  // See if we are adding a subscription that was created within this
  // repository or a different repository.
  if (converter.federationId() == federation_) {
    // Ensure the subscription RepoId values do not conflict.
    partPtr->last_subscription_key(converter.entityKey());
  }

  return true;
}

void TAO_DDS_DCPSInfo_i::remove_subscription(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& subscriptionId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Subscription))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  if (partPtr->remove_subscription(subscriptionId) != 0) {
    // throw exception because the subscription was not removed!
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  where->second->remove_dead_participants();

  if (this->um_
      && (partPtr->isOwner() == true)
      && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, participantId, subscriptionId);
    this->um_->destroy(path, Update::Actor, Update::DataReader);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(subscriptionId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::remove_subscription: ")
                 ACE_TEXT("pushing deletion of subscription %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }
}

OpenDDS::DCPS::AddDomainStatus TAO_DDS_DCPSInfo_i::add_domain_participant(
  DDS::DomainId_t domain,
  const DDS::DomainParticipantQos & qos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain))
{
  // A value to return.
  OpenDDS::DCPS::AddDomainStatus value;
  value.id        = OpenDDS::DCPS::GUID_UNKNOWN;
  value.federated = (this->federation_ != 0);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, value);

  // Grab the domain.
  DCPS_IR_Domain* domainPtr = this->domain(domain);

  if (0 == domainPtr) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Obtain a shiny new GUID value.
  OpenDDS::DCPS::RepoId participantId = domainPtr->get_next_participant_id();

  DCPS_IR_Participant* participant;
  ACE_NEW_RETURN(participant,
                 DCPS_IR_Participant(
                   this->federation_,
                   participantId,
                   domainPtr,
                   qos, um_),
                 value);

  // We created the participant, now we can return the Id value (eventually).
  value.id = participantId;

  // Determine if this is the 'special' repository internal participant
  // that publishes the built-in topics for a domain.
  if (domainPtr->participants().empty() && TheServiceParticipant->get_BIT()) {
    participant->isBitPublisher() = true;

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("participant %C in domain %d is BIT publisher for this domain.\n"),
                 std::string(converter).c_str(),
                 domain));
    }
  }

  // Assume responsibilty for writing back to the participant.
  participant->takeOwnership();

  int status = domainPtr->add_participant(participant);

  if (0 != status) {
    // Adding the participant failed return the invalid
    // pariticipant Id number.
    participantId = OpenDDS::DCPS::GUID_UNKNOWN;
    delete participant;
    participant = 0;

  } else if (this->um_ && (participant->isBitPublisher() == false)) {
    // Push this participant to interested observers.
    Update::UParticipant updateParticipant(
      domain,
      participant->owner(),
      participantId,
      const_cast<DDS::DomainParticipantQos &>(qos));
    this->um_->create(updateParticipant);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("pushing creation of participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domain));
    }
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    OpenDDS::DCPS::RepoIdConverter converter(participantId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) (RepoId)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
               ACE_TEXT("domain %d loaded participant %C at 0x%x.\n"),
               domain,
               std::string(converter).c_str(),
               participant));
  }

  return value;
}

bool
TAO_DDS_DCPSInfo_i::add_domain_participant(DDS::DomainId_t domainId
                                           , const OpenDDS::DCPS::RepoId& participantId
                                           , const DDS::DomainParticipantQos & qos)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain* domainPtr = this->domain(domainId);

  if (0 == domainPtr) {
    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("invalid domain Id: %d\n"),
                 domainId));
    }

    return false;
  }

  // Prepare to manipulate the participant's Id value.
  OpenDDS::DCPS::RepoIdConverter converter(participantId);

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = domainPtr->participant(participantId);

  if (0 != partPtr) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("participant id %C already exists.\n"),
                 std::string(converter).c_str()));
    }

    return false;
  }

  DCPS_IR_Participant* participant;
  ACE_NEW_RETURN(participant,
                 DCPS_IR_Participant(this->federation_,
                                     participantId,
                                     domainPtr,
                                     qos, um_), 0);

  switch (domainPtr->add_participant(participant)) {
  case -1: {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
               ACE_TEXT("failed to load participant %C in domain %d.\n"),
               std::string(converter).c_str(),
               domainId));
  }
  delete participant;
  return false;

  case 1:

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) WARNING: (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("attempt to load duplicate participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }

    delete participant;
    return false;

  case 0:
  default:
    break;
  }

  // See if we are adding a participant that was created within this
  // repository or a different repository.
  if (converter.federationId() == this->federation_) {
    // Ensure the participant GUID values do not conflict.
    domainPtr->last_participant_key(converter.participantId());

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
                 ACE_TEXT("Adjusting highest participant Id value to at least %d.\n"),
                 converter.participantId()));
    }
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::add_domain_participant: ")
               ACE_TEXT("loaded participant %C at 0x%x in domain %d.\n"),
               std::string(converter).c_str(),
               participant,
               domainId));
  }

  return true;
}

bool
TAO_DDS_DCPSInfo_i::remove_by_owner(
  DDS::DomainId_t domain,
  long              owner)
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, false);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domain);

  if (where == this->domains_.end()) {
    return false;
  }

  std::vector<OpenDDS::DCPS::RepoId> candidates;

  for (DCPS_IR_Participant_Map::const_iterator
       current = where->second->participants().begin();
       current != where->second->participants().end();
       ++current) {
    if (current->second->owner() == owner) {
      candidates.push_back(current->second->get_id());
    }
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::remove_by_owner: ")
               ACE_TEXT("%d participants to remove from domain %d.\n"),
               candidates.size(),
               domain));
  }

  bool status = true;

  for (unsigned int index = 0; index < candidates.size(); ++index) {
    DCPS_IR_Participant* participant
    = where->second->participant(candidates[index]);

    std::vector<OpenDDS::DCPS::RepoId> keylist;

    // Remove Subscriptions
    for (DCPS_IR_Subscription_Map::const_iterator
         current = participant->subscriptions().begin();
         current != participant->subscriptions().end();
         ++current) {
      keylist.push_back(current->second->get_id());
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(candidates[index]);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::remove_by_owner: ")
                 ACE_TEXT("%d subscriptions to remove from participant %C.\n"),
                 keylist.size(),
                 std::string(converter).c_str()));
    }

    for (unsigned int key = 0; key < keylist.size(); ++key) {
      if (participant->remove_subscription(keylist[ key]) != 0) {
        status = false;
      }
    }

    // Remove Publications
    keylist.clear();

    for (DCPS_IR_Publication_Map::const_iterator
         current = participant->publications().begin();
         current != participant->publications().end();
         ++current) {
      keylist.push_back(current->second->get_id());
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(candidates[index]);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::remove_by_owner: ")
                 ACE_TEXT("%d publications to remove from participant %C.\n"),
                 keylist.size(),
                 std::string(converter).c_str()));
    }

    for (unsigned int key = 0; key < keylist.size(); ++key) {
      if (participant->remove_publication(keylist[ key]) != 0) {
        status = false;
      }
    }

    // Remove Topics
    keylist.clear();

    for (DCPS_IR_Topic_Map::const_iterator
         current = participant->topics().begin();
         current != participant->topics().end();
         ++current) {
      keylist.push_back(current->second->get_id());
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(candidates[index]);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) (bool)TAO_DDS_DCPSInfo_i::remove_by_owner: ")
                 ACE_TEXT("%d topics to remove from participant %C.\n"),
                 keylist.size(),
                 std::string(converter).c_str()));
    }

    for (unsigned int key = 0; key < keylist.size(); ++key) {
      DCPS_IR_Topic* discard;

      if (participant->remove_topic_reference(keylist[ key], discard) != 0) {
        status = false;
      }
    }

    // Remove Participant
    this->remove_domain_participant(domain, candidates[ index]);
  }

  return status;
}

void
TAO_DDS_DCPSInfo_i::disassociate_participant(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& local_id,
  const OpenDDS::DCPS::RepoId& remote_id)
ACE_THROW_SPEC((CORBA::SystemException,
                OpenDDS::DCPS::Invalid_Domain,
                OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  DCPS_IR_Domain_Map::iterator it(this->domains_.find(domainId));
  if (it == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* participant = it->second->participant(local_id);
  if (participant == 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Disassociate from participant temporarily:
  const DCPS_IR_Subscription_Map& subscriptions = participant->subscriptions();
  for (DCPS_IR_Subscription_Map::const_iterator sub(subscriptions.begin());
       sub != subscriptions.end(); ++sub) {
    sub->second->disassociate_participant(remote_id, true);
  }

  const DCPS_IR_Publication_Map& publications = participant->publications();
  for (DCPS_IR_Publication_Map::const_iterator pub(publications.begin());
       pub != publications.end(); ++pub) {
    pub->second->disassociate_participant(remote_id, true);
  }

  it->second->remove_dead_participants();
}

void
TAO_DDS_DCPSInfo_i::disassociate_subscription(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& local_id,
  const OpenDDS::DCPS::RepoId& remote_id)
ACE_THROW_SPEC((CORBA::SystemException,
                OpenDDS::DCPS::Invalid_Domain,
                OpenDDS::DCPS::Invalid_Participant,
                OpenDDS::DCPS::Invalid_Subscription))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  DCPS_IR_Domain_Map::iterator it(this->domains_.find(domainId));
  if (it == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* participant = it->second->participant(participantId);
  if (participant == 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* subscription;
  if (participant->find_subscription_reference(local_id, subscription)
      != 0 || subscription == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  // Disassociate from publication temporarily:
  subscription->disassociate_publication(remote_id, true);

  it->second->remove_dead_participants();
}

void
TAO_DDS_DCPSInfo_i::disassociate_publication(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const OpenDDS::DCPS::RepoId& local_id,
  const OpenDDS::DCPS::RepoId& remote_id)
ACE_THROW_SPEC((CORBA::SystemException,
                OpenDDS::DCPS::Invalid_Domain,
                OpenDDS::DCPS::Invalid_Participant,
                OpenDDS::DCPS::Invalid_Publication))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  DCPS_IR_Domain_Map::iterator it(this->domains_.find(domainId));
  if (it == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* participant = it->second->participant(participantId);
  if (participant == 0) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* publication;
  if (participant->find_publication_reference(local_id, publication)
      != 0 || publication == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  // Disassociate from subscription temporarily:
  publication->disassociate_subscription(remote_id, true);

  it->second->remove_dead_participants();
}

void TAO_DDS_DCPSInfo_i::remove_domain_participant(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  DCPS_IR_Participant* participant = where->second->participant(participantId);

  if (participant == 0) {
    OpenDDS::DCPS::RepoIdConverter converter(participantId);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: (bool)TAO_DDS_DCPSInfo_i::remove_domain_participant: ")
               ACE_TEXT("failed to locate participant %C in domain %d.\n"),
               std::string(converter).c_str(),
               domainId));
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Determine if we should propagate this event;  we need to cache this
  // result as the participant will be gone by the time we use the result.
  bool sendUpdate = (participant->isOwner() == true)
                    && (participant->isBitPublisher() == false);

  CORBA::Boolean dont_notify_lost = 0;
  int status = where->second->remove_participant(participantId, dont_notify_lost);

  if (0 != status) {
    // Removing the participant failed
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  // Update any concerned observers that the participant was destroyed.
  if (this->um_ && sendUpdate) {
    Update::IdPath path(
      where->second->get_id(),
      participantId,
      participantId);
    this->um_->destroy(path, Update::Participant);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::remove_domain_participant: ")
                 ACE_TEXT("pushing deletion of participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }
}

void TAO_DDS_DCPSInfo_i::ignore_domain_participant(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& myParticipantId,
  const OpenDDS::DCPS::RepoId& ignoreId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(myParticipantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  partPtr->ignore_participant(ignoreId);

  where->second->remove_dead_participants();
}

void TAO_DDS_DCPSInfo_i::ignore_topic(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& myParticipantId,
  const OpenDDS::DCPS::RepoId& ignoreId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(myParticipantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  partPtr->ignore_topic(ignoreId);

  where->second->remove_dead_participants();
}

void TAO_DDS_DCPSInfo_i::ignore_subscription(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& myParticipantId,
  const OpenDDS::DCPS::RepoId& ignoreId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Subscription))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(myParticipantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  partPtr->ignore_subscription(ignoreId);

  where->second->remove_dead_participants();
}

void TAO_DDS_DCPSInfo_i::ignore_publication(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& myParticipantId,
  const OpenDDS::DCPS::RepoId& ignoreId)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Publication))
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(myParticipantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  partPtr->ignore_publication(ignoreId);

  where->second->remove_dead_participants();
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_publication_qos(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const DDS::DataWriterQos & qos,
  const DDS::PublisherQos & publisherQos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Publication))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* pub;

  if (partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  Update::SpecificQos qosType;

  if (pub->set_qos(qos, publisherQos, qosType) == false)  // not compatible
    return 0;

  if (this->um_ && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, partId, dwId);

    switch (qosType) {
    case Update::DataWriterQos:
      this->um_->update(path, qos);
      break;

    case Update::PublisherQos:
      this->um_->update(path, publisherQos);
      break;

    case Update::NoQos:
    default:
      break;
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(dwId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::update_publication_qos: ")
                 ACE_TEXT("pushing update of publication %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return 1;
}

void
TAO_DDS_DCPSInfo_i::update_publication_qos(
  DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const DDS::DataWriterQos&  qos)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* pub;

  if (partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  pub->set_qos(qos);
}

void
TAO_DDS_DCPSInfo_i::update_publication_qos(
  DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& dwId,
  const DDS::PublisherQos&   qos)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Publication* pub;

  if (partPtr->find_publication_reference(dwId, pub) != 0 || pub == 0) {
    throw OpenDDS::DCPS::Invalid_Publication();
  }

  pub->set_qos(qos);
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_subscription_qos(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const DDS::DataReaderQos & qos,
  const DDS::SubscriberQos & subscriberQos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Subscription))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* sub;

  if (partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  Update::SpecificQos qosType;

  if (sub->set_qos(qos, subscriberQos, qosType) == false)
    return 0;

  if (this->um_ && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, partId, drId);

    switch (qosType) {
    case Update::DataReaderQos:
      this->um_->update(path, qos);
      break;

    case Update::SubscriberQos:
      this->um_->update(path, subscriberQos);
      break;

    case Update::NoQos:
    default:
      break;
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(drId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::update_subscription_qos: ")
                 ACE_TEXT("pushing update of subscription %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return 1;
}

void
TAO_DDS_DCPSInfo_i::update_subscription_qos(
  DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const DDS::DataReaderQos&  qos)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* sub;

  if (partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  sub->set_qos(qos);
}

void
TAO_DDS_DCPSInfo_i::update_subscription_qos(
  DDS::DomainId_t            domainId,
  const OpenDDS::DCPS::RepoId& partId,
  const OpenDDS::DCPS::RepoId& drId,
  const DDS::SubscriberQos&  qos)
{
  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, this->lock_);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(partId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Subscription* sub;

  if (partPtr->find_subscription_reference(drId, sub) != 0 || sub == 0) {
    throw OpenDDS::DCPS::Invalid_Subscription();
  }

  sub->set_qos(qos);
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_topic_qos(
  const OpenDDS::DCPS::RepoId& topicId,
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const DDS::TopicQos & qos)
ACE_THROW_SPEC((CORBA::SystemException
                 , OpenDDS::DCPS::Invalid_Domain
                 , OpenDDS::DCPS::Invalid_Participant
                 , OpenDDS::DCPS::Invalid_Topic))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  DCPS_IR_Topic* topic;

  if (partPtr->find_topic_reference(topicId, topic) != 0) {
    throw OpenDDS::DCPS::Invalid_Topic();
  }

  if (topic->set_topic_qos(qos) == false)
    return 0;

  if (this->um_
      && (partPtr->isOwner() == true)
      && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, participantId, topicId);
    this->um_->update(path, qos);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(topicId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::update_topic_qos: ")
                 ACE_TEXT("pushing update of topic %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return 1;
}

CORBA::Boolean TAO_DDS_DCPSInfo_i::update_domain_participant_qos(
  DDS::DomainId_t domainId,
  const OpenDDS::DCPS::RepoId& participantId,
  const DDS::DomainParticipantQos & qos)
ACE_THROW_SPEC((CORBA::SystemException,
                 OpenDDS::DCPS::Invalid_Domain,
                 OpenDDS::DCPS::Invalid_Participant))
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->lock_, 0);

  // Grab the domain.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domainId);

  if (where == this->domains_.end()) {
    throw OpenDDS::DCPS::Invalid_Domain();
  }

  // Grab the participant.
  DCPS_IR_Participant* partPtr
  = where->second->participant(participantId);

  if (0 == partPtr) {
    throw OpenDDS::DCPS::Invalid_Participant();
  }

  if (partPtr->set_qos(qos) == false)
    return 0;

  if (this->um_
      && (partPtr->isOwner() == true)
      && (partPtr->isBitPublisher() == false)) {
    Update::IdPath path(domainId, participantId, participantId);
    this->um_->update(path, qos);

    if (OpenDDS::DCPS::DCPS_debug_level > 4) {
      OpenDDS::DCPS::RepoIdConverter converter(participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::update_domain_participant_qos: ")
                 ACE_TEXT("pushing update of participant %C in domain %d.\n"),
                 std::string(converter).c_str(),
                 domainId));
    }
  }

  return 1;
}

DCPS_IR_Domain*
TAO_DDS_DCPSInfo_i::domain(DDS::DomainId_t domain)
{
  if (domain == OpenDDS::DCPS::Service_Participant::ANY_DOMAIN) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::domain: ")
               ACE_TEXT("ANY_DOMAIN not supported for operations.\n")));
    return 0;
  }

  // Check if the domain is already in the map.
  DCPS_IR_Domain_Map::iterator where = this->domains_.find(domain);

  if (where == this->domains_.end()) {
    // We will attempt to insert a new domain, go ahead and allocate it.
    DCPS_IR_Domain* domainPtr;
    ACE_NEW_RETURN(domainPtr,
                   DCPS_IR_Domain(domain, this->participantIdGenerator_),
                   0);

    // We need to insert the domain into the map at this time since it
    // might be looked up during the init_built_in_topics() call.
    this->domains_.insert(
      where,
      DCPS_IR_Domain_Map::value_type(domain, domainPtr));

    int bit_status = 0;

    if (TheServiceParticipant->get_BIT()) {
#if !defined (DDS_HAS_MINIMUM_BIT)
      bit_status = domainPtr->init_built_in_topics(this->federation_ != 0);
#endif // !defined (DDS_HAS_MINIMUM_BIT)
    }

    if (0 != bit_status) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::domain: ")
                 ACE_TEXT("failed to initialize the Built-In Topics ")
                 ACE_TEXT("when loading domain %d.\n"),
                 domain));
      this->domains_.erase(domain);
      delete domainPtr;
      return 0;
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::domain: ")
                 ACE_TEXT("successfully loaded domain %d at %x.\n"),
                 domain,
                 domainPtr));
    }

    return domainPtr;

  } else {
    return where->second;
  }
}

int TAO_DDS_DCPSInfo_i::init_transport(int listen_address_given,
                                       const ACE_TCHAR* listen_str)
{
  int status = 0;

  try {

#ifndef ACE_AS_STATIC_LIBS
    if (ACE_Service_Config::current()->find(ACE_TEXT("DCPS_SimpleTcpLoader"))
        < 0 /* not found (-1) or suspended (-2) */) {
      static const ACE_TCHAR directive[] =
        ACE_TEXT("dynamic DCPS_SimpleTcpLoader Service_Object * ")
        ACE_TEXT("SimpleTcp:_make_DCPS_SimpleTcpLoader() \"-type SimpleTcp\"");
      ACE_Service_Config::process_directive(directive);
    }
#endif

    OpenDDS::DCPS::TransportImpl_rch trans_impl
    = TheTransportFactory->create_transport_impl(OpenDDS::DCPS::BIT_ALL_TRAFFIC,
                                                 ACE_TEXT("SimpleTcp"),
                                                 OpenDDS::DCPS::DONT_AUTO_CONFIG);

    OpenDDS::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->get_or_create_configuration(OpenDDS::DCPS::BIT_ALL_TRAFFIC,
                                                       ACE_TEXT("SimpleTcp"));

    config->datalink_release_delay_ = 0;

    OpenDDS::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*>(config.in());

    tcp_config->conn_retry_attempts_ = 0;

    if (listen_address_given) {
      tcp_config->local_address_ = ACE_INET_Addr(listen_str);
      tcp_config->local_address_str_ = listen_str;
    }

    if (trans_impl->configure(config.in()) != 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::init_transport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  } catch (...) {
    // TransportFactory is extremely varied in the exceptions that
    // it throws on failure; do not allow exceptions to bubble up
    // beyond this point.
    status = 1;
  }

  return status;
}

bool
TAO_DDS_DCPSInfo_i::receive_image(const Update::UImage& image)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::receive_image: ")
               ACE_TEXT("processing persistent data.\n")));
  }

  for (Update::UImage::ParticipantSeq::const_iterator
       iter = image.participants.begin();
       iter != image.participants.end(); iter++) {
    const Update::UParticipant* part = *iter;

    if (!this->add_domain_participant(part->domainId, part->participantId
                                      , part->participantQos)) {
      OpenDDS::DCPS::RepoIdConverter converter(part->participantId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("failed to add participant %C to domain %d.\n"),
                 std::string(converter).c_str(),
                 part->domainId));
      return false;

    } else if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter converter(part->participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("added participant %C to domain %d.\n"),
                 std::string(converter).c_str(),
                 part->domainId));
    }
  }

  for (Update::UImage::TopicSeq::const_iterator iter = image.topics.begin();
       iter != image.topics.end(); iter++) {
    const Update::UTopic* topic = *iter;

    if (!this->add_topic(topic->topicId, topic->domainId
                         , topic->participantId, topic->name.c_str()
                         , topic->dataType.c_str(), topic->topicQos)) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(topic->topicId);
      OpenDDS::DCPS::RepoIdConverter part_converter(topic->participantId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("failed to add topic %C to participant %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(part_converter).c_str()));
      return false;

    } else if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter topic_converter(topic->topicId);
      OpenDDS::DCPS::RepoIdConverter part_converter(topic->participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("added topic %C to participant %C.\n"),
                 std::string(topic_converter).c_str(),
                 std::string(part_converter).c_str()));
    }
  }

  for (Update::UImage::ReaderSeq::const_iterator iter = image.actors.begin();
       iter != image.actors.end(); iter++) {
    const Update::URActor* sub = *iter;

    if (!this->add_subscription(sub->domainId, sub->participantId
                                , sub->topicId, sub->actorId
                                , sub->callback.c_str(), sub->drdwQos
                                , sub->transportInterfaceInfo
                                , sub->pubsubQos)) {
      OpenDDS::DCPS::RepoIdConverter sub_converter(sub->actorId);
      OpenDDS::DCPS::RepoIdConverter part_converter(sub->participantId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("failed to add subscription %C to participant %C.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(part_converter).c_str()));
      return false;

    } else if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter sub_converter(sub->actorId);
      OpenDDS::DCPS::RepoIdConverter part_converter(sub->participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("added subscription %C to participant %C.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(part_converter).c_str()));
    }
  }

  for (Update::UImage::WriterSeq::const_iterator iter = image.wActors.begin();
       iter != image.wActors.end(); iter++) {
    const Update::UWActor* pub = *iter;

    if (!this->add_publication(pub->domainId, pub->participantId
                               , pub->topicId, pub->actorId
                               , pub->callback.c_str() , pub->drdwQos
                               , pub->transportInterfaceInfo
                               , pub->pubsubQos)) {
      OpenDDS::DCPS::RepoIdConverter pub_converter(pub->actorId);
      OpenDDS::DCPS::RepoIdConverter part_converter(pub->participantId);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("failed to add publication %C to participant %C.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(part_converter).c_str()));
      return false;

    } else if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      OpenDDS::DCPS::RepoIdConverter pub_converter(pub->actorId);
      OpenDDS::DCPS::RepoIdConverter part_converter(pub->participantId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TAO_DDS_DCPSInfo_i::receive_image: ")
                 ACE_TEXT("added publication %C to participant %C.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(part_converter).c_str()));
    }
  }

  return true;
}

void
TAO_DDS_DCPSInfo_i::add(Update::Updater* updater)
{
  if (this->um_) {
    this->um_->add(updater);
  }
}

bool
TAO_DDS_DCPSInfo_i::init_persistence()
{
  um_ = ACE_Dynamic_Service<UpdateManagerSvc>::instance
        ("UpdateManagerSvc");

  if (um_ != 0) {
    um_->add(this);

    // Request persistent image.
    if (reincarnate_) {
      um_->requestImage();
    }

  } else {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("TAO_DDS_DCPSInfo_i> Failed to discover ")
                      ACE_TEXT("UpdateManagerSvc.\n")), false);
  }

  return true;
}

bool
TAO_DDS_DCPSInfo_i::init_reassociation(const ACE_Time_Value& delay)
{
  if (this->reassociate_timer_id_ != -1) return false;  // already scheduled

  ACE_Reactor* reactor = this->orb_->orb_core()->reactor();

  this->reassociate_timer_id_ = reactor->schedule_timer(this, 0, delay, delay);
  return this->reassociate_timer_id_ != -1;
}

void
TAO_DDS_DCPSInfo_i::finalize()
{
  if (reassociate_timer_id_ != -1) {
    ACE_Reactor* reactor = this->orb_->orb_core()->reactor();

    reactor->cancel_timer(this->reassociate_timer_id_);
    this->reassociate_timer_id_ = -1;
  }
}

const DCPS_IR_Domain_Map&
TAO_DDS_DCPSInfo_i::domains() const
{
  return this->domains_;
}
