/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H
#define OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H

#include "dds/DdsDcpsPublicationC.h"
#include "dds/DdsDcpsSubscriptionExtC.h"
#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (DDS_HAS_MINIMUM_BIT)
#include "dds/DdsDcpsCoreTypeSupportC.h"
#endif // !defined (DDS_HAS_MINIMUM_BIT)

#include "EntityImpl.h"
#include "Definitions.h"
#include "TopicImpl.h"
#include "InstanceHandle.h"
#include "OwnershipManager.h"
#include "GuidBuilder.h"

#include "dds/DCPS/transport/framework/TransportImpl_rch.h"

#include "dds/DCPS/PoolAllocator.h"

#include "Recorder.h"
#include "Replayer.h"

#include "dds/DCPS/security/framework/SecurityConfig_rch.h"

#include "ace/Null_Mutex.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PublisherImpl;
class SubscriberImpl;
class DataWriterImpl;
class DomainParticipantFactoryImpl;
class Monitor;


class RecorderImpl;
class ReplayerImpl;

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
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DomainParticipant>
  , public virtual OpenDDS::DCPS::EntityImpl
  , public virtual ACE_Event_Handler
{
public:
  typedef Objref_Servant_Pair <SubscriberImpl, DDS::Subscriber,
                               DDS::Subscriber_ptr, DDS::Subscriber_var> Subscriber_Pair;

  typedef Objref_Servant_Pair <PublisherImpl, DDS::Publisher,
                               DDS::Publisher_ptr, DDS::Publisher_var> Publisher_Pair;

  typedef Objref_Servant_Pair <TopicImpl, DDS::Topic,
                               DDS::Topic_ptr, DDS::Topic_var> Topic_Pair;

  typedef OPENDDS_SET(Subscriber_Pair) SubscriberSet;
  typedef OPENDDS_SET(Publisher_Pair) PublisherSet;

  class OpenDDS_Dcps_Export RepoIdSequence {
  public:
    explicit RepoIdSequence(const RepoId& base);
    RepoId next();
  private:
    RepoId base_;          // will be combined with serial to produce next
    long serial_;          // will be incremented each time
    GuidBuilder builder_;  // used to modify base
  };

  struct RefCounted_Topic {
    RefCounted_Topic()
      : client_refs_(0)
    {
    }

    explicit RefCounted_Topic(const Topic_Pair& pair)
      : pair_(pair),
      client_refs_(1)
    {
    }

    /// The topic object reference.
    Topic_Pair pair_;
    /// The reference count on the obj_.
    CORBA::ULong client_refs_;
  };

  typedef OPENDDS_MAP(OPENDDS_STRING, RefCounted_Topic) TopicMap;

  typedef OPENDDS_MAP(OPENDDS_STRING, DDS::TopicDescription_var) TopicDescriptionMap;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::InstanceHandle_t, GUID_tKeyLessThan) HandleMap;
  typedef OPENDDS_MAP(DDS::InstanceHandle_t, RepoId) RepoIdMap;

  DomainParticipantImpl(DomainParticipantFactoryImpl *     factory,
                        const DDS::DomainId_t&             domain_id,
                        const DDS::DomainParticipantQos &  qos,
                        DDS::DomainParticipantListener_ptr a_listener,
                        const DDS::StatusMask &            mask);

  virtual ~DomainParticipantImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual DDS::Publisher_ptr create_publisher(
    const DDS::PublisherQos &  qos,
    DDS::PublisherListener_ptr a_listener,
    DDS::StatusMask            mask);

  virtual DDS::ReturnCode_t delete_publisher(
    DDS::Publisher_ptr p);

  virtual DDS::Subscriber_ptr create_subscriber(
    const DDS::SubscriberQos &  qos,
    DDS::SubscriberListener_ptr a_listener,
    DDS::StatusMask             mask);

  virtual DDS::ReturnCode_t delete_subscriber(
    DDS::Subscriber_ptr s);

  virtual DDS::Subscriber_ptr get_builtin_subscriber();

  virtual DDS::Topic_ptr create_topic(
    const char *           topic_name,
    const char *           type_name,
    const DDS::TopicQos &  qos,
    DDS::TopicListener_ptr a_listener,
    DDS::StatusMask        mask);

  virtual DDS::ReturnCode_t delete_topic(
    DDS::Topic_ptr a_topic);

  virtual DDS::Topic_ptr find_topic(
    const char *            topic_name,
    const DDS::Duration_t & timeout);

  virtual DDS::TopicDescription_ptr lookup_topicdescription(
    const char * name);

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC

  virtual DDS::ContentFilteredTopic_ptr create_contentfilteredtopic(
    const char *           name,
    DDS::Topic_ptr         related_topic,
    const char *           filter_expression,
    const DDS::StringSeq & expression_parameters);

  virtual DDS::ReturnCode_t delete_contentfilteredtopic(
    DDS::ContentFilteredTopic_ptr a_contentfilteredtopic);

