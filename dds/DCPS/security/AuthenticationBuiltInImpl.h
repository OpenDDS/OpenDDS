/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */



#ifndef OPENDDS_DCPS_SECURITY_AUTHENTICATIONBUILTINIMPL_H
#define OPENDDS_DCPS_SECURITY_AUTHENTICATIONBUILTINIMPL_H

#include "OpenDDS_Security_Export.h"
#include "Authentication/LocalAuthCredentialData.h"
#include "SSL/DiffieHellman.h"

#include <dds/DdsSecurityCoreC.h>
#include <dds/Versioned_Namespace.h>
#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/GuidUtils.h>

#include <ace/Thread_Mutex.h>

#include <map>
#include <string>
#include <memory>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

const char Identity_Status_Token_Class_Id[] = "DDS:Auth:PKI-DH:1.0";
const char Auth_Peer_Cred_Token_Class_Id[] = "DDS:Auth:PKI-DH:1.0";

const char dds_cert_sn[] = "dds.cert.sn";
const char dds_cert_algo[] = "dds.cert.algo";

const char dds_ca_sn[] = "dds.ca.sn";
const char dds_ca_algo[] = "dds.ca.algo";

/**
* @class AuthenticationBuiltInImpl
*
* @brief Implements the DDS built-in version of the Authentication
* plugin for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/
class OpenDDS_Security_Export AuthenticationBuiltInImpl
  : public virtual DDS::Security::Authentication
{
public:

  /// include in PropertyQosPolicy to add optional properties to Handshake tokens
  static const char* PROPERTY_HANDSHAKE_DEBUG;

  AuthenticationBuiltInImpl();
  virtual ~AuthenticationBuiltInImpl();

  virtual ::DDS::Security::ValidationResult_t validate_local_identity(
    ::DDS::Security::IdentityHandle & local_identity_handle,
    DCPS::GUID_t & adjusted_participant_guid,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::DomainParticipantQos & participant_qos,
    const DCPS::GUID_t & candidate_participant_guid,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_identity_token(
    ::DDS::Security::IdentityToken & identity_token,
    ::DDS::Security::IdentityHandle handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_identity_status_token(
    ::DDS::Security::IdentityStatusToken & identity_status_token,
    ::DDS::Security::IdentityHandle handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_permissions_credential_and_token(
    ::DDS::Security::IdentityHandle handle,
    const ::DDS::Security::PermissionsCredentialToken & permissions_credential,
    const ::DDS::Security::PermissionsToken & permissions_token,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::ValidationResult_t validate_remote_identity(
    ::DDS::Security::IdentityHandle & remote_identity_handle,
    ::DDS::Security::AuthRequestMessageToken & local_auth_request_token,
    const ::DDS::Security::AuthRequestMessageToken & remote_auth_request_token,
    ::DDS::Security::IdentityHandle local_identity_handle,
    const ::DDS::Security::IdentityToken & remote_identity_token,
    const DCPS::GUID_t & remote_participant_guid,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::ValidationResult_t begin_handshake_request(
    ::DDS::Security::HandshakeHandle & handshake_handle,
    ::DDS::Security::HandshakeMessageToken & handshake_message,
    ::DDS::Security::IdentityHandle initiator_identity_handle,
    ::DDS::Security::IdentityHandle replier_identity_handle,
    const ::DDS::OctetSeq & serialized_local_participant_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::ValidationResult_t begin_handshake_reply(
    ::DDS::Security::HandshakeHandle & handshake_handle,
    ::DDS::Security::HandshakeMessageToken & handshake_message_out,
    ::DDS::Security::IdentityHandle initiator_identity_handle,
    ::DDS::Security::IdentityHandle replier_identity_handle,
    const ::DDS::OctetSeq & serialized_local_participant_data,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::ValidationResult_t process_handshake(
    ::DDS::Security::HandshakeMessageToken & handshake_message_out,
    const ::DDS::Security::HandshakeMessageToken & handshake_message_in,
    ::DDS::Security::HandshakeHandle handshake_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::SharedSecretHandle* get_shared_secret(
    ::DDS::Security::HandshakeHandle handshake_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean get_authenticated_peer_credential_token(
    ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
    ::DDS::Security::HandshakeHandle handshake_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_listener(
    ::DDS::Security::AuthenticationListener_ptr listener,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_identity_token(
    const ::DDS::Security::IdentityToken & token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_identity_status_token(
    const ::DDS::Security::IdentityStatusToken & token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_authenticated_peer_credential_token(
    const ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_handshake_handle(
    ::DDS::Security::HandshakeHandle handshake_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_identity_handle(
    ::DDS::Security::IdentityHandle identity_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_sharedsecret_handle(
    ::DDS::Security::SharedSecretHandle* sharedsecret_handle,
    ::DDS::Security::SecurityException & ex);

private:

  struct RemoteParticipantData : public DCPS::RcObject {
    typedef DCPS::RcHandle<RemoteParticipantData> shared_ptr;

    // Identity data

    DCPS::GUID_t participant_guid;
    DDS::Security::IdentityHandle local_participant;

    // Handshake data

    DDS::Security::AuthRequestMessageToken local_auth_request;
    DDS::Security::AuthRequestMessageToken remote_auth_request;
    DDS::Security::IdentityHandle initiator_identity;
    DDS::Security::IdentityHandle replier_identity;
    DDS::Security::SharedSecretHandle_var shared_secret;
    DDS::Security::HandshakeMessageToken request;
    DDS::Security::HandshakeMessageToken reply;
    DDS::Security::ValidationResult_t state;
    SSL::DiffieHellman::unique_ptr diffie_hellman;
    SSL::Certificate::unique_ptr certificate;
    DDS::OctetSeq c_perm;
    DDS::OctetSeq hash_c1;
    DDS::OctetSeq hash_c2;

    RemoteParticipantData()
      : participant_guid(DCPS::GUID_UNKNOWN)
      , local_participant(DDS::HANDLE_NIL)
      , initiator_identity(DDS::HANDLE_NIL)
      , replier_identity(DDS::HANDLE_NIL)
      , state(DDS::Security::VALIDATION_FAILED)
    {
    }
  };
  typedef std::map<DDS::Security::IdentityHandle, RemoteParticipantData::shared_ptr> RemoteParticipantMap;

  struct LocalParticipantData : public DCPS::RcObject {
    typedef DCPS::RcHandle<LocalParticipantData> shared_ptr;

    DCPS::GUID_t participant_guid;
    LocalAuthCredentialData::shared_ptr credentials;
    RemoteParticipantMap validated_remotes;
    bool handshake_debug;

    LocalParticipantData()
      : participant_guid(DCPS::GUID_UNKNOWN)
      , credentials()
      , validated_remotes()
      , handshake_debug(false)
    {
    }
    ~LocalParticipantData();
  };
  typedef std::map<DDS::Security::IdentityHandle, LocalParticipantData::shared_ptr> LocalParticipantMap;
  LocalParticipantMap local_participants_;

  LocalParticipantData::shared_ptr get_local_participant(DDS::Security::IdentityHandle handle);

  typedef std::pair<LocalParticipantData::shared_ptr, RemoteParticipantData::shared_ptr> HandshakeDataPair;
  typedef std::map<DDS::Security::HandshakeHandle, HandshakeDataPair> HandshakeDataMap;
  HandshakeDataMap handshake_data_;

  HandshakeDataPair get_handshake_data(DDS::Security::HandshakeHandle handle);

  /// @brief Finds the local and remote data objects associated with h1 and h2 and
  /// creates a new handshake pair with them. It does not matter which handle is local
  /// and which is remote.
  /// @param h1 Either a local or remote handle.
  /// @param h2 Either a local or remote handle.
  HandshakeDataPair make_handshake_pair(DDS::Security::IdentityHandle h1,
                                        DDS::Security::IdentityHandle h2);

  DDS::Security::ValidationResult_t process_handshake_reply(
    DDS::Security::HandshakeMessageToken & handshake_message_out,
    const DDS::Security::HandshakeMessageToken & handshake_message_in,
    DDS::Security::HandshakeHandle handshake_handle,
    DDS::Security::SecurityException & ex);

  DDS::Security::ValidationResult_t process_final_handshake(
    const DDS::Security::HandshakeMessageToken & handshake_message_in,
    DDS::Security::HandshakeHandle handshake_handle,
    DDS::Security::SecurityException & ex);

  bool is_handshake_initiator(const DCPS::GUID_t& local, const DCPS::GUID_t& remote);

  bool check_class_versions(const char* remote_class_id);

  std::string build_class_id(const std::string& message_ext);

  std::string get_extension(const char* class_id);

  CORBA::Long get_next_handle();

  struct was_guid_validated
  {
    was_guid_validated(const DCPS::GUID_t& expected) : expected_(expected) {}

    bool operator()(const RemoteParticipantMap::value_type& validated) const
    {
      return (expected_ == validated.second->participant_guid);
    }
  private:
    const DCPS::GUID_t& expected_;
  };

  struct local_has_remote_handle
  {
    local_has_remote_handle(DDS::Security::IdentityHandle h) : h_(h) {}

    bool operator()(const LocalParticipantMap::value_type& local) const
    {
      const RemoteParticipantMap& remotes = local.second->validated_remotes;
      return remotes.find(h_) != remotes.end();
    }
  private:
    DDS::Security::IdentityHandle h_;
  };

  DDS::Security::AuthenticationListener_ptr listener_ptr_;

  ACE_Thread_Mutex identity_mutex_;
  ACE_Thread_Mutex handshake_mutex_;
  ACE_Thread_Mutex handle_mutex_;

  CORBA::Long next_handle_;

};
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
