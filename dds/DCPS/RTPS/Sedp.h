/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SEDP_H
#define OPENDDS_RTPS_SEDP_H
#ifndef DDS_HAS_MINIMUM_BIT

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/DataSampleHeader.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#include "ace/Task_Ex_T.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Thread_Mutex.h"
#include "dds/DCPS/PoolAllocator.h"
#include <map>
#include <set>


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace DDS {
  class TopicBuiltinTopicDataDataReaderImpl;
  class PublicationBuiltinTopicDataDataReaderImpl;
  class SubscriptionBuiltinTopicDataDataReaderImpl;
}

namespace OpenDDS {
namespace RTPS {

enum RtpsFlags { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4 };

class RtpsDiscovery;
class Spdp;

typedef std::set<DCPS::RepoId, DCPS::GUID_tKeyLessThan> RepoIdSet;

class WaitForAcks;

class Sedp {
public:
  Sedp(const DCPS::RepoId& participant_id,
       Spdp& owner,
       ACE_Thread_Mutex& lock);

  DDS::ReturnCode_t init(const DCPS::RepoId& guid,
                         const RtpsDiscovery& disco,
                         DDS::DomainId_t domainId);

  /// request for acknowledgement from all Sedp threads (Task)
  void acknowledge();

  void shutdown();

  // @brief return the ip address we have bound to.
  // Valid after init() call
  const ACE_INET_Addr& local_address() const;
  const ACE_INET_Addr& multicast_group() const;

  void ignore(const DCPS::RepoId& to_ignore);

  bool ignoring(const DCPS::RepoId& guid) const {
    return ignored_guids_.count(guid);
  }
  bool ignoring(const char* topic_name) const {
    return ignored_topics_.count(topic_name);
  }

  DCPS::RepoId bit_key_to_repo_id(const char* bit_topic_name,
                                  const DDS::BuiltinTopicKey_t& key);

  void associate(const SPDPdiscoveredParticipantData& pdata);
  bool disassociate(const SPDPdiscoveredParticipantData& pdata);

  // Topic
  DCPS::TopicStatus assert_topic(DCPS::RepoId_out topicId,
                                 const char* topicName,
                                 const char* dataTypeName,
                                 const DDS::TopicQos& qos,
                                 bool hasDcpsKey);
  DCPS::TopicStatus remove_topic(const DCPS::RepoId& topicId,
                                 OPENDDS_STRING& name);
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        OPENDDS_STRING& name);
  struct TopicDetails {
    OPENDDS_STRING data_type_;
    DDS::TopicQos qos_;
    DCPS::RepoId repo_id_;
  };

  // Publication
  DCPS::RepoId add_publication(const DCPS::RepoId& topicId,
                               DCPS::DataWriterCallbacks* publication,
                               const DDS::DataWriterQos& qos,
                               const DCPS::TransportLocatorSeq& transInfo,
                               const DDS::PublisherQos& publisherQos);
  void remove_publication(const DCPS::RepoId& publicationId);
  bool update_publication_qos(const DCPS::RepoId& publicationId,
                              const DDS::DataWriterQos& qos,
                              const DDS::PublisherQos& publisherQos);

  // Subscription
  DCPS::RepoId add_subscription(const DCPS::RepoId& topicId,
                                DCPS::DataReaderCallbacks* subscription,
                                const DDS::DataReaderQos& qos,
                                const DCPS::TransportLocatorSeq& transInfo,
                                const DDS::SubscriberQos& subscriberQos,
                                const char* filterExpr,
                                const DDS::StringSeq& params);
  void remove_subscription(const DCPS::RepoId& subscriptionId);
  bool update_subscription_qos(const DCPS::RepoId& subscriptionId,
                               const DDS::DataReaderQos& qos,
                               const DDS::SubscriberQos& subscriberQos);
  bool update_subscription_params(const DCPS::RepoId& subId,
                                  const DDS::StringSeq& params);

  // Managing reader/writer associations
  void association_complete(const DCPS::RepoId& localId,
                            const DCPS::RepoId& remoteId);

  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  static const bool host_is_bigendian_;
#ifndef ACE_LYNXOS_MAJOR
private:
#endif
  DCPS::RepoId participant_id_;
  Spdp& spdp_;
  ACE_Thread_Mutex& lock_;