#endif

#ifndef OPENDDS_NO_MULTI_TOPIC

  virtual DDS::MultiTopic_ptr create_multitopic(
    const char *           name,
    const char *           type_name,
    const char *           subscription_expression,
    const DDS::StringSeq & expression_parameters);

  virtual DDS::ReturnCode_t delete_multitopic(DDS::MultiTopic_ptr a_multitopic);

#endif

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  RcHandle<FilterEvaluator> get_filter_eval(const char* filter);
  void deref_filter_eval(const char* filter);

#endif

  virtual DDS::ReturnCode_t delete_contained_entities();

  virtual CORBA::Boolean contains_entity(DDS::InstanceHandle_t a_handle);

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DomainParticipantQos & qos);

  virtual DDS::ReturnCode_t get_qos(
    DDS::DomainParticipantQos & qos);

  virtual DDS::ReturnCode_t set_listener(
    DDS::DomainParticipantListener_ptr a_listener,
    DDS::StatusMask                    mask);

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

  /**
   * Set Argument to Current System Time
   */
  virtual DDS::ReturnCode_t get_current_time(DDS::Time_t& current_time);

#if !defined (DDS_HAS_MINIMUM_BIT)

  virtual DDS::ReturnCode_t get_discovered_participants(
    DDS::InstanceHandleSeq & participant_handles);

  virtual DDS::ReturnCode_t get_discovered_participant_data(
    DDS::ParticipantBuiltinTopicData & participant_data,
    DDS::InstanceHandle_t              participant_handle);

  virtual DDS::ReturnCode_t get_discovered_topics(
    DDS::InstanceHandleSeq & topic_handles);

  virtual DDS::ReturnCode_t get_discovered_topic_data(
    DDS::TopicBuiltinTopicData & topic_data,
    DDS::InstanceHandle_t        topic_handle);

#endif

  virtual DDS::ReturnCode_t enable();

  /// Following methods are not the idl interfaces and are
  /// local operations.

  /**
   *  Return the id given by discovery.
   */
  RepoId get_id();

  /**
   * Return a unique string based on repo ID.
   */
  OPENDDS_STRING get_unique_id();

  /**
   * Obtain a local handle representing a GUID.
   */
  DDS::InstanceHandle_t id_to_handle(const RepoId& id);

  /**
   * Obtain a GUID representing a local hande.
   * @return GUID_UNKNOWN if not found.
   */
  RepoId get_repoid(const DDS::InstanceHandle_t& id);

  /**
   *  Check if the topic is used by any datareader or datawriter.
   */
  bool is_clean() const;

  /**
   * This is used to retrieve the listener for a certain status change.
   * If this DomainParticipant has a registered listener and the status
   * kind is in the listener mask then the listener is returned.
   * Otherwise, return nil.
   */
  DDS::DomainParticipantListener_ptr listener_for(DDS::StatusKind kind);

  typedef OPENDDS_VECTOR(RepoId) TopicIdVec;
  /**
   * Populates an std::vector with the RepoId of the topics this
   * participant has created/found.
   */
  void get_topic_ids(TopicIdVec& topics);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE

  /** Accessor for ownership manager.
   */
  OwnershipManager* ownership_manager();


  /**
   * Called upon receiving new BIT publication data to
   * update the ownership strength of a publication.
   */
  void update_ownership_strength(const PublicationId& pub_id,
                                 const CORBA::Long&   ownership_strength);

