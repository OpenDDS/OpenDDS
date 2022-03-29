/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_SEDP_H
#define OPENDDS_DCPS_RTPS_SEDP_H

#include "TypeLookup.h"
#include "BaseMessageTypes.h"
#include "BaseMessageUtils.h"
#ifdef OPENDDS_SECURITY
#  include "SecurityHelpers.h"
#endif
#include "ICE/Ice.h"
#include "RtpsRpcTypeSupportImpl.h"
#include "RtpsCoreTypeSupportImpl.h"
#ifdef OPENDDS_SECURITY
#  include "RtpsSecurityC.h"
#endif

#include <dds/DCPS/BuiltInTopicDataReaderImpls.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/ConnectionRecords.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/DataReaderCallbacks.h>
#include <dds/DCPS/DataSampleElement.h>
#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/FibonacciSequence.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/JobQueue.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/RcHandle_T.h>
#include <dds/DCPS/Registered_Data_Types.h>
#include <dds/DCPS/NetworkAddress.h>
#include <dds/DCPS/SporadicTask.h>
#include <dds/DCPS/TopicDetails.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/transport/framework/TransportSendListener.h>
#include <dds/DCPS/transport/framework/TransportClient.h>
#include <dds/DCPS/transport/framework/TransportInst_rch.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

#ifndef ACE_HAS_CPP11
#  include <ace/Atomic_Op_T.h>
#endif
#include <ace/Task_Ex_T.h>
#include <ace/Thread_Mutex.h>

#ifdef ACE_HAS_CPP11
#  include <atomic>
#endif

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

#ifdef OPENDDS_SECURITY
enum AuthState {
  AUTH_STATE_HANDSHAKE,
  AUTH_STATE_AUTHENTICATED,
  AUTH_STATE_UNAUTHENTICATED
};

enum HandshakeState {
  HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST, //!< Requester should call begin_handshake_request
  HANDSHAKE_STATE_BEGIN_HANDSHAKE_REPLY, //!< Replier should call begin_handshake_reply
  HANDSHAKE_STATE_PROCESS_HANDSHAKE, //!< Requester and replier should call process handshake
  HANDSHAKE_STATE_DONE //!< Handshake concluded or timed out
};
#endif

class RtpsDiscovery;
class Spdp;
class Sedp;
class WaitForAcks;

#ifdef OPENDDS_SECURITY
typedef Security::SPDPdiscoveredParticipantData ParticipantData_t;
#else
typedef SPDPdiscoveredParticipantData ParticipantData_t;
#endif

#ifdef OPENDDS_SECURITY
typedef OPENDDS_MAP_CMP(GUID_t, DDS::Security::DatareaderCryptoTokenSeq, GUID_tKeyLessThan)
  DatareaderCryptoTokenSeqMap;
typedef OPENDDS_MAP_CMP(GUID_t, DDS::Security::DatawriterCryptoTokenSeq, GUID_tKeyLessThan)
  DatawriterCryptoTokenSeqMap;

inline bool has_security_data(Security::DiscoveredParticipantDataKind kind)
{
  return kind == Security::DPDK_ENHANCED || kind == Security::DPDK_SECURE;
}
#endif

using DCPS::RepoIdSet;

const int AC_EMPTY = 0;
const int AC_REMOTE_RELIABLE = 1 << 0;
const int AC_REMOTE_DURABLE = 1 << 1;
const int AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE = 1 << 2;
const int AC_SEND_LOCAL_TOKEN = 1 << 3;
const int AC_LOCAL_TOKENS_SENT = 1 << 4;

class BuiltinAssociationRecord {
public:
  BuiltinAssociationRecord(DCPS::TransportClient_rch transport_client,
                           const GUID_t& remote_id,
                           int flags)
    : transport_client_(transport_client)
    , remote_id_(remote_id)
    , flags_(flags)
  {}

  const GUID_t local_id() const
  {
    return transport_client_->get_repo_id();
  }

  const GUID_t& remote_id() const
  {
    // FUTURE: Remove this and just store the entity id.
    return remote_id_;
  }

  bool remote_reliable() const
  {
    return flags_ & AC_REMOTE_RELIABLE;
  }

  bool remote_durable() const
  {
    return flags_ & AC_REMOTE_DURABLE;
  }

  bool generate_remote_matched_crypto_handle() const
  {
    return flags_ & AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE;
  }

  bool send_local_token() const
  {
    return flags_ & AC_SEND_LOCAL_TOKEN;
  }

  void local_tokens_sent(bool flag)
  {
    if (flag) {
      flags_ |= AC_LOCAL_TOKENS_SENT;
    } else {
      flags_ &= ~AC_LOCAL_TOKENS_SENT;
    }
  }

  bool local_tokens_sent() const
  {
    return flags_ & AC_LOCAL_TOKENS_SENT;
  }

  const DCPS::TransportClient_rch transport_client_;

private:
  const GUID_t remote_id_;
  int flags_;
};

class WriterAssociationRecord : public DCPS::RcObject {
public:
  WriterAssociationRecord(DCPS::DataWriterCallbacks_wrch callbacks,
                          const GUID_t& writer_id,
                          const DCPS::ReaderAssociation& reader_association)
    : callbacks_(callbacks)
    , writer_id_(writer_id)
    , reader_association_(reader_association)
  {}

  const GUID_t& writer_id() const { return writer_id_; }
  const GUID_t& reader_id() const { return reader_association_.readerId; }

  const DCPS::DataWriterCallbacks_wrch callbacks_;
  const GUID_t writer_id_;
  const DCPS::ReaderAssociation reader_association_;
};
typedef RcHandle<WriterAssociationRecord> WriterAssociationRecord_rch;

class ReaderAssociationRecord : public DCPS::RcObject {
public:
  ReaderAssociationRecord(DCPS::DataReaderCallbacks_wrch callbacks,
                          const GUID_t& reader_id,
                          const DCPS::WriterAssociation& writer_association)
    : callbacks_(callbacks)
    , reader_id_(reader_id)
    , writer_association_(writer_association)
  {}

  const GUID_t& reader_id() const { return reader_id_; }
  const GUID_t& writer_id() const { return writer_association_.writerId; }

  const DCPS::DataReaderCallbacks_wrch callbacks_;
  const GUID_t reader_id_;
  const DCPS::WriterAssociation writer_association_;
};
typedef RcHandle<ReaderAssociationRecord> ReaderAssociationRecord_rch;

struct DiscoveredParticipant {

