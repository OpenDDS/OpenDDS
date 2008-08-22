// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManagerImpl.h"
#include "DCPSInfo_i.h"

namespace OpenDDS { namespace Federator {

void
ManagerImpl::unregisterCallback()
{
  /* This method intentionally left unimplemented. */
}

void
ManagerImpl::requestImage()
{
  /* This method intentionally left unimplemented. */
}

////////////////////////////////////////////////////////////////////////
//
// The following methods publish updates to the remainder of the
// federation.
//

void
ManagerImpl::create( const Update::UTopic& topic)
{
  TopicUpdate sample;
  sample.sender      = this->id();
  sample.action      = CreateEntity;

  sample.id          = topic.topicId;
  sample.domain      = topic.domainId;
  sample.participant = topic.participantId;
  sample.topic       = topic.name.c_str();
  sample.datatype    = topic.dataType.c_str();
  sample.qos         = topic.topicQos;

  this->topicWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::create( const Update::UParticipant& participant)
{
  ParticipantUpdate sample;
  sample.sender = this->id();
  sample.action = CreateEntity;

  sample.domain = participant.domainId;
  sample.id     = participant.participantId;
  sample.qos    = participant.participantQos;

  this->participantWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::create( const Update::URActor& reader)
{
  SubscriptionUpdate sample;
  sample.sender         = this->id();
  sample.action         = CreateEntity;

  sample.domain         = reader.domainId;
  sample.participant    = reader.participantId;
  sample.topic          = reader.topicId;
  sample.id             = reader.actorId;
  sample.callback       = reader.callback.c_str();
  sample.transport_id   = reader.transportInterfaceInfo.transport_id;
  sample.transport_blob = reader.transportInterfaceInfo.data;
  sample.datareader_qos = reader.drdwQos;
  sample.subscriber_qos = reader.pubsubQos;

  this->subscriptionWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::create( const Update::UWActor& writer)
{
  PublicationUpdate sample;
  sample.sender         = this->id();
  sample.action         = CreateEntity;

  sample.domain         = writer.domainId;
  sample.participant    = writer.participantId;
  sample.topic          = writer.topicId;
  sample.id             = writer.actorId;
  sample.callback       = writer.callback.c_str();
  sample.transport_id   = writer.transportInterfaceInfo.transport_id;
  sample.transport_blob = writer.transportInterfaceInfo.data;
  sample.datawriter_qos = writer.drdwQos;
  sample.publisher_qos  = writer.pubsubQos;

  this->publicationWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::create( const Update::OwnershipData& data)
{
  OwnerUpdate sample;
  sample.sender      = this->id();
  sample.action      = CreateEntity;

  sample.domain      = data.domain;
  sample.participant = data.participant;
  sample.owner       = data.owner;

  this->ownerWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::destroy(
  const Update::IdPath& id,
  Update::ItemType      type,
  Update::ActorType     actor
)
{
  switch( type) {
    case Update::Topic:
      {
        TopicUpdate sample;
        sample.sender      = this->id();
        sample.action      = DestroyEntity;

        sample.id          = id.id;
        sample.domain      = id.domain;
        sample.participant = id.participant;

        this->topicWriter_->write( sample, ::DDS::HANDLE_NIL);
      }
      break;

    case Update::Participant:
      {
        ParticipantUpdate sample;
        sample.sender = this->id();
        sample.action = DestroyEntity;

        sample.domain = id.domain;
        sample.id     = id.id;

        this->participantWriter_->write( sample, ::DDS::HANDLE_NIL);
      }
      break;

    case Update::Actor:
      // This is VERY annoying.
      switch( actor) {
        case Update::DataWriter:
          {
            PublicationUpdate sample;
            sample.sender         = this->id();
            sample.action         = DestroyEntity;

            sample.domain         = id.domain;
            sample.participant    = id.participant;
            sample.id             = id.id;

            this->publicationWriter_->write( sample, ::DDS::HANDLE_NIL);
          }
          break;

        case Update::DataReader:
          {
            SubscriptionUpdate sample;
            sample.sender         = this->id();
            sample.action         = DestroyEntity;

            sample.domain         = id.domain;
            sample.participant    = id.participant;
            sample.id             = id.id;

            this->subscriptionWriter_->write( sample, ::DDS::HANDLE_NIL);
          }
          break;
      }
      break;
  }
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::DomainParticipantQos& qos)
{
  ParticipantUpdate sample;
  sample.sender = this->id();
  sample.action = UpdateQosValue1;

  sample.domain = id.domain;
  sample.id     = id.id;
  sample.qos    = qos;

  this->participantWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::TopicQos& qos)
{
  TopicUpdate sample;
  sample.sender      = this->id();
  sample.action      = UpdateQosValue1;

  sample.id          = id.id;
  sample.domain      = id.domain;
  sample.participant = id.participant;
  sample.qos         = qos;

  this->topicWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::DataWriterQos& qos)
{
  PublicationUpdate sample;
  sample.sender         = this->id();
  sample.action         = UpdateQosValue1;

  sample.domain         = id.domain;
  sample.participant    = id.participant;
  sample.id             = id.id;
  sample.datawriter_qos = qos;

  this->publicationWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::PublisherQos& qos)
{
  PublicationUpdate sample;
  sample.sender         = this->id();
  sample.action         = UpdateQosValue2;

  sample.domain         = id.domain;
  sample.participant    = id.participant;
  sample.id             = id.id;
  sample.publisher_qos  = qos;

  this->publicationWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::DataReaderQos& qos)
{
  SubscriptionUpdate sample;
  sample.sender         = this->id();
  sample.action         = UpdateQosValue1;

  sample.domain         = id.domain;
  sample.participant    = id.participant;
  sample.id             = id.id;
  sample.datareader_qos = qos;

  this->subscriptionWriter_->write( sample, ::DDS::HANDLE_NIL);
}

void
ManagerImpl::update( const Update::IdPath& id, const ::DDS::SubscriberQos& qos)
{
  SubscriptionUpdate sample;
  sample.sender         = this->id();
  sample.action         = UpdateQosValue2;

  sample.domain         = id.domain;
  sample.participant    = id.participant;
  sample.id             = id.id;
  sample.subscriber_qos = qos;

  this->subscriptionWriter_->write( sample, ::DDS::HANDLE_NIL);
}

////////////////////////////////////////////////////////////////////////
//
// The following methods process updates received from the remainder
// of the federation.
//

void
ManagerImpl::processCreate( const OwnerUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  // We could generate an error message here.  Instead we let action be irrelevant.
  this->info_->changeOwnership(
    sample->domain,
    sample->participant,
    sample->sender,
    sample->owner
  );
}

void
ManagerImpl::processCreate( const PublicationUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  ::OpenDDS::DCPS::TransportInterfaceInfo transportInfo;
  transportInfo.transport_id = sample->transport_id;
  transportInfo.data         = sample->transport_blob;

  this->info_->add_publication(
    sample->domain,
    sample->participant,
    sample->topic,
    sample->id,
    sample->callback,
    sample->datawriter_qos,
    transportInfo,
    sample->publisher_qos
  );
}

void
ManagerImpl::processCreate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  ::OpenDDS::DCPS::TransportInterfaceInfo transportInfo;
  transportInfo.transport_id = sample->transport_id;
  transportInfo.data         = sample->transport_blob;

  this->info_->add_subscription(
    sample->domain,
    sample->participant,
    sample->topic,
    sample->id,
    sample->callback,
    sample->datareader_qos,
    transportInfo,
    sample->subscriber_qos
  );
}

void
ManagerImpl::processCreate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->add_domain_participant(
    sample->domain,
    sample->id,
    sample->qos
  );
}

void
ManagerImpl::processCreate( const TopicUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->add_topic(
    sample->id,
    sample->domain,
    sample->participant,
    sample->topic,
    sample->datatype,
    sample->qos
  );
}

void
ManagerImpl::processUpdateQos1( const OwnerUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->changeOwnership(
    sample->domain,
    sample->participant,
    sample->sender,
    sample->owner
  );
}

void
ManagerImpl::processUpdateQos1( const PublicationUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_publication_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->datawriter_qos
  );
}

void
ManagerImpl::processUpdateQos2( const PublicationUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_publication_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->publisher_qos
  );
}

void
ManagerImpl::processUpdateQos1( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_subscription_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->datareader_qos
  );
}

void
ManagerImpl::processUpdateQos2( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_subscription_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->subscriber_qos
  );
}

void
ManagerImpl::processUpdateQos1( const ParticipantUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_domain_participant_qos(
    sample->domain,
    sample->id,
    sample->qos
  );
}

void
ManagerImpl::processUpdateQos1( const TopicUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_topic_qos(
    sample->id,
    sample->domain,
    sample->participant,
    sample->qos
  );
}

void
ManagerImpl::processDelete( const OwnerUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  // We could generate an error message here.  Instead we let action be irrelevant.
  this->info_->changeOwnership(
    sample->domain,
    sample->participant,
    sample->sender,
    sample->owner
  );
}

void
ManagerImpl::processDelete( const PublicationUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->remove_publication(
    sample->domain,
    sample->participant,
    sample->id
  );
}

void
ManagerImpl::processDelete( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->remove_subscription(
    sample->domain,
    sample->participant,
    sample->id
  );
}

void
ManagerImpl::processDelete( const ParticipantUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->remove_domain_participant(
    sample->domain,
    sample->id
  );
}

void
ManagerImpl::processDelete( const TopicUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->remove_topic(
    sample->domain,
    sample->participant,
    sample->id
  );
}

}} // End namespace OpenDDS::Federator

