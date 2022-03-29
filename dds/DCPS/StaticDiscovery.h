/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_STATICDISCOVERY_H
#define OPENDDS_DCPS_STATICDISCOVERY_H

#include "WaitSet.h"
#include "PoolAllocator.h"
#include "TopicDetails.h"
#include "SporadicTask.h"
#include "GuidUtils.h"
#include "Marked_Default_Qos.h"
#include "DCPS_Utils.h"
#include "BuiltInTopicDataReaderImpls.h"
#include "dcps_export.h"

#include <ace/Configuration.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class StaticDiscovery;

typedef RcHandle<StaticDiscovery> StaticDiscovery_rch;

class OpenDDS_Dcps_Export EndpointRegistry {
public:
  struct Topic {
    OPENDDS_STRING name;
    OPENDDS_STRING type_name;
  };
  typedef OPENDDS_MAP(OPENDDS_STRING, Topic) TopicMapType;
  TopicMapType topic_map;

  typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataReaderQos) DataReaderQosMapType;
  DataReaderQosMapType datareaderqos_map;

  typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataWriterQos) DataWriterQosMapType;
  DataWriterQosMapType datawriterqos_map;

  typedef OPENDDS_MAP(OPENDDS_STRING, DDS::SubscriberQos) SubscriberQosMapType;
  SubscriberQosMapType subscriberqos_map;

  typedef OPENDDS_MAP(OPENDDS_STRING, DDS::PublisherQos) PublisherQosMapType;
  PublisherQosMapType publisherqos_map;

  typedef OPENDDS_SET_CMP(RepoId, GUID_tKeyLessThan) RepoIdSetType;
  struct Reader {
    OPENDDS_STRING topic_name;
    DDS::DataReaderQos qos;
    DDS::SubscriberQos subscriber_qos;
    OPENDDS_STRING trans_cfg;
    TransportLocatorSeq trans_info;
    RepoIdSetType best_effort_writers;
    RepoIdSetType reliable_writers;
    Reader(const OPENDDS_STRING& tn,
           const DDS::DataReaderQos& q,
           const DDS::SubscriberQos& sq,
           const OPENDDS_STRING& transport_cfg,
           const TransportLocatorSeq& ti)
      : topic_name(tn)
      , qos(q)
      , subscriber_qos(sq)
      , trans_cfg(transport_cfg)
      , trans_info(ti)
    {}
  };
  typedef OPENDDS_MAP_CMP(RepoId, Reader, GUID_tKeyLessThan) ReaderMapType;
  ReaderMapType reader_map;

  struct Writer {
    OPENDDS_STRING topic_name;
    DDS::DataWriterQos qos;
    DDS::PublisherQos publisher_qos;
    OPENDDS_STRING trans_cfg;
    TransportLocatorSeq trans_info;
    RepoIdSetType best_effort_readers;
    RepoIdSetType reliable_readers;
    Writer(const OPENDDS_STRING& tn,
           const DDS::DataWriterQos& q,
           const DDS::PublisherQos& pq,
           const OPENDDS_STRING& transport_cfg,
           const TransportLocatorSeq& ti)
      : topic_name(tn)
      , qos(q)
      , publisher_qos(pq)
      , trans_cfg(transport_cfg)
      , trans_info(ti)
    {}
  };
  typedef OPENDDS_MAP_CMP(RepoId, Writer, GUID_tKeyLessThan) WriterMapType;
  WriterMapType writer_map;

  struct StaticDiscGuidDomainEqual {

    bool
    operator() (const GuidPrefix_t& lhs, const GuidPrefix_t& rhs) const
    {
      return std::memcmp(&lhs[2], &rhs[2], sizeof(DDS::DomainId_t)) == 0;
    }
  };
  struct StaticDiscGuidPartEqual {

    bool
    operator() (const GuidPrefix_t& lhs, const GuidPrefix_t& rhs) const
    {
      return std::memcmp(&lhs[6], &rhs[6], 6) == 0;
    }
  };

  void match();

  static EntityId_t build_id(const unsigned char* entity_key /* length of 3 */,
                             const unsigned char entity_kind);

  static RepoId build_id(DDS::DomainId_t domain,
                         const unsigned char* participant_id /* length of 6 */,
                         const EntityId_t& entity_id);
};