  DiscoveredParticipant()
    : location_ih_(DDS::HANDLE_NIL)
    , bit_ih_(DDS::HANDLE_NIL)
    , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
    , have_spdp_info_(false)
    , have_sedp_info_(false)
    , have_auth_req_msg_(false)
    , have_handshake_msg_(false)
    , handshake_resend_falloff_(TimeDuration::zero_value)
    , auth_state_(AUTH_STATE_HANDSHAKE)
    , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
    , is_requester_(false)
    , auth_req_sequence_number_(0)
    , handshake_sequence_number_(0)
    , identity_handle_(DDS::HANDLE_NIL)
    , handshake_handle_(DDS::HANDLE_NIL)
    , permissions_handle_(DDS::HANDLE_NIL)
    , extended_builtin_endpoints_(0)
    , participant_tokens_sent_(false)
#endif
  {
#ifdef OPENDDS_SECURITY
    security_info_.participant_security_attributes = 0;
    security_info_.plugin_participant_security_attributes = 0;
#endif
  }

  DiscoveredParticipant(const ParticipantData_t& p,
                        const SequenceNumber& seq,
                        const TimeDuration& resend_period)
    : pdata_(p)
    , location_ih_(DDS::HANDLE_NIL)
    , bit_ih_(DDS::HANDLE_NIL)
    , max_seq_(seq)
    , seq_reset_count_(0)
#ifdef OPENDDS_SECURITY
    , have_spdp_info_(false)
    , have_sedp_info_(false)
    , have_auth_req_msg_(false)
    , have_handshake_msg_(false)
    , handshake_resend_falloff_(resend_period)
    , auth_state_(AUTH_STATE_HANDSHAKE)
    , handshake_state_(HANDSHAKE_STATE_BEGIN_HANDSHAKE_REQUEST)
    , is_requester_(false)
    , auth_req_sequence_number_(0)
    , handshake_sequence_number_(0)
    , identity_handle_(DDS::HANDLE_NIL)
    , handshake_handle_(DDS::HANDLE_NIL)
    , permissions_handle_(DDS::HANDLE_NIL)
    , extended_builtin_endpoints_(0)
    , participant_tokens_sent_(false)
#endif
  {
    const GUID_t guid = DCPS::make_part_guid(p.participantProxy.guidPrefix);
    assign(location_data_.guid, guid);
    location_data_.location = 0;
    location_data_.change_mask = 0;
    location_data_.local_timestamp.sec = 0;
    location_data_.local_timestamp.nanosec = 0;
    location_data_.ice_timestamp.sec = 0;
    location_data_.ice_timestamp.nanosec = 0;
    location_data_.relay_timestamp.sec = 0;
    location_data_.relay_timestamp.nanosec = 0;
    location_data_.local6_timestamp.sec = 0;
    location_data_.local6_timestamp.nanosec = 0;
    location_data_.ice6_timestamp.sec = 0;
    location_data_.ice6_timestamp.nanosec = 0;
    location_data_.relay6_timestamp.sec = 0;
    location_data_.relay6_timestamp.nanosec = 0;

#ifdef OPENDDS_SECURITY
    security_info_.participant_security_attributes = 0;
    security_info_.plugin_participant_security_attributes = 0;
#else
    ACE_UNUSED_ARG(resend_period);
#endif
  }

  ParticipantData_t pdata_;

  struct LocationUpdate {
    DCPS::ParticipantLocation mask_;
    ACE_INET_Addr from_;
    SystemTimePoint timestamp_;
    LocationUpdate() {}
    LocationUpdate(DCPS::ParticipantLocation mask,
      const ACE_INET_Addr& from,
      const SystemTimePoint& timestamp)
      : mask_(mask), from_(from), timestamp_(timestamp) {}
  };
  typedef OPENDDS_VECTOR(LocationUpdate) LocationUpdateList;
  LocationUpdateList location_updates_;
  DCPS::ParticipantLocationBuiltinTopicData location_data_;
  DDS::InstanceHandle_t location_ih_;

  ACE_INET_Addr last_recv_address_;
  MonotonicTimePoint discovered_at_;
  MonotonicTimePoint lease_expiration_;
  DDS::InstanceHandle_t bit_ih_;
  SequenceNumber max_seq_;
  ACE_UINT16 seq_reset_count_;
  typedef OPENDDS_LIST(BuiltinAssociationRecord) BuiltinAssociationRecords;
  BuiltinAssociationRecords builtin_pending_records_;
  BuiltinAssociationRecords builtin_associated_records_;
  typedef OPENDDS_LIST(WriterAssociationRecord_rch) WriterAssociationRecords;
  WriterAssociationRecords writer_pending_records_;
  WriterAssociationRecords writer_associated_records_;
  typedef OPENDDS_LIST(ReaderAssociationRecord_rch) ReaderAssociationRecords;
  ReaderAssociationRecords reader_pending_records_;
  ReaderAssociationRecords reader_associated_records_;
#ifdef OPENDDS_SECURITY
  bool have_spdp_info_;
  ICE::AgentInfo spdp_info_;
  bool have_sedp_info_;
  ICE::AgentInfo sedp_info_;
  bool have_auth_req_msg_;
  DDS::Security::ParticipantStatelessMessage auth_req_msg_;
  bool have_handshake_msg_;
  DDS::Security::ParticipantStatelessMessage handshake_msg_;
  DCPS::FibonacciSequence<TimeDuration> handshake_resend_falloff_;
  MonotonicTimePoint stateless_msg_deadline_;

  MonotonicTimePoint handshake_deadline_;
  AuthState auth_state_;
  HandshakeState handshake_state_;
  bool is_requester_;
  CORBA::LongLong auth_req_sequence_number_;
  CORBA::LongLong handshake_sequence_number_;

  DDS::Security::IdentityToken identity_token_;
  DDS::Security::PermissionsToken permissions_token_;
  DDS::Security::PropertyQosPolicy property_qos_;
  DDS::Security::ParticipantSecurityInfo security_info_;
  DDS::Security::IdentityStatusToken identity_status_token_;
  DDS::Security::IdentityHandle identity_handle_;
  DDS::Security::HandshakeHandle handshake_handle_;
  DDS::Security::AuthRequestMessageToken local_auth_request_token_;
  DDS::Security::AuthRequestMessageToken remote_auth_request_token_;
  DDS::Security::AuthenticatedPeerCredentialToken authenticated_peer_credential_token_;
  DDS::Security::SharedSecretHandle_var shared_secret_handle_;
  DDS::Security::PermissionsHandle permissions_handle_;
  DDS::Security::ParticipantCryptoTokenSeq crypto_tokens_;
  DDS::Security::ExtendedBuiltinEndpointSet_t extended_builtin_endpoints_;
  bool participant_tokens_sent_;
#endif
};

