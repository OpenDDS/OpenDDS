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
}

void
ManagerImpl::initialize()
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::initialize()\n")
    ));
  }

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
    throw Unavailable();
  }
}

// IDL methods.

RepoKey
ManagerImpl::federationId()
ACE_THROW_SPEC ((
  ::CORBA::SystemException
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::federationId()\n")
    ));
  }
  return this->config_.federationId();
}

Status
ManagerImpl::join_federation( const char * endpoint)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  Unavailable
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::join_federation()\n")
    ));
  }

  const std::string iorPrefix = "corbaloc:";
  std::string       remoteIor = iorPrefix + endpoint + "/" + FEDERATOR_IORTABLE_KEY;

  // Resolve remote federator
  CORBA::Object_var obj
    = this->orb_->string_to_object( remoteIor.c_str());
  if( CORBA::is_nil( obj.in())) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ManagerImpl::join_federation - ")
      ACE_TEXT("unable to resolve remote federator.\n")
    ));
    throw Unavailable();
  }

  // Narrow the IOR to a Manager object reference.
  ::OpenDDS::Federator::Manager_var remoteFederator
    = ::OpenDDS::Federator::Manager::_narrow( obj.in());
  if( CORBA::is_nil( remoteFederator.in() ) ) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ManagerImpl::join_federation - ")
      ACE_TEXT("unable to narrow remote federator.\n")
    ));
    throw Unavailable();
  }

  // Obtain the remote repository federator Id value.
  RepoKey remote = remoteFederator->federationId();

  // Check that we are not recursing via a callback from that repository.
  if( remote == this->joining_) {
    // Do not block on recursion.  This is an expected path and will
    // result in deadlock if we block here.
    return Joining;
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
}

Status
ManagerImpl::remove_connection ( RepoKey remoteId)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ConnectionBusy
))
{
  if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) INFO: ManagerImpl::remove_connection()\n")
    ));
  }

  // Obtain the lock while we mess about with the structures.
  ACE_GUARD_THROW_EX( ACE_SYNCH_MUTEX, guard, this->lock_, ConnectionBusy());

  RemoteLinkMap::iterator location
    = this->remoteLink_.find( remoteId);
  if( location != this->remoteLink_.end()) {
    // Release the resources associated with the remote repository.
    delete location->second;

    // Remove the remote repository from our map.
    this->remoteLink_.erase( location);

  } else {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ManagerImpl::remove_connection() ")
      ACE_TEXT("on repository %d - ")
      ACE_TEXT("attempt to remove non-MST repository %d from MST list.\n"),
      this->id(),
      remoteId
    ));
    throw ConnectionBusy();
  }

  if( 0 == this->remoteLink_.size()) {
    // We no longer are connected to any remote repository, so we need to
    // clean up all of our internal mappings to remove ourselves from the
    // federation.  We cannot participate in the federation without being
    // connected to at least one other repository within the federation.
    for( RepoToIdMap::const_iterator current = this->inboundMap_.find( remoteId);
         current != this->inboundMap_.end();
         ++current) {
      this->unfederate( current->first);
    }

  } else {
    /// @TODO: Figure out how to determine if we can reach the remote
    ///        repository and unfederate() it if we cant.
  }

  // Publish (<self>,<remote>,OFF,<sequence>) on all LinkState
  // publications.

  /// @TODO: Implement this.

  return Unfederated;
}