  struct Msg {
    enum MsgType { MSG_PARTICIPANT, MSG_WRITER, MSG_READER, MSG_PARTICIPANT_DATA,
                   MSG_REMOVE_FROM_PUB_BIT, MSG_REMOVE_FROM_SUB_BIT,
                   MSG_FINI_BIT, MSG_STOP } type_;
    DCPS::MessageId id_;
    const void* payload_;
    Msg(MsgType mt, DCPS::MessageId id, const void* p)
      : type_(mt), id_(id), payload_(p) {}
  };

  class Endpoint : public DCPS::TransportClient {
  public:
    Endpoint(const DCPS::RepoId& repo_id, Sedp& sedp)
      : repo_id_(repo_id)
      , sedp_(sedp)
    {}

    virtual ~Endpoint();

    // Implementing TransportClient
    bool check_transport_qos(const DCPS::TransportInst&)
      { return true; }
    const DCPS::RepoId& get_repo_id() const
      { return repo_id_; }
    DDS::DomainId_t domain_id() const
      { return 0; } // not used for SEDP
    CORBA::Long get_priority_value(const DCPS::AssociationData&) const
      { return 0; }

    using DCPS::TransportClient::enable_transport_using_config;
    using DCPS::TransportClient::disassociate;

  protected:
    DCPS::RepoId repo_id_;
    Sedp& sedp_;
  };

  class Writer : public DCPS::TransportSendListener, public Endpoint {
  public:
    Writer(const DCPS::RepoId& pub_id, Sedp& sedp);
    virtual ~Writer();

    bool assoc(const DCPS::AssociationData& subscription);

    // Implementing TransportSendListener
    void data_delivered(const DCPS::DataSampleElement*);

    void data_dropped(const DCPS::DataSampleElement*, bool by_transport);

    void control_delivered(ACE_Message_Block* sample);

    void control_dropped(ACE_Message_Block* sample,
                         bool dropped_by_transport);

    void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
    void notify_connection_deleted(const DCPS::RepoId&) {}
    void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
    void retrieve_inline_qos_data(InlineQosData&) const {}

    DDS::ReturnCode_t write_sample(const ParameterList& plist,
                                   const DCPS::RepoId& reader,
                                   DCPS::SequenceNumber& sequence);
    DDS::ReturnCode_t write_sample(const ParticipantMessageData& pmd,
                                   const DCPS::RepoId& reader,
                                   DCPS::SequenceNumber& sequence);
    DDS::ReturnCode_t write_unregister_dispose(const DCPS::RepoId& rid);

    void end_historic_samples(const DCPS::RepoId& reader);

  private:
    DCPS::TransportSendElementAllocator alloc_;
    Header header_;
    DCPS::SequenceNumber seq_;

    void write_control_msg(ACE_Message_Block& payload,
                           size_t size,
                           DCPS::MessageId id,
                           DCPS::SequenceNumber seq = DCPS::SequenceNumber());

    void set_header_fields(DCPS::DataSampleHeader& dsh,
                           size_t size,
                           const DCPS::RepoId& reader,
                           DCPS::SequenceNumber& sequence,
                           DCPS::MessageId id = DCPS::SAMPLE_DATA);

  } publications_writer_, subscriptions_writer_, participant_message_writer_;

  class Reader
    : public DCPS::TransportReceiveListener
    , public Endpoint
    , public DCPS::RcObject<ACE_SYNCH_MUTEX>
  {
  public:
    Reader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Endpoint(sub_id, sedp)
      , shutting_down_(0)
    {}

    virtual ~Reader();

    bool assoc(const DCPS::AssociationData& publication);

    // Implementing TransportReceiveListener

    void data_received(const DCPS::ReceivedDataSample& sample);

    void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
    void notify_connection_deleted(const DCPS::RepoId&) {}
    void remove_associations(const DCPS::WriterIdSeq&, bool) {}

    void listener_add_ref() { _add_ref(); }
    void listener_remove_ref() { _remove_ref(); }

    ACE_Atomic_Op<ACE_SYNCH_MUTEX, long> shutting_down_;
  };

  typedef DCPS::RcHandle<Reader> Reader_rch;

  Reader_rch publications_reader_, subscriptions_reader_, participant_message_reader_;

  struct Task : ACE_Task_Ex<ACE_MT_SYNCH, Msg> {
    explicit Task(Sedp* sedp)
      : spdp_(&sedp->spdp_)
      , sedp_(sedp)
      , shutting_down_(false)
    {
      activate();
    }
    ~Task();

