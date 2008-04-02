// -*- C++ -*-
//
// $Id$

#include "FederatorRemoteLink.h"
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
# include "FederatorRemoteLink.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope.

  // Size of PARTITION names for federation communications.
  enum { PARTITIONNAME_SIZE = 17 };

} // End of anonymous namespace.

namespace OpenDDS { namespace Federator {

RemoteLink::RemoteLink(
  RepoKey                          self,
  RepoKey                          remote,
  const std::string&               nic,
  ::OpenDDS::DCPS::TransportIdType transportKey,
  ManagerImpl*                     manager,
  ::DDS::DomainParticipant_var     participant
) : federationId_( remote),
    transportKey_( transportKey)
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
      ACE_TEXT( "RemoteLink domain %d.\n"),
      this->federationId()
    ));
    throw Unavailable();
  }

  // Add type support for update topics
  TopicUpdateTypeSupportImpl* topicUpdate = new TopicUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != topicUpdate->register_type(
                             this->participant_,
                             TOPICUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on repository %d unable to install ")
      ACE_TEXT("TopicUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  ParticipantUpdateTypeSupportImpl* participantUpdate = new ParticipantUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != participantUpdate->register_type(
                             this->participant_,
                             PARTICIPANTUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on repository %d unable to install ")
      ACE_TEXT("ParticipantUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  PublicationUpdateTypeSupportImpl* publicationUpdate = new PublicationUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != publicationUpdate->register_type(
                             this->participant_,
                             PUBLICATIONUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on repository %d unable to install ")
      ACE_TEXT("PublicationUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  SubscriptionUpdateTypeSupportImpl* subscriptionUpdate = new SubscriptionUpdateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != subscriptionUpdate->register_type(
                             this->participant_,
                             SUBSCRIPTIONUPDATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on repository %d unable to install ")
      ACE_TEXT("SubscriptionUpdate type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  // Add type support for link state topics
  LinkStateTypeSupportImpl* linkState = new LinkStateTypeSupportImpl();
  if( ::DDS::RETCODE_OK != linkState->register_type(
                             this->participant_,
                             LINKSTATETYPENAME
                           )
    ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on repository %d unable to install ")
      ACE_TEXT("LinkState type support for repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: RemoteLink on %d initialized ")
      ACE_TEXT("with NIC %s to repository %d.\n"),
      self,
      nic.c_str(),
      this->federationId()
    ));
  }

  // Create, configure and install the transport on this connection.
  ::OpenDDS::DCPS::TransportImpl_rch transport
    = TheTransportFactory->create_transport_impl(
        this->transportKey_, "SimpleTcp", OpenDDS::DCPS::DONT_AUTO_CONFIG
      );

  OpenDDS::DCPS::TransportConfiguration_rch transportConfig
    = TheTransportFactory->create_configuration( this->transportKey_, "SimpleTcp");

  OpenDDS::DCPS::SimpleTcpConfiguration* tcpConfig
    = static_cast< OpenDDS::DCPS::SimpleTcpConfiguration*>( transportConfig.in());

  ACE_INET_Addr reader_address( nic.c_str());
  tcpConfig->local_address_ = reader_address;

  if( transport->configure( tcpConfig) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on %d failed to initialize inbound ")
      ACE_TEXT("transport with NIC %s in partition %s to repository %d failed.\n"),
      this->federationId(),
      nic.c_str(),
      this->external().c_str(),
      self
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // The subscriber is for the inbound partition only.
  ::DDS::SubscriberQos subscriberQos;
  this->participant_->get_default_subscriber_qos( subscriberQos);

  subscriberQos.partition.name.length( 1);
  subscriberQos.partition.name[0] = ACE_OS::strdup( this->inbound_.c_str());

  // Create the data subscriber.
  ::DDS::Subscriber_var subscriber
    = this->participant_->create_subscriber(
        subscriberQos, ::DDS::SubscriberListener::_nil()
      );
  if( CORBA::is_nil( subscriber.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on %d create subscriber ")
      ACE_TEXT("in partition %s to repository %d failed.\n"),
      this->federationId(),
      this->inbound_.c_str(),
      self
    ));
    throw Unavailable();
  }

  // And attach the transport to it.
  OpenDDS::DCPS::SubscriberImpl* subscriberServant
    = OpenDDS::DCPS::reference_to_servant< OpenDDS::DCPS::SubscriberImpl>(
        subscriber.in()
      );
  if( 0 == subscriberServant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) INFO: failed to extract servant for ")
      ACE_TEXT("subscriber on repository %d from repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  switch( subscriberServant->attach_transport( transport.in())) {
    case OpenDDS::DCPS::ATTACH_OK:
         break;

    case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
    case OpenDDS::DCPS::ATTACH_ERROR:
    case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
    default:
         ACE_ERROR((LM_ERROR,
           ACE_TEXT("(%P|%t) INFO: failed to attach transport to ")
           ACE_TEXT("subscriber on repository %d from repository %d.\n"),
           self,
           this->federationId()
         ));
         throw Unavailable();
  }

  // Initialize the subscriptions
  this->subscriptions_.initialize( subscriber.in(), this->participant_.in(), manager);

  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: RemoteLink on %d initialized ")
      ACE_TEXT("in partition %s to repository %d.\n"),
      this->federationId(),
      this->inbound_.c_str(),
      self
    ));
  }

  //
  // Now for the publications.
  //

  // Create, configure and install the transport for the publications.
  transport = TheTransportFactory->create_transport_impl(
                this->transportKey_ + 1,
                "SimpleTcp",
                OpenDDS::DCPS::DONT_AUTO_CONFIG
              );

  transportConfig
    = TheTransportFactory->create_configuration( this->transportKey_ + 1, "SimpleTcp");

  tcpConfig
    = static_cast< OpenDDS::DCPS::SimpleTcpConfiguration*>( transportConfig.in());

  ACE_INET_Addr writer_address( nic.c_str());
  tcpConfig->local_address_ = writer_address;

  if( transport->configure( tcpConfig) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on %d failed to initialize outbound ")
      ACE_TEXT("transport with NIC %s in partition %s to repository %d failed.\n"),
      this->federationId(),
      nic.c_str(),
      this->external().c_str(),
      self
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // The publications are for the external partition only.
  ::DDS::PublisherQos publisherQos;
  this->participant_->get_default_publisher_qos( publisherQos);

  publisherQos.partition.name.length( 1);
  publisherQos.partition.name[0] = ACE_OS::strdup( this->external_.c_str());

  // Create the data publisher.
  ::DDS::Publisher_var publisher
    = this->participant_->create_publisher(
        publisherQos, ::DDS::PublisherListener::_nil()
      );
  if( CORBA::is_nil( publisher.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: RemoteLink on %d create publisher ")
      ACE_TEXT("in partition %s to repository %d failed.\n"),
      this->federationId(),
      this->external_.c_str(),
      self
    ));
    throw Unavailable();
  }

  // And attach the transport to it.
  OpenDDS::DCPS::PublisherImpl* publisherServant
    = OpenDDS::DCPS::reference_to_servant< OpenDDS::DCPS::PublisherImpl>(
        publisher.in()
      );
  if( 0 == publisherServant) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) INFO: failed to extract servant for ")
      ACE_TEXT("publisher on repository %d to repository %d.\n"),
      self,
      this->federationId()
    ));
    throw Unavailable();
  }

  switch( publisherServant->attach_transport( transport.in())) {
    case OpenDDS::DCPS::ATTACH_OK:
         break;

    case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
    case OpenDDS::DCPS::ATTACH_ERROR:
    case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
    default:
         ACE_ERROR((LM_ERROR,
           ACE_TEXT("(%P|%t) INFO: failed to attach transport to ")
           ACE_TEXT("publisher on repository %d to repository %d.\n"),
           self,
           this->federationId()
         ));
         throw Unavailable();
  }

  // Now we can initialize the publications.
  this->publications_.initialize( participant.in(), publisher.in());
}

RemoteLink::~RemoteLink()
{
  if( 0 == CORBA::is_nil( this->participant_)) {
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RemoteLink unable to release resources for domain %d.\n"),
        this->federationId()
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant( this->participant_)
             ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RemoteLink unable to release the participant for domain %d.\n"),
        this->federationId()));
    }
  }

  TheTransportFactory->release( this->transportKey_);
  TheTransportFactory->release( this->transportKey_ + 1);
  /// @TODO: Check that this release will not interfere with the
  ///        aggregated Publications destructor operations.
}

}} // End namespace OpenDDS::Federator

