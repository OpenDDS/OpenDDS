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

#include "dds/DdsSecurityCoreTypeSupportImpl.h"
#include "dds/DdsSecurityWrappers.h"
#include "dds/DCPS/RTPS/RtpsSecurityC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class RtpsDiscovery;
class Spdp;

class WaitForAcks;

class Sedp : public OpenDDS::DCPS::EndpointManager<OpenDDS::Security::SPDPdiscoveredParticipantData> {
public:
  Sedp(const DCPS::RepoId& participant_id,
       Spdp& owner,
       ACE_Thread_Mutex& lock);

  DDS::ReturnCode_t init(const DCPS::RepoId& guid,
                         const RtpsDiscovery& disco,
                         DDS::DomainId_t domainId);

  DDS::ReturnCode_t init_security(DDS::Security::IdentityHandle id_handle,
                                  DDS::Security::PermissionsHandle perm_handle,
                                  DDS::Security::ParticipantCryptoHandle crypto_handle);

  /// request for acknowledgement from all Sedp threads (Task)
  void acknowledge();

  void shutdown();
  void unicast_locators(OpenDDS::DCPS::LocatorSeq& locators) const;

  // @brief return the ip address we have bound to.
  // Valid after init() call
  const ACE_INET_Addr& local_address() const;
  const ACE_INET_Addr& multicast_group() const;
  bool map_ipv4_to_ipv6() const;

