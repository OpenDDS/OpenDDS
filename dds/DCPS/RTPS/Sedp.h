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

#include "dds/DCPS/RTPS/ICE/Ice.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/DiscoveryBase.h"
#include "dds/DCPS/JobQueue.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportInst_rch.h"

#include "dds/DCPS/PoolAllocator.h"

#ifdef OPENDDS_SECURITY
#include "dds/DCPS/RTPS/RtpsSecurityC.h"
#endif

#include "ace/Atomic_Op.h"
#include "ace/Task_Ex_T.h"
#include "ace/Thread_Mutex.h"
#include "ace/Condition_Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;
class Spdp;
class WaitForAcks;

#ifdef OPENDDS_SECURITY
struct DiscoveredPublication_SecurityWrapper;
struct DiscoveredSubscription_SecurityWrapper;
typedef Security::SPDPdiscoveredParticipantData ParticipantData_t;
#else
typedef SPDPdiscoveredParticipantData ParticipantData_t;
#endif

class Sedp : public DCPS::EndpointManager<ParticipantData_t> {
public:
  Sedp(const DCPS::RepoId& participant_id,
       Spdp& owner,
       ACE_Thread_Mutex& lock);

  ~Sedp();

  DDS::ReturnCode_t init(const DCPS::RepoId& guid,
                         const RtpsDiscovery& disco,
                         DDS::DomainId_t domainId);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t init_security(DDS::Security::IdentityHandle id_handle,
                                  DDS::Security::PermissionsHandle perm_handle,
                                  DDS::Security::ParticipantCryptoHandle crypto_handle);
#endif

  /// request for acknowledgement from all Sedp threads (Task)
  void acknowledge();

  void shutdown();
  DCPS::LocatorSeq unicast_locators() const;
  DCPS::LocatorSeq multicast_locators() const;

  // @brief return the ip address we have bound to.
  // Valid after init() call
  const ACE_INET_Addr& local_address() const;
  const ACE_INET_Addr& multicast_group() const;
  bool map_ipv4_to_ipv6() const;

  void associate(const ParticipantData_t& pdata);

#ifdef OPENDDS_SECURITY
  void associate_preauth(const Security::SPDPdiscoveredParticipantData& pdata);
  void associate_volatile(const Security::SPDPdiscoveredParticipantData& pdata);
  void associate_secure_writers_to_readers(const Security::SPDPdiscoveredParticipantData& pdata);
  void associate_secure_readers_to_writers(const Security::SPDPdiscoveredParticipantData& pdata);

  /// Create and send keys the first time
  void send_builtin_crypto_tokens(const Security::SPDPdiscoveredParticipantData& pdata);

  /// Create and send keys for individual endpoints.
  void send_builtin_crypto_tokens(const DCPS::RepoId& dstParticipant,
                                  const DCPS::EntityId_t& dstEntity, const DCPS::RepoId& src);
#endif

  bool disassociate(const ParticipantData_t& pdata);

  void update_locators(const ParticipantData_t& pdata);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_stateless_message(DDS::Security::ParticipantStatelessMessage& msg,
                                            const DCPS::RepoId& reader);

  DDS::ReturnCode_t write_volatile_message(DDS::Security::ParticipantVolatileMessageSecure& msg,
                                           const DCPS::RepoId& reader);

  void write_durable_dcps_participant_secure(const DCPS::RepoId& reader);

  DDS::ReturnCode_t write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                                  const DCPS::RepoId& part);
#endif

  DDS::ReturnCode_t write_dcps_participant_dispose(const DCPS::RepoId& part);

  // Topic
  bool update_topic_qos(const DCPS::RepoId& topicId, const DDS::TopicQos& qos);

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
  void signal_liveliness_unsecure(DDS::LivelinessQosPolicyKind kind);

#ifdef OPENDDS_SECURITY
  void signal_liveliness_secure(DDS::LivelinessQosPolicyKind kind);
#endif

  static const bool host_is_bigendian_;

  ICE::Endpoint* get_ice_endpoint();

  void rtps_relay_address(const ACE_INET_Addr& address);

  void stun_server_address(const ACE_INET_Addr& address);

