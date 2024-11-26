/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_SPDP_H
#define OPENDDS_DCPS_RTPS_SPDP_H

#include "Sedp.h"
#include "rtps_export.h"
#include "ICE/Ice.h"
#include "RtpsCoreC.h"

#include <dds/DCPS/AtomicBool.h>
#include <dds/DCPS/BuiltInTopicDataReaderImpls.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/Discovery.h>
#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/JobQueue.h>
#include <dds/DCPS/MultiTask.h>
#include <dds/DCPS/MulticastManager.h>
#include <dds/DCPS/PeriodicTask.h>
#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/RcEventHandler.h>
#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/ReactorTask.h>
#include <dds/DCPS/SporadicTask.h>
#include <dds/DCPS/TimeTypes.h>

#include <dds/DCPS/security/framework/SecurityConfig_rch.h>
#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/security/framework/SecurityConfig.h>
#endif

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <ace/SOCK_Dgram.h>
#include <ace/SOCK_Dgram_Mcast.h>
#include <ace/Thread_Mutex.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class RtpsDiscoveryConfig;
class RtpsDiscovery;

const char SPDP_AGENT_INFO_KEY[] = "SPDP";
const char SEDP_AGENT_INFO_KEY[] = "SEDP";

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class OpenDDS_Rtps_Export Spdp
  : public virtual DCPS::RcObject
#if OPENDDS_CONFIG_SECURITY
  , public virtual ICE::AgentInfoListener
#endif
{
public:
  typedef OPENDDS_MAP_CMP(GUID_t, DiscoveredParticipant,
                          GUID_tKeyLessThan) DiscoveredParticipantMap;
  typedef DiscoveredParticipantMap::iterator DiscoveredParticipantIter;
  typedef DiscoveredParticipantMap::const_iterator DiscoveredParticipantConstIter;


  Spdp(DDS::DomainId_t domain,
       DCPS::GUID_t& guid,
       const DDS::DomainParticipantQos& qos,
       RtpsDiscovery* disco,
       XTypes::TypeLookupService_rch tls);

#if OPENDDS_CONFIG_SECURITY
  Spdp(DDS::DomainId_t domain,
       const DCPS::GUID_t& guid,
       const DDS::DomainParticipantQos& qos,
       RtpsDiscovery* disco,
       XTypes::TypeLookupService_rch tls,
       DDS::Security::IdentityHandle id_handle,
       DDS::Security::PermissionsHandle perm_handle,
       DDS::Security::ParticipantCryptoHandle crypto_handle);
#endif

  ~Spdp();

  // Participant
  const DCPS::GUID_t& guid() const { return guid_; }
  void init_bit(RcHandle<DCPS::BitSubscriber> bit_subscriber);
  void fini_bit();

  bool get_default_locators(const DCPS::GUID_t& part_id,
                            DCPS::LocatorSeq& target,
                            bool& inlineQos);

  bool get_last_recv_locator(const DCPS::GUID_t& part_id,
                             DCPS::LocatorSeq& target,
                             bool& inlineQos);

  // Managing reader/writer associations
  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  // Is Spdp fully initialized?
  bool initialized()
  {
    return initialized_flag_;
  }

  void shutdown();

  // Is Spdp shutting down?
  bool shutting_down()
  {
    return shutdown_flag_;
  }

  bool associated() const;
  bool has_discovered_participant(const DCPS::GUID_t& guid) const;
  ACE_CDR::ULong get_participant_flags(const DCPS::GUID_t& guid) const;

#if OPENDDS_CONFIG_SECURITY
  Security::SecurityConfig_rch get_security_config() const { return security_config_; }
  DDS::Security::ParticipantCryptoHandle crypto_handle() const { return crypto_handle_; }
  DDS::Security::ParticipantCryptoHandle remote_crypto_handle(const DCPS::GUID_t& remote_participant) const;

  void handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg);
  void send_handshake_request(const DCPS::GUID_t& guid, DiscoveredParticipant& dp);
  void handle_handshake_message(const DDS::Security::ParticipantStatelessMessage& msg);
  bool handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
  DDS::OctetSeq local_participant_data_as_octets() const;
