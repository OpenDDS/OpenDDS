// -*- C++ -*-
//
// $Id$

#include "FederatorRemoteData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"

#include "LinkStateTypeSupportC.h"
#include "LinkStateTypeSupportImpl.h"

#include "ParticipantUpdateTypeSupportC.h"
#include "ParticipantUpdateTypeSupportImpl.h"
#include "PublicationUpdateTypeSupportC.h"
#include "PublicationUpdateTypeSupportImpl.h"
#include "SubscriptionUpdateTypeSupportC.h"
#include "SubscriptionUpdateTypeSupportImpl.h"
#include "TopicUpdateTypeSupportC.h"
#include "TopicUpdateTypeSupportImpl.h"


#if !defined (__ACE_INLINE__)
# include "FederatorRemoteData.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope.

  // Size of PARTITION names for federation communications.
  enum { PARTITIONNAME_SIZE = 17 };

} // End of anonymous namespace.

namespace OpenDDS { namespace Federator {

RemoteData::RemoteData( RepoKey self, RepoKey remote, const std::string& nic)
 : federationId_( remote)
{
  char buffer[PARTITIONNAME_SIZE];

  ACE_OS::sprintf( &buffer[0], "%08.8x-%08.8x", this->federationId(), self);
  this->inbound_ = &buffer[0];

  ACE_OS::sprintf( &buffer[0], "%08.8x-%08.8x", self, this->federationId());
  this->external_ = &buffer[0];

  // Grab a participant for publishing locally.
  this->participant_
    = TheParticipantFactory->create_participant(
        this->federationId(),
        PARTICIPANT_QOS_DEFAULT,
        ::DDS::DomainParticipantListener::_nil()
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: create_participant failed for ")
      ACE_TEXT( "RemoteData domain %d.\n"),
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Create, configure and install the transport on this connection.
  this->transport_
    = TheTransportFactory->create_transport_impl(
        this->federationId(), "SimpleTcp", OpenDDS::DCPS::DONT_AUTO_CONFIG
      );

  OpenDDS::DCPS::TransportConfiguration_rch transportConfig
    = TheTransportFactory->create_configuration( this->federationId(), "SimpleTcp");

  OpenDDS::DCPS::SimpleTcpConfiguration* tcpConfig
    = static_cast< OpenDDS::DCPS::SimpleTcpConfiguration*>( transportConfig.in());

  ACE_INET_Addr writer_address( nic.c_str());
  tcpConfig->local_address_ = writer_address;

  if( this->transport_->configure( tcpConfig) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on %d failed to initialize transport ")
      ACE_TEXT("with NIC %s in partition %s to repository %d failed.\n"),
      this->federationId(),
      nic.c_str(),
      this->external().c_str(),
      self
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Add type support for update topics
  TopicUpdateTypeSupportImpl* topicUpdate = new TopicUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != topicUpdate->register_type(
                             this->participant_,
                             TOPICUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on repository %d unable to install ")
      ACE_TEXT("TopicUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  ParticipantUpdateTypeSupportImpl* participantUpdate = new ParticipantUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != participantUpdate->register_type(
                             this->participant_,
                             PARTICIPANTUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on repository %d unable to install ")
      ACE_TEXT("ParticipantUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  PublicationUpdateTypeSupportImpl* publicationUpdate = new PublicationUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != publicationUpdate->register_type(
                             this->participant_,
                             PUBLICATIONUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on repository %d unable to install ")
      ACE_TEXT("PublicationUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  SubscriptionUpdateTypeSupportImpl* subscriptionUpdate = new SubscriptionUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != subscriptionUpdate->register_type(
                             this->participant_,
                             SUBSCRIPTIONUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on repository %d unable to install ")
      ACE_TEXT("SubscriptionUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Add type support for link state topics
  LinkStateTypeSupportImpl* linkState = new LinkStateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != linkState->register_type(
                             this->participant_,
                             LINKSTATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteData on repository %d unable to install ")
      ACE_TEXT("LinkState type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: RemoteData on %d initialized ")
      ACE_TEXT("with NIC %s to repository %d.\n"),
      self,
      nic.c_str(),
      this->federationId()
    ));
  }

}

RemoteData::~RemoteData()
{
  if( 0 == CORBA::is_nil( this->participant_)) {
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RemoteData unable to release resources for domain %d.\n"),
        this->federationId()
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant( this->participant_)
             ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RemoteData unable to release the participant for domain %d.\n"),
        this->federationId()));
    }
  }

  TheTransportFactory->release( this->federationId());
}

}} // End namespace OpenDDS::Federator

