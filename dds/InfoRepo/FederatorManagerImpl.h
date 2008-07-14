// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGERIMPL_H
#define FEDERATORMANAGERIMPL_H

#include "federator_export.h"
#include "FederatorS.h"
#include "FederatorConfig.h"
#include "UpdateProcessor_T.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "ace/Condition_T.h"

#include <set>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export ManagerImpl 
  : public virtual POA_OpenDDS::Federator::Manager,
    public virtual UpdateProcessor< TopicUpdate>,
    public virtual UpdateProcessor< ParticipantUpdate>,
    public virtual UpdateProcessor< SubscriptionUpdate>,
    public virtual UpdateProcessor< PublicationUpdate> {
  public:
    /// Default constructor.
    ManagerImpl( Config& config);

    /// Virtual destructor.
    virtual ~ManagerImpl();

    // IDL methods.

    virtual ::CORBA::Boolean discover_federation (
        const char * ior
      )
      ACE_THROW_SPEC ((
        ::CORBA::SystemException,
        Incomplete
      ));

    virtual ::CORBA::Boolean join_federation (
        Manager_ptr peer,
        FederationDomain federation
      )
      ACE_THROW_SPEC ((
        ::CORBA::SystemException,
        Incomplete
      ));

    virtual RepoKey federation_id (
        void
      )
      ACE_THROW_SPEC ((
        ::CORBA::SystemException
      ));

    virtual ::OpenDDS::DCPS::DCPSInfo_ptr repository (
        void
      )
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

    // Update methods.

    /// Create a proxy for a new publication.
    void processCreate( const PublicationUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new subscription.
    void processCreate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new participant.
    void processCreate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new topic.
    void processCreate( const TopicUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a publication.
    void processUpdate( const PublicationUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a subscription.
    void processUpdate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a participant.
    void processUpdate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a topic.
    void processUpdate( const TopicUpdate* sample, const ::DDS::SampleInfo* info);

    /// Delete a proxy for a publication.
    void processDelete( const PublicationUpdate* sample, const ::DDS::SampleInfo* info);

    /// Delete a proxy for a subscription.
    void processDelete( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* info);

    /// Delete a proxy for a participant.
    void processDelete( const ParticipantUpdate* sample, const ::DDS::SampleInfo* info);

    /// Delete a proxy for a topic.
    void processDelete( const TopicUpdate* sample, const ::DDS::SampleInfo* info);

  private:
    /// Map remote Id value to a local Id value.
    typedef std::map< ::OpenDDS::DCPS::RepoId, ::OpenDDS::DCPS::RepoId> RemoteToLocalMap;

    /// Map a repository federation Id value to its Id mappings.
    typedef std::map< RepoKey, RemoteToLocalMap> RepoToIdMap;

    /// Inverse mappings - Local Id value to Federation Id value.
    typedef std::map< ::OpenDDS::DCPS::RepoId, FederationId> LocalToFederationMap;

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

#endif // FEDERATORMANAGERIMPL_H

