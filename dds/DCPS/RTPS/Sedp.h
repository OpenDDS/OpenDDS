/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SEDP_H
#define OPENDDS_RTPS_SEDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/DiscoveryBase.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#include "ace/Task_Ex_T.h"
#include "ace/Thread_Mutex.h"
#include "ace/Condition_Thread_Mutex.h"
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

enum RtpsFlags { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4 };

class RtpsDiscovery;
class Spdp;

class WaitForAcks;

class Sedp : public OpenDDS::DCPS::EndpointManager<SPDPdiscoveredParticipantData> {
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
  void unicast_locators(OpenDDS::DCPS::LocatorSeq& locators) const;

  // @brief return the ip address we have bound to.
  // Valid after init() call
  const ACE_INET_Addr& local_address() const;
  const ACE_INET_Addr& multicast_group() const;
  bool map_ipv4_to_ipv6() const;

  void associate(const SPDPdiscoveredParticipantData& pdata);
  bool disassociate(const SPDPdiscoveredParticipantData& pdata);

  // Topic
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos,
                        OPENDDS_STRING& name);

  // Publication
  bool update_publication_qos(const DCPS::RepoId& publicationId,
                              const DDS::DataWriterQos& qos,
                              const DDS::PublisherQos& publisherQos);

  // Subscription
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
private:
  Spdp& spdp_;

  struct Msg : public OpenDDS::DCPS::PoolAllocationBase {
    enum MsgType { MSG_PARTICIPANT, MSG_WRITER, MSG_READER, MSG_PARTICIPANT_DATA,
                   MSG_REMOVE_FROM_PUB_BIT, MSG_REMOVE_FROM_SUB_BIT,
                   MSG_FINI_BIT, MSG_STOP } type_;
    DCPS::MessageId id_;
    union {
      const SPDPdiscoveredParticipantData* dpdata_;
      const OpenDDS::DCPS::DiscoveredWriterData* wdata_;
      const OpenDDS::DCPS::DiscoveredReaderData* rdata_;
      const ParticipantMessageData* pmdata_;
      DDS::InstanceHandle_t ih_;
    };
    Msg(MsgType mt, DCPS::MessageId id, const SPDPdiscoveredParticipantData* dpdata)
      : type_(mt), id_(id), dpdata_(dpdata) {}
    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata)
      : type_(mt), id_(id), wdata_(wdata) {}
    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata)
      : type_(mt), id_(id), rdata_(rdata) {}
    Msg(MsgType mt, DCPS::MessageId id, const ParticipantMessageData* pmdata)
      : type_(mt), id_(id), pmdata_(pmdata) {}
    Msg(MsgType mt, DCPS::MessageId id, DDS::InstanceHandle_t ih)
      : type_(mt), id_(id), ih_(ih) {}
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

    void _add_ref() {}
    void _remove_ref() {}

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

    virtual void _add_ref() { DCPS::RcObject<ACE_SYNCH_MUTEX>::_add_ref(); }
    virtual void _remove_ref() { DCPS::RcObject<ACE_SYNCH_MUTEX>::_remove_ref(); }

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
    void enqueue(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata);
    void enqueue(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata);
    void enqueue(DCPS::MessageId id, const ParticipantMessageData* data);
    void enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

    void acknowledge();
    void shutdown();


  private:
    int svc();

    void svc_i(const SPDPdiscoveredParticipantData* pdata);
    void svc_i(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata);
    void svc_i(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata);
    void svc_i(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

    Spdp* spdp_;
    Sedp* sedp_;
    bool shutting_down_;
  } task_;

  // Transport
  DCPS::TransportInst_rch transport_inst_;

#ifndef DDS_HAS_MINIMUM_BIT
  OpenDDS::DCPS::TopicBuiltinTopicDataDataReaderImpl* topic_bit();
  OpenDDS::DCPS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  OpenDDS::DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  void populate_discovered_writer_msg(
      OpenDDS::DCPS::DiscoveredWriterData& dwd,
      const DCPS::RepoId& publication_id,
      const LocalPublication& pub);

  void populate_discovered_reader_msg(
      OpenDDS::DCPS::DiscoveredReaderData& drd,
      const DCPS::RepoId& subscription_id,
      const LocalSubscription& sub);

  struct LocalParticipantMessage : LocalEndpoint {
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalParticipantMessage,
    DCPS::GUID_tKeyLessThan) LocalParticipantMessageMap;
  typedef LocalParticipantMessageMap::iterator LocalParticipantMessageIter;
  typedef LocalParticipantMessageMap::const_iterator LocalParticipantMessageCIter;
  LocalParticipantMessageMap local_participant_messages_;

  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::DCPS::DiscoveredWriterData& wdata);
  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::DCPS::DiscoveredReaderData& rdata);
  void data_received(DCPS::MessageId message_id,
                     const ParticipantMessageData& data);

  typedef std::pair<DCPS::MessageId, OpenDDS::DCPS::DiscoveredWriterData> MsgIdWtrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdWtrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredPublicationMap;
  DeferredPublicationMap deferred_publications_;  // Publications that Spdp has not discovered.

  typedef std::pair<DCPS::MessageId, OpenDDS::DCPS::DiscoveredReaderData> MsgIdRdrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdRdrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredSubscriptionMap;
  DeferredSubscriptionMap deferred_subscriptions_; // Subscriptions that Sedp has not discovered.

  void assign_bit_key(DiscoveredPublication& pub);
  void assign_bit_key(DiscoveredSubscription& sub);

  template<typename Map>
  void remove_entities_belonging_to(Map& m, DCPS::RepoId participant);

  void remove_from_bit_i(const DiscoveredPublication& pub);
  void remove_from_bit_i(const DiscoveredSubscription& sub);

  virtual DDS::ReturnCode_t remove_publication_i(const DCPS::RepoId& publicationId);
  virtual DDS::ReturnCode_t remove_subscription_i(const DCPS::RepoId& subscriptionId);

  // Topic:

  DCPS::RepoIdSet defer_match_endpoints_, associated_participants_;

  void inconsistent_topic(const DCPS::RepoIdSet& endpoints) const;

  virtual bool shutting_down() const;

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                   DiscoveredSubscriptionIter& iter,
                                                   const DCPS::RepoId& reader);

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                   DiscoveredPublicationIter& iter,
                                                   const DCPS::RepoId& writer);

  virtual bool defer_writer(const DCPS::RepoId& writer,
                            const DCPS::RepoId& writer_participant);

  virtual bool defer_reader(const DCPS::RepoId& reader,
                            const DCPS::RepoId& reader_participant);

  static DCPS::RepoId make_id(const DCPS::RepoId& participant_id,
                              const EntityId_t& entity);

  static void set_inline_qos(DCPS::TransportLocatorSeq& locators);

  void write_durable_publication_data(const DCPS::RepoId& reader);
  void write_durable_subscription_data(const DCPS::RepoId& reader);
  void write_durable_participant_message_data(const DCPS::RepoId& reader);

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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SEDP_H