private:

  class AssociationComplete : public DCPS::JobQueue::Job {
  public:
    AssociationComplete(Sedp* sedp, const DCPS::RepoId& local, const DCPS::RepoId& remote) : sedp_(sedp), local_(local), remote_(remote) {}
    void execute();
  private:
    Sedp* sedp_;
    DCPS::RepoId local_, remote_;
  };

  Spdp& spdp_;
  DCPS::SequenceNumber participant_secure_sequence_;

#ifdef OPENDDS_SECURITY
  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;
#endif

  struct Msg : public DCPS::PoolAllocationBase {
    enum MsgType {
      MSG_PARTICIPANT,
      MSG_WRITER,
      MSG_READER,
      MSG_PARTICIPANT_DATA,
      MSG_REMOVE_FROM_PUB_BIT,
      MSG_REMOVE_FROM_SUB_BIT,
      MSG_FINI_BIT,
      MSG_STOP,

#ifdef OPENDDS_SECURITY
      MSG_PARTICIPANT_STATELESS_DATA,
      MSG_PARTICIPANT_VOLATILE_SECURE,
      MSG_PARTICIPANT_DATA_SECURE,
      MSG_WRITER_SECURE,
      MSG_READER_SECURE,
      MSG_DCPS_PARTICIPANT_SECURE
#endif

    } type_;

    DCPS::MessageId id_;

    union {
      const ParticipantData_t* dpdata_;

      const DiscoveredPublication* wdata_;

#ifdef OPENDDS_SECURITY
      const DiscoveredPublication_SecurityWrapper* wdata_secure_;
#endif

      const DiscoveredSubscription* rdata_;

#ifdef OPENDDS_SECURITY
      const DiscoveredSubscription_SecurityWrapper* rdata_secure_;
#endif

      const ParticipantMessageData* pmdata_;
      DDS::InstanceHandle_t ih_;

#ifdef OPENDDS_SECURITY
      const DDS::Security::ParticipantGenericMessage* pgmdata_;
#endif
    };

    Msg(MsgType mt, DCPS::MessageId id, const ParticipantData_t* dpdata)
      : type_(mt), id_(id), dpdata_(dpdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const DiscoveredPublication* wdata)
      : type_(mt), id_(id), wdata_(wdata) {}

#ifdef OPENDDS_SECURITY
    Msg(MsgType mt, DCPS::MessageId id, const DiscoveredPublication_SecurityWrapper* wdata)
      : type_(mt), id_(id), wdata_secure_(wdata) {}
#endif

    Msg(MsgType mt, DCPS::MessageId id, const DiscoveredSubscription* rdata)
      : type_(mt), id_(id), rdata_(rdata) {}

#ifdef OPENDDS_SECURITY
    Msg(MsgType mt, DCPS::MessageId id, const DiscoveredSubscription_SecurityWrapper* rdata)
      : type_(mt), id_(id), rdata_secure_(rdata) {}
#endif

    Msg(MsgType mt, DCPS::MessageId id, const ParticipantMessageData* pmdata)
      : type_(mt), id_(id), pmdata_(pmdata) {}

    Msg(MsgType mt, DCPS::MessageId id, DDS::InstanceHandle_t ih)
      : type_(mt), id_(id), ih_(ih) {}

#ifdef OPENDDS_SECURITY
    Msg(MsgType mt, DCPS::MessageId id, const DDS::Security::ParticipantGenericMessage* data)
      : type_(mt), id_(id), pgmdata_(data) {}
#endif

    static const char* msgTypeToString(MsgType type);
    const char* msgTypeToString() const;
  };


  class Endpoint : public DCPS::TransportClient {
  public:
    Endpoint(const DCPS::RepoId& repo_id, Sedp& sedp)
      : repo_id_(repo_id)
      , sedp_(sedp)
      , shutting_down_(false)
#ifdef OPENDDS_SECURITY
      , participant_crypto_handle_(DDS::HANDLE_NIL)
      , endpoint_crypto_handle_(DDS::HANDLE_NIL)
#endif
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

#ifdef OPENDDS_SECURITY
    void set_crypto_handles(DDS::Security::ParticipantCryptoHandle p,
                            DDS::Security::NativeCryptoHandle e = DDS::HANDLE_NIL)
    {
      participant_crypto_handle_ = p;
      endpoint_crypto_handle_ = e;
    }

    DDS::Security::ParticipantCryptoHandle get_crypto_handle() const
    {
      return participant_crypto_handle_;
    }

    DDS::Security::NativeCryptoHandle get_endpoint_crypto_handle() const
    {
      return endpoint_crypto_handle_;
    }
#endif

    void shutting_down() { shutting_down_ = true; }

  protected:
    DCPS::RepoId repo_id_;
    Sedp& sedp_;
    ACE_Atomic_Op<ACE_Thread_Mutex, bool> shutting_down_;
#ifdef OPENDDS_SECURITY
    DDS::Security::ParticipantCryptoHandle participant_crypto_handle_;
    DDS::Security::NativeCryptoHandle endpoint_crypto_handle_;
#endif
  };

  class Writer : public DCPS::TransportSendListener, public Endpoint {
  public:
    Writer(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1);
    virtual ~Writer();

    bool assoc(const DCPS::AssociationData& subscription);
    void transport_assoc_done(int flags, const DCPS::RepoId& remote);

    // Implementing TransportSendListener
    void data_delivered(const DCPS::DataSampleElement*);

    void data_dropped(const DCPS::DataSampleElement*, bool by_transport);

    void control_delivered(const DCPS::Message_Block_Ptr& sample);

    void control_dropped(const DCPS::Message_Block_Ptr& sample,
                         bool dropped_by_transport);

    void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
    void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
    void retrieve_inline_qos_data(InlineQosData&) const {}

    void send_sample(const ACE_Message_Block& data,
                     size_t size,
                     const DCPS::RepoId& reader,
                     DCPS::SequenceNumber& sequence,
                     bool historic = false);

    DDS::ReturnCode_t write_parameter_list(const ParameterList& plist,
                                           const DCPS::RepoId& reader,
                                           DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_participant_message(const ParticipantMessageData& pmd,
                                                const DCPS::RepoId& reader,
                                                DCPS::SequenceNumber& sequence);

#ifdef OPENDDS_SECURITY
    DDS::ReturnCode_t write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                                              const DCPS::RepoId& reader,
                                              DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_volatile_message_secure(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                                    const DCPS::RepoId& reader,
                                                    DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                                    const DCPS::RepoId& reader, DCPS::SequenceNumber& sequence);
#endif

    DDS::ReturnCode_t write_unregister_dispose(const DCPS::RepoId& rid, CORBA::UShort pid = PID_ENDPOINT_GUID);

    void end_historic_samples(const DCPS::RepoId& reader);

    const DCPS::SequenceNumber& get_seq() const
    {
      return seq_;
    }


  private:
    Header header_;
    DCPS::SequenceNumber seq_;

    void write_control_msg(DCPS::Message_Block_Ptr payload,
                           size_t size,
                           DCPS::MessageId id,
                           DCPS::SequenceNumber seq = DCPS::SequenceNumber());

    void set_header_fields(DCPS::DataSampleHeader& dsh,
                           size_t size,
                           const DCPS::RepoId& reader,
                           DCPS::SequenceNumber& sequence,
                           bool historic_sample = false,
                           DCPS::MessageId id = DCPS::SAMPLE_DATA);

  };

  typedef DCPS::RcHandle<Writer> Writer_rch;

  Writer_rch publications_writer_;

#ifdef OPENDDS_SECURITY
  Writer_rch publications_secure_writer_;
#endif

  Writer_rch subscriptions_writer_;

#ifdef OPENDDS_SECURITY
  Writer_rch subscriptions_secure_writer_;
#endif

  Writer_rch participant_message_writer_;

#ifdef OPENDDS_SECURITY
  Writer_rch participant_message_secure_writer_;
  Writer_rch participant_stateless_message_writer_;
  Writer_rch dcps_participant_secure_writer_;

  Writer_rch participant_volatile_message_secure_writer_;
#endif

  class Reader
    : public DCPS::TransportReceiveListener
    , public Endpoint
  {
  public:
    Reader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Endpoint(sub_id, sedp)
    {}

    virtual ~Reader();

    bool assoc(const DCPS::AssociationData& publication);

    // Implementing TransportReceiveListener

    void data_received(const DCPS::ReceivedDataSample& sample);

    void notify_subscription_disconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_reconnected(const DCPS::WriterIdSeq&) {}
    void notify_subscription_lost(const DCPS::WriterIdSeq&) {}
    void remove_associations(const DCPS::WriterIdSeq&, bool) {}
  };

  typedef DCPS::RcHandle<Reader> Reader_rch;

  Reader_rch publications_reader_;

#ifdef OPENDDS_SECURITY
  Reader_rch publications_secure_reader_;
#endif

  Reader_rch subscriptions_reader_;

#ifdef OPENDDS_SECURITY
  Reader_rch subscriptions_secure_reader_;
#endif

  Reader_rch participant_message_reader_;

#ifdef OPENDDS_SECURITY
  Reader_rch participant_message_secure_reader_;
  Reader_rch participant_stateless_message_reader_;
  Reader_rch participant_volatile_message_secure_reader_;
  Reader_rch dcps_participant_secure_reader_;
#endif

  static const size_t TASK_MQ_BYTES = sizeof(Msg) * 1024 * 32;

  struct Task : ACE_Task_Ex<ACE_MT_SYNCH, Msg> {
    explicit Task(Sedp* sedp)
      : ACE_Task_Ex<ACE_MT_SYNCH, Msg>(0, new ACE_Message_Queue_Ex<Msg, ACE_MT_SYNCH>(TASK_MQ_BYTES, TASK_MQ_BYTES))
      , spdp_(&sedp->spdp_)
      , sedp_(sedp)
      , shutting_down_(false)
    {
      delete_msg_queue_ = true;
      activate();
    }
    ~Task();

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<ParticipantData_t> pdata, bool bSecureParticipant = false);

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<DiscoveredPublication> wdata);
    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<DiscoveredSubscription> rdata);