class StaticParticipant;
class StaticEndpointManager
  : public RcEventHandler
  , public DiscoveryListener
{
protected:
  struct DiscoveredSubscription : PoolAllocationBase {
    DiscoveredSubscription()
      : bit_ih_(DDS::HANDLE_NIL)
      , participant_discovered_at_(monotonic_time_zero())
      , transport_context_(0)
    {
    }

    explicit DiscoveredSubscription(const DiscoveredReaderData& r)
      : reader_data_(r)
      , bit_ih_(DDS::HANDLE_NIL)
      , participant_discovered_at_(monotonic_time_zero())
      , transport_context_(0)
    {
    }

    RepoIdSet matched_endpoints_;
    DiscoveredReaderData reader_data_;
    DDS::InstanceHandle_t bit_ih_;
    MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    XTypes::TypeInformation type_info_;

    const char* get_topic_name() const
    {
      return reader_data_.ddsSubscriptionData.topic_name;
    }

    const char* get_type_name() const
    {
      return reader_data_.ddsSubscriptionData.type_name;
    }
  };

  typedef OPENDDS_MAP_CMP(GUID_t, DiscoveredSubscription,
                          GUID_tKeyLessThan) DiscoveredSubscriptionMap;

  typedef DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;

  struct DiscoveredPublication : PoolAllocationBase {
    DiscoveredPublication()
    : bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(monotonic_time_zero())
    , transport_context_(0)
    {
    }

    explicit DiscoveredPublication(const DiscoveredWriterData& w)
    : writer_data_(w)
    , bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(monotonic_time_zero())
    , transport_context_(0)
    {
    }

    RepoIdSet matched_endpoints_;
    DiscoveredWriterData writer_data_;
    DDS::InstanceHandle_t bit_ih_;
    MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    XTypes::TypeInformation type_info_;

    const char* get_topic_name() const
    {
      return writer_data_.ddsPublicationData.topic_name;
    }

    const char* get_type_name() const
    {
      return writer_data_.ddsPublicationData.type_name;
    }
  };

  typedef OPENDDS_MAP_CMP(GUID_t, DiscoveredPublication,
                          GUID_tKeyLessThan) DiscoveredPublicationMap;
  typedef DiscoveredPublicationMap::iterator DiscoveredPublicationIter;

  struct LocalEndpoint {
    LocalEndpoint()
      : topic_id_(GUID_UNKNOWN)
      , participant_discovered_at_(monotonic_time_zero())
      , transport_context_(0)
      , sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
    {
    }

    GUID_t topic_id_;
    TransportLocatorSeq trans_info_;
    MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    RepoIdSet matched_endpoints_;
    SequenceNumber sequence_;
    RepoIdSet remote_expectant_opendds_associations_;
    XTypes::TypeInformation type_info_;
  };

  struct LocalPublication : LocalEndpoint {
    DataWriterCallbacks_wrch publication_;
    DDS::DataWriterQos qos_;
    DDS::PublisherQos publisher_qos_;
  };

  struct LocalSubscription : LocalEndpoint {
    DataReaderCallbacks_wrch subscription_;
    DDS::DataReaderQos qos_;
    DDS::SubscriberQos subscriber_qos_;
    ContentFilterProperty_t filterProperties;
  };

  typedef OPENDDS_MAP_CMP(GUID_t, LocalPublication,
                          GUID_tKeyLessThan) LocalPublicationMap;
  typedef LocalPublicationMap::iterator LocalPublicationIter;
  typedef LocalPublicationMap::const_iterator LocalPublicationCIter;

  typedef OPENDDS_MAP_CMP(GUID_t, LocalSubscription,
                          GUID_tKeyLessThan) LocalSubscriptionMap;
  typedef LocalSubscriptionMap::iterator LocalSubscriptionIter;
  typedef LocalSubscriptionMap::const_iterator LocalSubscriptionCIter;

  typedef OPENDDS_MAP_CMP(GUID_t, String, GUID_tKeyLessThan) TopicNameMap;

  static DDS::BuiltinTopicKey_t get_key(const DiscoveredPublication& pub)
  {
    return pub.writer_data_.ddsPublicationData.key;
  }
  static DDS::BuiltinTopicKey_t get_key(const DiscoveredSubscription& sub)
  {
    return sub.reader_data_.ddsSubscriptionData.key;
  }

  virtual void remove_from_bit_i(const DiscoveredPublication& /*pub*/) { }
  virtual void remove_from_bit_i(const DiscoveredSubscription& /*sub*/) { }

  virtual DDS::ReturnCode_t write_publication_data(const GUID_t& /*rid*/,
                                                   LocalPublication& /*pub*/,
                                                   const GUID_t& reader = GUID_UNKNOWN)
  {
    ACE_UNUSED_ARG(reader);
    return DDS::RETCODE_OK;
  }

  virtual DDS::ReturnCode_t write_subscription_data(const GUID_t& /*rid*/,
                                                    LocalSubscription& /*pub*/,
                                                    const GUID_t& reader = GUID_UNKNOWN)
  {
    ACE_UNUSED_ARG(reader);
    return DDS::RETCODE_OK;
  }

  virtual bool send_type_lookup_request(const XTypes::TypeIdentifierSeq& /*type_ids*/,
                                        const GUID_t& /*endpoint*/,
                                        bool /*is_discovery_protected*/,
                                        bool /*send_get_types*/)
  {
    return true;
  }

public:
  StaticEndpointManager(const RepoId& participant_id,
                        ACE_Thread_Mutex& lock,
                        const EndpointRegistry& registry,
                        StaticParticipant& participant);

  ~StaticEndpointManager();

  void init_bit();

  virtual void assign_publication_key(RepoId& rid,
                                      const RepoId& topicId,
                                      const DDS::DataWriterQos& qos);
  virtual void assign_subscription_key(RepoId& rid,
                                       const RepoId& topicId,
                                       const DDS::DataReaderQos& qos);

  virtual bool update_topic_qos(const RepoId& /*topicId*/,
                                const DDS::TopicQos& /*qos*/);

  virtual bool update_publication_qos(const RepoId& /*publicationId*/,
                                      const DDS::DataWriterQos& /*qos*/,
                                      const DDS::PublisherQos& /*publisherQos*/);

  virtual bool update_subscription_qos(const RepoId& /*subscriptionId*/,
                                       const DDS::DataReaderQos& /*qos*/,
                                       const DDS::SubscriberQos& /*subscriberQos*/);

  virtual bool update_subscription_params(const RepoId& /*subId*/,
                                          const DDS::StringSeq& /*params*/);

  virtual bool disassociate();

  virtual DDS::ReturnCode_t add_publication_i(const RepoId& /*rid*/,
                                              LocalPublication& /*pub*/);

  virtual DDS::ReturnCode_t remove_publication_i(const RepoId& /*publicationId*/,
                                                 LocalPublication& /*pub*/);

  virtual DDS::ReturnCode_t add_subscription_i(const RepoId& /*rid*/,
                                               LocalSubscription& /*pub*/);

  virtual DDS::ReturnCode_t remove_subscription_i(const RepoId& /*subscriptionId*/,
                                                  LocalSubscription& /*sub*/);

  virtual bool is_expectant_opendds(const GUID_t& endpoint) const;

  virtual bool shutting_down() const;

  virtual void populate_transport_locator_sequence(TransportLocatorSeq*& /*tls*/,
                                                   DiscoveredSubscriptionIter& /*iter*/,
                                                   const RepoId& /*reader*/);

  virtual void populate_transport_locator_sequence(TransportLocatorSeq*& /*tls*/,
                                                   DiscoveredPublicationIter& /*iter*/,
                                                   const RepoId& /*reader*/);

  virtual void reader_exists(const RepoId& readerid, const RepoId& writerid);
  virtual void reader_does_not_exist(const RepoId& readerid, const RepoId& writerid);
  virtual void writer_exists(const RepoId& writerid, const RepoId& readerid);
  virtual void writer_does_not_exist(const RepoId& writerid, const RepoId& readerid);
  void cleanup_type_lookup_data(const GuidPrefix_t& prefix,
                                const XTypes::TypeIdentifier& ti,
                                bool secure);

#ifndef DDS_HAS_MINIMUM_BIT
  PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  void type_lookup_init(ReactorInterceptor_rch reactor_interceptor);
  void type_lookup_fini();
  void type_lookup_service(const XTypes::TypeLookupService_rch type_lookup_service);

  void purge_dead_topic(const String& topic_name);

  void ignore(const GUID_t& to_ignore);
  bool ignoring(const GUID_t& guid) const;
  bool ignoring(const char* topic_name) const;

  TopicStatus assert_topic(GUID_t& topicId, const char* topicName,
                           const char* dataTypeName, const DDS::TopicQos& qos,
                           bool hasDcpsKey, TopicCallbacks* topic_callbacks);
  TopicStatus find_topic(const char* topicName,
                         CORBA::String_out dataTypeName,
                         DDS::TopicQos_out qos,
                         GUID_t& topicId);
  TopicStatus remove_topic(const GUID_t& topicId);


  GUID_t add_publication(const GUID_t& topicId,
                         DataWriterCallbacks_rch publication,
                         const DDS::DataWriterQos& qos,
                         const TransportLocatorSeq& transInfo,
                         const DDS::PublisherQos& publisherQos,
                         const XTypes::TypeInformation& type_info);
  void remove_publication(const GUID_t& publicationId);
  void update_publication_locators(const GUID_t& publicationId,
                                   const TransportLocatorSeq& transInfo);

  GUID_t add_subscription(const GUID_t& topicId,
                          DataReaderCallbacks_rch subscription,
                          const DDS::DataReaderQos& qos,
                          const TransportLocatorSeq& transInfo,
                          const DDS::SubscriberQos& subscriberQos,
                          const char* filterClassName,
                          const char* filterExpr,
                          const DDS::StringSeq& params,
                          const XTypes::TypeInformation& type_info);
  void remove_subscription(const GUID_t& subscriptionId);
  void update_subscription_locators(const GUID_t& subscriptionId,
                                    const TransportLocatorSeq& transInfo);

  void match_endpoints(GUID_t repoId, const TopicDetails& td,
                       bool remove = false);

  void remove_assoc(const GUID_t& remove_from, const GUID_t& removing);

  virtual void add_assoc_i(const GUID_t& /* local_guid */, const LocalPublication& /* lpub */,
                           const GUID_t& /* remote_guid */, const DiscoveredSubscription& /* dsub */) {}
  virtual void remove_assoc_i(const GUID_t& /* local_guid */, const LocalPublication& /* lpub */,
                              const GUID_t& /* remote_guid */) {}
  virtual void add_assoc_i(const GUID_t& /* local_guid */, const LocalSubscription& /* lsub */,
                           const GUID_t& /* remote_guid */, const DiscoveredPublication& /* dpub */) {}
  virtual void remove_assoc_i(const GUID_t& /* local_guid */, const LocalSubscription& /* lsub */,
                              const GUID_t& /* remote_guid */) {}

private:
  struct MatchingData {
    MatchingData()
      : got_minimal(false), got_complete(false)
    {}

    /// Sequence number of the first request for remote minimal types.
    SequenceNumber rpc_seqnum_minimal;

    /// Whether all minimal types are obtained.
    bool got_minimal;

    /// Sequence number of the first request for remote complete types.
    /// Set to SEQUENCENUMBER_UNKNOWN if there is no such request.
    SequenceNumber rpc_seqnum_complete;

    /// Whether all complete types are obtained.
    bool got_complete;

    MonotonicTimePoint time_added_to_map;
  };

  struct MatchingPair {
    MatchingPair(GUID_t writer, GUID_t reader)
      : writer_(writer), reader_(reader) {}

    GUID_t writer_;
    GUID_t reader_;

    bool operator<(const MatchingPair& a_other) const
    {
      if (GUID_tKeyLessThan()(writer_, a_other.writer_)) return true;

      if (GUID_tKeyLessThan()(a_other.writer_, writer_)) return false;

      if (GUID_tKeyLessThan()(reader_, a_other.reader_)) return true;

      if (GUID_tKeyLessThan()(a_other.reader_, reader_)) return false;

      return false;
    }
  };
  typedef OPENDDS_MAP(MatchingPair, MatchingData) MatchingDataMap;
  typedef MatchingDataMap::iterator MatchingDataIter;

  void match(const GUID_t& writer, const GUID_t& reader);
  void need_minimal_and_or_complete_types(const XTypes::TypeInformation* type_info,
                                          bool& need_minimal,
                                          bool& need_complete) const;
  void remove_expired_endpoints(const MonotonicTimePoint& /*now*/);
  void match_continue(const GUID_t& writer, const GUID_t& reader);

  void remove_from_bit(const DiscoveredPublication& pub)
  {
    remove_from_bit_i(pub);
  }

  void remove_from_bit(const DiscoveredSubscription& sub)
  {
    remove_from_bit_i(sub);
  }

  GUID_t make_topic_guid();

  bool has_dcps_key(const GUID_t& topicId) const;

  ACE_Thread_Mutex& lock_;
  GUID_t participant_id_;
  RepoIdSet ignored_guids_;
  unsigned int topic_counter_;
  LocalPublicationMap local_publications_;
  LocalSubscriptionMap local_subscriptions_;
  DiscoveredPublicationMap discovered_publications_;
  DiscoveredSubscriptionMap discovered_subscriptions_;
  TopicDetailsMap topics_;
  TopicNameMap topic_names_;
  OPENDDS_SET(OPENDDS_STRING) ignored_topics_;
  OPENDDS_SET_CMP(GUID_t, GUID_tKeyLessThan) relay_only_readers_;
  const EndpointRegistry& registry_;
#ifndef DDS_HAS_MINIMUM_BIT
  StaticParticipant& participant_;
#endif

  XTypes::TypeLookupService_rch type_lookup_service_;
  MatchingDataMap matching_data_buffer_;
  typedef PmfSporadicTask<StaticEndpointManager> StaticEndpointManagerSporadic;
  RcHandle<StaticEndpointManagerSporadic> type_lookup_reply_deadline_processor_;
  TimeDuration max_type_lookup_service_reply_period_;
  SequenceNumber type_lookup_service_sequence_number_;

  struct TypeIdOrigSeqNumber {
    GuidPrefix_t participant; // Prefix of remote participant
    XTypes::TypeIdentifier type_id; // Remote type
    SequenceNumber seq_number; // Of the original request
    bool secure; // Communicate via secure endpoints or not
    MonotonicTimePoint time_started;
  };

  // Map from the sequence number of the most recent request for a type to its TypeIdentifier
  // and the sequence number of the first request sent for that type. Every time a new request
  // is sent for a type, a new entry must be stored.
  typedef OPENDDS_MAP(SequenceNumber, TypeIdOrigSeqNumber) OrigSeqNumberMap;
  OrigSeqNumberMap orig_seq_numbers_;
};

