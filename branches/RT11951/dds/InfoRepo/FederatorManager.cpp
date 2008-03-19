// -*- C++ -*-
//
// $Id$

#include "FederatorManager.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

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

  // Avoid recursion.
  if( this->joining_) {
    // Be optimistic.
    return Joining;

  } else {
    this->joining_ = true;
  }

  // Resolve remote federator

  // Obtain the remote repository federator Id value.

  // Call remote repository join_federation() for symmetry.

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
  this->joining_ = false;

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

