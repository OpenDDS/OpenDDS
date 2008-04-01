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
#include "FederatorPublications.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"

#include <string>

namespace OpenDDS { namespace Federator {

/**
 * @class RemoteLink
 *
 * @brief Encapsulate the information and entities for a remote connection.
 *
 * This class encapsulates all of the information about a connection with
 * a remote (other) repository with which we are federated.  This
 * includes the remote repositories federation Id value, the partitions
 * where we receive data (inbound) and where we publish data (external)
 * as well as the subscriptions and publications with this remote
 * repository.
 *
 * A note about partitions and their names.  For generality, there are
 * four separate partitions known within this repository.  They include:
 *
 *   inbound  - a partition on which we receive data from the remote
 *              repository.
 *              Its value is: "<remote>-<self>"
 *
 *   external - a partition on which we send data to the remote
 *              repository.
 *              Its value is: "<remote>-<self>"
 *
 *   The next two partitions are not used for federation since the
 *   Federator and DCPSInfoRepo objects are collocated within the same
 *   executable process.
 *
 *   outbound - a partition on which we receive data from the repository
 *              with which we are associated.
 *              Its value is: "<self>-0"
 *
 *   internal - a partition on which we send data to the repository with
 *              which we are associated.
 *              Its value is: "<self>-<self>"
 *
 * A separate transport implementation is created for the inbound and
 * outbound data.  This is required since the inbound data is part of the
 * remote repository domain and the outbound data is part of the local
 * repository domain.  Since the publications and subscriptions reside
 * within different repositories, separate transport implementations are
 * required.  A transport may not be shared between repositories.
 *
 * In order to do this, the transports are keyed in the transport factory
 * using the supplied transport key and the next sequential key.  The
 * code which instantiates on object of this type is reponsible for
 * ensuring that these two contiguous key values are available in the
 * transport implementation factory.
 */
class OpenDDS_Federator_Export RemoteLink  {
  public:
    /**
     * @brief Construct with the information for the remote repository.
     *
     * @param self         - our own repository federation Id value
     * @param remote       - remote repository federation Id value
     * @param transportKey - the transport key value to use
     * @param nic          - local network interface on which to send
     * @param manager      - called back when subscriptions receive data
     */
    RemoteLink(
      RepoKey                          self,
      RepoKey                          remote,
      const std::string&               nic,
      ::OpenDDS::DCPS::TransportIdType transportKey,
      ManagerImpl*                     manager,
      ::DDS::DomainParticipant_var     participant
    );

    /// Virtual destructor.
    virtual ~RemoteLink();

    /// Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    /// Last processed sequence number from this repository.
    ::OpenDDS::DCPS::SequenceNumber& lastSeen();
    ::OpenDDS::DCPS::SequenceNumber  lastSeen() const;

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

    /// Base value of transport keys used by this remote repository.
    ::OpenDDS::DCPS::TransportIdType transportKey_;

    /// Last processed sequence number from this repository.
    ::OpenDDS::DCPS::SequenceNumber lastSeen_;

    /// Inbound partition.
    std::string inbound_;

    /// External partition.
    std::string external_;

    /// DomainParticipant in remote repository for <remote> domain.
    ::DDS::DomainParticipant_var participant_;

    /// Encapsulated Update and LinkState subscriptions for this repository.
    Subscriptions subscriptions_;

    /// Encapsulated Update and LinkState publications for this repository.
    Publications publications_;

    /// DomainParticipant in remote repository for <remote> domain.
    ::OpenDDS::Federator::Manager_var manager_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorRemoteLink.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORREMOTELINK_H

