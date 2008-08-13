// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManagerImpl.h"
#include "DCPSInfo_i.h"

namespace OpenDDS { namespace Federator {

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