#endif

  bool federated() const {
    return this->federated_;
  }


  Recorder_ptr create_recorder(DDS::Topic_ptr               a_topic,
                               const DDS::SubscriberQos &   subscriber_qos,
                               const DDS::DataReaderQos &   datareader_qos,
                               const RecorderListener_rch & a_listener,
                               DDS::StatusMask              mask);

  Replayer_ptr create_replayer(DDS::Topic_ptr               a_topic,
                               const DDS::PublisherQos &    publisher_qos,
                               const DDS::DataWriterQos &   datawriter_qos,
                               const ReplayerListener_rch & a_listener,
                               DDS::StatusMask              mask);

  DDS::Topic_ptr create_typeless_topic(
    const char *           topic_name,
    const char *           type_name,
    bool                   type_has_keys,
    const DDS::TopicQos &  qos,
    DDS::TopicListener_ptr a_listener,
    DDS::StatusMask        mask);

  void delete_recorder(Recorder_ptr recorder);
  void delete_replayer(Replayer_ptr replayer);

  void add_adjust_liveliness_timers(DataWriterImpl* writer);
  void remove_adjust_liveliness_timers();

#if defined(OPENDDS_SECURITY)
  void set_security_config(const Security::SecurityConfig_rch& config);

  DDS::Security::ParticipantCryptoHandle crypto_handle() const
  {
    return part_crypto_handle_;
  }
#endif

private:

  bool validate_publisher_qos(DDS::PublisherQos & publisher_qos);
  bool validate_subscriber_qos(DDS::SubscriberQos & subscriber_qos);

  /** The implementation of create_topic.
   */

  enum {
    TOPIC_TYPE_HAS_KEYS =1,
    TOPIC_TYPELESS = 2
  } TopicTypeMask;

  DDS::Topic_ptr create_topic_i(
    const char *           topic_name,
    const char *           type_name,
    const DDS::TopicQos &  qos,
    DDS::TopicListener_ptr a_listener,
    DDS::StatusMask        mask,
    int                    topic_mask);

  DDS::Topic_ptr create_new_topic(
    const char *                   topic_name,
    const char *                   type_name,
    const DDS::TopicQos &          qos,
    DDS::TopicListener_ptr         a_listener,
    const DDS::StatusMask &        mask,
    OpenDDS::DCPS::TypeSupport_ptr type_support);

  /** Delete the topic with option of whether the
   *  topic object reference should be removed.
   */
  DDS::ReturnCode_t delete_topic_i(
    DDS::Topic_ptr a_topic,
    bool           remove_objref);

  DomainParticipantFactoryImpl* factory_;
  /// The default topic qos.
  DDS::TopicQos default_topic_qos_;
  /// The default publisher qos.
  DDS::PublisherQos default_publisher_qos_;
  /// The default subscriber qos.
  DDS::SubscriberQos default_subscriber_qos_;

  /// The qos of this DomainParticipant.
  DDS::DomainParticipantQos qos_;
  /// Used to notify the entity for relevant events.
  DDS::DomainParticipantListener_var listener_;
  /// The StatusKind bit mask indicates which status condition change
  /// can be notified by the listener of this entity.
  DDS::StatusMask listener_mask_;

  #if defined(OPENDDS_SECURITY)
  /// This participant id handle given by authentication.
  DDS::Security::IdentityHandle id_handle_;
  /// This participant permissions handle given by access constrol.
  DDS::Security::PermissionsHandle perm_handle_;
  /// This participant crypto handle given by crypto
  DDS::Security::ParticipantCryptoHandle part_crypto_handle_;
  #endif

  /// The id of the domain that creates this participant.
  const DDS::DomainId_t domain_id_;
  /// This participant id given by discovery.
  RepoId dp_id_;

  /// Whether this DomainParticipant is attached to a federated
  /// repository.
  bool federated_;

  /// Collection of publishers.
  PublisherSet publishers_;
  /// Collection of subscribers.
  SubscriberSet subscribers_;
  /// Collection of topics.
  TopicMap topics_;