class Sedp : public DCPS::RcEventHandler {
private:
  struct DiscoveredSubscription : PoolAllocationBase {
    DiscoveredSubscription()
    : bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#ifdef OPENDDS_SECURITY
    , have_ice_agent_info_(false)
#endif
    {
    }

    explicit DiscoveredSubscription(const DCPS::DiscoveredReaderData& r)
    : reader_data_(r)
    , bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#ifdef OPENDDS_SECURITY
    , security_attribs_(DDS::Security::EndpointSecurityAttributes())
    , have_ice_agent_info_(false)
#endif
    {
    }

    RepoIdSet matched_endpoints_;
    DCPS::DiscoveredReaderData reader_data_;
    DDS::InstanceHandle_t bit_ih_;
    DCPS::MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    XTypes::TypeInformation type_info_;

#ifdef OPENDDS_SECURITY
    DDS::Security::EndpointSecurityAttributes security_attribs_;
    bool have_ice_agent_info_;
    ICE::AgentInfo ice_agent_info_;
#endif

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
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#ifdef OPENDDS_SECURITY
    , have_ice_agent_info_(false)
#endif
    {
    }

    explicit DiscoveredPublication(const DCPS::DiscoveredWriterData& w)
    : writer_data_(w)
    , bit_ih_(DDS::HANDLE_NIL)
    , participant_discovered_at_(DCPS::monotonic_time_zero())
    , transport_context_(0)
#ifdef OPENDDS_SECURITY
    , have_ice_agent_info_(false)
#endif
    {
    }

    RepoIdSet matched_endpoints_;
    DCPS::DiscoveredWriterData writer_data_;
    DDS::InstanceHandle_t bit_ih_;
    DCPS::MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    XTypes::TypeInformation type_info_;

#ifdef OPENDDS_SECURITY
    DDS::Security::EndpointSecurityAttributes security_attribs_;
    bool have_ice_agent_info_;
    ICE::AgentInfo ice_agent_info_;
#endif

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

  void populate_origination_locator(const GUID_t& id, DCPS::TransportLocator& tl);

public:
  Sedp(const DCPS::RepoId& participant_id,
       Spdp& owner,
       ACE_Thread_Mutex& lock);

  ~Sedp();

  DDS::ReturnCode_t init(const DCPS::RepoId& guid,
                         const RtpsDiscovery& disco,
                         DDS::DomainId_t domainId,
                         XTypes::TypeLookupService_rch tls);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t init_security(DDS::Security::IdentityHandle id_handle,
                                  DDS::Security::PermissionsHandle perm_handle,
                                  DDS::Security::ParticipantCryptoHandle crypto_handle);
#endif

  void shutdown();
  DCPS::LocatorSeq unicast_locators() const;
  DCPS::LocatorSeq multicast_locators() const;

  // @brief return the ip address we have bound to.
  // Valid after init() call
  const DCPS::NetworkAddress& local_address() const;
#ifdef ACE_HAS_IPV6
  const DCPS::NetworkAddress& ipv6_local_address() const;
#endif
  const DCPS::NetworkAddress& multicast_group() const;

  void associate(DiscoveredParticipant& participant
#ifdef OPENDDS_SECURITY
                 , const DDS::Security::ParticipantSecurityAttributes& participant_sec_attr
#endif
                 );
  void generate_remote_matched_crypto_handle(const BuiltinAssociationRecord& record);
  bool ready(const DiscoveredParticipant& participant,
             const GUID_t& local_id,
             const GUID_t& remote_id,
             bool local_tokens_sent) const;
  void process_association_records_i(DiscoveredParticipant& participant);
  void generate_remote_matched_crypto_handles(DiscoveredParticipant& participant);

#ifdef OPENDDS_SECURITY
  void disassociate_volatile(DiscoveredParticipant& participant);
  void cleanup_volatile_crypto(const DCPS::RepoId& remote);
  void associate_volatile(DiscoveredParticipant& participant);

  void remove_remote_crypto_handle(const DCPS::RepoId& participant, const EntityId_t& entity);

  void send_builtin_crypto_tokens(const DCPS::RepoId& remoteId);

  /// Create and send keys for individual endpoints.
  void send_builtin_crypto_tokens(const DCPS::RepoId& dst, const DCPS::RepoId& src);

  void resend_user_crypto_tokens(const DCPS::RepoId& remote_participant);
#endif

  bool disassociate(DiscoveredParticipant& participant);

  void update_locators(const ParticipantData_t& pdata);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
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

  void association_complete_i(const DCPS::RepoId& localId,
                              const DCPS::RepoId& remoteId);

  void data_acked_i(const DCPS::RepoId& local_id,
                    const DCPS::RepoId& remote_id);

  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);
  void signal_liveliness_unsecure(DDS::LivelinessQosPolicyKind kind);

  bool send_type_lookup_request(const XTypes::TypeIdentifierSeq& type_ids,
                                const DCPS::RepoId& reader,
                                bool is_discovery_protected, bool send_get_types,
                                const SequenceNumber& seq_num);

#ifdef OPENDDS_SECURITY
  void signal_liveliness_secure(DDS::LivelinessQosPolicyKind kind);
#endif

  DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint();

  void rtps_relay_only_now(bool f);
  void use_rtps_relay_now(bool f);
  void use_ice_now(bool f);
  void rtps_relay_address(const ACE_INET_Addr& address);
  void stun_server_address(const ACE_INET_Addr& address);

  DCPS::ReactorTask_rch reactor_task() const { return reactor_task_; }

  DCPS::JobQueue_rch job_queue() const { return job_queue_; }

  void append_transport_statistics(DCPS::TransportStatisticsSequence& seq);

  void ignore(const GUID_t& to_ignore);

  bool ignoring(const GUID_t& guid) const
  {
    return ignored_guids_.count(guid);
  }
  bool ignoring(const char* topic_name) const
  {
    return ignored_topics_.count(topic_name);
  }

  DCPS::TopicStatus assert_topic(
    GUID_t& topicId, const char* topicName,
    const char* dataTypeName, const DDS::TopicQos& qos,
    bool hasDcpsKey, DCPS::TopicCallbacks* topic_callbacks);

  DCPS::TopicStatus find_topic(
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId);

