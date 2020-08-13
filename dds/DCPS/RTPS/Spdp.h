/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPS_SPDP_H
#define OPENDDS_RTPS_SPDP_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"

#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/RcEventHandler.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/PeriodicTask.h"
#include "dds/DCPS/SporadicTask.h"
#include "dds/DCPS/MultiTask.h"
#include "dds/DCPS/JobQueue.h"
#include "dds/DCPS/NetworkConfigMonitor.h"
#include "dds/DCPS/RTPS/ICE/Ice.h"

#include "RtpsCoreC.h"
#include "Sedp.h"
#include "rtps_export.h"

#include "ace/Atomic_Op.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"
#include "ace/Condition_Thread_Mutex.h"

#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/PoolAllocationBase.h"

#include "dds/DCPS/security/framework/SecurityConfig_rch.h"
#ifdef OPENDDS_SECURITY
#include "dds/DCPS/security/framework/SecurityConfig.h"
#endif

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

class RtpsDiscoveryConfig;
class RtpsDiscovery;

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class OpenDDS_Rtps_Export Spdp : public DCPS::LocalParticipant<Sedp>
#ifdef OPENDDS_SECURITY
                               , public ICE::AgentInfoListener
#endif
{
public:

  Spdp(DDS::DomainId_t domain,
       DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos,
       RtpsDiscovery* disco);

#ifdef OPENDDS_SECURITY
  Spdp(DDS::DomainId_t domain,
       const DCPS::RepoId& guid,
       const DDS::DomainParticipantQos& qos,
       RtpsDiscovery* disco,
       DDS::Security::IdentityHandle id_handle,
       DDS::Security::PermissionsHandle perm_handle,
       DDS::Security::ParticipantCryptoHandle crypto_handle);
#endif

  ~Spdp();

  // Participant
  const DCPS::RepoId& guid() const { return guid_; }
  void init_bit(const DDS::Subscriber_var& bit_subscriber);
  void fini_bit();

  bool get_default_locators(const DCPS::RepoId& part_id,
                            DCPS::LocatorSeq& target,
                            bool& inlineQos);

  // Managing reader/writer associations
  void signal_liveliness(DDS::LivelinessQosPolicyKind kind);

  // Is Spdp shutting down?
  bool shutting_down() { return shutdown_flag_.value(); }

  bool associated() const;
  bool has_discovered_participant(const DCPS::RepoId& guid);

  WaitForAcks& wait_for_acks();

#ifdef OPENDDS_SECURITY
  Security::SecurityConfig_rch get_security_config() const { return security_config_; }
  DDS::Security::ParticipantCryptoHandle crypto_handle() const { return crypto_handle_; }
  DDS::Security::ParticipantCryptoHandle remote_crypto_handle(const DCPS::RepoId& remote_participant) const;

  void handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg);
  void send_handshake_request(const DCPS::RepoId& guid, DiscoveredParticipant& dp);
  void handle_handshake_message(const DDS::Security::ParticipantStatelessMessage& msg);
  bool handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg,
                                        bool& send_our_tokens);
  bool seen_crypto_tokens_from(const DCPS::RepoId& sender);
  DDS::OctetSeq local_participant_data_as_octets() const;
#endif

  void handle_participant_data(DCPS::MessageId id,
                               const ParticipantData_t& pdata,
                               const DCPS::SequenceNumber& seq,
                               const ACE_INET_Addr& from,
                               bool from_sedp);

  static bool validateSequenceNumber(const DCPS::SequenceNumber& seq, DiscoveredParticipantIter& iter);

#ifdef OPENDDS_SECURITY
  void process_handshake_deadlines(const DCPS::MonotonicTimePoint& tv);
  void process_handshake_resends(const DCPS::MonotonicTimePoint& tv);

  /**
   * Write Secured Updated DP QOS
   *
   * lock_ must be aquired before calling this.
   */
  void write_secure_updates();
  void write_secure_disposes();
  bool is_security_enabled() const { return security_enabled_; }
  bool security_builtins_associated(const DCPS::RepoId& remoteParticipant) const;
#endif

  bool is_expectant_opendds(const GUID_t& participant) const;

#ifdef OPENDDS_SECURITY
  typedef std::pair<DDS::Security::ParticipantCryptoHandle, DDS::Security::SharedSecretHandle_var> ParticipantCryptoInfoPair;
  ParticipantCryptoInfoPair lookup_participant_crypto_info(const DCPS::RepoId& id) const;
  void send_participant_crypto_tokens(const DCPS::RepoId& id);

  DDS::DomainId_t get_domain_id() const { return domain_; }
  DDS::Security::PermissionsHandle lookup_participant_permissions(const DCPS::RepoId& id) const;

  DCPS::AuthState lookup_participant_auth_state(const DCPS::RepoId& id) const;

  void process_participant_ice(const ParameterList& plist,
                               const ParticipantData_t& pdata,
                               const DCPS::RepoId& guid);

  bool remote_is_requester(const DCPS::RepoId& guid) const;
  const ParticipantData_t& get_participant_data(const DCPS::RepoId& guid) const;

