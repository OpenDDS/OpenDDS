// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManager.h"
#include "FederatorConfig.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

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

#include <string>

namespace OpenDDS { namespace Federator {

FederatorManager::FederatorManager( Config& config)
 : config_( config)
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::FederatorManager()\n")
    ));
  }
  // Initialization is deferred until the service has been initialized.
}

FederatorManager::~FederatorManager()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::~FederatorManager()\n")
    ));
  }

  // Remove links with any remote repositories.
  for( RemoteLinkMap::iterator current = this->remoteLink_.begin();
       current != this->remoteLink_.end();
       ++current) {
    delete current->second;
  }
  this->remoteLink_.erase( this->remoteLink_.begin(), this->remoteLink_.end());

  // Remove our local participant and contained entities.
  if( 0 == CORBA::is_nil( this->participant_)) {
    if( ::DDS::RETCODE_PRECONDITION_NOT_MET
         == this->participant_->delete_contained_entities()
      ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Federator::Manager ")
        ACE_TEXT("unable to release resources for repository %d.\n"),
        this->config_.federationId()
      ));

    } else if( ::DDS::RETCODE_PRECONDITION_NOT_MET
               == TheParticipantFactory->delete_participant( this->participant_)
             ) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Federator::Manager ")
        ACE_TEXT("unable to release the participant for repository %d.\n"),
        this->config_.federationId()));
    }
  }

  // Release our internal transport.
  TheTransportFactory->release( this->config_.federationId());
}

void
FederatorManager::initialize()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::initialize()\n")
    ));
  }

  // Add participant for <self> domain
  this->participant_
    = TheParticipantFactory->create_participant(
        this->config_.federationId(),
        PARTICIPANT_QOS_DEFAULT,
        ::DDS::DomainParticipantListener::_nil()
      );
  if( CORBA::is_nil( this->participant_.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: create_participant failed for RemoteLink ")
      ACE_TEXT( "domain %d.\n"),
      this->config_.federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Create, configure and install the transport within this repository
  this->transport_
    = TheTransportFactory->create_transport_impl(
        this->config_.federationId(), "SimpleTcp", OpenDDS::DCPS::DONT_AUTO_CONFIG
      );

  OpenDDS::DCPS::TransportConfiguration_rch transportConfig
    = TheTransportFactory->create_configuration( this->config_.federationId(), "SimpleTcp");

  OpenDDS::DCPS::SimpleTcpConfiguration* tcpConfig
    = static_cast< OpenDDS::DCPS::SimpleTcpConfiguration*>( transportConfig.in());

  if( this->transport_->configure( tcpConfig) != 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Federator::Manager on %d ")
      ACE_TEXT("failed to initialize transport.\n"),
      this->config_.federationId()
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install ")
      ACE_TEXT("TopicUpdate type support for repository %d.\n"),
      this->config_.federationId()
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install ")
      ACE_TEXT("ParticipantUpdate type support for repository %d.\n"),
      this->config_.federationId()
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install ")
      ACE_TEXT("PublicationUpdate type support for repository %d.\n"),
      this->config_.federationId()
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install ")
      ACE_TEXT("SubscriptionUpdate type support for repository %d.\n"),
      this->config_.federationId()
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
      ACE_TEXT("(%P|%t) ERROR: Unable to install LinkState type support for repository %d.\n"),
      this->config_.federationId()
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Subscribe to LinkState data on <self> domain in any partition

  // Add publication for LinkState data on <self> domain in any partition

}

// IDL methods.

::OpenDDS::Federator::RepoKey
FederatorManager::federationId()
ACE_THROW_SPEC ((
  ::CORBA::SystemException
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::federationId()\n")
    ));
  }
  return this->config_.federationId();
}