  DCPS::TopicStatus remove_topic(const GUID_t& topicId);

  GUID_t add_publication(
    const GUID_t& topicId,
    DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const XTypes::TypeInformation& type_info);

  void remove_publication(const GUID_t& publicationId);

  void update_publication_locators(const GUID_t& publicationId,
                                   const DCPS::TransportLocatorSeq& transInfo);

  GUID_t add_subscription(
    const GUID_t& topicId,
    DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const XTypes::TypeInformation& type_info);

  void remove_subscription(const GUID_t& subscriptionId);

  void update_subscription_locators(const GUID_t& subscriptionId,
                                    const DCPS::TransportLocatorSeq& transInfo);

#ifdef OPENDDS_SECURITY
  inline Security::HandleRegistry_rch get_handle_registry() const
  {
    return handle_registry_;
  }
#endif

  void type_lookup_service(const XTypes::TypeLookupService_rch type_lookup_service)
  {
    type_lookup_service_ = type_lookup_service;
  }

  RcHandle<DCPS::TransportInst> transport_inst() const { return transport_inst_; }

private:
  bool remote_knows_about_local_i(const GUID_t& local, const GUID_t& remote) const;
#ifdef OPENDDS_SECURITY
  bool remote_is_authenticated_i(const GUID_t& local, const GUID_t& remote, const DiscoveredParticipant& participant) const;
  bool local_has_remote_participant_token_i(const GUID_t& local, const GUID_t& remote) const;
  bool remote_has_local_participant_token_i(const GUID_t& local, const GUID_t& remote, const DiscoveredParticipant& participant) const;
  bool local_has_remote_endpoint_token_i(const GUID_t& local, const GUID_t& remote) const;
  bool remote_has_local_endpoint_token_i(const GUID_t& local, bool local_tokens_sent,
                                         const GUID_t& remote) const;
#endif

  void type_lookup_init(DCPS::ReactorInterceptor_rch reactor_interceptor)
  {
    if (!type_lookup_reply_deadline_processor_) {
      type_lookup_reply_deadline_processor_ =
        DCPS::make_rch<EndpointManagerSporadic>(TheServiceParticipant->time_source(), reactor_interceptor,
                                                rchandle_from(this), &Sedp::remove_expired_endpoints);
    }
  }

  void type_lookup_fini()
  {
    if (type_lookup_reply_deadline_processor_) {
      type_lookup_reply_deadline_processor_->cancel();
      type_lookup_reply_deadline_processor_.reset();
    }
  }

  void purge_dead_topic(const String& topic_name)
  {
    DCPS::TopicDetailsMap::iterator top_it = topics_.find(topic_name);
    topic_names_.erase(top_it->second.topic_id());
    topics_.erase(top_it);
  }

#ifdef OPENDDS_SECURITY
  void cleanup_secure_writer(const GUID_t& publicationId);
  void cleanup_secure_reader(const GUID_t& subscriptionId);
#endif

  struct LocalEndpoint {
    LocalEndpoint()
      : topic_id_(GUID_UNKNOWN)
      , participant_discovered_at_(DCPS::monotonic_time_zero())
      , transport_context_(0)
      , sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
#ifdef OPENDDS_SECURITY
      , have_ice_agent_info(false)
    {
      security_attribs_.base.is_read_protected = false;
      security_attribs_.base.is_write_protected = false;
      security_attribs_.base.is_discovery_protected = false;
      security_attribs_.base.is_liveliness_protected = false;
      security_attribs_.is_submessage_protected = false;
      security_attribs_.is_payload_protected = false;
      security_attribs_.is_key_protected = false;
      security_attribs_.plugin_endpoint_attributes = 0;
    }
#else
    {}
#endif

    GUID_t topic_id_;
    DCPS::TransportLocatorSeq trans_info_;
    DCPS::MonotonicTime_t participant_discovered_at_;
    ACE_CDR::ULong transport_context_;
    RepoIdSet matched_endpoints_;
    SequenceNumber sequence_;
    RepoIdSet remote_expectant_opendds_associations_;
    XTypes::TypeInformation type_info_;
#ifdef OPENDDS_SECURITY
    bool have_ice_agent_info;
    ICE::AgentInfo ice_agent_info;
    DDS::Security::EndpointSecurityAttributes security_attribs_;
#endif
  };

  struct LocalPublication : LocalEndpoint {
    DCPS::DataWriterCallbacks_wrch publication_;
    DDS::DataWriterQos qos_;
    DDS::PublisherQos publisher_qos_;
  };

  struct LocalSubscription : LocalEndpoint {
    DCPS::DataReaderCallbacks_wrch subscription_;
    DDS::DataReaderQos qos_;
    DDS::SubscriberQos subscriber_qos_;
    DCPS::ContentFilterProperty_t filterProperties;
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
    {
      return true;
    }

    DCPS::RepoId get_repo_id() const
    {
      return repo_id_;
    }

    DDS::DomainId_t domain_id() const
    {
      return 0; // not used for SEDP
    }

    CORBA::Long get_priority_value(const DCPS::AssociationData&) const
    {
      return 0;
    }

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

    DDS::Subscriber_var get_builtin_subscriber() const;

    // Return instance_name field in RPC type lookup request for a given RepoId
    // (as per chapter 7.6.3.3.4 of XTypes spec)
    OPENDDS_STRING get_instance_name(const DCPS::RepoId& id) const
    {
      const DCPS::RepoId participant = make_id(id, ENTITYID_PARTICIPANT);
      return OPENDDS_STRING("dds.builtin.TOS.") +
        DCPS::to_hex_dds_string(&participant.guidPrefix[0], sizeof(DCPS::GuidPrefix_t)) +
        DCPS::to_hex_dds_string(&participant.entityId.entityKey[0], sizeof(DCPS::EntityKey_t)) +
        DCPS::to_dds_string(unsigned(participant.entityId.entityKind), true);
    }

    EntityId_t counterpart_entity_id() const;
    GUID_t make_counterpart_guid(const DCPS::GUID_t& remote_part) const;
    bool associated_with_counterpart(const DCPS::GUID_t& remote_part) const;
    bool pending_association_with_counterpart(const DCPS::GUID_t& remote_part) const;
    bool associated_with_counterpart_if_not_pending(const DCPS::GUID_t& remote_part) const;

  protected:
    DCPS::RepoId repo_id_;
    Sedp& sedp_;
#ifdef ACE_HAS_CPP11
    std::atomic<bool> shutting_down_;
#else
    ACE_Atomic_Op<ACE_Thread_Mutex, bool> shutting_down_;
#endif
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

