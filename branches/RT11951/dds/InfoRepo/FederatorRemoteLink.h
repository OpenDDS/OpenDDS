// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORREMOTELINK_H
#define FEDERATORREMOTELINK_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "FederatorC.h"
#include "FederatorSubscriptions.h"
#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "dds/DdsDcpsDomainC.h"

#include <string>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export RemoteLink  {
  public:
    /**
     * @brief Construct with the information for the remote repository.
     *
     * @param self    - our own repository federation Id value
     * @param remote  - remote repository federation Id value
     * @param nic     - network interface on which to send to this repository
     * @param manager - called back when subscriptions receive data
     */
    RemoteLink(
      RepoKey            self,
      RepoKey            remote,
      const std::string& nic,
      FederatorManager*  manager
    );

    /// Virtual destructor.
    virtual ~RemoteLink();

    /// Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    /// Access the value of the inbound partition.
    const std::string& inbound() const;

    /// Access the value of the external partition.
    const std::string& external() const;

    /// Add this remote repository to the minimum spanning tree.
    void addToMst();

    /// Remove this remote repository from the minimum spanning tree.
    void removeFromMst();

  private:
    /// Configured Federation Id value.
    RepoKey federationId_;

    /// Inbound partition.
    std::string inbound_;

    /// External partition.
    std::string external_;

    /// Internal transport for <remote> domain
    OpenDDS::DCPS::TransportImpl_rch transport_;

    /// DomainParticipant in remote repository for <remote> domain.
    ::DDS::DomainParticipant_var participant_;

    /// Encapsulated Update and LinkState subscriptions for this repository.
    Subscriptions subscriptions_;

    /// DomainParticipant in remote repository for <remote> domain.
    ::OpenDDS::Federator::Manager_var manager_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorRemoteLink.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORREMOTELINK_H

