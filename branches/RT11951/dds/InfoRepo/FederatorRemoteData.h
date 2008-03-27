// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORREMOTEDATA_H
#define FEDERATORREMOTEDATA_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "FederatorC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsPublicationC.h"

#include <string>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export RemoteData  {
  public:
    /// Construct with a federation Id for the remote repository.
    RemoteData( RepoKey self, RepoKey remote);

    /// Virtual destructor.
    virtual ~RemoteData();

    /// Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    /// Access the value of the inbound partition.
    const std::string& inbound() const;

    /// Access the value of the external partition.
    const std::string& external() const;

  private:
    /// Configured Federation Id value.
    RepoKey federationId_;

    /// Inbound partition.
    std::string inbound_;

    /// External partition.
    std::string external_;

    /// DomainParticipant
    ::DDS::DomainParticipant_var participant_;

    /// DataReader
    ::DDS::DataReader_var reader_;

    /// DataWriter in the local domain to this repository
    ::DDS::DataWriter_var writer_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorRemoteData.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORREMOTEDATA_H