    void data_acked(const GUID_t& remote);


    void control_delivered(const DCPS::Message_Block_Ptr& sample);

    void control_dropped(const DCPS::Message_Block_Ptr& sample,
                         bool dropped_by_transport);

    void notify_publication_disconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const DCPS::ReaderIdSeq&) {}
    void remove_associations(const DCPS::ReaderIdSeq&, bool) {}
    void replay_durable_data_for(const DCPS::RepoId& remote_sub_id);
    void retrieve_inline_qos_data(InlineQosData&) const {}

    const DCPS::SequenceNumber& get_seq() const
    {
      return seq_;
    }

    DDS::ReturnCode_t write_parameter_list(const ParameterList& plist,
      const DCPS::RepoId& reader,
      DCPS::SequenceNumber& sequence);

    void end_historic_samples(const DCPS::RepoId& reader);
    void request_ack(const DCPS::RepoId& reader);

    void send_deferred_samples(const GUID_t& reader);

  protected:
    typedef OPENDDS_MAP(DCPS::SequenceNumber, DCPS::DataSampleElement*)
      PerReaderDeferredSamples;
    typedef OPENDDS_MAP(GUID_t, PerReaderDeferredSamples) DeferredSamples;
    DeferredSamples deferred_samples_;

    void send_sample(DCPS::Message_Block_Ptr payload,
                     size_t size,
                     const DCPS::RepoId& reader,
                     DCPS::SequenceNumber& sequence,
                     bool historic = false);

    void send_sample_i(DCPS::DataSampleElement* el);

    void set_header_fields(DCPS::DataSampleHeader& dsh,
                           size_t size,
                           const DCPS::RepoId& reader,
                           DCPS::SequenceNumber& sequence,
                           bool historic_sample = false,
                           DCPS::MessageId id = DCPS::SAMPLE_DATA);

    void write_control_msg(DCPS::Message_Block_Ptr payload,
      size_t size,
      DCPS::MessageId id,
      DCPS::SequenceNumber seq = DCPS::SequenceNumber());

    virtual bool deferrable() const
    {
      return false;
    }

    DCPS::SequenceNumber seq_;
  };

  class SecurityWriter : public Writer {
  public:
    SecurityWriter(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1)
      : Writer(pub_id, sedp, seq_init)
    {}

    virtual ~SecurityWriter();

#ifdef OPENDDS_SECURITY
    DDS::ReturnCode_t write_stateless_message(const DDS::Security::ParticipantStatelessMessage& msg,
                                              const DCPS::RepoId& reader,
                                              DCPS::SequenceNumber& sequence);

    DDS::ReturnCode_t write_volatile_message_secure(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                                    const DCPS::RepoId& reader,
                                                    DCPS::SequenceNumber& sequence);
#endif
  };

  typedef DCPS::RcHandle<SecurityWriter> SecurityWriter_rch;


  class LivelinessWriter : public Writer {
  public:
    LivelinessWriter(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1)
      : Writer(pub_id, sedp, seq_init)
    {}

    virtual ~LivelinessWriter();

    DDS::ReturnCode_t write_participant_message(const ParticipantMessageData& pmd,
      const DCPS::RepoId& reader,
      DCPS::SequenceNumber& sequence);
  };

  typedef DCPS::RcHandle<LivelinessWriter> LivelinessWriter_rch;

  class DiscoveryWriter : public Writer {
  public:
    DiscoveryWriter(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1)
      : Writer(pub_id, sedp, seq_init)
    {}

    virtual ~DiscoveryWriter();

#ifdef OPENDDS_SECURITY
    DDS::ReturnCode_t write_dcps_participant_secure(const Security::SPDPdiscoveredParticipantData& msg,
                                                    const DCPS::RepoId& reader, DCPS::SequenceNumber& sequence);
#endif

    DDS::ReturnCode_t write_unregister_dispose(const DCPS::RepoId& rid, CORBA::UShort pid = PID_ENDPOINT_GUID);
  };

  typedef DCPS::RcHandle<DiscoveryWriter> DiscoveryWriter_rch;

  class TypeLookupRequestWriter : public Writer {
  public:
    TypeLookupRequestWriter(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1)
      : Writer(pub_id, sedp, seq_init)
    {}

    virtual ~TypeLookupRequestWriter();

    bool send_type_lookup_request(
      const XTypes::TypeIdentifierSeq& type_ids,
      const DCPS::RepoId& reader,
      const DCPS::SequenceNumber& rpc_sequence,
      CORBA::ULong tl_kind);

  protected:
    virtual bool deferrable() const
    {
      return true;
    }
  };

  typedef DCPS::RcHandle<TypeLookupRequestWriter> TypeLookupRequestWriter_rch;

  class TypeLookupReplyWriter : public Writer {
  public:
    TypeLookupReplyWriter(const DCPS::RepoId& pub_id, Sedp& sedp, ACE_INT64 seq_init = 1)
      : Writer(pub_id, sedp, seq_init)
    {}

    virtual ~TypeLookupReplyWriter();

    bool send_type_lookup_reply(
      XTypes::TypeLookup_Reply& type_lookup_reply,
      const DCPS::RepoId& reader);

  protected:
    virtual bool deferrable() const
    {
      return true;
    }
  };

  typedef DCPS::RcHandle<TypeLookupReplyWriter> TypeLookupReplyWriter_rch;

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

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility) = 0;
  };

  typedef DCPS::RcHandle<Reader> Reader_rch;

  class DiscoveryReader : public Reader {
  public:
    DiscoveryReader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Reader(sub_id, sedp)
    {}

    virtual ~DiscoveryReader();

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility);
  };

  typedef DCPS::RcHandle<DiscoveryReader> DiscoveryReader_rch;

  class LivelinessReader : public Reader {
  public:
    LivelinessReader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Reader(sub_id, sedp)
    {}

    virtual ~LivelinessReader();

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility);
  };

  typedef DCPS::RcHandle<LivelinessReader> LivelinessReader_rch;

  class SecurityReader : public Reader {
  public:
    SecurityReader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Reader(sub_id, sedp)
    {}

    virtual ~SecurityReader();

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility);
  };

  typedef DCPS::RcHandle<SecurityReader> SecurityReader_rch;

  class TypeLookupRequestReader : public Reader {
  public:
    TypeLookupRequestReader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Reader(sub_id, sedp)
    {
      instance_name_ = get_instance_name(sub_id);
    }

    virtual ~TypeLookupRequestReader();

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility);

    bool process_type_lookup_request(DCPS::Serializer& ser,
      XTypes::TypeLookup_Reply& type_lookup_reply);

    bool process_get_types_request(const XTypes::TypeLookup_Request& type_lookup_request,
      XTypes::TypeLookup_Reply& type_lookup_reply);

    bool process_get_dependencies_request(const XTypes::TypeLookup_Request& request,
      XTypes::TypeLookup_Reply& reply);

    void gen_continuation_point(XTypes::OctetSeq32& cont_point) const;

    // The instance name of the local participant
    OPENDDS_STRING instance_name_;
  };

  typedef DCPS::RcHandle<TypeLookupRequestReader> TypeLookupRequestReader_rch;

  class TypeLookupReplyReader : public Reader {
  public:
    TypeLookupReplyReader(const DCPS::RepoId& sub_id, Sedp& sedp)
      : Reader(sub_id, sedp)
    {}

    virtual ~TypeLookupReplyReader();

    void get_continuation_point(const GuidPrefix_t& guid_prefix,
                                const XTypes::TypeIdentifier& remote_ti,
                                XTypes::OctetSeq32& cont_point) const;

    void cleanup(const DCPS::GuidPrefix_t& guid_prefix, const XTypes::TypeIdentifier& ti);

  private:
    virtual void data_received_i(const DCPS::ReceivedDataSample& sample,
      const DCPS::EntityId_t& entity_id,
      DCPS::Serializer& ser,
      DCPS::Extensibility extensibility);

    bool process_type_lookup_reply(
      const DCPS::ReceivedDataSample&, DCPS::Serializer& ser, bool is_discovery_protected);
    bool process_get_types_reply(const XTypes::TypeLookup_Reply& reply);
    bool process_get_dependencies_reply(
      const DCPS::ReceivedDataSample& sample, const XTypes::TypeLookup_Reply& reply,
      const DCPS::SequenceNumber& seq_num, bool is_discovery_protected);

    typedef std::pair<XTypes::OctetSeq32, XTypes::TypeIdentifierSeq> ContinuationPair;

    // Map from each remote type to the most recent continuation_point
    // and all dependencies received for that type so far.
    typedef OPENDDS_MAP(XTypes::TypeIdentifier, ContinuationPair) RemoteDependencies;

    // NOTE(sonndinh): We can later replace this with GUID_t as the key of DependenciesMap.
    struct GuidPrefixWrapper {
      GuidPrefixWrapper(const GuidPrefix_t& prefix)
      {
        DCPS::assign(prefix_, prefix);
      }

      bool operator<(const GuidPrefixWrapper& other) const
      {
        return std::memcmp(&prefix_[0], &other.prefix_[0], sizeof(GuidPrefix_t)) < 0;
      }

      GuidPrefix_t prefix_;
    };

    // Map from each remote participant to the data stored for its types.
    typedef OPENDDS_MAP(GuidPrefixWrapper, RemoteDependencies) DependenciesMap;
    DependenciesMap dependencies_;
  };

  typedef DCPS::RcHandle<TypeLookupReplyReader> TypeLookupReplyReader_rch;

  void cleanup_type_lookup_data(const DCPS::GuidPrefix_t& guid_prefix,
                                const XTypes::TypeIdentifier& ti,
                                bool secure);

  // Transport
  DCPS::TransportInst_rch transport_inst_;
  DCPS::TransportConfig_rch transport_cfg_;
  DCPS::ReactorTask_rch reactor_task_;
  DCPS::JobQueue_rch job_queue_;

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
#ifdef OPENDDS_SECURITY
  LocalParticipantMessageMap local_participant_messages_secure_;
