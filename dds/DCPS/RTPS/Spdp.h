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

class RtpsDiscovery;

/// Each instance of class Spdp represents the implementation of the RTPS
/// Simple Participant Discovery Protocol for a single local DomainParticipant.
class OpenDDS_Rtps_Export Spdp : public DCPS::LocalParticipant<Sedp> {
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

  void handle_auth_request(const DDS::Security::ParticipantStatelessMessage& msg);
  void handle_handshake_message(const DDS::Security::ParticipantStatelessMessage& msg);
  void handle_participant_crypto_tokens(const DDS::Security::ParticipantVolatileMessageSecure& msg);
#endif

  void handle_participant_data(DCPS::MessageId id, const ParticipantData_t& pdata);

#ifdef OPENDDS_SECURITY
  void check_auth_states(const ACE_Time_Value& tv);
  /**
   * Write Secured Updated DP QOS
   *
   * lock_ must be aquired before calling this.
   */
  void write_secure_updates();
  void write_secure_disposes();
  bool is_security_enabled() const { return security_enabled_; }
#endif

  bool is_opendds(const GUID_t& participant) const;

#ifdef OPENDDS_SECURITY
  typedef std::pair<DDS::Security::ParticipantCryptoHandle, DDS::Security::SharedSecretHandle_var> ParticipantCryptoInfoPair;
  ParticipantCryptoInfoPair lookup_participant_crypto_info(const DCPS::RepoId& id) const;
  void send_participant_crypto_tokens(const DCPS::RepoId& id);

  DDS::DomainId_t get_domain_id() const { return domain_; }
  DDS::Security::PermissionsHandle lookup_participant_permissions(const DCPS::RepoId& id) const;

  DCPS::AuthState lookup_participant_auth_state(const DCPS::RepoId& id) const;
#endif

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

  ACE_Reactor* reactor() const;

  RtpsDiscovery* disco_;

  // Participant:
  const DDS::DomainId_t domain_;
  DCPS::RepoId guid_;
  DCPS::LocatorSeq sedp_unicast_, sedp_multicast_;

  void data_received(const DataSubmessage& data, const ParameterList& plist);

  void match_unauthenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp);

#ifdef OPENDDS_SECURITY
  bool match_authenticated(const DCPS::RepoId& guid, DiscoveredParticipant& dp);
  void attempt_authentication(const DCPS::RepoId& guid, DiscoveredParticipant& dp);
#endif

#ifndef DDS_HAS_MINIMUM_BIT
  DCPS::ParticipantBuiltinTopicDataDataReaderImpl* part_bit();
#endif /* DDS_HAS_MINIMUM_BIT */

  struct SpdpTransport : public DCPS::RcEventHandler {
    SpdpTransport(Spdp* outer, bool securityGuids);
    ~SpdpTransport();

    virtual int handle_timeout(const ACE_Time_Value&, const void*);
    virtual int handle_input(ACE_HANDLE h);
    virtual int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE);

    void open();
    void write();
    void write_i();
    void close();
    void dispose_unregister();
    bool open_unicast_socket(u_short port_common, u_short participant_id);
    void acknowledge();

    Spdp* outer_;
    Header hdr_;
    DataSubmessage data_;
    DCPS::SequenceNumber seq_;
    ACE_Time_Value lease_duration_;
    ACE_SOCK_Dgram unicast_socket_;
    ACE_SOCK_Dgram_Mcast multicast_socket_;
    OPENDDS_SET(ACE_INET_Addr) send_addrs_;
    ACE_Message_Block buff_, wbuff_;
    ACE_Time_Value disco_resend_period_;
    ACE_Time_Value last_disco_resend_;
  } *tport_;

  ACE_Event_Handler_var eh_; // manages our refcount on tport_
  bool eh_shutdown_;
  ACE_Condition_Thread_Mutex shutdown_cond_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> shutdown_flag_; // Spdp shutting down

  void remove_expired_participants();
  void get_discovered_participant_ids(DCPS::RepoIdSet& results) const;

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
  DDS::Security::ParticipantCryptoTokenSeq crypto_tokens_;

  DDS::Security::ParticipantSecurityAttributes participant_sec_attr_;
#endif
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_RTPS_SPDP_H
