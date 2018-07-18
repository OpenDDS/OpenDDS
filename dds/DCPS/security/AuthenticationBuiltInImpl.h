/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */



#ifndef DDS_DCPS_AUTHENTICATION_BUILTIN_IMPL_H
#define DDS_DCPS_AUTHENTICATION_BUILTIN_IMPL_H

#include "dds/DCPS/security/DdsSecurity_Export.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "dds/DCPS/dcps_export.h"
#include "ace/Thread_Mutex.h"
#include <map>
#include <string>
#include <memory>

#include "Authentication/LocalCredentialData.h"
#include "SSL/DiffieHellman.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {


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
class DdsSecurity_Export  AuthenticationBuiltInImpl
	: public virtual DDS::Security::Authentication
{
public:
  AuthenticationBuiltInImpl();
  virtual ~AuthenticationBuiltInImpl();

  virtual ::DDS::Security::ValidationResult_t validate_local_identity(
    ::DDS::Security::IdentityHandle & local_identity_handle,
    ::OpenDDS::DCPS::GUID_t & adjusted_participant_guid,
    ::DDS::Security::DomainId_t domain_id,
    const ::DDS::DomainParticipantQos & participant_qos,
    const ::OpenDDS::DCPS::GUID_t & candidate_participant_guid,
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
    const ::OpenDDS::DCPS::GUID_t & remote_participant_guid,
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

  struct IdentityData : public DCPS::RcObject<ACE_SYNCH_MUTEX>
  {
    OpenDDS::DCPS::GUID_t participant_guid;
    LocalAuthCredentialData::shared_ptr local_credential_data;
    DDS::Security::PermissionsCredentialToken permissions_cred_token;
    DDS::Security::PermissionsToken permissions_token;
    DDS::Security::IdentityToken identity_token;
    DDS::Security::IdentityHandle local_handle;
    DDS::Security::AuthRequestMessageToken local_auth_request;
    DDS::Security::AuthRequestMessageToken remote_auth_request;
  };
  typedef DCPS::RcHandle<IdentityData> IdentityData_Ptr;
  typedef std::map<DDS::Security::IdentityHandle, IdentityData_Ptr> Identity_Handle_Data;
  Identity_Handle_Data identity_data_;

  struct HandshakeData : public DCPS::RcObject<ACE_SYNCH_MUTEX>
  {
    DDS::Security::IdentityHandle local_identity_handle;
    DDS::Security::IdentityHandle remote_identity_handle;
    DDS::Security::SharedSecretHandle_var secret_handle;
    DDS::Security::HandshakeMessageToken request_token;
    DDS::Security::HandshakeMessageToken reply_token;
    DDS::Security::ValidationResult_t validation_state;
    OpenDDS::Security::SSL::DiffieHellman::unique_ptr diffie_hellman;
    OpenDDS::Security::SSL::Certificate::unique_ptr remote_cert;
    DDS::OctetSeq access_permissions;
    DDS::OctetSeq hash_c1;
    DDS::OctetSeq hash_c2;
    bool local_initiator;
  };
  typedef DCPS::RcHandle<HandshakeData> HandshakeData_Ptr;
  typedef std::map<DDS::Security::HandshakeHandle, HandshakeData_Ptr> Handshake_Handle_Data;
  Handshake_Handle_Data handshake_data_;

  DDS::Security::ValidationResult_t process_handshake_reply(
    DDS::Security::HandshakeMessageToken & handshake_message_out,
    const DDS::Security::HandshakeMessageToken & handshake_message_in,
    DDS::Security::HandshakeHandle handshake_handle,
    DDS::Security::SecurityException & ex);

  DDS::Security::ValidationResult_t process_final_handshake(
    const DDS::Security::HandshakeMessageToken & handshake_message_in,
    DDS::Security::HandshakeHandle handshake_handle,
    DDS::Security::SecurityException & ex);

  HandshakeData_Ptr get_handshake_data(DDS::Security::HandshakeHandle handle);
  IdentityData_Ptr get_identity_data(DDS::Security::IdentityHandle handle);

  bool is_handshake_initiator(const OpenDDS::DCPS::GUID_t& local, const OpenDDS::DCPS::GUID_t& remote);

  bool check_class_versions(const char* remote_class_id);

  std::string build_class_id(const std::string& message_ext);

  std::string get_extension(const char* class_id);

  uint64_t get_next_handle();

  DDS::Security::AuthenticationListener_ptr listener_ptr_;

  ACE_Thread_Mutex identity_mutex_;
  ACE_Thread_Mutex handshake_mutex_;
  ACE_Thread_Mutex handle_mutex_;

  uint64_t next_handle_;

};
} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
