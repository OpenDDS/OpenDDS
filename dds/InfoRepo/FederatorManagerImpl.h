/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
#include "ace/Synch_Traits.h"

#include <list>
#include <map>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

class TAO_DDS_DCPSInfo_i;

namespace OpenDDS {
namespace Federator {

class OpenDDS_Federator_Export ManagerImpl
  : public virtual POA_OpenDDS::Federator::Manager,
    public virtual Update::Updater,
    public virtual UpdateProcessor<OwnerUpdate>,
    public virtual UpdateProcessor<TopicUpdate>,
    public virtual UpdateProcessor<ParticipantUpdate>,
    public virtual UpdateProcessor<SubscriptionUpdate>,
    public virtual UpdateProcessor<PublicationUpdate> {
public:
  ManagerImpl(Config& config);

  virtual ~ManagerImpl();

  // IDL methods.

  virtual CORBA::Boolean discover_federation(
    const char * ior);

  virtual Manager_ptr join_federation(
    Manager_ptr peer,
    FederationDomain federation);

  virtual void leave_federation(
    RepoKey id);

  virtual RepoKey federation_id();

  virtual OpenDDS::DCPS::DCPSInfo_ptr repository();

  virtual void initializeOwner(
    const OpenDDS::Federator::OwnerUpdate & data);

  virtual void initializeTopic(
    const OpenDDS::Federator::TopicUpdate & data);

  virtual void initializeParticipant(
    const OpenDDS::Federator::ParticipantUpdate & data);

  virtual void initializePublication(
    const OpenDDS::Federator::PublicationUpdate & data);

  virtual void initializeSubscription(
    const OpenDDS::Federator::SubscriptionUpdate & data);

  virtual void leave_and_shutdown();

  virtual void shutdown();

  // Servant methods

  /// Establish the update publications and subscriptions.
  void initialize();

  /// Release resources gracefully.
  void finalize();

  /// Accessors for the DCPSInfo reference.
  TAO_DDS_DCPSInfo_i*& info();
  TAO_DDS_DCPSInfo_i*  info() const;

  /// Capture a remote callable reference to the DCPSInfo.
  void localRepo(::OpenDDS::DCPS::DCPSInfo_ptr repo);

  /// Accessors for the federation Id value.
//  void id(RepoKey val);
  const TAO_DDS_DCPSFederationId&  id() const;

  /// Accessors for the ORB.
  CORBA::ORB_ptr orb();
  void orb(CORBA::ORB_ptr value);

  /// Push our current state to a remote repository.
  void pushState(Manager_ptr peer);

  /// Handle any deferred updates that might have become processable.
  void processDeferred();

  //
  // Updater methods.
  //

  virtual void unregisterCallback();

  virtual void requestImage();

  virtual void create(const Update::UTopic& topic);
  virtual void create(const Update::UParticipant& participant);
  virtual void create(const Update::URActor& reader);
  virtual void create(const Update::UWActor& writer);
  virtual void create(const Update::OwnershipData& data);

  virtual void update(const Update::IdPath& id, const DDS::DomainParticipantQos& qos);
  virtual void update(const Update::IdPath& id, const DDS::TopicQos&             qos);
  virtual void update(const Update::IdPath& id, const DDS::DataWriterQos&        qos);
  virtual void update(const Update::IdPath& id, const DDS::PublisherQos&         qos);
  virtual void update(const Update::IdPath& id, const DDS::DataReaderQos&        qos);
  virtual void update(const Update::IdPath& id, const DDS::SubscriberQos&        qos);
  virtual void update(const Update::IdPath& id, const DDS::StringSeq&     exprParams);

  virtual void destroy(const Update::IdPath& id, Update::ItemType type, Update::ActorType actor);

  //
  // UpdateProcessor<> methods.
  //
  // "using" directive to fix "Hides the virtual function in virtual base" warning on
  // SunOS compiler.
  using UpdateProcessor<OwnerUpdate>::processUpdateQos2;
  using UpdateProcessor<TopicUpdate>::processUpdateQos2;
  using UpdateProcessor<ParticipantUpdate>::processUpdateQos2;
  using UpdateProcessor<OwnerUpdate>::processUpdateFilterExpressionParams;
  using UpdateProcessor<TopicUpdate>::processUpdateFilterExpressionParams;
  using UpdateProcessor<ParticipantUpdate>::processUpdateFilterExpressionParams;
  using UpdateProcessor<PublicationUpdate>::processUpdateFilterExpressionParams;

  /// Null implementation for OwnerUpdate samples.
  void processCreate(const OwnerUpdate* sample, const DDS::SampleInfo* info);

  /// Create a proxy for a new publication.
  void processCreate(const PublicationUpdate* sample, const DDS::SampleInfo* info);

  /// Create a proxy for a new subscription.
  void processCreate(const SubscriptionUpdate* sample, const DDS::SampleInfo* info);

  /// Create a proxy for a new participant.
  void processCreate(const ParticipantUpdate* sample, const DDS::SampleInfo* info);

  /// Create a proxy for a new topic.
  void processCreate(const TopicUpdate* sample, const DDS::SampleInfo* info);

  /// Process ownership changes.
  void processUpdateQos1(const OwnerUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy DataWriterQos for a publication.
  void processUpdateQos1(const PublicationUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy PublisherQos for a publication.
  void processUpdateQos2(const PublicationUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy DataReaderQos for a subscription.
  void processUpdateQos1(const SubscriptionUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy SubscriberQos for a subscription.
  void processUpdateQos2(const SubscriptionUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy filter expression params for a subscription.
  void processUpdateFilterExpressionParams(const SubscriptionUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy ParticipantQos for a participant.
  void processUpdateQos1(const ParticipantUpdate* sample, const DDS::SampleInfo* info);

  /// Update the proxy TopicQos for a topic.
  void processUpdateQos1(const TopicUpdate* sample, const DDS::SampleInfo* info);

  /// Null implementation for OwnerUpdate samples.
  void processDelete(const OwnerUpdate* sample, const DDS::SampleInfo* info);

  /// Delete a proxy for a publication.
  void processDelete(const PublicationUpdate* sample, const DDS::SampleInfo* info);

  /// Delete a proxy for a subscription.
  void processDelete(const SubscriptionUpdate* sample, const DDS::SampleInfo* info);

  /// Delete a proxy for a participant.
  void processDelete(const ParticipantUpdate* sample, const DDS::SampleInfo* info);

  /// Delete a proxy for a topic.
  void processDelete(const TopicUpdate* sample, const DDS::SampleInfo* info);

private:
  /// Critical section MUTEX.
  ACE_SYNCH_MUTEX lock_;

  /// Condition used to gate joining activities.
  ACE_Condition<ACE_SYNCH_MUTEX> joining_;

  /// Simple recursion avoidance during the join operations.
  RepoKey joiner_;

  /// Repository to which we joined.
  RepoKey joinRepo_;

  /// Flag indicating that we are actively participating in a
  /// federation of repositories.
  bool federated_;

  /// Map type to hold references to federated repository Managers.
  typedef std::map<RepoKey, Manager_var> IdToManagerMap;

  /// The peer with which we have federated.
  IdToManagerMap peers_;

  /// The packet sequence number for data that we publish.
  OpenDDS::DCPS::SequenceNumber sequence_;

  /// The configuration information for this manager.
  Config& config_;

  /// The Info object reference to update.
  TAO_DDS_DCPSInfo_i* info_;

  /// Remotely callable reference to the local repository.
  OpenDDS::DCPS::DCPSInfo_var localRepo_;

  /// The ORB in which we are activated.
  CORBA::ORB_var orb_;

  /// Multicast responder
  InfoRepoMulticastResponder multicastResponder_;

  /// local DomainParticipant
  DDS::DomainParticipant_var federationParticipant_;

  /// TopicUpdate listener
  UpdateListener<OwnerUpdate, OwnerUpdateDataReader> ownerListener_;

  /// TopicUpdate listener
  UpdateListener<TopicUpdate, TopicUpdateDataReader> topicListener_;

  /// ParticipantUpdate listener
  UpdateListener<ParticipantUpdate, ParticipantUpdateDataReader> participantListener_;

  /// PublicationUpdate listener
  UpdateListener<PublicationUpdate, PublicationUpdateDataReader> publicationListener_;

  /// SubscriptionUpdate listener
  UpdateListener<SubscriptionUpdate, SubscriptionUpdateDataReader> subscriptionListener_;

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

  /// Deferred ownership updates
  std::list<OwnerUpdate> deferredOwnerships_;

  /// Deferred topic updates
  std::list<TopicUpdate> deferredTopics_;

  /// Deferred publication updates
  std::list<PublicationUpdate> deferredPublications_;

  /// Deferred subscription updates
  std::list<SubscriptionUpdate> deferredSubscriptions_;

  /// Is multicast enabled?
  bool multicastEnabled_;

  /// Protect deferred updates.
  ACE_Thread_Mutex deferred_lock_;
};

}
} // End namespace OpenDDS::Federator

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "FederatorManagerImpl.inl"
#endif  /* __ACE_INLINE__ */

#endif /* FEDERATORMANAGERIMPL_H */
