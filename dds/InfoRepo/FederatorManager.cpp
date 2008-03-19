// -*- C++ -*-
//
// $Id$

#include "FederatorManager.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

namespace OpenDDS { namespace Federator {

FederatorManager::FederatorManager()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) INFO: FederatorManager::FederatorManager()\n")
  ));
}

/// Virtual destructor.
FederatorManager::~FederatorManager()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) INFO: FederatorManager::~FederatorManager()\n")
  ));
}

// IDL methods.

::OpenDDS::Federator::Status
FederatorManager::join_federation (
  const char * /* endpoint */
)
ACE_THROW_SPEC ((
  ::CORBA::SystemException,
  ::OpenDDS::Federator::Unavailable
))
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) INFO: FederatorManager::join_federation()\n")
  ));
  return Unfederated;
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
    ACE_TEXT("%T (%P|%t) INFO: FederatorManager::remove_connection()\n")
  ));
  return Unfederated;
}

::OpenDDS::Federator::RepoKey
FederatorManager::federationId()
ACE_THROW_SPEC ((
  ::CORBA::SystemException
))
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%T (%P|%t) INFO: FederatorManager::federationId()\n")
  ));
  return 0;
}

}} // End namespace OpenDDS::Federator