#if !defined(OPENDDS_NO_CONTENT_FILTERED_TOPIC) || !defined(OPENDDS_NO_MULTI_TOPIC)
  /// Collection of TopicDescriptions which are not also Topics
  TopicDescriptionMap topic_descrs_;
#endif
  /// Bidirectional collection of handles <--> RepoIds.
  HandleMap handles_;
  RepoIdMap repoIds_;
  /// Collection of ignored participants.
  HandleMap ignored_participants_;
  /// Collection of ignored topics.
  HandleMap ignored_topics_;
  /// Protect the publisher collection.
  ACE_Recursive_Thread_Mutex publishers_protector_;
  /// Protect the subscriber collection.
  ACE_Recursive_Thread_Mutex subscribers_protector_;
  /// Protect the topic collection.
  ACE_Recursive_Thread_Mutex topics_protector_;
  /// Protect the handle collection.
  ACE_Recursive_Thread_Mutex handle_protector_;
  /// Protect the shutdown.
  ACE_Thread_Mutex shutdown_mutex_;
  ACE_Condition<ACE_Thread_Mutex> shutdown_condition_;
  DDS::ReturnCode_t shutdown_result_;
  bool shutdown_complete_;

  /// The built in topic subscriber.
  DDS::Subscriber_var bit_subscriber_;

  /// Instance handle generators for non-repo backed entities
  /// (i.e. subscribers and publishers).
  InstanceHandleGenerator participant_handles_;

  unique_ptr<Monitor> monitor_;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  OwnershipManager owner_man_;
#endif

  /// Publisher ID generator.
  RepoIdSequence pub_id_gen_;
  RepoId nextPubId();

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  ACE_Thread_Mutex filter_cache_lock_;
  OPENDDS_MAP(OPENDDS_STRING, RcHandle<FilterEvaluator> ) filter_cache_;
#endif

  typedef OPENDDS_SET_CMP(Recorder_var, VarLess<Recorder> ) RecorderSet;
  typedef OPENDDS_SET_CMP(Replayer_var, VarLess<Replayer> ) ReplayerSet;

  RecorderSet recorders_;
  ReplayerSet replayers_;

#if defined(OPENDDS_SECURITY)
  Security::SecurityConfig_rch security_config_;
#endif

  /// Protect the recorders collection.
  ACE_Recursive_Thread_Mutex recorders_protector_;
  /// Protect the replayers collection.
  ACE_Recursive_Thread_Mutex replayers_protector_;

  class LivelinessTimer : public ACE_Event_Handler {
  public:
    LivelinessTimer(DomainParticipantImpl& impl, DDS::LivelinessQosPolicyKind kind);
    virtual ~LivelinessTimer();
    void add_adjust(OpenDDS::DCPS::DataWriterImpl* writer);
    void remove_adjust();
    int handle_timeout(const ACE_Time_Value &tv, const void * /* arg */);
    virtual void dispatch(const MonotonicTimePoint& tv) = 0;

  protected:
    DomainParticipantImpl& impl_;
    const DDS::LivelinessQosPolicyKind kind_;

    TimeDuration interval () const { return interval_; }

  private:
    TimeDuration interval_;
    bool recalculate_interval_;
    MonotonicTimePoint last_liveliness_check_;
    bool scheduled_;
    ACE_Thread_Mutex lock_;
  };

  class AutomaticLivelinessTimer : public LivelinessTimer {
  public:
    AutomaticLivelinessTimer(DomainParticipantImpl& impl);
    virtual void dispatch(const MonotonicTimePoint& tv);
  };
  AutomaticLivelinessTimer automatic_liveliness_timer_;

  class ParticipantLivelinessTimer : public LivelinessTimer {
  public:
    ParticipantLivelinessTimer(DomainParticipantImpl& impl);
    virtual void dispatch(const MonotonicTimePoint& tv);
  };
  ParticipantLivelinessTimer participant_liveliness_timer_;

  TimeDuration liveliness_check_interval(DDS::LivelinessQosPolicyKind kind);
  bool participant_liveliness_activity_after(const MonotonicTimePoint& tv);
  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  MonotonicTimePoint last_liveliness_activity_;

  virtual int handle_exception(ACE_HANDLE fd);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DOMAIN_PARTICIPANT_IMPL_H  */
