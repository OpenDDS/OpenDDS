/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H
#define OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H

#include "EntityImpl.h"
#include "Definitions.h"
#include "TopicImpl.h"
#include "InstanceHandle.h"
#include "OwnershipManager.h"
#include "GuidBuilder.h"
#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsInfoC.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DdsDcpsInfrastructureTypeSupportC.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "dds/DCPS/transport/framework/TransportImpl_rch.h"
#include "ace/Null_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class FailoverListener;
class PublisherImpl;
class SubscriberImpl;
class DomainParticipantFactoryImpl;
class Monitor;

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
class FilterEvaluator;
#endif

/**
* @class DomainParticipantImpl
*
* @brief Implements the OpenDDS::DCPS::DomainParticipant interfaces.
*
* This class acts as an entrypoint of the service and a factory
* for publisher, subscriber and topic. It also acts as a container
* for the publisher, subscriber and topic objects.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*/
class OpenDDS_Dcps_Export DomainParticipantImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DomainParticipant>,
    public virtual OpenDDS::DCPS::EntityImpl {
public:
  typedef Objref_Servant_Pair <SubscriberImpl, DDS::Subscriber,
          DDS::Subscriber_ptr, DDS::Subscriber_var> Subscriber_Pair;

  typedef Objref_Servant_Pair <PublisherImpl, DDS::Publisher,
          DDS::Publisher_ptr, DDS::Publisher_var> Publisher_Pair;

  typedef Objref_Servant_Pair <TopicImpl, DDS::Topic,
          DDS::Topic_ptr, DDS::Topic_var> Topic_Pair;

  typedef std::set<Subscriber_Pair> SubscriberSet;
  typedef std::set<Publisher_Pair> PublisherSet;

  class OpenDDS_Dcps_Export RepoIdSequence {
  public:
    explicit RepoIdSequence(RepoId& base);
    RepoId next();
  private:
    RepoId      base_;     // will be combined with serial to produce next
    long        serial_;   // will be incremeneted each time
    GuidBuilder builder_;  // used to modify base
  };

  struct RefCounted_Topic {
    RefCounted_Topic()
      : client_refs_(0)
    {}

    explicit RefCounted_Topic(const Topic_Pair& pair)
      : pair_(pair),
        client_refs_(1)
    {}

    /// The topic object reference.
    Topic_Pair     pair_;
    /// The reference count on the obj_.
    CORBA::Long    client_refs_;
  };

  typedef std::map<std::string, RefCounted_Topic> TopicMap;

  typedef std::map<std::string, DDS::TopicDescription_var> TopicDescriptionMap;

  typedef std::map<RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan> HandleMap;
  typedef std::map<DDS::InstanceHandle_t, RepoId> RepoIdMap;

  ///Constructor
  DomainParticipantImpl(DomainParticipantFactoryImpl *       factory,
                        const DDS::DomainId_t&               domain_id,
                        const RepoId&                        dp_id,
                        const DDS::DomainParticipantQos &    qos,
                        DDS::DomainParticipantListener_ptr   a_listener,
                        const DDS::StatusMask &              mask,
                        bool                                 federated = false);

  ///Destructor
  virtual ~DomainParticipantImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual DDS::Publisher_ptr create_publisher(
    const DDS::PublisherQos & qos,
    DDS::PublisherListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::ReturnCode_t delete_publisher(
    DDS::Publisher_ptr p);

  virtual DDS::Subscriber_ptr create_subscriber(
    const DDS::SubscriberQos & qos,
    DDS::SubscriberListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::ReturnCode_t delete_subscriber(
    DDS::Subscriber_ptr s);

  virtual DDS::Subscriber_ptr get_builtin_subscriber();

  virtual DDS::Topic_ptr create_topic(
    const char * topic_name,
    const char * type_name,
    const DDS::TopicQos & qos,
    DDS::TopicListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::ReturnCode_t delete_topic(
    DDS::Topic_ptr a_topic);

  virtual DDS::Topic_ptr find_topic(
    const char * topic_name,
    const DDS::Duration_t & timeout);

  virtual DDS::TopicDescription_ptr lookup_topicdescription(
    const char * name);


#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  virtual DDS::ContentFilteredTopic_ptr create_contentfilteredtopic(
    const char * name,
    DDS::Topic_ptr related_topic,
    const char * filter_expression,
    const DDS::StringSeq & expression_parameters);

  virtual DDS::ReturnCode_t delete_contentfilteredtopic(
    DDS::ContentFilteredTopic_ptr a_contentfilteredtopic);

  virtual DDS::MultiTopic_ptr create_multitopic(
    const char * name,
    const char * type_name,
    const char * subscription_expression,
    const DDS::StringSeq & expression_parameters);

  virtual DDS::ReturnCode_t delete_multitopic(DDS::MultiTopic_ptr a_multitopic);

  RcHandle<FilterEvaluator> get_filter_eval(const char* filter);
  void deref_filter_eval(const char* filter);

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  virtual DDS::ReturnCode_t delete_contained_entities();

  virtual CORBA::Boolean contains_entity(DDS::InstanceHandle_t a_handle);

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DomainParticipantQos & qos);

  virtual DDS::ReturnCode_t get_qos(
    DDS::DomainParticipantQos & qos);

  virtual DDS::ReturnCode_t set_listener(
    DDS::DomainParticipantListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::DomainParticipantListener_ptr get_listener();

  virtual DDS::ReturnCode_t ignore_participant(
    DDS::InstanceHandle_t handle);

  virtual DDS::ReturnCode_t ignore_topic(
    DDS::InstanceHandle_t handle);

  virtual DDS::ReturnCode_t ignore_publication(
    DDS::InstanceHandle_t handle);

  virtual DDS::ReturnCode_t ignore_subscription(
    DDS::InstanceHandle_t handle);

  virtual DDS::DomainId_t get_domain_id();

  virtual DDS::ReturnCode_t assert_liveliness();

  virtual DDS::ReturnCode_t set_default_publisher_qos(
    const DDS::PublisherQos & qos);

  virtual DDS::ReturnCode_t get_default_publisher_qos(
    DDS::PublisherQos & qos);

  virtual DDS::ReturnCode_t set_default_subscriber_qos(
    const DDS::SubscriberQos & qos);

  virtual DDS::ReturnCode_t get_default_subscriber_qos(
    DDS::SubscriberQos & qos);

  virtual DDS::ReturnCode_t set_default_topic_qos(
    const DDS::TopicQos & qos);

  virtual DDS::ReturnCode_t get_default_topic_qos(
    DDS::TopicQos & qos);

  virtual DDS::ReturnCode_t get_current_time(
    DDS::Time_t & current_time);

#if !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t get_discovered_participants(
    DDS::InstanceHandleSeq & participant_handles);

  virtual DDS::ReturnCode_t get_discovered_participant_data(
    DDS::ParticipantBuiltinTopicData & participant_data,
    DDS::InstanceHandle_t participant_handle);

  virtual DDS::ReturnCode_t get_discovered_topics(
    DDS::InstanceHandleSeq & topic_handles);

  virtual DDS::ReturnCode_t get_discovered_topic_data(
    DDS::TopicBuiltinTopicData & topic_data,
    DDS::InstanceHandle_t topic_handle);

#endif

  virtual DDS::ReturnCode_t enable();

  /// Following methods are not the idl interfaces and are
  /// local operations.

  /**
  *  Return the id given by the DCPSInfo repositoy.
  */
  RepoId get_id();

  /**
   * Return a unique string based on repo ID.
   */
  std::string get_unique_id();

  /**
   * Obtain a local handle representing a GUID.
   */
  DDS::InstanceHandle_t get_handle(const RepoId& id = GUID_UNKNOWN);
  /**
   * Obtain a GUID representing a local hande.
   * @return GUID_UNKNOWN if not found.
   */
  RepoId get_repoid(const DDS::InstanceHandle_t& id);

  /**
  *  Associate the servant with the object reference.
  *  This is required to pass to the topic servant.
  */
  void set_object_reference(const DDS::DomainParticipant_ptr& dp);

  /**
  *  Check if the topic is used by any datareader or datawriter.
  */
  int is_clean() const;

  /**
  * This is used to retrieve the listener for a certain status change.
  * If this DomainParticipant has a registered listener and the status
  * kind is in the listener mask then the listener is returned.
  * Otherwise, return nil.
  */
  DDS::DomainParticipantListener* listener_for(DDS::StatusKind kind);

  typedef std::vector<RepoId> TopicIdVec;
  /**
  * Populates an std::vector with the RepoId of the topics this
  * participant has created/found.
  */
  void get_topic_ids(TopicIdVec& topics);

  /** Accessor for ownership manager.
  */
  OwnershipManager* ownership_manager();

  /**
  * Called upon receiving new BIT publication data to
  * update the ownership strength of a publication.
  */
  void update_ownership_strength(const PublicationId& pub_id,
                                 const CORBA::Long& ownership_strength);

  bool federated() const { return this->federated_; }

private:

  /** The implementation of create_topic.
  */
  DDS::Topic_ptr create_topic_i(
    const RepoId topic_id,
    const char * topic_name,
    const char * type_name,
    const DDS::TopicQos & qos,
    DDS::TopicListener_ptr a_listener,
    const DDS::StatusMask & mask);

  /** Delete the topic with option of whether the
   *  topic object reference should be removed.
   */
  DDS::ReturnCode_t delete_topic_i(
    DDS::Topic_ptr a_topic,
    bool             remove_objref);

  DomainParticipantFactoryImpl* factory_;
  /// The default topic qos.
  DDS::TopicQos        default_topic_qos_;
  /// The default publisher qos.
  DDS::PublisherQos    default_publisher_qos_;
  /// The default subscriber qos.
  DDS::SubscriberQos   default_subscriber_qos_;

  /// The qos of this DomainParticipant.
  DDS::DomainParticipantQos            qos_;
  /// Used to notify the entity for relevant events.
  DDS::DomainParticipantListener_var   listener_;
  /// The DomainParticipant listener servant.
  DDS::DomainParticipantListener*      fast_listener_;
  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask                      listener_mask_;
  /// The id of the domain that creates this participant.
  DDS::DomainId_t                      domain_id_;
  /// This participant id given by DCPSInfo repository.
  RepoId                               dp_id_;

  /// Whether this DomainParticipant is attached to a federated
  /// repository.
  bool                                 federated_;

  /// Collection of publishers.
  PublisherSet   publishers_;
  /// Collection of subscribers.
  SubscriberSet  subscribers_;
  /// Collection of topics.
  TopicMap       topics_;
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  /// Collection of TopicDescriptions which are not also Topics
  TopicDescriptionMap topic_descrs_;
#endif
  /// Bidirectional collection of handles <--> RepoIds.
  HandleMap      handles_;
  RepoIdMap      repoIds_;
  /// Collection of ignored participants.
  HandleMap      ignored_participants_;
  /// Collection of ignored topics.
  HandleMap      ignored_topics_;
  /// Protect the publisher collection.
  ACE_Recursive_Thread_Mutex   publishers_protector_;
  /// Protect the subscriber collection.
  ACE_Recursive_Thread_Mutex   subscribers_protector_;
  /// Protect the topic collection.
  ACE_Recursive_Thread_Mutex   topics_protector_;
  /// Protect the handle collection.
  ACE_Recursive_Thread_Mutex   handle_protector_;

  /// The object reference activated from this servant.
  DDS::DomainParticipant_var participant_objref_;

  /// The built in topic subscriber.
  DDS::Subscriber_var        bit_subscriber_;

  /// Listener to initiate failover with.
  FailoverListener*    failoverListener_;

  /// Instance handle generators for non-repo backed entities
  /// (i.e. subscribers and publishers).
  InstanceHandleGenerator participant_handles_;

  Monitor* monitor_;

  OwnershipManager owner_man_;

  /// Publisher ID generator.
  RepoIdSequence   pub_id_gen_;
  RepoId nextPubId();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  ACE_Thread_Mutex filter_cache_lock_;
  std::map<std::string, RcHandle<FilterEvaluator> > filter_cache_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H  */
