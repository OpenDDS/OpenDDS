// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/debug.h"
#include "EntityProfiles.h"
#include "Entities.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"

#if !defined (__ACE_INLINE__)
# include "Entities.inl"
#endif /* ! __ACE_INLINE__ */

OpenDDS::Model::Entities::Entities()
{
}

OpenDDS::Model::Entities::~Entities()
{
}

void
OpenDDS::Model::Entities::registerTypes( const std::string& participant)
{
  std::queue<DDS::TypeSupport_ptr>& queue = this->typeSupport_[ participant];
  if( queue.empty()) {
    return;
  }

  DDS::DomainParticipant_var p = this->participant( participant);
  if( !p) {
    return;
  }

  while( !queue.empty()) {
    queue.front()->register_type( p, 0);
    queue.pop();
  }
}

DDS::DomainParticipant_var
OpenDDS::Model::Entities::participant( const std::string& name)
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
    return 0;
  }
  
  // Create it.
  ParticipantProfile* profile = where->second;
  this->participantByString_[ name]
    = this->delegate_.createParticipant(
        profile->domainId,
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK /// @TODO: May need to 'using' this.
      );
  /// @TODO: Verify that this handles null return values correctly.
  return DDS::DomainParticipant::_duplicate(
    this->participantByString_[ name]
  );
}

DDS::Topic_var
OpenDDS::Model::Entities::topic(
  const std::string& name,
  const std::string& participant
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
    return 0;
  }
  TopicProfile* profile = where->second;

  // Find the containing participant.
  DDS::DomainParticipant_var p = this->participant( participant);
  if( !p) {
    return 0;
  }

  // Create it.
  this->topicByParticipant_[ participant][ name]
    = this->delegate_.createTopic(
        p,
        name.c_str(),
        profile->type.c_str(),
        profile->qos,
        OpenDDS::DCPS::DEFAULT_STATUS_MASK
      );
  return DDS::Topic::_duplicate(
    this->topicByParticipant_[ participant][ name]
  );
}

DDS::Publisher_var
OpenDDS::Model::Entities::publisher( const std::string& name)
{
  StringToPublisherMap::const_iterator which
    = this->publisherByString_.find( name);
  if( which != this->publisherByString_.end()) {
    return DDS::Publisher::_duplicate( which->second);
  }

  return 0;
}

DDS::Subscriber_var
OpenDDS::Model::Entities::subscriber( const std::string& name)
{
  StringToSubscriberMap::const_iterator which
    = this->subscriberByString_.find( name);
  if( which != this->subscriberByString_.end()) {
    return DDS::Subscriber::_duplicate( which->second);
  }

  return 0;
}

DDS::DataWriter_var
OpenDDS::Model::Entities::writer( const std::string& name)
{
  return 0;
}

DDS::DataReader_var
OpenDDS::Model::Entities::reader( const std::string& name)
{
  return 0;
}