    void enqueue(const SPDPdiscoveredParticipantData* pdata);
    void enqueue(DCPS::MessageId id, const DiscoveredWriterData* wdata);
    void enqueue(DCPS::MessageId id, const DiscoveredReaderData* rdata);
    void enqueue(DCPS::MessageId id, const ParticipantMessageData* data);
    void enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t* bit_ih);

    void acknowledge();
    void shutdown();


  private:
    int svc();

    void svc_i(const SPDPdiscoveredParticipantData* pdata);
    void svc_i(DCPS::MessageId id, const DiscoveredWriterData* wdata);
    void svc_i(DCPS::MessageId id, const DiscoveredReaderData* rdata);
    void svc_i(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t* bit_ih);

    Spdp* spdp_;
    Sedp* sedp_;
    bool shutting_down_;
  } task_;

  // Transport
  DCPS::TransportInst_rch transport_inst_;

  DDS::TopicBuiltinTopicDataDataReaderImpl* topic_bit();
  DDS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  DDS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();

  struct LocalEndpoint {
    LocalEndpoint() : topic_id_(DCPS::GUID_UNKNOWN), sequence_(DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {}
    DCPS::RepoId topic_id_;
    DCPS::TransportLocatorSeq trans_info_;
    RepoIdSet matched_endpoints_;
    DCPS::SequenceNumber sequence_;
    RepoIdSet remote_opendds_associations_;
  };

  struct LocalPublication : LocalEndpoint {
    DCPS::DataWriterCallbacks* publication_;
    DDS::DataWriterQos qos_;
    DDS::PublisherQos publisher_qos_;
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalPublication,
                   DCPS::GUID_tKeyLessThan) LocalPublicationMap;
  typedef LocalPublicationMap::iterator LocalPublicationIter;
  typedef LocalPublicationMap::const_iterator LocalPublicationCIter;
  LocalPublicationMap local_publications_;

  void populate_discovered_writer_msg(
      DiscoveredWriterData& dwd,
      const DCPS::RepoId& publication_id,
      const LocalPublication& pub);

  struct LocalSubscription : LocalEndpoint {
    DCPS::DataReaderCallbacks* subscription_;
    DDS::DataReaderQos qos_;
    DDS::SubscriberQos subscriber_qos_;
    OPENDDS_STRING filter_;
    DDS::StringSeq params_;
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalSubscription,
                   DCPS::GUID_tKeyLessThan) LocalSubscriptionMap;
  typedef LocalSubscriptionMap::iterator LocalSubscriptionIter;
  typedef LocalSubscriptionMap::const_iterator LocalSubscriptionCIter;
  LocalSubscriptionMap local_subscriptions_;

  void populate_discovered_reader_msg(
      DiscoveredReaderData& drd,
      const DCPS::RepoId& subscription_id,
      const LocalSubscription& sub);

  struct LocalParticipantMessage : LocalEndpoint {
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalParticipantMessage,
    DCPS::GUID_tKeyLessThan) LocalParticipantMessageMap;
  typedef LocalParticipantMessageMap::iterator LocalParticipantMessageIter;
  typedef LocalParticipantMessageMap::const_iterator LocalParticipantMessageCIter;
  LocalParticipantMessageMap local_participant_messages_;

  unsigned int publication_counter_, subscription_counter_;

  void data_received(DCPS::MessageId message_id,
                     const DiscoveredWriterData& wdata);
  void data_received(DCPS::MessageId message_id,
                     const DiscoveredReaderData& rdata);
  void data_received(DCPS::MessageId message_id,
                     const ParticipantMessageData& data);

  struct DiscoveredPublication {
    DiscoveredPublication() : bit_ih_(DDS::HANDLE_NIL) {}
    explicit DiscoveredPublication(const DiscoveredWriterData& w)
      : writer_data_(w), bit_ih_(DDS::HANDLE_NIL) {}
    DiscoveredWriterData writer_data_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredPublication,
                   DCPS::GUID_tKeyLessThan) DiscoveredPublicationMap;
  typedef DiscoveredPublicationMap::iterator DiscoveredPublicationIter;
  DiscoveredPublicationMap discovered_publications_;
  typedef std::pair<DCPS::MessageId, DiscoveredWriterData> MsgIdWtrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdWtrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredPublicationMap;
  DeferredPublicationMap deferred_publications_;  // Publications that Spdp has not discovered.

