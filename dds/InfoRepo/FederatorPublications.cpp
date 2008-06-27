// -*- C++ -*-
//
// $Id$

#include "FederatorPublications.h"
#include "FederatorC.h"
// #include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#if !defined (__ACE_INLINE__)
# include "FederatorPublications.inl"
#endif /* ! __ACE_INLINE__ */

namespace OpenDDS { namespace Federator {

Publications::Publications()
{
}

Publications::~Publications()
{
  // Remote the publications.
  this->publisher_->delete_contained_entities();

  // Remove the publisher.
  this->participant_->delete_publisher( this->publisher_);
}

void
Publications::initialize(
  ::DDS::DomainParticipant_ptr participant,
  ::DDS::Publisher_ptr         publisher
)
{
  this->participant_ = participant;
  this->publisher_   = publisher;

  // Each writer has the same QoS policy values.
  ::DDS::DataWriterQos writerQos;
  this->publisher_->get_default_datawriter_qos( writerQos);

  writerQos.durability.kind                          = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  writerQos.history.kind                             = ::DDS::KEEP_ALL_HISTORY_QOS;
  writerQos.resource_limits.max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
  writerQos.reliability.kind                         = ::DDS::RELIABLE_RELIABILITY_QOS;
  writerQos.reliability.max_blocking_time.sec        = 0;
  writerQos.reliability.max_blocking_time.nanosec    = 0;

  // Create the ParticipantUpdate Topic
  ::DDS::Topic_var topic = this->participant_->create_topic(
                             PARTICIPANTUPDATETOPICNAME,
                             PARTICIPANTUPDATETYPENAME,
                             TOPIC_QOS_DEFAULT,
                             ::DDS::TopicListener::_nil()
                           );

  // Create the ParticipantUpdate publication
  ::DDS::DataWriter_var writer = this->publisher_->create_datawriter(
                                   topic.in(),
                                   writerQos,
                                   ::DDS::DataWriterListener::_nil()
                                 );
  if( CORBA::is_nil( writer.in()) ) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("create_datawriter() for topic %s failed.\n"),
        PARTICIPANTUPDATETOPICNAME
      ));
      throw Unavailable();
  }
  this->participantWriter_ = ParticipantUpdateDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->participantWriter_)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("failed to narrow writer for topic %s.\n"),
        PARTICIPANTUPDATETOPICNAME
      ));
      throw Unavailable();
  }

  // Create the TopicUpdate Topic
  topic = this->participant_->create_topic(
            TOPICUPDATETOPICNAME,
            TOPICUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            ::DDS::TopicListener::_nil()
          );

  // Create the TopicUpdate publication
  writer = this->publisher_->create_datawriter(
        topic.in(),
        writerQos,
        ::DDS::DataWriterListener::_nil()
      );
  if( CORBA::is_nil( writer.in()) ) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("create_datawriter() for topic %s failed.\n"),
        TOPICUPDATETOPICNAME
      ));
      throw Unavailable();
  }
  this->topicWriter_ = TopicUpdateDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->topicWriter_)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("failed to narrow writer for topic %s.\n"),
        TOPICUPDATETOPICNAME
      ));
      throw Unavailable();
  }

  // Create the PublicationUpdate Topic
  topic = this->participant_->create_topic(
            PUBLICATIONUPDATETOPICNAME,
            PUBLICATIONUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            ::DDS::TopicListener::_nil()
          );

  // Create the PublicationUpdate publication
  writer = this->publisher_->create_datawriter(
        topic.in(),
        writerQos,
        ::DDS::DataWriterListener::_nil()
      );
  if( CORBA::is_nil( writer.in()) ) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("create_datawriter() for topic %s failed.\n"),
        PUBLICATIONUPDATETOPICNAME
      ));
      throw Unavailable();
  }
  this->publicationWriter_ = PublicationUpdateDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->publicationWriter_)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("failed to narrow writer for topic %s.\n"),
        PUBLICATIONUPDATETOPICNAME
      ));
      throw Unavailable();
  }

  // Create the SubscriptionUpdate Topic
  topic = this->participant_->create_topic(
            SUBSCRIPTIONUPDATETOPICNAME,
            SUBSCRIPTIONUPDATETYPENAME,
            TOPIC_QOS_DEFAULT,
            ::DDS::TopicListener::_nil()
          );

  // Create the SubscriptionUpdate publication
  writer = this->publisher_->create_datawriter(
        topic.in(),
        writerQos,
        ::DDS::DataWriterListener::_nil()
      );
  if( CORBA::is_nil( writer.in()) ) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("create_datawriter() for topic %s failed.\n"),
        SUBSCRIPTIONUPDATETOPICNAME
      ));
      throw Unavailable();
  }
  this->subscriptionWriter_ = SubscriptionUpdateDataWriter::_narrow( writer.in());
  if( CORBA::is_nil( this->subscriptionWriter_)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Publications::initialize() - ")
        ACE_TEXT("failed to narrow writer for topic %s.\n"),
        SUBSCRIPTIONUPDATETOPICNAME
      ));
      throw Unavailable();
  }

}

}} // End namespace OpenDDS::Federator

