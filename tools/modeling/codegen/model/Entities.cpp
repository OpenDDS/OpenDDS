// -*- C++ -*-
//

#include "EntityProfiles.h"
#include "Entities.h"
#include "dds/DCPS/debug.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"

#if !defined (__ACE_INLINE__)
# include "Entities.inl"
#endif /* ! __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::Model::Entities::Entities(int& argc, ACE_TCHAR** argv)
  : delegate_()
  , config_(argc, argv)
{
}

OpenDDS::Model::Entities::~Entities()
{
}

void
OpenDDS::Model::Entities::registerTypes( const OPENDDS_STRING& participant,
                                         const OPENDDS_STRING& transportConfig)
{
  OPENDDS_QUEUE(DDS::TypeSupport_ptr)& queue = this->typeSupport_[ participant];
  if( queue.empty()) {
    return;
  }

  DDS::DomainParticipant_var p = this->participant(participant,
                                                   transportConfig);
  if( !p) {
    return;
  }

  while( !queue.empty()) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Entities::registerTypes() - ")
        ACE_TEXT("Registering type [%C] in participant [%C].\n"),
        CORBA::String_var(queue.front()->get_type_name()).in(),
        participant.c_str()
      ));
    }
    DDS::ReturnCode_t result = queue.front()->register_type(p, "");
    if (result != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "(%P|%t) Entities::registerTypes() - "
                           "register_type returned %d\n", result));
    }
    queue.pop();
  }
}

DDS::DomainParticipant_var
OpenDDS::Model::Entities::participant(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& transportConfig
)
{
  // See if its already here.
  StringToParticipantMap::const_iterator which
    = this->participantByString_.find( name);
  if( which != this->participantByString_.end()) {
    return DDS::DomainParticipant::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::ParticipantProfileMap::const_iterator where
    = this->config_.participantProfileMap().find( name);
  if( where == this->config_.participantProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::participant() - ")
      ACE_TEXT("unable to find profile to configure participant: [%C].\n"),
      name.c_str()
    ));
    return 0;
  }

  // Create it.
  ParticipantProfile* profile = where->second;
  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::participant() - ")
      ACE_TEXT("Creating participant [%C].\n"),
      name.c_str()
    ));
  }
  this->participantByString_[ name]
    = this->delegate_.createParticipant(
        profile->domainId,
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK,
        transportConfig
      );
  return DDS::DomainParticipant::_duplicate(
    this->participantByString_[ name]
  );
}

DDS::Topic_var
OpenDDS::Model::Entities::topic(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& participant,
  const OPENDDS_STRING& transportConfig
)
{
  // See if its already here.
  StringToTopicMap::const_iterator which
    = this->topicByParticipant_[ participant].find( name);
  if( which != this->topicByParticipant_[ participant].end()) {
    return DDS::Topic::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::TopicProfileMap::const_iterator where
    = this->config_.topicProfileMap().find( name);
  if( where == this->config_.topicProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::topic() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("topic: [%C] in participant: [%C].\n"),
      name.c_str(), participant.c_str()
    ));
    return 0;
  }
  TopicProfile* profile = where->second;

  // Find the containing participant.
  DDS::DomainParticipant_var p = this->participant(participant,
                                                   transportConfig);
  if( !p) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::topic() - ")
      ACE_TEXT("unable to find participant: [%C] for topic [%C].\n"),
      participant.c_str(), name.c_str()
    ));
    return 0;
  }

  // Ensure that we have all available types registered for this
  // participant.
  this->registerTypes(participant, transportConfig);

  // Create it.
  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::topic() - ")
      ACE_TEXT("Creating topic [%C] in participant [%C] with type [%C].\n"),
      name.c_str(),
      participant.c_str(),
      profile->type.c_str()
    ));
  }
  this->topicByParticipant_[ participant][ name]
    = this->delegate_.createTopic(
        p,
        name.c_str(),
        this->typeNameByString_[profile->type],
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  return DDS::Topic::_duplicate(
    this->topicByParticipant_[ participant][ name]
  );
}