#endif

  void handle_participant_data(DCPS::MessageId id,
                               const ParticipantData_t& pdata,
                               const DCPS::MonotonicTimePoint& now,
                               const DCPS::SequenceNumber& seq,
                               const DCPS::NetworkAddress& from,
                               bool from_sedp);

  bool validateSequenceNumber(const DCPS::MonotonicTimePoint& now, const DCPS::SequenceNumber& seq, DiscoveredParticipantIter& iter);

#if OPENDDS_CONFIG_SECURITY
  void process_handshake_deadlines(const DCPS::MonotonicTimePoint& tv);
  void process_handshake_resends(const DCPS::MonotonicTimePoint& tv);

  /**
   * Write Secured Updated DP QOS
   *
   * lock_ must be acquired before calling this.
   */
  void write_secure_updates();
  void write_secure_disposes();
  bool is_security_enabled() const { return security_enabled_; }
#endif

  bool is_expectant_opendds(const GUID_t& participant) const;

#if OPENDDS_CONFIG_SECURITY
  typedef std::pair<DDS::Security::ParticipantCryptoHandle, DDS::Security::SharedSecretHandle_var> ParticipantCryptoInfoPair;
  ParticipantCryptoInfoPair lookup_participant_crypto_info(const DCPS::GUID_t& id) const;
  void send_participant_crypto_tokens(const DCPS::GUID_t& id);

  DDS::Security::PermissionsHandle lookup_participant_permissions(const DCPS::GUID_t& id) const;

  AuthState lookup_participant_auth_state(const GUID_t& id) const;

  void process_participant_ice(const ParameterList& plist,
                               const ParticipantData_t& pdata,
                               const DCPS::GUID_t& guid);
#endif

  DDS::DomainId_t get_domain_id() const { return domain_; }
  const ParticipantData_t& get_participant_data(const DCPS::GUID_t& guid) const;
  ParticipantData_t& get_participant_data(const DCPS::GUID_t& guid);
  DCPS::MonotonicTime_t get_participant_discovered_at() const;
  DCPS::MonotonicTime_t get_participant_discovered_at(const DCPS::GUID_t& guid) const;

  u_short get_spdp_port() const { return tport_ ? tport_->uni_port_ : 0; }

  u_short get_sedp_port() const { return sedp_->local_address().get_port_number(); }

#ifdef ACE_HAS_IPV6
  u_short get_ipv6_spdp_port() const { return tport_ ? tport_->ipv6_uni_port_ : 0; }

  u_short get_ipv6_sedp_port() const { return sedp_->ipv6_local_address().get_port_number(); }
#endif

  BuiltinEndpointSet_t available_builtin_endpoints() const { return available_builtin_endpoints_; }
#if OPENDDS_CONFIG_SECURITY
  DDS::Security::ExtendedBuiltinEndpointSet_t available_extended_builtin_endpoints() const
  {
    return available_extended_builtin_endpoints_;
  }