#endif

  void process_discovered_writer_data(DCPS::MessageId message_id,
                                      const DCPS::DiscoveredWriterData& wdata,
                                      const DCPS::RepoId& guid,
                                      const XTypes::TypeInformation& type_info
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
                                      const DCPS::RepoId& guid,
                                      const XTypes::TypeInformation& type_info
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

  /// This is a function to unify the notification of liveliness within RTPS
  /// The local participant map is checked for associated entities and then they are notified
  /// of liveliness if their QoS is compatible
  void notify_liveliness(const ParticipantMessageData& pmd);

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

  void assign_bit_key(DiscoveredPublication& pub);
  void assign_bit_key(DiscoveredSubscription& sub);

  template<typename Map>
  void remove_entities_belonging_to(Map& m, DCPS::RepoId participant, bool subscription, OPENDDS_VECTOR(typename Map::mapped_type)& to_remove_from_bit);

  void remove_from_bit_i(const DiscoveredPublication& pub);
  void remove_from_bit_i(const DiscoveredSubscription& sub);

  virtual DDS::ReturnCode_t remove_publication_i(const DCPS::RepoId& publicationId, LocalPublication& pub);
  virtual DDS::ReturnCode_t remove_subscription_i(const DCPS::RepoId& subscriptionId, LocalSubscription& sub);

  // Topic:

  // FURTURE: Remove this member.
  DCPS::RepoIdSet associated_participants_;

  virtual bool shutting_down() const;

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq& tls,
                                                   DiscoveredSubscriptionIter& iter,
                                                   const DCPS::RepoId& reader);

  virtual void populate_transport_locator_sequence(DCPS::TransportLocatorSeq& tls,
                                                   DiscoveredPublicationIter& iter,
                                                   const DCPS::RepoId& writer);

  static void set_inline_qos(DCPS::TransportLocatorSeq& locators);

  void write_durable_publication_data(const DCPS::RepoId& reader, bool secure);
  void write_durable_subscription_data(const DCPS::RepoId& reader, bool secure);

  void write_durable_participant_message_data(const DCPS::RepoId& reader);

#ifdef OPENDDS_SECURITY
  void write_durable_participant_message_data_secure(const DCPS::RepoId& reader);