#ifdef OPENDDS_SECURITY
    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<DiscoveredPublication_SecurityWrapper> wrapper);
    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<DiscoveredSubscription_SecurityWrapper> wrapper);
#endif

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<ParticipantMessageData> data);
    void enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

#ifdef OPENDDS_SECURITY
    void enqueue_participant_message_secure(DCPS::MessageId id, DCPS::unique_ptr<ParticipantMessageData> data);
    void enqueue_stateless_message(DCPS::MessageId id, DCPS::unique_ptr<DDS::Security::ParticipantStatelessMessage> data);
    void enqueue_volatile_message_secure(
      DCPS::MessageId id, DCPS::unique_ptr<DDS::Security::ParticipantVolatileMessageSecure> data);
#endif

    void acknowledge();
    void shutdown();

  private:
    int svc();

    void svc_i(const ParticipantData_t* pdata);

#ifdef OPENDDS_SECURITY
    void svc_secure_i(DCPS::MessageId id, const Security::SPDPdiscoveredParticipantData* pdata);
#endif

    void svc_i(DCPS::MessageId id, const DiscoveredPublication* wdata);
    void svc_i(DCPS::MessageId id, const DiscoveredSubscription* rdata);

#ifdef OPENDDS_SECURITY
    void svc_i(DCPS::MessageId id, const DiscoveredPublication_SecurityWrapper* wrapper);
    void svc_i(DCPS::MessageId id, const DiscoveredSubscription_SecurityWrapper* wrapper);