void
ManagerImpl::updateLinkState(
  LinkState         sample,
  ::DDS::SampleInfo /* info */
)
{
  ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->lock_);

  // First determine if this is new data.
  if( this->remoteLink_[ sample.source]->lastSeen()
      >= sample.packet
    ) {
    return;
  }

  // Indicate that we have procesed this sequence datum.
  this->remoteLink_[ sample.source]->lastSeen() = sample.packet;

  // Lists of repositories removed from and added to the MST.
  LinkStateManager::LinkList added;
  LinkStateManager::LinkList removed;

  // Update the topology and current MST.
  this->linkStateManager_.update( sample, removed, added);

  // Process removals first to avoid flooding historical data over
  // connections that will be removed during the same processing.
  for( int index = 0; index < int( removed.size()); ++index) {
    // This is ugly.
    RepoKey remote = this->id();
    if( remote == removed[ index].first) {
      remote = removed[ index].second;

    } else if( remote == removed[ index].second) {
      remote = removed[ index].first;
    }

    if( remote != this->id()) {
      // Maintain our set of MST connections.
      std::set< RepoKey>::iterator location = this->mstNodes_.find( remote);
      if( location != this->mstNodes_.end()) {
        this->mstNodes_.erase( location);

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ManagerImpl::updateLinkState() ")
          ACE_TEXT("on repository %d - ")
          ACE_TEXT("attempt to remove non-MST repository %d from MST list.\n"),
          this->id(),
          remote
        ));

        // Since our internal state has become noticably corrupt, should
        // we throw here?
        continue;
      }

      //
      // Since we require the same lock that is held during the
      // join_federation() call, that call _must_ have completed before
      // we can receive any LinkState updates that contain this
      // connection.  Which means that the internal state has been
      // corrupted if we cannot find the remote link in our maps.
      //
      RemoteLinkMap::const_iterator linkLocation
        = this->remoteLink_.find( remote);
      if( linkLocation != this->remoteLink_.end()) {
        // At this point, remote is the Id value of a remote repository
        // that we are directly connected to that is no longer on the MST.
        linkLocation->second->removeFromMst();

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ManagerImpl::updateLinkState() ")
          ACE_TEXT("on repository %d - ")
          ACE_TEXT("unable to unsubscribe from remote repository %d.\n"),
          this->id(),
          remote
        ));

        // Since our internal state has become noticably corrupt, should
        // we throw here?
        continue;
      }
    }
  }

  // Process additions second to avoid flooding historical data over
  // connections that have been removed during the same processing.
  for( int index = 0; index < int( added.size()); ++index) {
    // This is ugly.
    RepoKey remote = this->id();
    if( remote == added[ index].first) {
      remote = added[ index].second;

    } else if( remote == added[ index].second) {
      remote = added[ index].first;
    }

    if( remote != this->id()) {
      // Maintain our set of MST connections.
      this->mstNodes_.insert( remote);

      //
      // Since we require the same lock that is held during the
      // join_federation() call, that call _must_ have completed before
      // we can receive any LinkState updates that contain this
      // connection.  Which means that the internal state has been
      // corrupted if we cannot find the remote link in our maps.
      //
      RemoteLinkMap::const_iterator linkLocation
        = this->remoteLink_.find( remote);
      if( linkLocation != this->remoteLink_.end()) {
        // At this point, remote is the Id value of a remote repository
        // that we are directly connected to that is now on the MST.
        linkLocation->second->addToMst();

        // Check if we have this repository in our mappings already.
        RepoToIdMap::const_iterator inboundLocation
          = this->inboundMap_.find( remote);
        if( inboundLocation == this->inboundMap_.end()) {
          // If we do not already have this repository in our mappings,
          // publish all of our data to it, since it is a new repository
          // to the federation.

          /// @TODO: Implement this.

          // Add the remote repository to our mappings so that we do not
          // publish to it again.  Unless it departs and rejoins, of course.
          (void)this->inboundMap_.insert(
            RepoToIdMap::value_type(remote, RepoToIdMap::mapped_type())
          );
        }

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ManagerImpl::updateLinkState() ")
          ACE_TEXT("on repository %d - ")
          ACE_TEXT("unable to subscribe to remote repository %d.\n"),
          this->id(),
          remote
        ));

        // Since our internal state has become noticably corrupt, should
        // we throw here?
        continue;
      }
    }
  }
}

void
ManagerImpl::unfederate( RepoKey remote)
{
  // N.B. The caller of this method _must_ hold the Manager object lock
  //      during this call.

  RepoToIdMap::iterator inboundLocation = this->inboundMap_.find( remote);
  if( inboundLocation != this->inboundMap_.end()) {
    // Remove the inbound mappings and destroy the entities.
    for( RemoteToLocalMap::iterator current = inboundLocation->second.begin();
         current != inboundLocation->second.end();
         ++current) {

      /// @TODO: Delete the entities here.
      //::OpenDDS::DCPS::RepoId localId = current->second;

      // Remove from the outbound mappings as well.
      LocalToFederationMap::iterator outboundLocation
        = this->outboundMap_.find( current->second);
      if( outboundLocation != this->outboundMap_.end()) {
        this->outboundMap_.erase( outboundLocation);

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: ManagerImpl::unfederate() ")
          ACE_TEXT("on repository %d - ")
          ACE_TEXT("unable to locate corresponding outbound mapping ")
          ACE_TEXT("to remote(%d,%d) == local(%d).\n"),
          this->id(),
          remote,
          current->first,
          current->second
        ));
      }
    }

    // Empty the inbound remote to local mappings.
    inboundLocation->second.erase(
      inboundLocation->second.begin(),
      inboundLocation->second.end()
    );
  }

  // Finally remove the inbound mappings completely.
  this->inboundMap_.erase( inboundLocation);

}

}} // End namespace OpenDDS::Federator