class StaticParticipant : public RcObject {
public:
  StaticParticipant(RepoId& guid,
                    const DDS::DomainParticipantQos& qos,
                    const EndpointRegistry& registry)
    : qos_(qos)
    , endpoint_manager_(make_rch<StaticEndpointManager>(guid, ref(lock_), ref(registry), ref(*this)))
  {}

  void init_bit(const DDS::Subscriber_var& bit_subscriber)
  {
    bit_subscriber_ = bit_subscriber;
    endpoint_manager_->init_bit();
  }

  void fini_bit()
  {
    bit_subscriber_ = 0;
  }

  void shutdown() {}

  DDS::Subscriber_ptr init_bit(DomainParticipantImpl* participant);

  void fini_bit(DCPS::DomainParticipantImpl* participant);

  bool attach_participant(DDS::DomainId_t domainId, const GUID_t& participantId);

  bool remove_domain_participant(DDS::DomainId_t domain_id, const GUID_t& participantId);

  bool ignore_domain_participant(DDS::DomainId_t domain, const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_domain_participant_qos(DDS::DomainId_t domain, const GUID_t& participant,
    const DDS::DomainParticipantQos& qos);

  DCPS::TopicStatus assert_topic(
    GUID_t& topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    DCPS::TopicCallbacks* topic_callbacks);

  DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId);

  DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId);

  bool ignore_topic(DDS::DomainId_t domainId,
    const GUID_t& myParticipantId, const GUID_t& ignoreId);

  bool update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
    const GUID_t& participantId, const DDS::TopicQos& qos);

  GUID_t add_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info);

  bool remove_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& publicationId);

  bool ignore_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_publication_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);

  void update_publication_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DCPS::TransportLocatorSeq& transInfo);

  GUID_t add_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const XTypes::TypeInformation& type_info);

  bool remove_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& subscriptionId);

  bool ignore_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subQos);

  bool update_subscription_params(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DDS::StringSeq& params);

  void update_subscription_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DCPS::TransportLocatorSeq& transInfo);

  void ignore_domain_participant(const GUID_t& ignoreId)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    endpoint_manager().ignore(ignoreId);

    DiscoveredParticipantIter iter = participants_.find(ignoreId);
    if (iter != participants_.end()) {
      remove_discovered_participant(iter);
    }
  }

  virtual bool announce_domain_participant_qos()
  {
    return true;
  }

  bool update_domain_participant_qos(const DDS::DomainParticipantQos& qos)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
    qos_ = qos;
    return announce_domain_participant_qos();
  }

  TopicStatus assert_topic(
    GUID_t& topicId, const char* topicName,
    const char* dataTypeName, const DDS::TopicQos& qos,
    bool hasDcpsKey, TopicCallbacks* topic_callbacks)
  {
    if (std::strlen(topicName) > 256 || std::strlen(dataTypeName) > 256) {
      if (DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR LocalParticipant::assert_topic() - ")
                   ACE_TEXT("topic or type name length limit (256) exceeded\n")));
      }
      return PRECONDITION_NOT_MET;
    }

    return endpoint_manager().assert_topic(topicId, topicName, dataTypeName, qos, hasDcpsKey, topic_callbacks);
  }

  TopicStatus find_topic(
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId)
  {
    return endpoint_manager().find_topic(topicName, dataTypeName, qos, topicId);
  }

  TopicStatus remove_topic(const GUID_t& topicId)
  {
    return endpoint_manager().remove_topic(topicId);
  }

  void ignore_topic(const GUID_t& ignoreId)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    endpoint_manager().ignore(ignoreId);
  }

  bool update_topic_qos(const GUID_t& topicId, const DDS::TopicQos& qos)
  {
    return endpoint_manager().update_topic_qos(topicId, qos);
  }

  GUID_t add_publication(
    const GUID_t& topicId,
    DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info)
  {
    return endpoint_manager().add_publication(topicId, publication, qos,
                                              transInfo, publisherQos, type_info);
  }

  void remove_publication(const GUID_t& publicationId)
  {
    endpoint_manager().remove_publication(publicationId);
  }

  void ignore_publication(const GUID_t& ignoreId)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    return endpoint_manager().ignore(ignoreId);
  }

  bool update_publication_qos(
    const GUID_t& publicationId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos)
  {
    return endpoint_manager().update_publication_qos(publicationId, qos, publisherQos);
  }

  void update_publication_locators(const GUID_t& publicationId,
                                   const TransportLocatorSeq& transInfo)
  {
    endpoint_manager().update_publication_locators(publicationId, transInfo);
  }

  GUID_t add_subscription(
    const GUID_t& topicId,
    DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const XTypes::TypeInformation& type_info)
  {
    return endpoint_manager().add_subscription(topicId, subscription, qos, transInfo,
      subscriberQos, filterClassName, filterExpr, params, type_info);
  }

  void remove_subscription(const GUID_t& subscriptionId)
  {
    endpoint_manager().remove_subscription(subscriptionId);
  }

  void ignore_subscription(const GUID_t& ignoreId)
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    return endpoint_manager().ignore(ignoreId);
  }

  bool update_subscription_qos(
    const GUID_t& subscriptionId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subscriberQos)
  {
    return endpoint_manager().update_subscription_qos(subscriptionId, qos, subscriberQos);
  }

  bool update_subscription_params(const GUID_t& subId, const DDS::StringSeq& params)
  {
    return endpoint_manager().update_subscription_params(subId, params);
  }

  void update_subscription_locators(const GUID_t& subId, const TransportLocatorSeq& transInfo)
  {
    endpoint_manager().update_subscription_locators(subId, transInfo);
  }

  DDS::Subscriber_var bit_subscriber() const
  {
    return bit_subscriber_;
  }

  void type_lookup_service(const XTypes::TypeLookupService_rch type_lookup_service)
  {
    endpoint_manager().type_lookup_service(type_lookup_service);
  }

