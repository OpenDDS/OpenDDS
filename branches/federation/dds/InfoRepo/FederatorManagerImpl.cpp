// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManagerImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include "FederatorTypeSupportC.h"
#include "FederatorTypeSupportImpl.h"

#include <string>

#if !defined (__ACE_INLINE__)
# include "FederatorManagerImpl.inl"
#endif /* ! __ACE_INLINE__ */

namespace { // Anonymous namespace for file scope.

  // Starting key value for transport keys to use.
  enum { BASE_TRANSPORT_KEY_VALUE = 30};

} // End of anonymous namespace

namespace OpenDDS { namespace Federator {

ManagerImpl::ManagerImpl( Config& config)
 : config_( config),
   transportKeyValue_( BASE_TRANSPORT_KEY_VALUE)
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::ManagerImpl()\n")
    ));
  }
  // Initialization is deferred until the service has been initialized.
}

ManagerImpl::~ManagerImpl()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::~ManagerImpl()\n")
    ));
  }

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
}

void
ManagerImpl::initialize()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::initialize()\n")
    ));
  }

#if 0
  // N.B. The <self> domain participant uses the default repository which
  //      is the local repository.

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
    throw Unavailable();
  }

  // Add type support for update topics
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
    throw Unavailable();
  }

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
    throw Unavailable();
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
    throw Unavailable();
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
    throw Unavailable();
  }
#endif
}

// IDL methods.

RepoKey
ManagerImpl::federation_id( void)
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::federation_id()\n")
    ));
  }
  return this->config_.federationId();
}

::OpenDDS::DCPS::DCPSInfo_ptr
ManagerImpl::repository( void )
ACE_THROW_SPEC (( ::CORBA::SystemException))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::repository()\n")
    ));
  }
  return TheServiceParticipant->get_repository( this->config_.federationId());
}

::CORBA::Boolean
ManagerImpl::discover_federation ( const char * ior )
ACE_THROW_SPEC (( ::CORBA::SystemException, Incomplete))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::discover_federation( %s)\n"),
      ior
    ));
  }
  return false;
}

::CORBA::Boolean
ManagerImpl::join_federation(
  Manager_ptr peer,
  FederationDomain federation

) ACE_THROW_SPEC (( ::CORBA::SystemException, Incomplete))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::join_federation( peer, %x)\n"),
      federation
    ));
  }

  // Obtain the remote repository federator Id value.
  RepoKey remote = peer->federation_id();

  // Check that we are not recursing via a callback from that repository.
  if( remote == this->joining_) {
    // Do not block on recursion.  This is an expected path and will
    // result in deadlock if we block here.
    return true;
  }

  return false;
#if 0
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
    Error_While_Federating
  );

  // Double checked lock synchronization pattern.
  if( remote == this->joining_) {
    return Joining;
  }

  // Once we are in the critical path, check that we have not already
  // joined with this remote repository.  We do this inside the lock so
  // that we avoid collisions accessing the remoteLink_ container.
  RemoteLinkMap::const_iterator location
    = this->remoteLink_.find( remote);
  if( location != this->remoteLink_.end()) {
    // We have already established a connection with this remote
    // repository, no further processing required.
    return Already_Federated;

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
  /// @TODO: Make the protocol selectable - at least from the config.
  std::string localEndpoint( "iiop:");
  localEndpoint += this->config_.route( remoteHost) +  ":";
  localEndpoint += this->config_.federationPort();

  try {
    // Call remote repository join_federation() for symmetry.  This is the
    // source of the recursion that we are guarding against at the
    // beginning of this method.
    remoteFederator->join_federation( localEndpoint.c_str());

  } catch( const CORBA::Exception& ex) {
    ex._tao_print_exception( "ERROR: Unable to join remote repository: ");
    throw Unavailable();
  }

  // Build remote repository DCPSInfoRepo IOR
  remoteIor = iorPrefix + endpoint + "/" + REPOSITORY_IORTABLE_KEY;

  // Add remote repository to Service_Participant in the <remote> domain
  TheServiceParticipant->set_repo_ior( remoteIor.c_str(), remote);

  // Store information and initialize interaction with the <remote> repository.
  RemoteLink* link
    = new RemoteLink(
        this->config_.federationId(),     // Self
        remote,                           // Remote
        this->config_.route( remoteHost), // Local endpoint NIC
        this->transportKeyValue_,         // Transport key to use
        this,                             // Callback
        this->participant_.in()           // Local participant
      );
  (void)this->remoteLink_.insert(
    RemoteLinkMap::value_type( remote, link)
  );

  // The link will consume two transport keys: one for attaching to
  // Subscribers within our own repository and the other for attaching
  // to Publishers in the remote repository.
  //
  /// @TODO: Put a mechanism in place to reuse keys when a remote link is
  ///        disconnected.
  this->transportKeyValue_ += 2;

  // Publish (<self>,<remote>,ON,<sequence>) on all LinkState
  // publications.

  /// @TODO: Implement this.

  // Federation is complete.
  this->joining_ = NIL_REPOSITORY;

  return Federated;
#endif
}

}} // End namespace OpenDDS::Federator