#endif

    void svc_i(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

#ifdef OPENDDS_SECURITY
    void svc_participant_message_data_secure(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_stateless_message(DCPS::MessageId id, const DDS::Security::ParticipantStatelessMessage* data);
    void svc_volatile_message_secure(DCPS::MessageId id, const DDS::Security::ParticipantVolatileMessageSecure* data);
#endif

    Spdp* spdp_;
    Sedp* sedp_;
    bool shutting_down_;
  } task_;

  // Transport
  DCPS::TransportInst_rch transport_inst_;
  DCPS::TransportConfig_rch transport_cfg_;

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::TopicBuiltinTopicDataDataReaderImpl* topic_bit();
  DCPS::PublicationBuiltinTopicDataDataReaderImpl* pub_bit();
  DCPS::SubscriptionBuiltinTopicDataDataReaderImpl* sub_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  void populate_discovered_writer_msg(
      DCPS::DiscoveredWriterData& dwd,
      const DCPS::RepoId& publication_id,
      const LocalPublication& pub);

  void populate_discovered_reader_msg(
      DCPS::DiscoveredReaderData& drd,
      const DCPS::RepoId& subscription_id,
      const LocalSubscription& sub);

  struct LocalParticipantMessage : LocalEndpoint {
  };
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, LocalParticipantMessage,
    DCPS::GUID_tKeyLessThan) LocalParticipantMessageMap;
  typedef LocalParticipantMessageMap::iterator LocalParticipantMessageIter;
  typedef LocalParticipantMessageMap::const_iterator LocalParticipantMessageCIter;
  LocalParticipantMessageMap local_participant_messages_;

  void process_discovered_writer_data(DCPS::MessageId message_id,
                                      const DCPS::DiscoveredWriterData& wdata,
                                      const DCPS::RepoId& guid
#ifdef OPENDDS_SECURITY
                                      ,
                                      bool have_ice_agent_info,
                                      const ICE::AgentInfo& ice_agent_info,
                                      const DDS::Security::EndpointSecurityInfo* security_info = NULL
#endif
                                      );

  void data_received(DCPS::MessageId message_id,
                     const DiscoveredPublication& wdata);

