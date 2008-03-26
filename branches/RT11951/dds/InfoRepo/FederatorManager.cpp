// -*- C++ -*-
//
// $Id$

#include "DcpsInfo_pch.h"
#include "FederatorManager.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <string>

namespace OpenDDS { namespace Federator {

FederatorManager::FederatorManager()
 : federationId_( -1)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::FederatorManager()\n")
  ));
}

/// Virtual destructor.
FederatorManager::~FederatorManager()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::~FederatorManager()\n")
  ));

  /// @TODO: remove connections with any remote repositories in connected_.
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
  return this->federationId_;
}

::OpenDDS::Federator::Status
FederatorManager::join_federation ( const char * endpoint)
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

  // Obtain the remote repository federator Id value.
  RepoKey joiner = NIL_REPOSITORY;

  // Check that we are not recursing via a callback from that node.
  if( joiner == this->joining_) {
    // Do not block on recursion.  This is an expected path and will
    // result in deadlock if we block here.
    return Joining;
  }

  // This is the start of the critical section processing.  Only a single
  // remote repository can be in the process of federating at any time.
  //
  // N.B. This lock will be held through the synchronous call back to the
  //      remote endpoint to have it perform its federation processing.
  //      This may take a looooong time.
  //
  ACE_GUARD_RETURN( ACE_SYNCH_MUTEX, guard, this->lock_, Error_While_Federating);

  // Double checked lock synchronization pattern.
  if( joiner == this->joining_) {
    return Joining;
  }

  // Once we are in the critical path, check that we have not already
  // joined with this remote repository.
  std::set< RepoKey>::const_iterator location
    = this->connected_.find( joiner);
  if( location != this->connected_.end()) {
    // We have already established a connection with this remote node,
    // no further processing required.
    return Already_Federated;

  } else {
    // Mark our current processing partner.
    this->joining_ = joiner;
  }

  // Call remote repository join_federation() for symmetry.  This is the
  // source of the recursion that we are guarding against at the
  // beginning of this method.

  // Resolve remote repository DCPSInfoRepo

  // Add remote repository to Service_Participant

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
  (void)this->connected_.insert( this->joining_);
  this->joining_ = NIL_REPOSITORY;

  return Federated;
}

::OpenDDS::Federator::Status
FederatorManager::remove_connection (
  ::OpenDDS::Federator::RepoKey /* remoteId */
)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ::OpenDDS::Federator::ConnectionBusy
))
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) INFO: FederatorManager::remove_connection()\n")
  ));
  return Unfederated;
}

}} // End namespace OpenDDS::Federator