private:
  struct DiscoveredParticipant {

    DiscoveredParticipant()
    : location_ih_(DDS::HANDLE_NIL)
    , bit_ih_(DDS::HANDLE_NIL)
    , seq_reset_count_(0)
    {
    }

    struct LocationUpdate {
      ParticipantLocation mask_;
      ACE_INET_Addr from_;
      SystemTimePoint timestamp_;
      LocationUpdate() {}
      LocationUpdate(ParticipantLocation mask,
                     const ACE_INET_Addr& from,
                     const SystemTimePoint& timestamp)
        : mask_(mask), from_(from), timestamp_(timestamp) {}
    };
    typedef OPENDDS_VECTOR(LocationUpdate) LocationUpdateList;
    LocationUpdateList location_updates_;
    ParticipantLocationBuiltinTopicData location_data_;
    DDS::InstanceHandle_t location_ih_;

    ACE_INET_Addr local_address_;
    MonotonicTimePoint discovered_at_;
    MonotonicTimePoint lease_expiration_;
    DDS::InstanceHandle_t bit_ih_;
    SequenceNumber max_seq_;
    ACE_UINT16 seq_reset_count_;
  };

  typedef OPENDDS_MAP_CMP(GUID_t, DiscoveredParticipant,
                          GUID_tKeyLessThan) DiscoveredParticipantMap;
  typedef DiscoveredParticipantMap::iterator DiscoveredParticipantIter;
  typedef DiscoveredParticipantMap::const_iterator
    DiscoveredParticipantConstIter;

  void remove_discovered_participant(DiscoveredParticipantIter& iter);

  virtual void remove_discovered_participant_i(DiscoveredParticipantIter&) {}