#ifdef OPENDDS_SECURITY
  void data_received(DCPS::MessageId message_id,
                     const DiscoveredPublication_SecurityWrapper& wrapper);
#endif

  void process_discovered_reader_data(DCPS::MessageId message_id,
                                      const DCPS::DiscoveredReaderData& rdata,
                                      const DCPS::RepoId& guid
#ifdef OPENDDS_SECURITY
                                      ,
                                      bool have_ice_agent_info,
                                      const ICE::AgentInfo& ice_agent_info,
                                      const DDS::Security::EndpointSecurityInfo* security_info = NULL
#endif
                                      );

  void data_received(DCPS::MessageId message_id,
                     const DiscoveredSubscription& rdata);

#ifdef OPENDDS_SECURITY
  void data_received(DCPS::MessageId message_id,
                     const DiscoveredSubscription_SecurityWrapper& wrapper);
#endif

  void data_received(DCPS::MessageId message_id,
                     const ParticipantMessageData& data);

#ifdef OPENDDS_SECURITY
  void received_participant_message_data_secure(DCPS::MessageId message_id,
                                                const ParticipantMessageData& data);

  bool should_drop_stateless_message(const DDS::Security::ParticipantGenericMessage& msg);
  bool should_drop_volatile_message(const DDS::Security::ParticipantGenericMessage& msg);
  bool should_drop_message(const char* unsecure_topic_name);

  void received_stateless_message(DCPS::MessageId message_id,
                                  const DDS::Security::ParticipantStatelessMessage& data);

  void received_volatile_message_secure(DCPS::MessageId message_id,
                                        const DDS::Security::ParticipantVolatileMessageSecure& data);
