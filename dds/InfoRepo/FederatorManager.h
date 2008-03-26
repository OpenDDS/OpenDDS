// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGER_H
#define FEDERATORMANAGER_H

#include "federator_export.h"
#include "FederatorS.h"
#include "LinkStateManager.h"
#include "ace/Condition_T.h"

#include <set>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export FederatorManager 
  : public virtual POA_OpenDDS::Federator::Manager {
  public:
    /// Default constructor.
    FederatorManager();

    /// Virtual destructor.
    virtual ~FederatorManager();

    // IDL methods.

    virtual ::OpenDDS::Federator::Status join_federation (
      const char * endpoint
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      ::OpenDDS::Federator::Unavailable
    ));

    virtual ::OpenDDS::Federator::Status remove_connection (
      ::OpenDDS::Federator::RepoKey remoteId
    )
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      ::OpenDDS::Federator::ConnectionBusy
    ));

    virtual ::OpenDDS::Federator::RepoKey federationId()
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

    // Servant methods

    /// Accessors for the federation Id value.
    RepoKey& id();
    RepoKey  id() const;


  private:
    /// Critical section MUTEX.
    ACE_SYNCH_MUTEX lock_;

    /// Simple recursion avoidance during the join operations.
    RepoKey joining_;

    /// The repositories federation Id value within any federation.
    RepoKey federationId_;

    /// LinkState manager for distributing update data.
    LinkStateManager manager_;

    /// Set of directly connected repositories.
    std::set< RepoKey> connected_;

};

inline
RepoKey&
FederatorManager::id()
{
  return this->federationId_;
}

inline
RepoKey
FederatorManager::id() const
{
  return this->federationId_;
}

}} // End namespace OpenDDS::Federator

#endif // FEDERATORMANAGER_H