#ifndef DDS_HAS_MINIMUM_BIT
  ParticipantBuiltinTopicDataDataReaderImpl* part_bit()
  {
    DDS::Subscriber_var bit_sub(bit_subscriber());
    if (!bit_sub.in())
      return 0;

    DDS::DataReader_var d =
      bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
    return dynamic_cast<ParticipantBuiltinTopicDataDataReaderImpl*>(d.in());
  }

  ParticipantLocationBuiltinTopicDataDataReaderImpl* part_loc_bit()
  {
    DDS::Subscriber_var bit_sub(bit_subscriber());
    if (!bit_sub.in())
      return 0;

    DDS::DataReader_var d =
      bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_LOCATION_TOPIC);
    return dynamic_cast<ParticipantLocationBuiltinTopicDataDataReaderImpl*>(d.in());
  }

  ConnectionRecordDataReaderImpl* connection_record_bit()
  {
    DDS::Subscriber_var bit_sub(bit_subscriber());
    if (!bit_sub.in())
      return 0;

    DDS::DataReader_var d =
      bit_sub->lookup_datareader(BUILT_IN_CONNECTION_RECORD_TOPIC);
    return dynamic_cast<ConnectionRecordDataReaderImpl*>(d.in());
  }

  InternalThreadBuiltinTopicDataDataReaderImpl* internal_thread_bit()
  {
    DDS::Subscriber_var bit_sub(bit_subscriber());
    if (!bit_sub.in())
      return 0;

    DDS::DataReader_var d =
      bit_sub->lookup_datareader(BUILT_IN_INTERNAL_THREAD_TOPIC);
    return dynamic_cast<InternalThreadBuiltinTopicDataDataReaderImpl*>(d.in());
  }
