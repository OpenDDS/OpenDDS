// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGER_H
#define FEDERATORMANAGER_H

#include "FederatorS.h"

namespace OpenDDS { namespace Federator {

class FederatorManager : public virtual POA_OpenDDS::Federator::Manager {
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


  private:

};

}} // End namespace OpenDDS::Federator

#endif // FEDERATORMANAGER_H