#endif

  DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint_if_added();

  ParticipantData_t build_local_pdata(
#if OPENDDS_CONFIG_SECURITY
    bool always_in_the_clear,
    Security::DiscoveredParticipantDataKind kind
#endif
  );

  DCPS::RcHandle<RtpsDiscoveryConfig> config() const { return config_; }

  void append_transport_statistics(DCPS::TransportStatisticsSequence& seq);

  VendorId_t get_vendor_id(const GUID_t& guid) const;

  VendorId_t get_vendor_id_i(const GUID_t& guid) const;

  void ignore_domain_participant(const GUID_t& ignoreId);

  void remove_domain_participant(const GUID_t& removeId);

  bool update_domain_participant_qos(const DDS::DomainParticipantQos& qos);

  bool enable_flexible_types(const GUID_t& remoteParticipantId, const char* typeKey);
  DCPS::String find_flexible_types_key_i(const GUID_t& remoteEndpointId);

  bool has_domain_participant(const GUID_t& ignoreId) const;

  DCPS::TopicStatus assert_topic(GUID_t& topicId, const char* topicName,
    const char* dataTypeName, const DDS::TopicQos& qos,
    bool hasDcpsKey, DCPS::TopicCallbacks* topic_callbacks);

  DCPS::TopicStatus find_topic(
    const char* topicName,
    CORBA::String_out dataTypeName,
    DDS::TopicQos_out qos,
    GUID_t& topicId)
  {
    return endpoint_manager().find_topic(topicName, dataTypeName, qos, topicId);
  }

  DCPS::TopicStatus remove_topic(const GUID_t& topicId)
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

  bool add_publication(
    const GUID_t& topicId,
    DCPS::DataWriterCallbacks_rch publication,
    const DDS::DataWriterQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::PublisherQos& publisherQos,
    const DCPS::TypeInformation& type_info)
  {
    return endpoint_manager().add_publication(topicId, publication, qos, transInfo, publisherQos, type_info);
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
                                   const DCPS::TransportLocatorSeq& transInfo)
  {
    endpoint_manager().update_publication_locators(publicationId, transInfo);
  }

  bool add_subscription(
    const GUID_t& topicId,
    DCPS::DataReaderCallbacks_rch subscription,
    const DDS::DataReaderQos& qos,
    const DCPS::TransportLocatorSeq& transInfo,
    const DDS::SubscriberQos& subscriberQos,
    const char* filterClassName,
    const char* filterExpr,
    const DDS::StringSeq& params,
    const DCPS::TypeInformation& type_info)
  {
    return endpoint_manager().add_subscription(topicId,
                                               subscription,
                                               qos,
                                               transInfo,
                                               subscriberQos,
                                               filterClassName,
                                               filterExpr,
                                               params,
                                               type_info);
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

  void update_subscription_locators(const GUID_t& subId, const DCPS::TransportLocatorSeq& transInfo)
  {
    endpoint_manager().update_subscription_locators(subId, transInfo);
  }

  RcHandle<DCPS::TransportInst> sedp_transport_inst() const
  {
    return sedp_->transport_inst();
  }

  void request_remote_complete_type_objects(
    const GUID_t& remote_entity, const XTypes::TypeInformation& remote_type_info,
    DCPS::TypeObjReqCond& cond)
  {
    sedp_->request_remote_complete_type_objects(remote_entity, remote_type_info, cond);
  }

protected:
  Sedp& endpoint_manager() { return *sedp_; }

  void purge_discovered_participant(const DiscoveredParticipantIter& iter);

#ifndef DDS_HAS_MINIMUM_BIT
  void enqueue_location_update_i(DiscoveredParticipantIter iter, DCPS::ParticipantLocation mask, const DCPS::NetworkAddress& from, const char* reason);
  void process_location_updates_i(const DiscoveredParticipantIter& iter, const char* reason, bool force_publish = false);
  void publish_location_update_i(const DiscoveredParticipantIter& iter);
#endif

  bool announce_domain_participant_qos();

private:

#if OPENDDS_CONFIG_SECURITY
  typedef OPENDDS_MAP_CMP(GUID_t, DDS::Security::AuthRequestMessageToken, GUID_tKeyLessThan)
    PendingRemoteAuthTokenMap;
#endif

  void init(DDS::DomainId_t domain,
            DCPS::GUID_t& guid,
            const DDS::DomainParticipantQos& qos,
            XTypes::TypeLookupService_rch tls);

  mutable ACE_Thread_Mutex lock_;
  DCPS::RcHandle<DCPS::BitSubscriber> bit_subscriber_;
  DDS::DomainParticipantQos qos_;
  friend class Sedp;
  DiscoveredParticipantMap participants_;
  RtpsDiscovery* disco_;
  DCPS::RcHandle<RtpsDiscoveryConfig> config_;
  const CORBA::ULong participant_flags_;
  const DCPS::TimeDuration resend_period_;
  const double quick_resend_ratio_;
  const DCPS::TimeDuration min_resend_delay_;
  const DCPS::TimeDuration lease_duration_;
  const DCPS::TimeDuration lease_extension_;
  const DCPS::TimeDuration max_lease_duration_;
  const u_short max_spdp_sequence_msg_reset_checks_;
  const bool check_source_ip_;
  const bool undirected_spdp_;
#if OPENDDS_CONFIG_SECURITY
  const size_t max_participants_in_authentication_;
  const DCPS::TimeDuration security_unsecure_lease_duration_;
  const DCPS::TimeDuration auth_resend_period_;
  const DCPS::TimeDuration max_auth_time_;
  const bool secure_participant_user_data_;
#endif
  XTypes::TypeLookupService_rch type_lookup_service_;

  // Participant:
  const DDS::DomainId_t domain_;
  DCPS::GUID_t guid_;
  const DCPS::MonotonicTime_t participant_discovered_at_;
  bool is_application_participant_;
  DDS::UInt16 ipv4_participant_port_id_;
#ifdef ACE_HAS_IPV6
  DDS::UInt16 ipv6_participant_port_id_;
#endif

  void data_received(const DataSubmessage& data, const ParameterList& plist, const DCPS::NetworkAddress& from);

  void match_unauthenticated(const DiscoveredParticipantIter& dp_iter);

  /// Get this participant's BIT data. user_data may be omitting depending on
  /// security settings.
  DDS::ParticipantBuiltinTopicData get_part_bit_data(bool secure) const;

  /**
   * If this is true participant user data should only be sent and received
   * securely, otherwise the user data should be empty and participant bit
   * updates should be withheld from the user.
   */
  bool secure_part_user_data() const;

  void update_rtps_relay_application_participant_i(DiscoveredParticipantIter iter, bool new_participant);

#if OPENDDS_CONFIG_SECURITY
  DDS::ReturnCode_t send_handshake_message(const DCPS::GUID_t& guid,
                                           DiscoveredParticipant& dp,
                                           const DDS::Security::ParticipantStatelessMessage& msg);
  DCPS::MonotonicTimePoint schedule_handshake_resend(const DCPS::TimeDuration& time, const DCPS::GUID_t& guid);
  bool match_authenticated(const DCPS::GUID_t& guid, DiscoveredParticipantIter& iter);
  void attempt_authentication(const DiscoveredParticipantIter& iter, bool from_discovery);
  void update_agent_info(const DCPS::GUID_t& local_guid, const ICE::AgentInfo& agent_info);
  void remove_agent_info(const DCPS::GUID_t& local_guid);
#endif

  struct SpdpTransport
    : public virtual DCPS::RcEventHandler
    , public virtual DCPS::InternalDataReaderListener<DCPS::NetworkInterfaceAddress>
    , public virtual DCPS::ConfigListener
#if OPENDDS_CONFIG_SECURITY
    , public virtual ICE::Endpoint
#endif
  {
    typedef size_t WriteFlags;
    static const WriteFlags SEND_MULTICAST = (1 << 0);
    static const WriteFlags SEND_RELAY = (1 << 1);
    static const WriteFlags SEND_DIRECT = (1 << 2);

    class RegisterHandlers : public DCPS::ReactorTask::Command {
    public:
      RegisterHandlers(const DCPS::RcHandle<SpdpTransport>& tport,
        const DCPS::ReactorTask_rch& reactor_task)
        : tport_(tport)
        , reactor_task_(reactor_task)
      {
      }

      void execute(ACE_Reactor*)
      {
        DCPS::RcHandle<SpdpTransport> tport = tport_.lock();
        if (!tport) {
          return;
        }
        tport->register_handlers(reactor_task_);
      }

    private:
      DCPS::WeakRcHandle<SpdpTransport> tport_;
      DCPS::ReactorTask_rch reactor_task_;
    };

    explicit SpdpTransport(DCPS::RcHandle<Spdp> outer);
    ~SpdpTransport();

    const ACE_SOCK_Dgram& choose_recv_socket(ACE_HANDLE h) const;

    virtual int handle_input(ACE_HANDLE h);

    void open(const DCPS::ReactorTask_rch& reactor_task,
              const DCPS::JobQueue_rch& job_queue);
    void register_unicast_socket(
      ACE_Reactor* reactor, ACE_SOCK_Dgram& socket, const char* what);
    void register_handlers(const DCPS::ReactorTask_rch& reactor_task);
    void enable_periodic_tasks();

    void shorten_local_sender_delay_i();
    void write(WriteFlags flags);
    void write_i(WriteFlags flags);
    void write_i(const DCPS::GUID_t& guid, const DCPS::NetworkAddress& local_address, WriteFlags flags);
    void send(WriteFlags flags, const DCPS::NetworkAddress& local_address = DCPS::NetworkAddress());
    const ACE_SOCK_Dgram& choose_send_socket(const DCPS::NetworkAddress& addr) const;
    ssize_t send(const DCPS::NetworkAddress& addr);
    void close(const DCPS::ReactorTask_rch& reactor_task);
    void dispose_unregister();
    void set_unicast_socket_opts(DCPS::RcHandle<Spdp>& outer, ACE_SOCK_Dgram& sock, DDS::UInt16& port);
    bool open_unicast_socket(DDS::UInt16 participant_id);
#ifdef ACE_HAS_IPV6
    bool open_unicast_ipv6_socket(DDS::UInt16 participant_id);
#endif

    void on_data_available(DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> > reader);

    DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint();

#if OPENDDS_CONFIG_SECURITY
    ICE::AddressListType host_addresses() const;
    void send(const ACE_INET_Addr& address, const STUN::Message& message);
    ACE_INET_Addr stun_server_address() const;
  #ifndef DDS_HAS_MINIMUM_BIT
    void ice_connect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr);
    void ice_disconnect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr);
  #endif