#endif /* DDS_HAS_MINIMUM_BIT */

  StaticEndpointManager& endpoint_manager() { return *endpoint_manager_; }

  mutable ACE_Thread_Mutex lock_;
  DDS::Subscriber_var bit_subscriber_;
  DDS::DomainParticipantQos qos_;
  DiscoveredParticipantMap participants_;

  RcHandle<StaticEndpointManager> endpoint_manager_;
};

class OpenDDS_Dcps_Export StaticDiscovery : public Discovery {
public:
  explicit StaticDiscovery(const RepoKey& key);

  int load_configuration(ACE_Configuration_Heap& config);

  virtual GUID_t generate_participant_guid();

  virtual AddDomainStatus add_domain_participant(DDS::DomainId_t domain,
                                                 const DDS::DomainParticipantQos& qos,
                                                 XTypes::TypeLookupService_rch tls);

#if defined(OPENDDS_SECURITY)
  virtual AddDomainStatus add_domain_participant_secure(
    DDS::DomainId_t domain,
    const DDS::DomainParticipantQos& qos,
    XTypes::TypeLookupService_rch tls,
    const GUID_t& guid,
    DDS::Security::IdentityHandle id,
    DDS::Security::PermissionsHandle perm,
    DDS::Security::ParticipantCryptoHandle part_crypto);
#endif

