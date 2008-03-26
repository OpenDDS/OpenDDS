// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManager.h"
#include "FederatorConfig.h"
#include "dds/DCPS/Service_Participant.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <string>

namespace OpenDDS { namespace Federator {

FederatorManager::FederatorManager( Config& config)
 : config_( config)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::FederatorManager()\n")
  ));

  // Add participant for <self> domain

  // Subscribe to LinkState data on <self> domain in any partition

  // Add publication for LinkState data on <self> domain in any partition

}

/// Virtual destructor.
FederatorManager::~FederatorManager()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::~FederatorManager()\n")
  ));

  /// @TODO: remove connections with any remote repositories.
}

// IDL methods.

::OpenDDS::Federator::RepoKey
FederatorManager::federationId()
ACE_THROW_SPEC ((
  ::CORBA::SystemException
))
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::federationId()\n")
  ));
  return this->config_.federationId();
}

::OpenDDS::Federator::Status
FederatorManager::join_federation( const char * endpoint)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ::OpenDDS::Federator::Unavailable
))
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::join_federation()\n")
  ));

  const std::string iorPrefix = "corbaloc:";
  const std::string iorSuffix = "/Federator";

  std::string remoteIor = iorPrefix + endpoint + iorSuffix;

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
  ::OpenDDS::Federator::RepoKey joiner = remoteFederator->federationId();

  // Check that we are not recursing via a callback from that repository.
  if( joiner == this->joining_) {
    // Do not block on recursion.  This is an expected path and will
    // result in deadlock if we block here.
    return ::OpenDDS::Federator::Joining;
  }

  // This is the start of the critical section processing.  Only a single
  // remote repository can be in the process of federating at any time.
  //
  // N.B. This lock will be held through the synchronous call back to the
  //      remote endpoint to have it perform its federation processing.
  //      This may take a looooong time.
  //
  ACE_GUARD_RETURN(
    ACE_SYNCH_MUTEX,
    guard,
    this->lock_,
    ::OpenDDS::Federator::Error_While_Federating
  );

  // Double checked lock synchronization pattern.
  if( joiner == this->joining_) {
    return ::OpenDDS::Federator::Joining;
  }

  // Once we are in the critical path, check that we have not already
  // joined with this remote repository.  We do this inside the lock so
  // that we avoid collisions accessing the remoteData_ container.
  RemoteDataMap::const_iterator location
    = this->remoteData_.find( joiner);
  if( location != this->remoteData_.end()) {
    // We have already established a connection with this remote
    // repository, no further processing required.
    return ::OpenDDS::Federator::Already_Federated;

  } else {
    // Mark our current processing partner and store its information.
    this->joining_ = joiner;
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
  const std::string repoSuffix = "/DCPSInfoRepo";
  remoteIor = iorPrefix + endpoint + repoSuffix;

  // Add remote repository to Service_Participant in the remote domain
  TheServiceParticipant->set_repo_ior( remoteIor.c_str(), joiner);

  // Store information about this remote repository.
  RemoteData joinerData( this->config_.federationId(), joiner);
  (void)this->remoteData_.insert(
    RemoteDataMap::value_type( joiner, joinerData)
  );

  // Add publication for update topics in <self> domain for the new
  // "<self>-<remote>" partition.

  // Publish the local Entities on the new partition.

  // Add publication for LinkState data on <remote> domain in
  // "<self>-<remote>" partition.

  // Publish (<self>,<remote>,ON,<sequence>) on all LinkState
  // publications.

  // Subscribe to LinkState data on <remote> domain in "<remote>-<self>"
  // partition.

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
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::remove_connection()\n")
  ));
  return ::OpenDDS::Federator::Unfederated;
}

}} // End namespace OpenDDS::Federator

