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
  Update::ItemType      type,
  const Update::IdType& id,
  Update::ActorType     actor,
  long                  domain,
  const Update::IdType& participant
)
{
  switch( type) {
    case Update::Topic:
      {
        TopicUpdate sample;
        sample.sender      = this->id();
        sample.action      = DestroyEntity;

        sample.id          = id;
        sample.domain      = domain;
        sample.participant = participant;

        this->topicWriter_->write( sample, ::DDS::HANDLE_NIL);
      }
      break;

    case Update::Participant:
      {
        ParticipantUpdate sample;
        sample.sender = this->id();
        sample.action = DestroyEntity;

        sample.domain = domain;
        sample.id     = id;

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

            sample.domain         = domain;
            sample.participant    = participant;
            sample.id             = id;

            this->publicationWriter_->write( sample, ::DDS::HANDLE_NIL);
          }
          break;

        case Update::DataReader:
          {
            SubscriptionUpdate sample;
            sample.sender         = this->id();
            sample.action         = DestroyEntity;

            sample.domain         = domain;
            sample.participant    = participant;
            sample.id             = id;

            this->subscriptionWriter_->write( sample, ::DDS::HANDLE_NIL);
          }
          break;
      }
      break;
  }
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::DomainParticipantQos& qos)
{
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::TopicQos& qos)
{
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::DataWriterQos& qos)
{
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::PublisherQos& qos)
{
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::DataReaderQos& qos)
{
}

void
ManagerImpl::update( const Update::IdType& id, const ::DDS::SubscriberQos& qos)
{
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
ManagerImpl::processUpdate( const OwnerUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->changeOwnership(
    sample->domain,
    sample->participant,
    sample->sender,
    sample->owner
  );
}

void
ManagerImpl::processUpdate( const PublicationUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_publication_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->datawriter_qos,
    sample->publisher_qos
  );
}

void
ManagerImpl::processUpdate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_subscription_qos(
    sample->domain,
    sample->participant,
    sample->id,
    sample->datareader_qos,
    sample->subscriber_qos
  );
}

void
ManagerImpl::processUpdate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* /* info */)
{
  this->info_->update_domain_participant_qos(
    sample->domain,
    sample->id,
    sample->qos
  );
}

void
ManagerImpl::processUpdate( const TopicUpdate* sample, const ::DDS::SampleInfo* /* info */)
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