DDS::Publisher_var
OpenDDS::Model::Entities::publisher(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& transportConfig)
{
  StringToPublisherMap::const_iterator which
    = this->publisherByString_.find( name);
  if( which != this->publisherByString_.end()) {
    return DDS::Publisher::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::PublisherProfileMap::const_iterator where
    = this->config_.publisherProfileMap().find( name);
  if( where == this->config_.publisherProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::publisher() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("publisher: [%C].\n"),
      name.c_str()
    ));
    return 0;
  }
  PublisherProfile* profile = where->second;

  // Find the containing participant.
  DDS::DomainParticipant_var participant
    = this->participant( profile->participant, transportConfig);
  if( !participant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::publisher() - ")
      ACE_TEXT("unable to find participant: [%C] for publisher [%C].\n"),
      profile->participant.c_str(), name.c_str()
    ));
    return 0;
  }

  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::publisher() - ")
      ACE_TEXT("Creating publisher [%C] in participant [%C].\n"),
      name.c_str(),
      profile->participant.c_str()
    ));
  }
  this->publisherByString_[ name]
    = this->delegate_.createPublisher(
        participant,
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK,
        transportConfig
      );

  return this->publisherByString_[ name];
}

DDS::Subscriber_var
OpenDDS::Model::Entities::subscriber(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& transportConfig)
{
  StringToSubscriberMap::const_iterator which
    = this->subscriberByString_.find( name);
  if( which != this->subscriberByString_.end()) {
    return DDS::Subscriber::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::SubscriberProfileMap::const_iterator where
    = this->config_.subscriberProfileMap().find( name);
  if( where == this->config_.subscriberProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::subscriber() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("subscriber: [%C].\n"),
      name.c_str()
    ));
    return 0;
  }
  SubscriberProfile* profile = where->second;

  // Find the containing participant.
  DDS::DomainParticipant_var participant
    = this->participant( profile->participant, transportConfig);
  if( !participant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::subscriber() - ")
      ACE_TEXT("unable to find participant: [%C] for subscriber [%C].\n"),
      profile->participant.c_str(), name.c_str()
    ));
    return 0;
  }

  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::subscriber() - ")
      ACE_TEXT("Creating subscriber [%C] in participant [%C].\n"),
      name.c_str(),
      profile->participant.c_str()
    ));
  }
  this->subscriberByString_[ name]
    = this->delegate_.createSubscriber(
        participant,
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK,
        transportConfig
      );

  return this->subscriberByString_[ name];
}

DDS::DataWriter_var
OpenDDS::Model::Entities::writer(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& transportConfig)
{
  StringToDataWriterMap::const_iterator which
    = this->writerByString_.find( name);
  if( which != this->writerByString_.end()) {
    return DDS::DataWriter::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::WriterProfileMap::const_iterator where
    = this->config_.writerProfileMap().find( name);
  if( where == this->config_.writerProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::writer() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("writer: [%C].\n"),
      name.c_str()
    ));
    return 0;
  }
  WriterProfile* profile = where->second;

  // Find the containing Publisher.
  DDS::Publisher_var publisher = this->publisher(profile->publisher,
                                                 transportConfig);
  if( !publisher) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::writer() - ")
      ACE_TEXT("unable to find publisher: [%C] for writer [%C].\n"),
      profile->publisher.c_str(), name.c_str()
    ));
    return 0;
  }

  // We need the *name* of the participant in order to look up the Topic.
  // This should be Ok since we will only be configuring Writers that have
  // been successfully defined in a configuration file, implying that there
  // exists a defined [publisher] profile.
  Config::PublisherProfileMap::const_iterator location
    = this->config_.publisherProfileMap().find( profile->publisher);
  if( location == this->config_.publisherProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::writer() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("publisher: [%C] for writer [%C].\n"),
      profile->publisher.c_str(), name.c_str()
    ));
    return 0;
  }
  PublisherProfile* publisherProfile = location->second;

  // Find the Topic.
  DDS::Topic_var topic
    = this->topic( profile->topic,
                   publisherProfile->participant,
                   transportConfig);
  if( !topic) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::writer() - ")
      ACE_TEXT("unable to find topic: [%C] for writer [%C] in participant [%C].\n"),
      profile->topic.c_str(), name.c_str(),
      publisherProfile->participant.c_str()
    ));
    return 0;
  }

  DDS::DataWriterQos writerQos;
  DDS::TopicQos      topicQos;
  topic->get_qos( topicQos);
  publisher->get_default_datawriter_qos( writerQos);
  publisher->copy_from_topic_qos( writerQos, topicQos);
  profile->copyToWriterQos( writerQos);

  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::writer() - ")
      ACE_TEXT("Creating writer [%C] in publisher [%C] in participant [%C] ")
      ACE_TEXT("with topic [%C].\n"),
      name.c_str(),
      profile->publisher.c_str(),
      publisherProfile->participant.c_str(),
      profile->topic.c_str()
    ));
  }
  this->writerByString_[ name]
    = this->delegate_.createWriter(
        publisher,
        topic,
        writerQos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK,
        transportConfig
      );

  return this->writerByString_[ name];
}