#endif

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
                                            LocalSubscription& sub,
                                            const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_subscription_data_secure(const DCPS::RepoId& rid,
                                                   LocalSubscription& sub,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
#endif

  DDS::ReturnCode_t write_subscription_data_unsecure(const DCPS::RepoId& rid,
                                                     LocalSubscription& sub,
                                                     const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);

  DDS::ReturnCode_t write_participant_message_data(const DCPS::RepoId& rid,
                                                   LocalParticipantMessage& part,
                                                   const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t write_participant_message_data_secure(const DCPS::RepoId& rid,
                                                          LocalParticipantMessage& part,
                                                          const DCPS::RepoId& reader = DCPS::GUID_UNKNOWN);
#endif

  virtual bool is_expectant_opendds(const GUID_t& endpoint) const;

protected:

#ifdef OPENDDS_SECURITY
  DDS::Security::DatawriterCryptoHandle
  generate_remote_matched_writer_crypto_handle(const DCPS::RepoId& writer,
                                               const DCPS::RepoId& reader);
  DDS::Security::DatareaderCryptoHandle
  generate_remote_matched_reader_crypto_handle(const DCPS::RepoId& reader,
                                               const DCPS::RepoId& writer,
                                               bool relay_only);

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

  bool handle_datareader_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
  bool handle_datawriter_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);

  DDS::DomainId_t get_domain_id() const;

  struct PublicationAgentInfoListener : public ICE::AgentInfoListener
  {
    Sedp& sedp;
    PublicationAgentInfoListener(Sedp& a_sedp) : sedp(a_sedp) {}
    void update_agent_info(const DCPS::RepoId& a_local_guid,
                           const ICE::AgentInfo& a_agent_info);
    void remove_agent_info(const DCPS::RepoId& a_local_guid);
  };

  struct SubscriptionAgentInfoListener : public ICE::AgentInfoListener
  {
    Sedp& sedp;
    SubscriptionAgentInfoListener(Sedp& a_sedp) : sedp(a_sedp) {}
    void update_agent_info(const DCPS::RepoId& a_local_guid,
                           const ICE::AgentInfo& a_agent_info);
    void remove_agent_info(const DCPS::RepoId& a_local_guid);
  };

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

  void replay_durable_data_for(const DCPS::RepoId& remote_sub_id);

  static DDS::BuiltinTopicKey_t get_key(const DiscoveredPublication& pub)
  {
    return pub.writer_data_.ddsPublicationData.key;
  }
  static DDS::BuiltinTopicKey_t get_key(const DiscoveredSubscription& sub)
  {
    return sub.reader_data_.ddsSubscriptionData.key;
  }

  virtual void assign_publication_key(GUID_t& rid,
                                      const GUID_t& topicId,
                                      const DDS::DataWriterQos& /*qos*/)
  {
    rid.entityId.entityKind =
      has_dcps_key(topicId)
      ? DCPS::ENTITYKIND_USER_WRITER_WITH_KEY
      : DCPS::ENTITYKIND_USER_WRITER_NO_KEY;
    assign(rid.entityId.entityKey, publication_counter_++);
  }
  virtual void assign_subscription_key(GUID_t& rid,
                                       const GUID_t& topicId,
                                       const DDS::DataReaderQos& /*qos*/)
  {
    rid.entityId.entityKind =
      has_dcps_key(topicId)
      ? DCPS::ENTITYKIND_USER_READER_WITH_KEY
      : DCPS::ENTITYKIND_USER_READER_NO_KEY;
    assign(rid.entityId.entityKey, subscription_counter_++);
  }
  virtual void assign_topic_key(GUID_t& guid)
  {
    assign(guid.entityId.entityKey, topic_counter_++);

    if (topic_counter_ == 0x1000000) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Sedp::assign_topic_key: ")
                 ACE_TEXT("Exceeded Maximum number of topic entity keys!")
                 ACE_TEXT("Next key will be a duplicate!\n")));
      topic_counter_ = 0;
    }
  }

  void match_endpoints(GUID_t repoId, const DCPS::TopicDetails& td,
                       bool remove = false);

  void remove_assoc(const GUID_t& remove_from, const GUID_t& removing);

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

  typedef DCPS::PmfSporadicTask<Sedp> EndpointManagerSporadic;

  void match(const GUID_t& writer, const GUID_t& reader);

  void need_minimal_and_or_complete_types(const XTypes::TypeInformation* type_info,
                                          bool& need_minimal,
                                          bool& need_complete) const;

  void remove_expired_endpoints(const MonotonicTimePoint& /*now*/);

  void match_continue(const GUID_t& writer, const GUID_t& reader);

  void save_matching_data_and_get_typeobjects(const XTypes::TypeInformation* type_info,
                                              MatchingData& md, const MatchingPair& mp,
                                              const DCPS::RepoId& remote_id,
                                              bool is_discovery_protected,
                                              bool get_minimal, bool get_complete);
  void get_remote_type_objects(const XTypes::TypeIdentifierWithDependencies& tid_with_deps,
                               MatchingData& md, bool get_minimal, const DCPS::RepoId& remote_id,
                               bool is_discovery_protected);
#ifdef OPENDDS_SECURITY
  void match_continue_security_enabled(
    const GUID_t& writer, const GUID_t& reader, bool call_writer, bool call_reader);
#endif

  void remove_from_bit(const DiscoveredPublication& pub)
  {
    remove_from_bit_i(pub);
  }

  void remove_from_bit(const DiscoveredSubscription& sub)
  {
    remove_from_bit_i(sub);
  }

  GUID_t make_topic_guid()
  {
    GUID_t guid;
    guid = participant_id_;
    guid.entityId.entityKind = DCPS::ENTITYKIND_OPENDDS_TOPIC;
    assign_topic_key(guid);
    return guid;
  }

  bool has_dcps_key(const GUID_t& topicId) const
  {
    typedef OPENDDS_MAP_CMP(GUID_t, String, GUID_tKeyLessThan) TNMap;
    TNMap::const_iterator tn = topic_names_.find(topicId);
    if (tn == topic_names_.end()) return false;

    DCPS::TopicDetailsMap::const_iterator td = topics_.find(tn->second);
    if (td == topics_.end()) return false;

    return td->second.has_dcps_key();
  }

#ifdef OPENDDS_SECURITY
  inline bool is_security_enabled()
  {
    return (permissions_handle_ != DDS::HANDLE_NIL) && (access_control_ != 0);
  }

  inline void set_permissions_handle(DDS::Security::PermissionsHandle h)
  {
    permissions_handle_ = h;
  }

  inline DDS::Security::PermissionsHandle get_permissions_handle() const
  {
    return permissions_handle_;
  }

  inline void set_access_control(DDS::Security::AccessControl_var acl)
  {
    access_control_ = acl;
  }

  inline DDS::Security::AccessControl_var get_access_control() const
  {
    return access_control_;
  }

  inline void set_crypto_key_factory(DDS::Security::CryptoKeyFactory_var ckf)
  {
    crypto_key_factory_ = ckf;
  }

  inline DDS::Security::CryptoKeyFactory_var get_crypto_key_factory() const
  {
    return crypto_key_factory_;
  }

  inline void set_crypto_key_exchange(DDS::Security::CryptoKeyExchange_var ckf)
  {
    crypto_key_exchange_ = ckf;
  }

  inline DDS::Security::CryptoKeyExchange_var get_crypto_key_exchange() const
  {
    return crypto_key_exchange_;
  }

  inline void set_handle_registry(const Security::HandleRegistry_rch& hr)
  {
    handle_registry_ = hr;
  }