  void associate_preauth(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  void associate(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  void associate_volatile(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  void associate_secure_writers_to_readers(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  void associate_secure_readers_to_writers(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  void send_builtin_crypto_tokens(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);
  bool disassociate(const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata);

  DDS::ReturnCode_t write_stateless_message(DDS::Security::ParticipantStatelessMessage& msg,
                                            const DCPS::RepoId& reader);

  DDS::ReturnCode_t write_volatile_message(DDS::Security::ParticipantVolatileMessageSecure& msg,
                                           const DCPS::RepoId& reader);

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
  void signal_liveliness_unsecure(DDS::LivelinessQosPolicyKind kind);
  void signal_liveliness_secure(DDS::LivelinessQosPolicyKind kind);

  static const bool host_is_bigendian_;
private:
  Spdp& spdp_;
  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;

  struct Msg : public OpenDDS::DCPS::PoolAllocationBase {
    enum MsgType {
      MSG_PARTICIPANT,
      MSG_WRITER,
      MSG_READER,
      MSG_PARTICIPANT_DATA,
      MSG_REMOVE_FROM_PUB_BIT,
      MSG_REMOVE_FROM_SUB_BIT,
      MSG_FINI_BIT,
      MSG_STOP,
      MSG_PARTICIPANT_STATELESS_DATA,
      MSG_PARTICIPANT_VOLATILE_SECURE,
      MSG_PARTICIPANT_DATA_SECURE,
      MSG_WRITER_SECURE,
      MSG_READER_SECURE,
      MSG_DCPS_PARTICIPANT_SECURE
    } type_;

    DCPS::MessageId id_;

    union {
      const OpenDDS::Security::SPDPdiscoveredParticipantData* dpdata_;

      const OpenDDS::DCPS::DiscoveredWriterData* wdata_;
      const OpenDDS::Security::DiscoveredWriterData_SecurityWrapper* wdata_secure_;

      const OpenDDS::DCPS::DiscoveredReaderData* rdata_;
      const OpenDDS::Security::DiscoveredReaderData_SecurityWrapper* rdata_secure_;

      const ParticipantMessageData* pmdata_;
      DDS::InstanceHandle_t ih_;
      const DDS::Security::ParticipantGenericMessage* pgmdata_;
    };

    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::Security::SPDPdiscoveredParticipantData* dpdata)
      : type_(mt), id_(id), dpdata_(dpdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata)
      : type_(mt), id_(id), wdata_(wdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::Security::DiscoveredWriterData_SecurityWrapper* wdata)
      : type_(mt), id_(id), wdata_secure_(wdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata)
      : type_(mt), id_(id), rdata_(rdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const OpenDDS::Security::DiscoveredReaderData_SecurityWrapper* rdata)
      : type_(mt), id_(id), rdata_secure_(rdata) {}

    Msg(MsgType mt, DCPS::MessageId id, const ParticipantMessageData* pmdata)
      : type_(mt), id_(id), pmdata_(pmdata) {}

    Msg(MsgType mt, DCPS::MessageId id, DDS::InstanceHandle_t ih)
      : type_(mt), id_(id), ih_(ih) {}

    Msg(MsgType mt, DCPS::MessageId id, const DDS::Security::ParticipantGenericMessage* data)
      : type_(mt), id_(id), pgmdata_(data) {}

  };


  class Endpoint : public DCPS::TransportClient {
  public:
    Endpoint(const DCPS::RepoId& repo_id, Sedp& sedp)
      : repo_id_(repo_id)
      , sedp_(sedp)
      , participant_crypto_handle_(DDS::HANDLE_NIL)
      , endpoint_crypto_handle_(DDS::HANDLE_NIL)
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

  protected:
    DCPS::RepoId repo_id_;
    Sedp& sedp_;
    DDS::Security::ParticipantCryptoHandle participant_crypto_handle_;
    DDS::Security::NativeCryptoHandle endpoint_crypto_handle_;
  };

  class Writer : public DCPS::TransportSendListener, public Endpoint {
  public:
    Writer(const DCPS::RepoId& pub_id, Sedp& sedp);
    virtual ~Writer();

    bool assoc(const DCPS::AssociationData& subscription);

    // Implementing TransportSendListener
    void data_delivered(const DCPS::DataSampleElement*);

    void data_dropped(const DCPS::DataSampleElement*, bool by_transport);

    void control_delivered(const DCPS::Message_Block_Ptr& sample);

    void control_dropped(const DCPS::Message_Block_Ptr& sample,
                         bool dropped_by_transport);

    void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
    void notify_connection_deleted(const DCPS::RepoId&) {}
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

    DDS::ReturnCode_t write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                                              const DCPS::RepoId& reader,
                                              DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_volatile_message_secure(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                                    const DCPS::RepoId& reader,
                                                    DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_dcps_participant_secure(const OpenDDS::Security::SPDPdiscoveredParticipantData& msg,
                                                    const DCPS::RepoId& reader,
                                                    DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_unregister_dispose(const DCPS::RepoId& rid);

    void end_historic_samples(const DCPS::RepoId& reader);

  private:
    DCPS::TransportSendElementAllocator alloc_;
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

    void _add_ref() {}
    void _remove_ref() {}

  };

  Writer publications_writer_;
  Writer publications_secure_writer_;
  Writer subscriptions_writer_;
  Writer subscriptions_secure_writer_;
  Writer participant_message_writer_;
  Writer participant_message_secure_writer_;
  Writer participant_stateless_message_writer_;
  Writer participant_volatile_message_secure_writer_;
  Writer dcps_participant_secure_writer_;

  class Reader
    : public DCPS::TransportReceiveListener
    , public Endpoint
    , public DCPS::RcObject<ACE_SYNCH_MUTEX>
  {
  public:
    Reader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Endpoint(sub_id, sedp)
      , shutting_down_(false)
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

    ACE_Atomic_Op<ACE_SYNCH_MUTEX, bool> shutting_down_;
  };

  typedef DCPS::RcHandle<Reader> Reader_rch;

  Reader_rch publications_reader_;
  Reader_rch publications_secure_reader_;
  Reader_rch subscriptions_reader_;
  Reader_rch subscriptions_secure_reader_;
  Reader_rch participant_message_reader_;
  Reader_rch participant_message_secure_reader_;
  Reader_rch participant_stateless_message_reader_;
  Reader_rch participant_volatile_message_secure_reader_;
  Reader_rch dcps_participant_secure_reader_;

  struct Task : ACE_Task_Ex<ACE_MT_SYNCH, Msg> {
    explicit Task(Sedp* sedp)
      : spdp_(&sedp->spdp_)
      , sedp_(sedp)
      , shutting_down_(false)
    {
      activate();
    }
    ~Task();

    void enqueue(DCPS::unique_ptr<OpenDDS::Security::SPDPdiscoveredParticipantData> pdata);

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<OpenDDS::DCPS::DiscoveredWriterData> wdata);
    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<OpenDDS::Security::DiscoveredWriterData_SecurityWrapper> wrapper);

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<OpenDDS::DCPS::DiscoveredReaderData> rdata);
    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<OpenDDS::Security::DiscoveredReaderData_SecurityWrapper> wrapper);

    void enqueue(DCPS::MessageId id, DCPS::unique_ptr<ParticipantMessageData> data);
    void enqueue(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

    void enqueue_participant_message_secure(DCPS::MessageId id, DCPS::unique_ptr<ParticipantMessageData> data);
    void enqueue_stateless_message(DCPS::MessageId id, DCPS::unique_ptr<DDS::Security::ParticipantStatelessMessage> data);
    void enqueue_volatile_message_secure(DCPS::MessageId id, DCPS::unique_ptr<DDS::Security::ParticipantVolatileMessageSecure> data);

    void acknowledge();
    void shutdown();

  private:
    int svc();

    void svc_i(const OpenDDS::Security::SPDPdiscoveredParticipantData* pdata);
    void svc_secure_i(const OpenDDS::Security::SPDPdiscoveredParticipantData* pdata);

    void svc_i(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredWriterData* wdata);
    void svc_i(DCPS::MessageId id, const OpenDDS::Security::DiscoveredWriterData_SecurityWrapper* wrapper);

    void svc_i(DCPS::MessageId id, const OpenDDS::DCPS::DiscoveredReaderData* rdata);
    void svc_i(DCPS::MessageId id, const OpenDDS::Security::DiscoveredReaderData_SecurityWrapper* wrapper);

    void svc_i(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_i(Msg::MsgType which_bit, const DDS::InstanceHandle_t bit_ih);

    void svc_participant_message_data_secure(DCPS::MessageId id, const ParticipantMessageData* data);
    void svc_stateless_message(DCPS::MessageId id, const DDS::Security::ParticipantStatelessMessage* data);
    void svc_volatile_message_secure(DCPS::MessageId id, const DDS::Security::ParticipantVolatileMessageSecure* data);

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

  void process_discovered_writer_data(DCPS::MessageId message_id,
                                      const OpenDDS::DCPS::DiscoveredWriterData& wdata,
                                      const DCPS::RepoId& guid,
                                      const DDS::Security::EndpointSecurityInfo* security_info = NULL);

  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::DCPS::DiscoveredWriterData& wdata);

  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::Security::DiscoveredWriterData_SecurityWrapper& wrapper);

  void process_discovered_reader_data(DCPS::MessageId message_id,
                                      const OpenDDS::DCPS::DiscoveredReaderData& rdata,
                                      const DCPS::RepoId& guid,
                                      const DDS::Security::EndpointSecurityInfo* security_info = NULL);

  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::DCPS::DiscoveredReaderData& rdata);

  void data_received(DCPS::MessageId message_id,
                     const OpenDDS::Security::DiscoveredReaderData_SecurityWrapper& wrapper);

  void data_received(DCPS::MessageId message_id,
                     const ParticipantMessageData& data);

  void received_participant_message_data_secure(DCPS::MessageId message_id,
						const ParticipantMessageData& data);

  bool should_drop_stateless_message(const DDS::Security::ParticipantGenericMessage& msg);
  bool should_drop_volatile_message(const DDS::Security::ParticipantGenericMessage& msg);
  bool should_drop_message(const char* unsecure_topic_name);

  void received_stateless_message(DCPS::MessageId message_id,
                     const DDS::Security::ParticipantStatelessMessage& data);

  void received_volatile_message_secure(DCPS::MessageId message_id,
                     const DDS::Security::ParticipantVolatileMessageSecure& data);

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

  DCPS::TransportLocatorSeq
  add_security_info(const DCPS::TransportLocatorSeq& locators,
                    const DCPS::RepoId& writer, const DCPS::RepoId& reader);

  virtual bool defer_writer(const DCPS::RepoId& writer,
                            const DCPS::RepoId& writer_participant);

  virtual bool defer_reader(const DCPS::RepoId& reader,
                            const DCPS::RepoId& reader_participant);

  static DCPS::RepoId make_id(const DCPS::RepoId& participant_id,
                              const EntityId_t& entity);

  static void set_inline_qos(DCPS::TransportLocatorSeq& locators);

  void write_durable_publication_data(const DCPS::RepoId& reader);
  void write_durable_publication_data_secure(const DCPS::RepoId& reader);

  void write_durable_subscription_data(const DCPS::RepoId& reader);
  void write_durable_subscription_data_secure(const DCPS::RepoId& reader);

  void write_durable_participant_message_data(const DCPS::RepoId& reader);

  DDS::ReturnCode_t write_publication_data(const DCPS::RepoId& rid,
                                           LocalPublication& pub,
                                           const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_publication_data_secure(const DCPS::RepoId& rid,
                                                  LocalPublication& pub,
                                                  const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_publication_data_unsecure(const DCPS::RepoId& rid,
                                                    LocalPublication& pub,
                                                    const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_subscription_data(const DCPS::RepoId& rid,
                                            LocalSubscription& pub,
                                            const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_subscription_data_secure(const DCPS::RepoId& rid,
                                                   LocalSubscription& pub,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_subscription_data_unsecure(const DCPS::RepoId& rid,
                                                     LocalSubscription& pub,
                                                     const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_participant_message_data(const DCPS::RepoId& rid,
                                                   LocalParticipantMessage& part,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  bool is_opendds(const GUID_t& endpoint) const;

  DCPS::SequenceNumber automatic_liveliness_seq_;
  DCPS::SequenceNumber manual_liveliness_seq_;

  DCPS::SequenceNumber secure_automatic_liveliness_seq_;
  DCPS::SequenceNumber secure_manual_liveliness_seq_;

protected:
  DDS::Security::DatawriterCryptoHandle generate_remote_matched_writer_crypto_handle(const DCPS::RepoId& writer_part, const DDS::Security::DatareaderCryptoHandle& drch);
  DDS::Security::DatareaderCryptoHandle generate_remote_matched_reader_crypto_handle(const DCPS::RepoId& reader_part, const DDS::Security::DatawriterCryptoHandle& dwch, bool relay_only);
  void create_and_send_datareader_crypto_tokens(const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& local_reader, const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& remote_writer);
  void create_and_send_datawriter_crypto_tokens(const DDS::Security::DatawriterCryptoHandle& dwch, const DCPS::RepoId& local_writer, const DDS::Security::DatareaderCryptoHandle& drch, const DCPS::RepoId& remote_reader);
  void handle_datareader_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
  void handle_datawriter_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);

  DDS::DomainId_t get_domain_id() const;
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
