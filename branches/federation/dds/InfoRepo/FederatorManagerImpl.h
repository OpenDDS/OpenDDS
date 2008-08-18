// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORMANAGERIMPL_H
#define FEDERATORMANAGERIMPL_H

#include "federator_export.h"
#include "FederatorS.h"
#include "FederatorTypeSupportC.h"
#include "FederatorConfig.h"
#include "InfoRepoMulticastResponder.h"
#include "UpdateProcessor_T.h"
#include "UpdateListener_T.h"
#include "Updater.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "ace/Condition_T.h"

#include <set>

class TAO_DDS_DCPSInfo_i;

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export ManagerImpl 
  : public virtual POA_OpenDDS::Federator::Manager,
    public virtual ::Updater,
    public virtual UpdateProcessor< OwnerUpdate>,
    public virtual UpdateProcessor< TopicUpdate>,
    public virtual UpdateProcessor< ParticipantUpdate>,
    public virtual UpdateProcessor< SubscriptionUpdate>,
    public virtual UpdateProcessor< PublicationUpdate> {
  public:
    /// Default constructor.
    ManagerImpl(Config& config);

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

    /// Establish the update subscriptions.
    void initialize();

    /// Tear down the update subscriptions.
    void finalize();

    /// Accessors for the DCPSInfo reference.
    TAO_DDS_DCPSInfo_i*& info();
    TAO_DDS_DCPSInfo_i*  info() const;

    /// Accessors for the federation Id value.
    RepoKey& id();
    RepoKey  id() const;

    /// Accessors for the ORB.
    CORBA::ORB_ptr orb();
    void orb( CORBA::ORB_ptr value);

    //
    // Updater methods.
    //

    virtual void unregisterCallback();

    virtual void requestImage();

    virtual void add( const UpdateManager::UTopic& topic);

    virtual void add( const UpdateManager::UParticipant& participant);

    virtual void add( const UpdateManager::URActor& reader);

    virtual void add( const UpdateManager::UWActor& writer);

    virtual void add(
                   const long                    domain,
                   const ::OpenDDS::DCPS::GUID_t participant,
                   const long                    owner
                 );

    virtual void remove( ItemType type, const IdType& id);

    virtual void updateQos(
                   const ItemType& itemType,
                   const IdType&   id,
                   const QosSeq&   qos
                 );

    //
    // UpdateProcessor<> methods.
    //

    /// Null implementation for OwnerUpdate samples.
    void processCreate( const OwnerUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new publication.
    void processCreate( const PublicationUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new subscription.
    void processCreate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new participant.
    void processCreate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* info);

    /// Create a proxy for a new topic.
    void processCreate( const TopicUpdate* sample, const ::DDS::SampleInfo* info);

    /// Process ownership changes.
    void processUpdate( const OwnerUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a publication.
    void processUpdate( const PublicationUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a subscription.
    void processUpdate( const SubscriptionUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a participant.
    void processUpdate( const ParticipantUpdate* sample, const ::DDS::SampleInfo* info);

    /// Update the proxy for a topic.
    void processUpdate( const TopicUpdate* sample, const ::DDS::SampleInfo* info);

    /// Null implementation for OwnerUpdate samples.
    void processDelete( const OwnerUpdate* sample, const ::DDS::SampleInfo* info);

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

    /// Condition used to gate joining activities.
    ACE_Condition<ACE_SYNCH_MUTEX> joining_;

    /// Simple recursion avoidance during the join operations.
    RepoKey joiner_;

    /// The packet sequence number for data that we publish.
    ::OpenDDS::DCPS::SequenceNumber sequence_;

    /// The configuration information for this manager.
    Config& config_;

    /// The Info object reference to update.
    TAO_DDS_DCPSInfo_i* info_;

    /// The ORB in which we are activated.
    CORBA::ORB_var orb_;

    /// Multicast responder
    InfoRepoMulticastResponder multicastResponder_;

    /// local DomainParticipant
    ::DDS::DomainParticipant_var federationParticipant_;

    /// TopicUpdate listener
    UpdateListener< OwnerUpdate, OwnerUpdateDataReader> ownerListener_;

    /// TopicUpdate listener
    UpdateListener< TopicUpdate, TopicUpdateDataReader> topicListener_;

    /// ParticipantUpdate listener
    UpdateListener< ParticipantUpdate, ParticipantUpdateDataReader> participantListener_;

    /// PublicationUpdate listener
    UpdateListener< PublicationUpdate, PublicationUpdateDataReader> publicationListener_;

    /// SubscriptionUpdate listener
    UpdateListener< SubscriptionUpdate, SubscriptionUpdateDataReader> subscriptionListener_;

    /// TopicUpdate writer
    OwnerUpdateDataWriter_var ownerWriter_;

    /// TopicUpdate writer
    TopicUpdateDataWriter_var topicWriter_;

    /// ParticipantUpdate writer
    ParticipantUpdateDataWriter_var participantWriter_;

    /// PublicationUpdate writer
    PublicationUpdateDataWriter_var publicationWriter_;

    /// SubscriptionUpdate writer
    SubscriptionUpdateDataWriter_var subscriptionWriter_;

    /// Federation Id to local Id mappings.
    RepoToIdMap inboundMap_;

    /// Local Id value to Federation Id mappings.
    LocalToFederationMap outboundMap_;

    /// Is multicast enabled?
    bool multicastEnabled_;
};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorManagerImpl.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORMANAGERIMPL_H