#endif

  typedef std::pair<DCPS::MessageId, DiscoveredPublication> MsgIdWtrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdWtrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredPublicationMap;
  DeferredPublicationMap deferred_publications_;  // Publications that Spdp has not discovered.

  typedef std::pair<DCPS::MessageId, DiscoveredSubscription> MsgIdRdrDataPair;
  typedef OPENDDS_MAP_CMP(DCPS::RepoId, MsgIdRdrDataPair,
                   DCPS::GUID_tKeyLessThan) DeferredSubscriptionMap;
  DeferredSubscriptionMap deferred_subscriptions_; // Subscriptions that Sedp has not discovered.

  void assign_bit_key(DiscoveredPublication& pub);
  void assign_bit_key(DiscoveredSubscription& sub);

  template<typename Map>
  void remove_entities_belonging_to(Map& m, DCPS::RepoId participant);

  void remove_from_bit_i(const DiscoveredPublication& pub);
  void remove_from_bit_i(const DiscoveredSubscription& sub);

  virtual DDS::ReturnCode_t remove_publication_i(const DCPS::RepoId& publicationId, LocalPublication& pub);
  virtual DDS::ReturnCode_t remove_subscription_i(const DCPS::RepoId& subscriptionId, LocalSubscription& sub);

  // Topic:

  DCPS::RepoIdSet associated_participants_;

  virtual bool shutting_down() const;

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                   DiscoveredSubscriptionIter& iter,
                                                   const DCPS::RepoId& reader);

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq*& tls,
                                                   DiscoveredPublicationIter& iter,
                                                   const DCPS::RepoId& writer);

#ifdef OPENDDS_SECURITY
  DCPS::TransportLocatorSeq
  add_security_info(const DCPS::TransportLocatorSeq& locators,
                    const DCPS::RepoId& writer, const DCPS::RepoId& reader);