  struct DiscoveredSubscription {
    DiscoveredSubscription() : bit_ih_(DDS::HANDLE_NIL) {}
    explicit DiscoveredSubscription(const DiscoveredReaderData& r)
      : reader_data_(r), bit_ih_(DDS::HANDLE_NIL) {}
    DiscoveredReaderData reader_data_;
    DDS::InstanceHandle_t bit_ih_;
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, DiscoveredSubscription,
                   DCPS::GUID_tKeyLessThan) DiscoveredSubscriptionMap;
  typedef DiscoveredSubscriptionMap::iterator DiscoveredSubscriptionIter;
  DiscoveredSubscriptionMap discovered_subscriptions_;
  typedef std::pair<DCPS::MessageId, DiscoveredReaderData> MsgIdRdrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdRdrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredSubscriptionMap;
  DeferredSubscriptionMap deferred_subscriptions_; // Subscriptions that Sedp has not discovered.

  void assign_bit_key(DiscoveredPublication& pub);
  void assign_bit_key(DiscoveredSubscription& sub);
  void increment_key(DDS::BuiltinTopicKey_t& key);

  DDS::BuiltinTopicKey_t pub_bit_key_, sub_bit_key_;
  typedef OPENDDS_MAP_CMP(DDS::BuiltinTopicKey_t, DCPS::RepoId,
                   DCPS::BuiltinTopicKeyLess) BitKeyMap;
  BitKeyMap pub_key_to_id_, sub_key_to_id_;

  template<typename Map>
  void remove_entities_belonging_to(Map& m, DCPS::RepoId participant);

  void remove_from_bit(const DiscoveredPublication& pub);
  void remove_from_bit(const DiscoveredSubscription& sub);

  const char* get_topic_name(const DiscoveredPublication& pub) {
    return pub.writer_data_.ddsPublicationData.topic_name;
  }
  const char* get_topic_name(const DiscoveredSubscription& sub) {
    return sub.reader_data_.ddsSubscriptionData.topic_name;
  }

  RepoIdSet ignored_guids_;
  std::set<OPENDDS_STRING> ignored_topics_;

  // Topic:
  struct TopicDetailsEx : TopicDetails {
    bool has_dcps_key_;
    RepoIdSet endpoints_;
  };
  OPENDDS_MAP(OPENDDS_STRING, TopicDetailsEx) topics_;
  OPENDDS_MAP_CMP(DCPS::RepoId, OPENDDS_STRING, DCPS::GUID_tKeyLessThan) topic_names_;
  unsigned int topic_counter_;

  RepoIdSet defer_match_endpoints_, associated_participants_;

  void inconsistent_topic(const RepoIdSet& endpoints) const;
  bool has_dcps_key(const DCPS::RepoId& topicId) const;
  DCPS::RepoId make_topic_guid();

  void match_endpoints(DCPS::RepoId repoId, const TopicDetailsEx& td,
                       bool remove = false);
  void match(const DCPS::RepoId& writer, const DCPS::RepoId& reader);
  void remove_assoc(const DCPS::RepoId& remove_from,
                    const DCPS::RepoId& removing);

  static DCPS::RepoId make_id(const DCPS::RepoId& participant_id,
                              const EntityId_t& entity);

  static void set_inline_qos(DCPS::TransportLocatorSeq& locators);

  void write_durable_publication_data(const DCPS::RepoId& reader);
  void write_durable_subscription_data(const DCPS::RepoId& reader);
  void write_durable_participant_message_data(const DCPS::RepoId& reader);

  static bool is_opendds(const GUID_t& endpoint);

  DDS::ReturnCode_t write_publication_data(const DCPS::RepoId& rid,
                                           LocalPublication& pub,
                                           const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
  DDS::ReturnCode_t write_subscription_data(const DCPS::RepoId& rid,
                                            LocalSubscription& pub,
                                            const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
  DDS::ReturnCode_t write_participant_message_data(const DCPS::RepoId& rid,
                                                   LocalParticipantMessage& part,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DCPS::SequenceNumber automatic_liveliness_seq_;
  DCPS::SequenceNumber manual_liveliness_seq_;
};

/// A class to wait on acknowledgments from other threads
class WaitForAcks {
public:
  WaitForAcks();
  void ack();
  void wait_for_acks(unsigned int num_acks);
  void reset();
private:
  ACE_Thread_Mutex lock_;
  ACE_Condition_Thread_Mutex cond_;
  unsigned int acks_;
};

}
}

#endif // DDS_HAS_MINIMUM_BIT
#endif // OPENDDS_RTPS_SEDP_H