#endif

    DCPS::WeakRcHandle<Spdp> outer_;
    Header hdr_;
    UserTagSubmessage user_tag_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    DDS::UInt16 uni_port_;
    ACE_SOCK_Dgram unicast_socket_;
    OPENDDS_STRING multicast_interface_;
    DCPS::NetworkAddress multicast_address_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
#ifdef ACE_HAS_IPV6
    DDS::UInt16 ipv6_uni_port_;
    ACE_SOCK_Dgram unicast_ipv6_socket_;
    OPENDDS_STRING multicast_ipv6_interface_;
    DCPS::NetworkAddress multicast_ipv6_address_;
    ACE_SOCK_Dgram_Mcast multicast_ipv6_socket_;
#endif
    DCPS::MulticastManager multicast_manager_;
    DCPS::NetworkAddressSet send_addrs_;
    ACE_Message_Block buff_, wbuff_;
    typedef DCPS::PmfPeriodicTask<SpdpTransport> SpdpPeriodic;
    typedef DCPS::PmfSporadicTask<SpdpTransport> SpdpSporadic;
    typedef DCPS::PmfMultiTask<SpdpTransport> SpdpMulti;
    void send_local(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpMulti> local_send_task_;
    void send_directed(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> directed_send_task_;
    OPENDDS_LIST(DCPS::GUID_t) directed_guids_;
    void process_lease_expirations(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> lease_expiration_task_;
    void thread_status_task(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> thread_status_task_;
    DCPS::RcHandle<DCPS::InternalDataReader<DCPS::NetworkInterfaceAddress> > network_interface_address_reader_;
#if OPENDDS_CONFIG_SECURITY
    void process_handshake_deadlines(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> handshake_deadline_task_;
    void process_handshake_resends(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> handshake_resend_task_;
    void send_relay(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> relay_spdp_task_;
    void relay_stun_task(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> relay_stun_task_;
    ICE::ServerReflexiveStateMachine relay_srsm_;
    void process_relay_sra(ICE::ServerReflexiveStateMachine::StateChange);
    void disable_relay_stun_task();
#endif
    bool network_is_unreachable_;
    bool ice_endpoint_added_;

    DCPS::MonotonicTimePoint last_thread_status_harvest_;
    DCPS::ConfigReader_rch config_reader_;
    void on_data_available(DCPS::ConfigReader_rch reader);
  };

  DCPS::RcHandle<SpdpTransport> tport_;

#if OPENDDS_CONFIG_SECURITY
  class SendStun : public DCPS::Job {
  public:
    SendStun(const DCPS::RcHandle<SpdpTransport>& tport,
             const DCPS::NetworkAddress& address,
             const STUN::Message& message)
      : tport_(tport)
      , address_(address)
      , message_(message)
    {}
    void execute();
  private:
    DCPS::WeakRcHandle<SpdpTransport> tport_;
    DCPS::NetworkAddress address_;
    STUN::Message message_;
  };

#ifndef DDS_HAS_MINIMUM_BIT
  class IceConnect : public DCPS::Job {
  public:
    IceConnect(DCPS::RcHandle<Spdp> spdp,
               const ICE::GuidSetType& guids,
               const DCPS::NetworkAddress& addr,
               bool connect)
      : spdp_(spdp)
      , guids_(guids)
      , addr_(addr)
      , connect_(connect)
    {}
    void execute();
  private:
    DCPS::RcHandle<Spdp> spdp_;
    ICE::GuidSetType guids_;
    DCPS::NetworkAddress addr_;
    bool connect_;
  };
#endif /* DDS_HAS_MINIMUM_BIT */
#endif

  /// Spdp initialized
  AtomicBool initialized_flag_;

  bool eh_shutdown_;
  DCPS::ConditionVariable<ACE_Thread_Mutex> shutdown_cond_;
  /// Spdp shutting down
  AtomicBool shutdown_flag_;

  BuiltinEndpointSet_t available_builtin_endpoints_;
  DCPS::RcHandle<Sedp> sedp_;

  typedef OPENDDS_MULTIMAP(DCPS::MonotonicTimePoint, DCPS::GUID_t) TimeQueue;

  void remove_lease_expiration_i(DiscoveredParticipantIter iter);
  void update_lease_expiration_i(DiscoveredParticipantIter iter,
                                 const DCPS::MonotonicTimePoint& now);
  void process_lease_expirations(const DCPS::MonotonicTimePoint& now);
  TimeQueue lease_expirations_;

#if OPENDDS_CONFIG_SECURITY
  DDS::Security::ExtendedBuiltinEndpointSet_t available_extended_builtin_endpoints_;
  Security::SecurityConfig_rch security_config_;
  bool security_enabled_;

  DCPS::SequenceNumber stateless_sequence_number_;

  DDS::Security::IdentityHandle identity_handle_;
  DDS::Security::PermissionsHandle permissions_handle_;
  DDS::Security::ParticipantCryptoHandle crypto_handle_;

  DDS::Security::IdentityToken identity_token_;
  DDS::Security::IdentityStatusToken identity_status_token_;
  DDS::Security::PermissionsToken permissions_token_;
  DDS::Security::PermissionsCredentialToken permissions_credential_token_;

  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;

  DCPS::RcHandle<ICE::Agent> ice_agent_;

  void start_ice(DCPS::WeakRcHandle<ICE::Endpoint> endpoint, DCPS::GUID_t remote, BuiltinEndpointSet_t avail,
                 DDS::Security::ExtendedBuiltinEndpointSet_t extended_avail,
                 const ICE::AgentInfo& agent_info);
  void stop_ice(DCPS::WeakRcHandle<ICE::Endpoint> endpoint, DCPS::GUID_t remote, BuiltinEndpointSet_t avail,
                DDS::Security::ExtendedBuiltinEndpointSet_t extended_avail);

  void purge_handshake_deadlines(DiscoveredParticipantIter iter);
  TimeQueue handshake_deadlines_;

  void purge_handshake_resends(DiscoveredParticipantIter iter);
  TimeQueue handshake_resends_;

  size_t n_participants_in_authentication_;
  void set_auth_state(DiscoveredParticipant& dp, AuthState state);
#endif

  friend class ::DDS_TEST;
};

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SPDP_H
