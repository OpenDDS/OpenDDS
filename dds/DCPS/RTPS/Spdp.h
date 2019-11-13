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
#include "dds/DCPS/JobQueue.h"

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
  void handle_handshake_message(const DDS::Security::ParticipantStatelessMessage& msg);
  void handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
#endif

  void handle_participant_data(DCPS::MessageId id,
                               const ParticipantData_t& pdata,
                               const DCPS::SequenceNumber& seq,
                               const ACE_INET_Addr& from);

  static bool validateSequenceNumber(const DCPS::SequenceNumber& seq, DiscoveredParticipantIter& iter);

#ifdef OPENDDS_SECURITY
  void process_auth_deadlines(const DCPS::MonotonicTimePoint& tv);
  void process_auth_resends(const DCPS::MonotonicTimePoint& tv);

  /**
   * Write Secured Updated DP QOS
   *
   * lock_ must be aquired before calling this.
   */
  void write_secure_updates();
  void write_secure_disposes();
  bool is_security_enabled() const { return security_enabled_; }
#endif

  bool is_expectant_opendds(const GUID_t& participant) const;

#ifdef OPENDDS_SECURITY
  typedef std::pair<DDS::Security::ParticipantCryptoHandle, DDS::Security::SharedSecretHandle_var> ParticipantCryptoInfoPair;
  ParticipantCryptoInfoPair lookup_participant_crypto_info(const DCPS::RepoId& id) const;
  void send_participant_crypto_tokens(const DCPS::RepoId& id);

  DDS::DomainId_t get_domain_id() const { return domain_; }
  DDS::Security::PermissionsHandle lookup_participant_permissions(const DCPS::RepoId& id) const;

  DCPS::AuthState lookup_participant_auth_state(const DCPS::RepoId& id) const;
#endif

  void remove_send_addr(const ACE_INET_Addr& addr);
  void add_send_addr(const ACE_INET_Addr& addr);
  void remove_sedp_unicast(const ACE_INET_Addr& addr);
  void add_sedp_unicast(const ACE_INET_Addr& addr);

  DCPS::RcHandle<DCPS::JobQueue> job_queue() const { return tport_->job_queue_; }

  u_short get_spdp_port() const { return tport_ ? tport_->uni_port_ : 0; }

  u_short get_sedp_port() const { return sedp_.local_address().get_port_number(); }

  void sedp_rtps_relay_address(const ACE_INET_Addr& address) { sedp_.rtps_relay_address(address); }

  void sedp_stun_server_address(const ACE_INET_Addr& address) { sedp_.stun_server_address(address); }

  BuiltinEndpointSet_t available_builtin_endpoints() const { return available_builtin_endpoints_; }

protected:
  Sedp& endpoint_manager() { return sedp_; }

  ParticipantData_t build_local_pdata(
#ifdef OPENDDS_SECURITY
                                      Security::DiscoveredParticipantDataKind kind
#endif
                                      );

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
  DCPS::LocatorSeq sedp_unicast_, sedp_multicast_;

  void data_received(const DataSubmessage& data, const ParameterList& plist, const ACE_INET_Addr& from);

  void match_unauthenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp);

#ifdef OPENDDS_SECURITY
  bool match_authenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp);
  void attempt_authentication(const DCPS::RepoId& guid, DiscoveredParticipant& dp);
  void update_agent_info(const DCPS::RepoId& local_guid, const ICE::AgentInfo& agent_info);
#endif

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  struct SpdpTransport : public DCPS::RcEventHandler {
    typedef size_t WriteFlags;
    static const WriteFlags SEND_TO_LOCAL = (1 << 0);
    static const WriteFlags SEND_TO_RELAY = (1 << 1);

    SpdpTransport(Spdp* outer, bool securityGuids);
    ~SpdpTransport();

    virtual int handle_input(ACE_HANDLE h);
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

    void open();
    void write(WriteFlags flags);
    void write_i(WriteFlags flags);
    void write_i(const DCPS::RepoId& guid, WriteFlags flags);
    void send(WriteFlags flags);
    void close();
    void dispose_unregister();
    bool open_unicast_socket(u_short port_common, u_short participant_id);
    void acknowledge();
    void remove_send_addr(const ACE_INET_Addr& addr);
    void insert_send_addr(const ACE_INET_Addr& addr);

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    DCPS::TimeDuration lease_duration_;
    u_short uni_port_;
    ACE_SOCK_Dgram unicast_socket_;
    ACE_INET_Addr default_multicast_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
    OPENDDS_SET(ACE_INET_Addr) send_addrs_;
    ACE_Message_Block buff_, wbuff_;
    DCPS::ReactorTask reactor_task_;
    DCPS::RcHandle<DCPS::JobQueue> job_queue_;
    typedef DCPS::PmfPeriodicTask<SpdpTransport> SpdpPeriodic;
    typedef DCPS::PmfSporadicTask<SpdpTransport> SpdpSporadic;
    void send_local(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> local_sender_;
#ifdef OPENDDS_SECURITY
    void process_auth_deadlines(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> auth_deadline_processor_;
    void process_auth_resends(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpSporadic> auth_resend_processor_;
#endif
    void send_relay(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> relay_sender_;
    void send_relay_beacon(const DCPS::MonotonicTimePoint& now);
    DCPS::RcHandle<SpdpPeriodic> relay_beacon_;
  } *tport_;

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

  DDS::Security::IdentityHandle identity_handle_;
  DDS::Security::PermissionsHandle permissions_handle_;
  DDS::Security::ParticipantCryptoHandle crypto_handle_;

  DDS::Security::IdentityToken identity_token_;
  DDS::Security::IdentityStatusToken identity_status_token_;
  DDS::Security::PermissionsToken permissions_token_;
  DDS::Security::PermissionsCredentialToken permissions_credential_token_;

  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;

  typedef std::multimap<DCPS::MonotonicTimePoint, DCPS::RepoId> TimeQueue;
  TimeQueue auth_deadlines_;
  TimeQueue auth_resends_;
#endif

  void start_ice(ICE::Endpoint* endpoint, DCPS::RepoId remote, const BuiltinEndpointSet_t& avail, const ICE::AgentInfo& agent_info);
  void stop_ice(ICE::Endpoint* endpoint, DCPS::RepoId remote, const BuiltinEndpointSet_t& avail);

  void purge_auth_deadlines(DiscoveredParticipantIter iter);
  void purge_auth_resends(DiscoveredParticipantIter iter);

  friend class ::DDS_TEST;
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SPDP_H