#endif

  Spdp& spdp_;
  ACE_Thread_Mutex& lock_;
  GUID_t participant_id_;
  RepoIdSet ignored_guids_;
  unsigned int publication_counter_, subscription_counter_, topic_counter_;
  LocalPublicationMap local_publications_;
  LocalSubscriptionMap local_subscriptions_;
  DiscoveredPublicationMap discovered_publications_;
  DiscoveredSubscriptionMap discovered_subscriptions_;
  DCPS::TopicDetailsMap topics_;
  TopicNameMap topic_names_;
  OPENDDS_SET(String) ignored_topics_;
  OPENDDS_SET_CMP(GUID_t, GUID_tKeyLessThan) relay_only_readers_;
  XTypes::TypeLookupService_rch type_lookup_service_;
  OrigSeqNumberMap orig_seq_numbers_;
  MatchingDataMap matching_data_buffer_;
  RcHandle<EndpointManagerSporadic> type_lookup_reply_deadline_processor_;
  TimeDuration max_type_lookup_service_reply_period_;
  DCPS::SequenceNumber type_lookup_service_sequence_number_;
  const bool use_xtypes_;
  const bool use_xtypes_complete_;

#ifdef OPENDDS_SECURITY
  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;

  DDS::Security::AccessControl_var access_control_;
  DDS::Security::CryptoKeyFactory_var crypto_key_factory_;
  DDS::Security::CryptoKeyExchange_var crypto_key_exchange_;
  Security::HandleRegistry_rch handle_registry_;

  DDS::Security::PermissionsHandle permissions_handle_;
  DDS::Security::ParticipantCryptoHandle crypto_handle_;

  DatareaderCryptoTokenSeqMap pending_remote_reader_crypto_tokens_;
  DatawriterCryptoTokenSeqMap pending_remote_writer_crypto_tokens_;

  SequenceNumber participant_secure_sequence_;
#endif

  DiscoveryWriter_rch publications_writer_;
#ifdef OPENDDS_SECURITY
  DiscoveryWriter_rch publications_secure_writer_;
#endif
  DiscoveryWriter_rch subscriptions_writer_;
#ifdef OPENDDS_SECURITY
  DiscoveryWriter_rch subscriptions_secure_writer_;
#endif
  LivelinessWriter_rch participant_message_writer_;
#ifdef OPENDDS_SECURITY
  LivelinessWriter_rch participant_message_secure_writer_;
  SecurityWriter_rch participant_stateless_message_writer_;
  DiscoveryWriter_rch dcps_participant_secure_writer_;
  friend class Spdp;
  SecurityWriter_rch participant_volatile_message_secure_writer_;
#endif
  TypeLookupRequestWriter_rch type_lookup_request_writer_;
  TypeLookupReplyWriter_rch type_lookup_reply_writer_;
#ifdef OPENDDS_SECURITY
  TypeLookupRequestWriter_rch type_lookup_request_secure_writer_;
  TypeLookupReplyWriter_rch type_lookup_reply_secure_writer_;
#endif
  DiscoveryReader_rch publications_reader_;
#ifdef OPENDDS_SECURITY
  DiscoveryReader_rch publications_secure_reader_;
#endif
  DiscoveryReader_rch subscriptions_reader_;
#ifdef OPENDDS_SECURITY
  DiscoveryReader_rch subscriptions_secure_reader_;
#endif
  LivelinessReader_rch participant_message_reader_;
#ifdef OPENDDS_SECURITY
  LivelinessReader_rch participant_message_secure_reader_;
  SecurityReader_rch participant_stateless_message_reader_;
  SecurityReader_rch participant_volatile_message_secure_reader_;
  DiscoveryReader_rch dcps_participant_secure_reader_;
#endif
  TypeLookupRequestReader_rch type_lookup_request_reader_;
  TypeLookupReplyReader_rch type_lookup_reply_reader_;
#ifdef OPENDDS_SECURITY
  TypeLookupRequestReader_rch type_lookup_request_secure_reader_;
  TypeLookupReplyReader_rch type_lookup_reply_secure_reader_;
#endif

#ifdef OPENDDS_SECURITY
  DCPS::RcHandle<ICE::Agent> ice_agent_;
  RcHandle<PublicationAgentInfoListener> publication_agent_info_listener_;
  RcHandle<SubscriptionAgentInfoListener> subscription_agent_info_listener_;
#endif

  void cleanup_writer_association(DCPS::DataWriterCallbacks_wrch callbacks,
                                  const GUID_t& writer,
                                  const GUID_t& reader);

  void cleanup_reader_association(DCPS::DataReaderCallbacks_wrch callbacks,
                                  const GUID_t& reader,
                                  const GUID_t& writer);


  class WriterAddAssociation : public DCPS::JobQueue::Job {
  public:
    explicit WriterAddAssociation(const WriterAssociationRecord_rch& record)
      : record_(record)
    {}

  private:
    virtual void execute();

    const WriterAssociationRecord_rch record_;
  };

  class WriterRemoveAssociations : public DCPS::JobQueue::Job {
  public:
    explicit WriterRemoveAssociations(const WriterAssociationRecord_rch& record)
      : record_(record)
    {}

  private:
    virtual void execute();

    const WriterAssociationRecord_rch record_;
  };

  class ReaderAddAssociation : public DCPS::JobQueue::Job {
  public:
    explicit ReaderAddAssociation(const ReaderAssociationRecord_rch& record)
      : record_(record)
    {}

  private:
    virtual void execute();

    const ReaderAssociationRecord_rch record_;
  };

  class ReaderRemoveAssociations : public DCPS::JobQueue::Job {
  public:
    explicit ReaderRemoveAssociations(const ReaderAssociationRecord_rch& record)
      : record_(record)
    {}

  private:
    virtual void execute();

    const ReaderAssociationRecord_rch record_;
  };

};

bool locators_changed(const ParticipantProxy_t& x,
                      const ParticipantProxy_t& y);

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SEDP_H