#endif

  static void set_inline_qos(DCPS::TransportLocatorSeq& locators);

  void write_durable_publication_data(const DCPS::RepoId& reader, bool secure);
  void write_durable_subscription_data(const DCPS::RepoId& reader, bool secure);

  void write_durable_participant_message_data(const DCPS::RepoId& reader);

  DDS::ReturnCode_t add_publication_i(const DCPS::RepoId& rid,
                                      LocalPublication& pub);

  DDS::ReturnCode_t write_publication_data(const DCPS::RepoId& rid,
                                           LocalPublication& pub,
                                           const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_publication_data_secure(const DCPS::RepoId& rid,
                                                  LocalPublication& pub,
                                                  const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
#endif

  DDS::ReturnCode_t write_publication_data_unsecure(const DCPS::RepoId& rid,
                                                    LocalPublication& pub,
                                                    const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t add_subscription_i(const DCPS::RepoId& rid,
                                       LocalSubscription& sub);


  DDS::ReturnCode_t write_subscription_data(const DCPS::RepoId& rid,
                                            LocalSubscription& pub,
                                            const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_subscription_data_secure(const DCPS::RepoId& rid,
                                                   LocalSubscription& pub,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
#endif

  DDS::ReturnCode_t write_subscription_data_unsecure(const DCPS::RepoId& rid,
                                                     LocalSubscription& pub,
                                                     const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_participant_message_data(const DCPS::RepoId& rid,
                                                   LocalParticipantMessage& part,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  virtual bool is_expectant_opendds(const GUID_t& endpoint) const;

#ifdef OPENDDS_SECURITY
  DCPS::SequenceNumber secure_automatic_liveliness_seq_;
  DCPS::SequenceNumber secure_manual_liveliness_seq_;
#endif

  DCPS::SequenceNumber automatic_liveliness_seq_;
  DCPS::SequenceNumber manual_liveliness_seq_;

protected:

#ifdef OPENDDS_SECURITY
  DDS::Security::DatawriterCryptoHandle generate_remote_matched_writer_crypto_handle(
    const DCPS::RepoId& writer_part, const DDS::Security::DatareaderCryptoHandle& drch);
  DDS::Security::DatareaderCryptoHandle generate_remote_matched_reader_crypto_handle(
    const DCPS::RepoId& reader_part, const DDS::Security::DatawriterCryptoHandle& dwch, bool relay_only);

  void create_datareader_crypto_tokens(
    const DDS::Security::DatareaderCryptoHandle& drch,
    const DDS::Security::DatawriterCryptoHandle& dwch,
    DDS::Security::DatareaderCryptoTokenSeq& drcts);
  void send_datareader_crypto_tokens(
    const DCPS::RepoId& local_reader,
    const DCPS::RepoId& remote_writer,
    const DDS::Security::DatareaderCryptoTokenSeq& drcts);
  void create_and_send_datareader_crypto_tokens(
    const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& local_reader,
    const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& remote_writer);

  void create_datawriter_crypto_tokens(
    const DDS::Security::DatawriterCryptoHandle& dwch,
    const DDS::Security::DatareaderCryptoHandle& drch,
    DDS::Security::DatawriterCryptoTokenSeq& dwcts);
  void send_datawriter_crypto_tokens(
    const DCPS::RepoId& local_writer,
    const DCPS::RepoId& remote_reader,
    const DDS::Security::DatawriterCryptoTokenSeq& dwcts);
  void create_and_send_datawriter_crypto_tokens(
    const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& local_writer,
    const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& remote_reader);

  void handle_datareader_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
  void handle_datawriter_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);

  DDS::DomainId_t get_domain_id() const;

  DCPS::RepoIdSet associated_volatile_readers_;

  struct RemoteWriter {
    DCPS::RepoId local_reader, remote_writer;
    DDS::Security::DatareaderCryptoTokenSeq reader_tokens;
  };
  typedef OPENDDS_VECTOR(RemoteWriter) RemoteWriterVector;
  typedef OPENDDS_MAP_CMP(
    DCPS::RepoId, RemoteWriterVector, DCPS::GUID_tKeyLessThan) RemoteWriterVectors;
  RemoteWriterVectors datareader_crypto_tokens_;

  struct RemoteReader {
    DCPS::RepoId local_writer, remote_reader;
    DDS::Security::DatawriterCryptoTokenSeq writer_tokens;
  };
  typedef OPENDDS_VECTOR(RemoteReader) RemoteReaderVector;
  typedef OPENDDS_MAP_CMP(
    DCPS::RepoId, RemoteReaderVector, DCPS::GUID_tKeyLessThan) RemoteReaderVectors;
  RemoteReaderVectors datawriter_crypto_tokens_;

#endif

#ifdef OPENDDS_SECURITY
  struct PublicationAgentInfoListener : public ICE::AgentInfoListener
  {
    Sedp& sedp;
    PublicationAgentInfoListener(Sedp& a_sedp) : sedp(a_sedp) {}
    void update_agent_info(const DCPS::RepoId& a_local_guid,
                           const ICE::AgentInfo& a_agent_info);

  } publication_agent_info_listener_;

  struct SubscriptionAgentInfoListener : public ICE::AgentInfoListener
  {
    Sedp& sedp;
    SubscriptionAgentInfoListener(Sedp& a_sedp) : sedp(a_sedp) {}
    void update_agent_info(const DCPS::RepoId& a_local_guid,
                           const ICE::AgentInfo& a_agent_info);
  } subscription_agent_info_listener_;
#endif

  void add_assoc_i(const DCPS::RepoId& local_guid, const LocalPublication& lpub,
                   const DCPS::RepoId& remote_guid, const DiscoveredSubscription& dsub);
  void remove_assoc_i(const DCPS::RepoId& local_guid, const LocalPublication& lpub,
                      const DCPS::RepoId& remote_guid);
  void add_assoc_i(const DCPS::RepoId& local_guid, const LocalSubscription& lsub,
                   const DCPS::RepoId& remote_guid, const DiscoveredPublication& dpub);
  void remove_assoc_i(const DCPS::RepoId& local_guid, const LocalSubscription& lsub,
                      const DCPS::RepoId& remote_guid);
  void start_ice(const DCPS::RepoId& guid, const LocalPublication& lpub);
  void start_ice(const DCPS::RepoId& guid, const LocalSubscription& lsub);
  void start_ice(const DCPS::RepoId& guid, const DiscoveredPublication& dpub);
  void start_ice(const DCPS::RepoId& guid, const DiscoveredSubscription& dsub);
  void stop_ice(const DCPS::RepoId& guid, const DiscoveredPublication& dpub);
  void stop_ice(const DCPS::RepoId& guid, const DiscoveredSubscription& dsub);
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

bool locators_changed(const ParticipantProxy_t& x,
                      const ParticipantProxy_t& y);

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SEDP_H