  EndpointRegistry registry;

  static StaticDiscovery_rch instance() { return instance_; }

  DDS::Subscriber_ptr init_bit(DCPS::DomainParticipantImpl* participant);

  void fini_bit(DCPS::DomainParticipantImpl* participant);

  bool attach_participant(DDS::DomainId_t domainId, const GUID_t& participantId);

  bool remove_domain_participant(DDS::DomainId_t domain_id, const GUID_t& participantId);

  bool ignore_domain_participant(DDS::DomainId_t domain, const GUID_t& myParticipantId,
    const GUID_t& ignoreId);

  bool update_domain_participant_qos(DDS::DomainId_t domain, const GUID_t& participant,
    const DDS::DomainParticipantQos& qos);

  DCPS::TopicStatus assert_topic(
    GUID_t& topicId,
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    const char* dataTypeName,
    const DDS::TopicQos& qos,
    bool hasDcpsKey,
    DCPS::TopicCallbacks* topic_callbacks);

  DCPS::TopicStatus find_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId);

  DCPS::TopicStatus remove_topic(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId);

  bool ignore_topic(DDS::DomainId_t domainId,
    const GUID_t& myParticipantId, const GUID_t& ignoreId);

  bool update_topic_qos(const GUID_t& topicId, DDS::DomainId_t domainId,
    const GUID_t& participantId, const DDS::TopicQos& qos);

  GUID_t add_publication(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info);

  bool remove_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& publicationId);

  bool ignore_publication(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_publication_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DDS::DataWriterQos& qos,
    const DDS::PublisherQos& publisherQos);

  void update_publication_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& dwId,
    const DCPS::TransportLocatorSeq& transInfo);

  GUID_t add_subscription(
    DDS::DomainId_t domainId,
    const GUID_t& participantId,
    const GUID_t& topicId,
    DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const XTypes::TypeInformation& type_info);

  bool remove_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& subscriptionId);

  bool ignore_subscription(DDS::DomainId_t domainId, const GUID_t& participantId,
    const GUID_t& ignoreId);

  bool update_subscription_qos(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& drId,
    const DDS::DataReaderQos& qos,
    const DDS::SubscriberQos& subQos);

  bool update_subscription_params(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DDS::StringSeq& params);

  void update_subscription_locators(
    DDS::DomainId_t domainId,
    const GUID_t& partId,
    const GUID_t& subId,
    const DCPS::TransportLocatorSeq& transInfo);

private:
  typedef RcHandle<StaticParticipant> ParticipantHandle;
  typedef OPENDDS_MAP_CMP(GUID_t, ParticipantHandle, GUID_tKeyLessThan) ParticipantMap;
  typedef OPENDDS_MAP(DDS::DomainId_t, ParticipantMap) DomainParticipantMap;

  ParticipantHandle get_part(const DDS::DomainId_t domain_id, const GUID_t& part_id) const;

  void create_bit_dr(DDS::TopicDescription_ptr topic, const char* type,
                     SubscriberImpl* sub,
                     const DDS::DataReaderQos& qos);

  int parse_topics(ACE_Configuration_Heap& cf);
  int parse_datawriterqos(ACE_Configuration_Heap& cf);
  int parse_datareaderqos(ACE_Configuration_Heap& cf);
  int parse_publisherqos(ACE_Configuration_Heap& cf);
  int parse_subscriberqos(ACE_Configuration_Heap& cf);
  int parse_endpoints(ACE_Configuration_Heap& cf);

  void pre_writer(DataWriterImpl* writer);
  void pre_reader(DataReaderImpl* reader);

  static StaticDiscovery_rch instance_;

  mutable ACE_Thread_Mutex lock_;

  DomainParticipantMap participants_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_STATICDISCOVERY_STATICDISCOVERY_H */