#endif

  DCPS::RcHandle<DCPS::JobQueue> job_queue() const { return tport_->job_queue_; }

  u_short get_spdp_port() const { return tport_ ? tport_->uni_port_ : 0; }

  u_short get_sedp_port() const { return sedp_.local_address().get_port_number(); }

#ifdef ACE_HAS_IPV6
  u_short get_ipv6_spdp_port() const { return tport_ ? tport_->ipv6_uni_port_ : 0; }

  u_short get_ipv6_sedp_port() const { return sedp_.ipv6_local_address().get_port_number(); }
#endif

  void sedp_rtps_relay_address(const ACE_INET_Addr& address) { sedp_.rtps_relay_address(address); }

  void sedp_stun_server_address(const ACE_INET_Addr& address) { sedp_.stun_server_address(address); }

  BuiltinEndpointSet_t available_builtin_endpoints() const { return available_builtin_endpoints_; }

  ICE::Endpoint* get_ice_endpoint_if_added();

  ParticipantData_t build_local_pdata(
#ifdef OPENDDS_SECURITY
                                      Security::DiscoveredParticipantDataKind kind
#endif
                                      );
protected:
  Sedp& endpoint_manager() { return sedp_; }
  void remove_discovered_participant_i(DiscoveredParticipantIter iter);

#ifndef DDS_HAS_MINIMUM_BIT
  void enqueue_location_update_i(DiscoveredParticipantIter iter, DCPS::ParticipantLocation mask, const ACE_INET_Addr& from);
  void process_location_updates_i(DiscoveredParticipantIter iter);
#endif

  bool announce_domain_participant_qos();

private:

  void init(DDS::DomainId_t domain,
            DCPS::RepoId& guid,
            const DDS::DomainParticipantQos& qos,
            RtpsDiscovery* disco);

  RtpsDiscovery* disco_;
  DCPS::RcHandle<RtpsDiscoveryConfig> config_;

  // Participant:
  const DDS::DomainId_t domain_;
  DCPS::RepoId guid_;

  void data_received(const DataSubmessage& data, const ParameterList& plist, const ACE_INET_Addr& from);

  void match_unauthenticated(const DCPS::RepoId& guid, DiscoveredParticipantIter& dp_iter);

#ifdef OPENDDS_SECURITY
  DDS::ReturnCode_t send_handshake_message(const DCPS::RepoId& guid,
                                           DiscoveredParticipant& dp,
                                           const DDS::Security::ParticipantStatelessMessage& msg);
  DCPS::MonotonicTimePoint schedule_handshake_resend(const DCPS::TimeDuration& time, const DCPS::RepoId& guid);
  bool match_authenticated(const DCPS::RepoId& guid, DiscoveredParticipantIter& iter);
  void attempt_authentication(const DiscoveredParticipantIter& iter, bool from_discovery);
  void update_agent_info(const DCPS::RepoId& local_guid, const ICE::AgentInfo& agent_info);
#endif

  struct SpdpTransport : public virtual DCPS::RcEventHandler, public virtual DCPS::NetworkConfigListener
#ifdef OPENDDS_SECURITY
        , public ICE::Endpoint
#endif
{
    typedef size_t WriteFlags;
    static const WriteFlags SEND_TO_LOCAL = (1 << 0);
    static const WriteFlags SEND_TO_RELAY = (1 << 1);

    explicit SpdpTransport(Spdp* outer);
    ~SpdpTransport();

    const ACE_SOCK_Dgram& choose_recv_socket(ACE_HANDLE h) const;

    virtual int handle_input(ACE_HANDLE h);
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

    void open();
    void shorten_local_sender_delay_i();
    void write(WriteFlags flags);
    void write_i(WriteFlags flags);
    void write_i(const DCPS::RepoId& guid, WriteFlags flags);
    void send(WriteFlags flags);
    const ACE_SOCK_Dgram& choose_send_socket(const ACE_INET_Addr& addr) const;
    void send(const ACE_INET_Addr& addr);
    void close();
    void dispose_unregister();
    bool open_unicast_socket(u_short port_common, u_short participant_id);
#ifdef ACE_HAS_IPV6
    bool open_unicast_ipv6_socket(u_short port);
#endif
    void acknowledge();

    void join_multicast_group(const DCPS::NetworkInterface& nic,
                              bool all_interfaces = false);
    void leave_multicast_group(const DCPS::NetworkInterface& nic);
    void add_address(const DCPS::NetworkInterface& interface,
                     const ACE_INET_Addr& address);
    void remove_address(const DCPS::NetworkInterface& interface,
                        const ACE_INET_Addr& address);

    ICE::Endpoint* get_ice_endpoint();

#ifdef OPENDDS_SECURITY
    ICE::AddressListType host_addresses() const;
    void send(const ACE_INET_Addr& address, const STUN::Message& message);
    ACE_INET_Addr stun_server_address() const;
  #ifndef DDS_HAS_MINIMUM_BIT
    void ice_connect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr);
    void ice_disconnect(const ICE::GuidSetType& guids, const ACE_INET_Addr& addr);
  #endif
