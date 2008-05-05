// -*- C++ -*-
//
// $Id$

#include "FederatorSubscriptions.h"
#include "FederatorLinkListener.h"
#include "UpdateListener_T.h"
#include "FederatorC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include "ParticipantUpdateTypeSupportC.h"
#include "ParticipantUpdateTypeSupportImpl.h"
#include "TopicUpdateTypeSupportC.h"
#include "TopicUpdateTypeSupportImpl.h"
#include "SubscriptionUpdateTypeSupportC.h"
#include "SubscriptionUpdateTypeSupportImpl.h"
#include "PublicationUpdateTypeSupportC.h"
#include "PublicationUpdateTypeSupportImpl.h"

#if !defined (__ACE_INLINE__)
# include "FederatorSubscriptions.inl"
#endif /* ! __ACE_INLINE__ */

namespace OpenDDS { namespace Federator {

Subscriptions::Subscriptions()
 : readers_( 4)
{
}

Subscriptions::~Subscriptions()
{
  // At this point, the participant->delete_contained_entities() has been
  // called, so non of our service Entities remain valid.

  delete this->linkListener_;

  for( unsigned int index = 0; index < this->listeners_.size(); ++index) {
    delete this->listeners_[ index];
  }

  // The readers and subscriber are deleted recursively when the
  // containing participant has delete_contained_entities called on it.
  // The references that we have are deleted as we are destroyed.
}

void
Subscriptions::initialize(
  ::DDS::Subscriber_ptr        subscriber,
  ::DDS::DomainParticipant_ptr participant,
  ManagerImpl*                 manager
)
{
  this->subscriber_ = subscriber;
  this->manager_    = manager;

  // Create the LinkState listener
  this->linkListener_ = new LinkListener( *manager);

  // Create the LinkState Topic
  ::DDS::Topic_var topic
    = participant->create_topic(
        LINKSTATETOPICNAME,
        LINKSTATETYPENAME,
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil()
      );

  // Obtain LinkState Topic Description. 
  ::DDS::TopicDescription_var description
    = participant->lookup_topicdescription( LINKSTATETOPICNAME);

  // Create the LinkState subscription
  this->linkReader_
    = this->subscriber_->create_datareader(
        description,
        DATAREADER_QOS_DEFAULT,
        this->linkListener_
      );

  // Create the ParticipantUpdate Topic
  ::DDS::Topic_var participantTopic
    = participant->create_topic(
        PARTICIPANTUPDATETOPICNAME,
        PARTICIPANTUPDATETYPENAME,
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil()
      );

  // Create the TopicUpdate Topic
  ::DDS::Topic_var topicTopic
    = participant->create_topic(
        TOPICUPDATETOPICNAME,
        TOPICUPDATETYPENAME,
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil()
      );

  // Create the PublicationUpdate Topic
  ::DDS::Topic_var publicationTopic
    = participant->create_topic(
        PUBLICATIONUPDATETOPICNAME,
        PUBLICATIONUPDATETYPENAME,
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil()
      );

  // Create the SubscriptionUpdate Topic
  ::DDS::Topic_var subscriptionTopic
    = participant->create_topic(
        SUBSCRIPTIONUPDATETOPICNAME,
        SUBSCRIPTIONUPDATETYPENAME,
        TOPIC_QOS_DEFAULT,
        ::DDS::TopicListener::_nil()
      );
}

void
Subscriptions::subscribeToUpdates( ::DDS::DomainParticipant_ptr participant)
{
  // Create the ParticipantUpdate Listener
  UpdateListener< ParticipantUpdate, ParticipantUpdateDataReader>* participantListener
    = new UpdateListener< ParticipantUpdate, ParticipantUpdateDataReader>( *this->manager_);
  this->listeners_.push_back( participantListener);

  // Obtain ParticipantUpdate Topic Description. 
  ::DDS::TopicDescription_var participantDescription
    = participant->lookup_topicdescription( PARTICIPANTUPDATETOPICNAME);

  // Create the ParticipantUpdate subscription
  this->readers_[ 0]
    = this->subscriber_->create_datareader(
        participantDescription,
        DATAREADER_QOS_DEFAULT,
        participantListener
      );

  // Create the TopicUpdate Listener
  UpdateListener< TopicUpdate, TopicUpdateDataReader>* topicListener
    = new UpdateListener< TopicUpdate, TopicUpdateDataReader>( *this->manager_);
  this->listeners_.push_back( topicListener);

  // Obtain TopicUpdate Description. 
  ::DDS::TopicDescription_var topicDescription
    = participant->lookup_topicdescription( TOPICUPDATETOPICNAME);

  // Create the TopicUpdate subscription
  this->readers_[ 1]
    = this->subscriber_->create_datareader(
        topicDescription,
        DATAREADER_QOS_DEFAULT,
        topicListener
      );

  // Create the PublicationUpdate Listener
  UpdateListener< PublicationUpdate, PublicationUpdateDataReader>* publicationListener
    = new UpdateListener< PublicationUpdate, PublicationUpdateDataReader>( *this->manager_);
  this->listeners_.push_back( publicationListener);

  // Obtain PublicationUpdate Topic Description. 
  ::DDS::TopicDescription_var publicationDescription
    = participant->lookup_topicdescription( PUBLICATIONUPDATETOPICNAME);

  // Create the PublicationUpdate subscription
  this->readers_[ 2]
    = this->subscriber_->create_datareader(
        publicationDescription,
        DATAREADER_QOS_DEFAULT,
        publicationListener
      );

  // Create the SubscriptionUpdate Listener
  UpdateListener< SubscriptionUpdate, SubscriptionUpdateDataReader>* subscriptionListener
    = new UpdateListener< SubscriptionUpdate, SubscriptionUpdateDataReader>( *this->manager_);
  this->listeners_.push_back( subscriptionListener);

  // Obtain SubscriptionUpdate Topic Description. 
  ::DDS::TopicDescription_var subscriptionDescription
    = participant->lookup_topicdescription( SUBSCRIPTIONUPDATETOPICNAME);

  // Create the SubscriptionUpdate subscription
  this->readers_[ 3]
    = this->subscriber_->create_datareader(
        subscriptionDescription,
        DATAREADER_QOS_DEFAULT,
        subscriptionListener
      );

}

void
Subscriptions::unsubscribeFromUpdates()
{
  // set_listeners() to nil and remove the readers from the service.
  for( unsigned int index = 0; index < this->readers_.size(); ++index) {
    this->readers_[ index]->set_listener(
      ::DDS::DataReaderListener::_nil(),
      OpenDDS::DCPS::DEFAULT_STATUS_KIND_MASK
    );
    this->subscriber_->delete_datareader( this->readers_[ index].in());
  }

  // Lose the actual listener servants.
  for( unsigned int index = 0; index < this->listeners_.size(); ++index) {
    delete this->listeners_[ index];
    this->listeners_[ index] = 0;
  }

  // Delete the updateReaders.
  this->readers_.erase( this->readers_.begin(), this->readers_.end());
}

}} // End namespace OpenDDS::Federator