DDS::DataReader_var
OpenDDS::Model::Entities::reader(
  const OPENDDS_STRING& name,
  const OPENDDS_STRING& transportConfig)
{
  StringToDataReaderMap::const_iterator which
    = this->readerByString_.find( name);
  if( which != this->readerByString_.end()) {
    return DDS::DataReader::_duplicate( which->second);
  }

  // See if there is a configuration profile for it.
  Config::ReaderProfileMap::const_iterator where
    = this->config_.readerProfileMap().find( name);
  if( where == this->config_.readerProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::reader() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("reader: [%C].\n"),
      name.c_str()
    ));
    return 0;
  }
  ReaderProfile* profile = where->second;

  // Find the containing Subscriber.
  DDS::Subscriber_var subscriber = this->subscriber(profile->subscriber,
                                                    transportConfig);
  if( !subscriber) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::reader() - ")
      ACE_TEXT("unable to find subscriber: [%C] for reader [%C].\n"),
      profile->subscriber.c_str(), name.c_str()
    ));
    return 0;
  }

  // We need the *name* of the participant in order to look up the Topic.
  // This should be Ok since we will only be configuring Readers that have
  // been successfully defined in a configuration file, implying that there
  // exists a defined [subscriber] profile.
  Config::SubscriberProfileMap::const_iterator location
    = this->config_.subscriberProfileMap().find( profile->subscriber);
  if( location == this->config_.subscriberProfileMap().end()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::reader() - ")
      ACE_TEXT("unable to find profile to configure ")
      ACE_TEXT("subscriber: [%C] for reader [%C].\n"),
      profile->subscriber.c_str(), name.c_str()
    ));
    return 0;
  }
  SubscriberProfile* subscriberProfile = location->second;

  // Find the Topic.
  DDS::Topic_var topic
    = this->topic(profile->topic,
                  subscriberProfile->participant,
                  transportConfig);
  if( !topic) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Entities::reader() - ")
      ACE_TEXT("unable to find topic: [%C] for reader [%C] in participant [%C].\n"),
      profile->topic.c_str(), name.c_str(),
      subscriberProfile->participant.c_str()
    ));
    return 0;
  }

  DDS::DataReaderQos readerQos;
  DDS::TopicQos      topicQos;
  topic->get_qos( topicQos);
  subscriber->get_default_datareader_qos( readerQos);
  subscriber->copy_from_topic_qos( readerQos, topicQos);
  profile->copyToReaderQos( readerQos);

  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Entities::reader() - ")
      ACE_TEXT("Creating reader [%C] in subscriber [%C] in participant [%C] ")
      ACE_TEXT("with topic [%C].\n"),
      name.c_str(),
      profile->subscriber.c_str(),
      subscriberProfile->participant.c_str(),
      profile->topic.c_str()
    ));
  }
  this->readerByString_[ name]
    = this->delegate_.createReader(
        subscriber,
        topic,
        readerQos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK,
        transportConfig
      );

  return this->readerByString_[ name];
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