#endif

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    DCPS::TimeDuration lease_duration_;
    u_short uni_port_;
    u_short mc_port_;
    ACE_SOCK_Dgram unicast_socket_;
    OPENDDS_STRING multicast_interface_;
    ACE_INET_Addr multicast_address_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
#ifdef ACE_HAS_IPV6
    u_short ipv6_uni_port_;
    ACE_SOCK_Dgram unicast_ipv6_socket_;
    OPENDDS_STRING multicast_ipv6_interface_;
    ACE_INET_Addr multicast_ipv6_address_;
    ACE_SOCK_Dgram_Mcast multicast_ipv6_socket_;
    OPENDDS_SET(OPENDDS_STRING) joined_ipv6_interfaces_;
#endif
    OPENDDS_SET(OPENDDS_STRING) joined_interfaces_;
    OPENDDS_SET(ACE_INET_Addr) send_addrs_;
    ACE_Message_Block buff_, wbuff_;
    DCPS::ReactorTask reactor_task_;
    DCPS::RcHandle<DCPS::JobQueue> job_queue_;
    typedef DCPS::PmfPeriodicTask<SpdpTransport> SpdpPeriodic;
    typedef DCPS::PmfSporadicTask<SpdpTransport> SpdpSporadic;
    typedef DCPS::PmfMultiTask<SpdpTransport> SpdpMulti;
    void send_local(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpMulti> local_sender_;
#ifdef OPENDDS_SECURITY
    void process_handshake_deadlines(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> handshake_deadline_processor_;
    void process_handshake_resends(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> handshake_resend_processor_;
#endif
    void send_relay(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> relay_sender_;
    void send_relay_beacon(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> relay_beacon_;
    bool network_is_unreachable_;
    bool ice_endpoint_added_;
  } *tport_;

  struct ChangeMulticastGroup : public DCPS::JobQueue::Job {
    enum CmgAction {CMG_JOIN, CMG_LEAVE};

    ChangeMulticastGroup(DCPS::RcHandle<SpdpTransport> tport,
                         const DCPS::NetworkInterface& nic, CmgAction action,
                         bool all_interfaces = false)
      : tport_(tport)
      , nic_(nic)
      , action_(action)
      , all_interfaces_(all_interfaces)
    {}

    void execute()
    {
      if (action_ == CMG_JOIN) {
        tport_->join_multicast_group(nic_, all_interfaces_);
      } else {
        tport_->leave_multicast_group(nic_);
      }
    }

    DCPS::RcHandle<SpdpTransport> tport_;
    DCPS::NetworkInterface nic_;
    CmgAction action_;
    bool all_interfaces_;
  };

#ifdef OPENDDS_SECURITY
  class SendStun : public DCPS::JobQueue::Job {
  public:
    SendStun(DCPS::RcHandle<SpdpTransport> tport,
             const ACE_INET_Addr& address,
             const STUN::Message& message)
      : tport_(tport)
      , address_(address)
      , message_(message)
    {}
    void execute();
  private:
    DCPS::RcHandle<SpdpTransport> tport_;
    ACE_INET_Addr address_;
    STUN::Message message_;
  };

#ifndef DDS_HAS_MINIMUM_BIT
  class IceConnect : public DCPS::JobQueue::Job {
  public:
    IceConnect(DCPS::RcHandle<Spdp> spdp,
               const ICE::GuidSetType& guids,
               const ACE_INET_Addr& addr,
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
    ACE_INET_Addr addr_;
    bool connect_;
  };
#endif /* DDS_HAS_MINIMUM_BIT */
#endif

  ACE_Event_Handler_var eh_; // manages our refcount on tport_
  bool eh_shutdown_;
  ACE_Condition_Thread_Mutex shutdown_cond_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> shutdown_flag_; // Spdp shutting down

  void remove_expired_participants();
  void get_discovered_participant_ids(DCPS::RepoIdSet& results) const;

  BuiltinEndpointSet_t available_builtin_endpoints_;
  Sedp sedp_;
  // wait for acknowledgments from SpdpTransport and Sedp::Task
  // when BIT is being removed (fini_bit)
  WaitForAcks wait_for_acks_;

#ifdef OPENDDS_SECURITY
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

  typedef std::multimap<DCPS::MonotonicTimePoint, DCPS::RepoId> TimeQueue;
  TimeQueue handshake_deadlines_;
  TimeQueue handshake_resends_;

  void start_ice(ICE::Endpoint* endpoint, DCPS::RepoId remote, const BuiltinEndpointSet_t& avail, const ICE::AgentInfo& agent_info);
  void stop_ice(ICE::Endpoint* endpoint, DCPS::RepoId remote, const BuiltinEndpointSet_t& avail);

  void purge_handshake_deadlines(DiscoveredParticipantIter iter);
  void purge_handshake_resends(DiscoveredParticipantIter iter);

#endif

  friend class ::DDS_TEST;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SPDP_H