::OpenDDS::Federator::Status
FederatorManager::join_federation( const char * endpoint)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ::OpenDDS::Federator::Unavailable
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::join_federation()\n")
    ));
  }

  const std::string iorPrefix = "corbaloc:";
  std::string       remoteIor = iorPrefix + endpoint + "/" + FEDERATOR_IORTABLE_KEY;

  // Resolve remote federator
  CORBA::Object_var obj
    = this->_this()->_get_orb()->string_to_object( remoteIor.c_str());
  if( CORBA::is_nil( obj.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: FederatorManager::join_federation - ")
      ACE_TEXT("unable to resolve remote federator.\n")
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Narrow the IOR to a Messenger object reference.
  ::OpenDDS::Federator::Manager_var remoteFederator
    = ::OpenDDS::Federator::Manager::_narrow( obj.in());
  if( CORBA::is_nil( remoteFederator.in() ) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: FederatorManager::join_federation - ")
      ACE_TEXT("unable to narrow remote federator.\n")
    ));
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Obtain the remote repository federator Id value.
  ::OpenDDS::Federator::RepoKey remote = remoteFederator->federationId();

  // Check that we are not recursing via a callback from that repository.
  if( remote == this->joining_) {
    // Do not block on recursion.  This is an expected path and will
    // result in deadlock if we block here.
    return ::OpenDDS::Federator::Joining;
  }

  // This is the start of the critical section processing.  Only a single
  // remote repository can be in the process of federating at any time.
  //
  // N.B. This lock will be held through the synchronous call back to the
  //      <remote> endpoint to have it perform its federation processing.
  //      This may take a looooong time.
  //
  ACE_GUARD_RETURN(
    ACE_SYNCH_MUTEX,
    guard,
    this->lock_,
    ::OpenDDS::Federator::Error_While_Federating
  );

  // Double checked lock synchronization pattern.
  if( remote == this->joining_) {
    return ::OpenDDS::Federator::Joining;
  }

  // Once we are in the critical path, check that we have not already
  // joined with this remote repository.  We do this inside the lock so
  // that we avoid collisions accessing the remoteLink_ container.
  RemoteLinkMap::const_iterator location
    = this->remoteLink_.find( remote);
  if( location != this->remoteLink_.end()) {
    // We have already established a connection with this remote
    // repository, no further processing required.
    return ::OpenDDS::Federator::Already_Federated;

  } else {
    // Mark our current processing partner and store its information.
    this->joining_ = remote;
  }

  // Extract the remote hostname from the supplied endpoint.
  std::string remoteEndpoint( endpoint);
  size_t start = 1 + remoteEndpoint.find_first_of( ':');
  size_t end   = remoteEndpoint.find_first_of( ':', start);
  std::string remoteHost = remoteEndpoint.substr( start, end - start);

  // Form our local endpoint information to send to the remote end.
  std::string localEndpoint( this->config_.route( remoteHost));
  localEndpoint += ':' + this->config_.federationPort();

  try {
    // Call remote repository join_federation() for symmetry.  This is the
    // source of the recursion that we are guarding against at the
    // beginning of this method.
    remoteFederator->join_federation( localEndpoint.c_str());

  } catch( const CORBA::Exception& ex) {
    ex._tao_print_exception( "ERROR: Unable to join remote repository: ");
    throw ::OpenDDS::Federator::Unavailable();
  }

  // Build remote repository DCPSInfoRepo IOR
  remoteIor = iorPrefix + endpoint + REPOSITORY_IORTABLE_KEY;

  // Add remote repository to Service_Participant in the <remote> domain
  TheServiceParticipant->set_repo_ior( remoteIor.c_str(), remote);

  // Store information and initialize interaction with the <remote> repository.
  RemoteLink* link
    = new RemoteLink(
        this->config_.federationId(),     // Self
        remote,                           // Remote
        this->config_.route( remoteHost), // Local endpoint NIC
        this                              // Callback
      );
  (void)this->remoteLink_.insert(
    RemoteLinkMap::value_type( remote, link)
  );

  // Add publication for update topics in <self> domain for the new
  // "<self>-<remote>" partition.

  // Publish the local Entities on the new partition.

  // Add publication for LinkState data on <remote> domain in
  // "<self>-<remote>" partition.

  // Publish (<self>,<remote>,ON,<sequence>) on all LinkState
  // publications.

  // Federation is complete.
  this->joining_ = ::OpenDDS::Federator::NIL_REPOSITORY;

  return ::OpenDDS::Federator::Federated;
}

::OpenDDS::Federator::Status
FederatorManager::remove_connection ( RepoKey /* remoteId */)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ::OpenDDS::Federator::ConnectionBusy
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: FederatorManager::remove_connection()\n")
    ));
  }
  return ::OpenDDS::Federator::Unfederated;
}

void
FederatorManager::updateLinkState(
  ::OpenDDS::Federator::LinkState /* sample */,
  ::DDS::SampleInfo /* info */
)
{
  /// @TODO: Implement this
}

}} // End namespace OpenDDS::Federator

