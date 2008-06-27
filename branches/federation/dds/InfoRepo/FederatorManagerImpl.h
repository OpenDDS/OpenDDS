// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGERIMPL_H
#define FEDERATORMANAGERIMPL_H

#include "federator_export.h"
#include "FederatorS.h"
#include "FederatorConfig.h"
#include "FederatorRemoteLink.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "ace/Condition_T.h"

#include <set>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export ManagerImpl 
  : public virtual POA_OpenDDS::Federator::Manager {
  public:
    /// Default constructor.
    ManagerImpl( Config& config);

    /// Virtual destructor.
    virtual ~ManagerImpl();

    // IDL methods.

    virtual Status join_federation ( const char * endpoint)
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      Unavailable
    ));

    virtual Status remove_connection ( RepoKey remoteId)
    ACE_THROW_SPEC ((
      ::CORBA::SystemException,
      ConnectionBusy
    ));

    virtual RepoKey federationId()
    ACE_THROW_SPEC ((
      ::CORBA::SystemException
    ));

    // Servant methods

    /// Late initialization.
    // N.B. - We need to defer intialization until after the service is
    //        initialized, which will occur long after we are constructed.
    void initialize();

    /// Accessors for the federation Id value.
    RepoKey& id();
    RepoKey  id() const;

    /// Accessors for the ORB.
    CORBA::ORB_ptr orb();
    void orb( CORBA::ORB_ptr value);

    /// Callback with new repository information.
    template< class SampleType>
    void update( SampleType& sample, ::DDS::SampleInfo& info);

  private:
    /**
     * Type mapping remote repository federation Id values to
     * information about those repositories.  Store a pointer to avoid
     * unnecessary copies in and out of the container.  Ownership is held
     * by the container.
     */
    typedef std::map< RepoKey, RemoteLink*> RemoteLinkMap;

    /// Map remote Id value to a local Id value.
    typedef std::map< ::OpenDDS::DCPS::RepoId, ::OpenDDS::DCPS::RepoId> RemoteToLocalMap;

    /// Map a repository federation Id value to its Id mappings.
    typedef std::map< RepoKey, RemoteToLocalMap> RepoToIdMap;

    /// Inverse mappings - Local Id value to Federation Id value.
    typedef std::map< ::OpenDDS::DCPS::RepoId, FederationId> LocalToFederationMap;

    /// Remove all remote repository information from out tables.
    void unfederate( RepoKey remote);

    /// Critical section MUTEX.
    ACE_SYNCH_MUTEX lock_;

    /// Simple recursion avoidance during the join operations.
    RepoKey joining_;

    /// The packet sequence number for data that we publish.
    ::OpenDDS::DCPS::SequenceNumber sequence_;

    /// The configuration information for this manager.
    Config& config_;

    /// The ORB in which we are activated.
    CORBA::ORB_var orb_;

    /// Remote repositories that are part of the Minimum Spanning Tree.
    std::set< RepoKey> mstNodes_;

    /// Retained information about remote repositories.
    RemoteLinkMap remoteLink_;

    /// Next unused transport key value.
    ::OpenDDS::DCPS::TransportIdType transportKeyValue_;

    /// local DomainParticipant
    ::DDS::DomainParticipant_var participant_;

    /// local Publisher
    ::DDS::Publisher_var publisher_;

    /// local LinkState listener.
    ::DDS::DataReader_var linkReader_;

    /// Federation Id to local Id mappings.
    RepoToIdMap inboundMap_;

    /// Local Id value to Federation Id mappings.
    LocalToFederationMap outboundMap_;

};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorManagerImpl.inl"
#endif  /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "FederatorManager_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("FederatorManager_T.cpp template inst")
#pragma implementation ("FederatorManager_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif // FEDERATORMANAGERIMPL_H

