/*
*
*
* Distributed under the DDS License.
* See: http://www.opendds.org/license.html
*/

#include "dds/DdsSecurityEntities.h"
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "dds/DCPS/security/TokenReader.h"
#include "dds/DCPS/security/TokenWriter.h"
#include "dds/DCPS/GuidUtils.h"
#include "ace/config-macros.h"
#include "ace/Guard_T.h"
#include <sstream>
#include <vector>

#include "SSL/Certificate.h"
#include "SSL/PrivateKey.h"
#include "SSL/Utils.h"

// Temporary include for get macaddress for unique guids
#include <ace/OS_NS_netdb.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

// Supported message versions and IDs - using these for stub version
static const std::string Auth_Plugin_Name("DDS:Auth:PKI-DH");
static const std::string Auth_Plugin_Major_Version("1");
static const std::string Auth_Plugin_Minor_Version("0");
static const std::string Identity_Token_Class_Id("DDS:Auth:PKI-DH:1.0");
static const std::string Auth_Peer_Cred_Token_Class_Id("DDS:Auth:PKI-DH:1.0");
static const std::string Auth_Request_Class_Ext("AuthReq");
static const std::string Handshake_Request_Class_Ext("Req");
static const std::string Handshake_Reply_Class_Ext("Reply");
static const std::string Handshake_Final_Class_Ext("Final");

// Until a real implementation is created, a stub sequence will be sent for data
static const DDS::OctetSeq Empty_Seq;
static const std::string AlgoName("RSASSA-PSS-SHA256");
static const std::string AgreementAlgo("DH+MODP-2048-256");

// Temporary const for generating unique guids until real validation is in place
static const short NODE_ID_SIZE = 6;

AuthenticationBuiltInImpl::AuthenticationBuiltInImpl()
: listener_ptr_()
, identity_mutex_()
, handshake_mutex_()
, handle_mutex_()
, next_handle_(1ULL)
{
}

AuthenticationBuiltInImpl::~AuthenticationBuiltInImpl()
{
  // - Clean up resources used by this implementation
}

class LocalIdentityData
{
public:
  LocalIdentityData(const DDS::PropertySeq& props)
  {
    std::string name, value, pkey_uri, password;
    for (size_t i = 0; i < props.length(); ++i) {
      name = props[i].name;
      value = props[i].value;

      if (name == "dds.sec.auth.identity_ca") {
          ca_cert_ = SSL::Certificate(value);

      } else if (name == "dds.sec.auth.private_key") {
          pkey_uri = value;

      } else if (name == "dds.sec.auth.identity_certificate") {
          participant_cert_ = SSL::Certificate(value);

      } else if (name == "dds.sec.auth.password") {
          password = value;
      }
    }

    participant_pkey_ = SSL::PrivateKey(pkey_uri, password);
  }

  ~LocalIdentityData()
  {

  }

  SSL::Certificate& get_ca_cert()
  {
    return ca_cert_;
  }

  SSL::Certificate& get_participant_cert()
  {
    return participant_cert_;
  }

  SSL::PrivateKey& get_participant_private_key()
  {
    return participant_pkey_;
  }

  bool validate()
  {
    return (X509_V_OK == participant_cert_.validate(ca_cert_));
  }

private:

  SSL::Certificate ca_cert_;
  SSL::Certificate participant_cert_;
  SSL::PrivateKey participant_pkey_;
};

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::validate_local_identity(
  ::DDS::Security::IdentityHandle & local_identity_handle,
  ::OpenDDS::DCPS::GUID_t & adjusted_participant_guid,
  ::DDS::Security::DomainId_t domain_id,
  const ::DDS::DomainParticipantQos & participant_qos,
  const ::OpenDDS::DCPS::GUID_t & candidate_participant_guid,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(domain_id);
  ACE_UNUSED_ARG(participant_qos);
  ACE_UNUSED_ARG(ex);

  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_OK;

  LocalIdentityData id_data(participant_qos.property.value);

  if (id_data.validate()) {

    SSL::make_adjusted_guid(candidate_participant_guid,
                            adjusted_participant_guid,
                            id_data.get_participant_cert());

    local_identity_handle = get_next_handle();
    IdentityData_Ptr newDataPtr(new IdentityData());

    // Temporary hack to produce unique guids until real auth is written
    // TODO Replace this!
    if (candidate_participant_guid == OpenDDS::DCPS::GUID_UNKNOWN) {

      static ACE_UINT16 counter = 1024;
      ACE_UINT16 pid = ACE_OS::getpid();

      ACE_OS::macaddr_node_t macaddress;
      ACE_OS::getmacaddress(&macaddress); // ignore return, assume success!

      adjusted_participant_guid.guidPrefix[0] = DCPS::VENDORID_OCI[0];
      adjusted_participant_guid.guidPrefix[1] = DCPS::VENDORID_OCI[1];
      ACE_OS::memcpy(&adjusted_participant_guid.guidPrefix[2], macaddress.node, NODE_ID_SIZE);
      adjusted_participant_guid.guidPrefix[8] = static_cast<CORBA::Octet>(pid >> 8);
      adjusted_participant_guid.guidPrefix[9] = static_cast<CORBA::Octet>(pid & 0xFF);
      adjusted_participant_guid.guidPrefix[10] = static_cast<CORBA::Octet>(counter >> 8);
      adjusted_participant_guid.guidPrefix[11] = static_cast<CORBA::Octet>(counter & 0xFF);

      ++counter;
    }
    else {
      adjusted_participant_guid = candidate_participant_guid;

    }
    newDataPtr->participant_guid = adjusted_participant_guid;

    // Mutex used to protect the identity_data structure
    // Probably not needed in the stub
    {
      ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);
      identity_data_[local_identity_handle] = newDataPtr;
    }

  } else {
    result = DDS::Security::VALIDATION_FAILED;

  }

  return result;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_identity_token(
  ::DDS::Security::IdentityToken & identity_token,
  ::DDS::Security::IdentityHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  // Populate a simple version of an IdentityToken as long as the handle is known
  IdentityData_Ptr local_data = get_identity_data(handle);
  if (local_data) {
    OpenDDS::Security::TokenWriter identity_wrapper(identity_token, Auth_Plugin_Name, 4, 0);
    identity_wrapper.set_property(0, "dds.cert.sn", "MySubjectName", true);
    identity_wrapper.set_property(1, "dds.cert.algo", "RSA-2048", true);
    identity_wrapper.set_property(2, "dds.ca.sn", "MyCASubjectName", true);
    identity_wrapper.set_property(3, "dds.ca.algo", "RSA-204", true);

    status = true;
  } else {
    // No real information on what should be in these security exceptions
    set_security_error(ex, -1, 0, "Unknown Identity handle");
  }
  return status;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_identity_status_token(
  ::DDS::Security::IdentityStatusToken & identity_status_token,
  ::DDS::Security::IdentityHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  // Populate a simple version of an IdentityStatusToken as long as the handle is known
  IdentityData_Ptr local_data = get_identity_data(handle);
  if (local_data) {
    OpenDDS::Security::TokenWriter identity_stat_wrapper(identity_status_token, Auth_Plugin_Name, 1, 0);
    identity_stat_wrapper.set_property(0, "dds.ocps_status", "TBD", true);

    status = true;
  } else {
    // No real information on what should be in these security exceptions
    set_security_error(ex, -1, 0, "Unknown Identity handle");
  }

  return status;
}

::CORBA::Boolean AuthenticationBuiltInImpl::set_permissions_credential_and_token(
  ::DDS::Security::IdentityHandle handle,
  const ::DDS::Security::PermissionsCredentialToken & permissions_credential,
  const ::DDS::Security::PermissionsToken & permissions_token,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  IdentityData_Ptr local_data = get_identity_data(handle);
  if (local_data) {
    // This is not thread safe, but should not pose a problem in the stub implementation
    // as this function should be called before this identity handle is used for any
    // handshake actions
    local_data->permissions_cred_token = permissions_credential;
    local_data->permissions_token = permissions_token;
    status = true;
  } else {
    // TBD - Need definition on what to put into this
    set_security_error(ex, -1, 0, "Identity handle not recognized");
  }
  return status;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::validate_remote_identity(
  ::DDS::Security::IdentityHandle & remote_identity_handle,
  ::DDS::Security::AuthRequestMessageToken & local_auth_request_token,
  const ::DDS::Security::AuthRequestMessageToken & remote_auth_request_token,
  ::DDS::Security::IdentityHandle local_identity_handle,
  const ::DDS::Security::IdentityToken & remote_identity_token,
  const ::OpenDDS::DCPS::GUID_t & remote_participant_guid,
  ::DDS::Security::SecurityException & ex)
{
  // Validate that the local idenitiy handle is valid, and that the class IDs for
  // the local and remote identity token's are compatible.
  IdentityData_Ptr local_data = get_identity_data(local_identity_handle);
  if (!local_data) {
    set_security_error(ex, -1, 0, "Local participant ID not found");
    return DDS::Security::VALIDATION_FAILED;
  }
  if (!check_class_versions(remote_identity_token.class_id)) {
    set_security_error(ex, -1, 0, "Remote class ID is not compatible");
    return DDS::Security::VALIDATION_FAILED;
  }

  // If the remote_auth_request_token is TokenNIL, then the local_auth_request_token
  // needs to be populated and have a future_challenge property supplied.  Otherwise
  // make sure it is set to TokenNIL
  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_FAILED;
  OpenDDS::Security::TokenReader remote_request(remote_auth_request_token);
  if (remote_request.is_nil()) {
    OpenDDS::Security::TokenWriter auth_req_wrapper(local_auth_request_token, 
                                        build_class_id(Auth_Request_Class_Ext), 
                                        1, 
                                        0);
    auth_req_wrapper.set_property(0, "future_challenge", "TODO", true);
  } else {
    local_auth_request_token = DDS::Security::TokenNIL;
  }

  // Retain all of the data needed for a handshake with the remote participant
  IdentityData_Ptr newIdentityData(new IdentityData());
  newIdentityData->participant_guid = remote_participant_guid;
  newIdentityData->local_handle = local_identity_handle;
  newIdentityData->local_auth_request = local_auth_request_token;
  newIdentityData->remote_auth_request = remote_auth_request_token;

  remote_identity_handle = get_next_handle();
  {
    ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);
    identity_data_[remote_identity_handle] = newIdentityData;
  }
  // A Lexigraphical comparison of the guids determines which direction the handshake takes
  if (is_handshake_initiator(local_data->participant_guid, remote_participant_guid)) {
    result = DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST;
  } else {
    result = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  }

  return result;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::begin_handshake_request(
  ::DDS::Security::HandshakeHandle & handshake_handle,
  ::DDS::Security::HandshakeMessageToken & handshake_message,
  ::DDS::Security::IdentityHandle initiator_identity_handle,
  ::DDS::Security::IdentityHandle replier_identity_handle,
  const ::DDS::OctetSeq & serialized_local_participant_data,
  ::DDS::Security::SecurityException & ex)
{
  // Verify that the serialize data is not empty, and that the local and remote
  // handles were linked by a call to validate_remote_identity
  if (serialized_local_participant_data.length() == 0) {
    set_security_error(ex, -1, 0, "No participant data provided");
    return DDS::Security::VALIDATION_FAILED;
}

  IdentityData_Ptr remoteDataPtr = get_identity_data(replier_identity_handle);
  if (!remoteDataPtr) {
    set_security_error(ex, -1, 0, "Unknown remote participant");
    return DDS::Security::VALIDATION_FAILED;
  }
  if (remoteDataPtr->local_handle != initiator_identity_handle) {
    set_security_error(ex, -1, 0, "Participants are not matched");
    return DDS::Security::VALIDATION_FAILED;
  }

  // Populate the handshake output message with some stubbed out properties
  OpenDDS::Security::TokenWriter handshake_wrapper(
  handshake_message, build_class_id(Handshake_Request_Class_Ext), 0, 8);

  unsigned int prop_index = 0;
  handshake_wrapper.set_bin_property(prop_index++, "c.id", Empty_Seq, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.perm", Empty_Seq, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.pdata", serialized_local_participant_data, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.dsign_algo", AlgoName, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.kagree_algo", AgreementAlgo, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.hash_c1", Empty_Seq, true);
  handshake_wrapper.set_bin_property(prop_index++, "c.ocsp_status", Empty_Seq, true);

  // If the remote_auth_request_token is not TokenNIL, then use the challenge from it.
  // Otherwise just use a stubbed out sequence
  OpenDDS::Security::TokenReader auth_wrapper(remoteDataPtr->local_auth_request);
  if (!auth_wrapper.is_nil()) {
    const DDS::OctetSeq& challenge_data = auth_wrapper.get_bin_property_value("future_challenge");
    handshake_wrapper.set_bin_property(prop_index++, "c.challenge1", challenge_data, true);
  } else {
    handshake_wrapper.set_bin_property(prop_index++, "c.challenge1", Empty_Seq, true);
  }

  // The stub doesn't worry about any pre-existing handshakes between these two participants
  // and will always just create a new handshake session
  HandshakeData_Ptr newHandshakeData(new HandshakeData());
  newHandshakeData->local_identity_handle = initiator_identity_handle;
  newHandshakeData->remote_identity_handle = replier_identity_handle;
  newHandshakeData->local_initiator = true;
  newHandshakeData->validation_state = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  handshake_handle = get_next_handle();
  {
    ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);
    handshake_data_[handshake_handle] = newHandshakeData;
  }

  return DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::begin_handshake_reply(
  ::DDS::Security::HandshakeHandle & handshake_handle,
  ::DDS::Security::HandshakeMessageToken & handshake_message_out,
  ::DDS::Security::IdentityHandle initiator_identity_handle,
  ::DDS::Security::IdentityHandle replier_identity_handle,
  const ::DDS::OctetSeq & serialized_local_participant_data,
  ::DDS::Security::SecurityException & ex)
{
  // Verify that the serialize data is not empty, and that the local and remote
  // handles were linked by a call to validate_remote_identity
  if (serialized_local_participant_data.length() == 0) {
    set_security_error(ex, -1, 0, "No participant data provided");
    return DDS::Security::VALIDATION_FAILED;
  }

  // In this case the remote is the initiator
  IdentityData_Ptr remoteDataPtr = get_identity_data(initiator_identity_handle);
  if (!remoteDataPtr) {
    set_security_error(ex, -1, 0, "Unknown remote participant");
    return DDS::Security::VALIDATION_FAILED;
  }
  if (remoteDataPtr->local_handle != replier_identity_handle) {
    set_security_error(ex, -1, 0, "Participants are not matched");
    return DDS::Security::VALIDATION_FAILED;
  }

  // Populate a stub handshake reply message.
  OpenDDS::Security::TokenWriter reply_msg(
  handshake_message_out, build_class_id(Handshake_Reply_Class_Ext), 0, 13);
  int prop_index = 0;
  reply_msg.set_bin_property(prop_index++, "c.id", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "c.perm", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "c.pdata", serialized_local_participant_data, true);
  reply_msg.set_bin_property(prop_index++, "c.dsign_algo", AlgoName, true);
  reply_msg.set_bin_property(prop_index++, "c.kagree_algo", AgreementAlgo, true);
  reply_msg.set_bin_property(prop_index++, "hash_c2", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "dh2", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "hash_c1", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "dh1", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "challenge1", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "challenge2", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "ocsp_status", Empty_Seq, true);
  reply_msg.set_bin_property(prop_index++, "signature", Empty_Seq, true);

  // If the local_auth_request_token is not TokenNIL, then use the challenge from it.
  // Otherwise just use a stubbed out sequence
  OpenDDS::Security::TokenReader auth_wrapper(remoteDataPtr->local_auth_request);
  if (!auth_wrapper.is_nil()) {
    const DDS::OctetSeq& challenge_data = auth_wrapper.get_bin_property_value("future_challenge");
    reply_msg.set_bin_property(prop_index++, "challenge1", challenge_data, true);
  }
  else {
    reply_msg.set_bin_property(prop_index++, "challenge1", Empty_Seq, true);
  }

  // The stub doesn't worry about any pre-existing handshakes between these two participants
  // and will always just create a new handshake session
  HandshakeData_Ptr newHandshakeData(new HandshakeData());
  newHandshakeData->local_identity_handle = replier_identity_handle;
  newHandshakeData->remote_identity_handle = initiator_identity_handle;
  newHandshakeData->local_initiator = false;
  newHandshakeData->validation_state = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  handshake_handle = get_next_handle();
  {
    ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);
    handshake_data_[handshake_handle] = newHandshakeData;
  }

  return DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::process_handshake(
  ::DDS::Security::HandshakeMessageToken & handshake_message_out,
  const ::DDS::Security::HandshakeMessageToken & handshake_message_in,
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  // - SecurityException is populated if VALIDATION_FAILED
  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_OK;

  // Handle differently based on which direction this handshake is going
  std::string incoming_class_ext = get_extension(handshake_message_in.class_id);
  if (0 == Handshake_Reply_Class_Ext.compare(incoming_class_ext))
  {
    result = process_handshake_reply(handshake_message_out, handshake_message_in, handshake_handle, ex);
  }
  else if (0 == Handshake_Final_Class_Ext.compare(incoming_class_ext))
  {
    result = process_final_handshake(handshake_message_in, handshake_handle, ex);
  }

  return result;
}

::DDS::Security::SharedSecretHandle AuthenticationBuiltInImpl::get_shared_secret(
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  DDS::Security::SharedSecretHandle secret_handle = 0; // Nil for invalid handle

  // Return a non-zero value if the handshake handle is valid
  HandshakeData_Ptr handshakeData = get_handshake_data(handshake_handle);
  if (handshakeData) {
    secret_handle = handshakeData->secret_handle;
  } else {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
  }

  return secret_handle;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_authenticated_peer_credential_token(
  ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean result = false;

  HandshakeData_Ptr handshakeData = get_handshake_data(handshake_handle);
  if (handshakeData) {
    OpenDDS::Security::TokenWriter peer_token(peer_credential_token, Auth_Peer_Cred_Token_Class_Id, 2, 0);
    peer_token.set_property(0, "c.id", "CertificateContents", true);
    peer_token.set_property(1, "c.perm", "PermissionsDocument", true);
    result = true;
  } else {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
  }

  return result;
}

::CORBA::Boolean AuthenticationBuiltInImpl::set_listener(
  ::DDS::Security::AuthenticationListener_ptr listener,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean results = false;

  if (NULL == listener) {
    set_security_error(ex, -1, 0, "Null listener provided");
  } else {
    results = true;
    listener_ptr_ = listener;
  }
  return results;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_identity_token(
  const ::DDS::Security::IdentityToken & token,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here in the stub version
  ACE_UNUSED_ARG(token);
  ACE_UNUSED_ARG(ex);
  return true;
}


::CORBA::Boolean AuthenticationBuiltInImpl::return_identity_status_token(
  const ::DDS::Security::IdentityStatusToken & token,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here in the stub version
  ACE_UNUSED_ARG(token);
  ACE_UNUSED_ARG(ex);
  return true;
}


::CORBA::Boolean AuthenticationBuiltInImpl::return_authenticated_peer_credential_token(
  const ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here in the stub version
  ACE_UNUSED_ARG(peer_credential_token);
  ACE_UNUSED_ARG(ex);
  return true;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_handshake_handle(
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean results = false;

  // Cleanup the handshake data from the map
  ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);

  Handshake_Handle_Data::iterator iHandshake = handshake_data_.find(handshake_handle);
  if (iHandshake != handshake_data_.end()) {
    delete iHandshake->second;
    iHandshake->second = 0;
    handshake_data_.erase(iHandshake);
    results = true;
  } else {
    set_security_error(ex, -1, 0, "Handshake handle not recognized");
  }
  return results;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_identity_handle(
  ::DDS::Security::IdentityHandle identity_handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean results = false;

  // Cleanup the identity data from the map
  ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);

  Identity_Handle_Data::iterator iData = identity_data_.find(identity_handle);
  if (iData != identity_data_.end()) {
    delete iData->second;
    iData->second = 0;
    identity_data_.erase(iData);
    results = true;
  } else {
    set_security_error(ex, -1, 0, "Handshake handle not recognized");
  }
  return results;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_sharedsecret_handle(
  ::DDS::Security::SharedSecretHandle sharedsecret_handle,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here in the stub version
  ACE_UNUSED_ARG(sharedsecret_handle);
  ACE_UNUSED_ARG(ex);
  return true;
}


DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::process_handshake_reply(
  DDS::Security::HandshakeMessageToken & handshake_message_out,
  const DDS::Security::HandshakeMessageToken & handshake_message_in,
  DDS::Security::HandshakeHandle handshake_handle,
  DDS::Security::SecurityException & ex)
{
  // The real version of this method will have to validate the credentials on the input message
  // but this stub version will just verify that the pre-requistes have been met and then fill
  // out a simple final message
  ACE_UNUSED_ARG(handshake_message_in);

  HandshakeData_Ptr handshakePtr = get_handshake_data(handshake_handle);
  if (!handshakePtr) {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
    return DDS::Security::VALIDATION_FAILED;
  }

  IdentityData_Ptr remoteDataPtr = get_identity_data(handshakePtr->remote_identity_handle);
  if (!remoteDataPtr) {
    set_security_error(ex, -1, 0, "Unknown remote participant for handshake");
    return DDS::Security::VALIDATION_FAILED;
  }

  if (handshakePtr->validation_state != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
    set_security_error(ex, -1, 0, "Handshake state is not valid");
    return DDS::Security::VALIDATION_FAILED;
  }

  // This stub just verifies that the handshake was started properly, it doesn't
  // actually verify anything
  OpenDDS::Security::TokenWriter final_msg(handshake_message_out, build_class_id(Handshake_Final_Class_Ext), 0, 7);
  int prop_index = 0;
  final_msg.set_bin_property(prop_index++, "hash_c1", Empty_Seq, true); // Optional (troubleshooting)
  final_msg.set_bin_property(prop_index++, "hash_c2", Empty_Seq, true); // Optional (troubleshooting)
  final_msg.set_bin_property(prop_index++, "dh1", Empty_Seq, true);     // Optional (troubleshooting)
  final_msg.set_bin_property(prop_index++, "dh2", Empty_Seq, true);     // Optional (troubleshooting)
  final_msg.set_bin_property(prop_index++, "challenge_1", Empty_Seq, true);
  final_msg.set_bin_property(prop_index++, "challenge_2", Empty_Seq, true);
  final_msg.set_bin_property(prop_index++, "signature", Empty_Seq, true);

  // The handshake is now complete, assign a shared secret handle in the stub
  handshakePtr->validation_state = DDS::Security::VALIDATION_OK_FINAL_MESSAGE;
  handshakePtr->secret_handle = get_next_handle();

  return DDS::Security::VALIDATION_OK_FINAL_MESSAGE;
}

DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::process_final_handshake(
  const DDS::Security::HandshakeMessageToken & handshake_message_in,
  DDS::Security::HandshakeHandle handshake_handle,
  DDS::Security::SecurityException & ex)
{
  // The real version of this method will have to validate the credentials on the input message
  // but this stub version will just verify that the pre-requistes have been met and then return
  ACE_UNUSED_ARG(handshake_message_in);

  HandshakeData_Ptr handshakePtr = get_handshake_data(handshake_handle);
  if (!handshakePtr) {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
    return DDS::Security::VALIDATION_FAILED;
  }

  IdentityData_Ptr remoteDataPtr = get_identity_data(handshakePtr->remote_identity_handle);
  if (!remoteDataPtr) {
    set_security_error(ex, -1, 0, "Unknown remote participant for handshake");
    return DDS::Security::VALIDATION_FAILED;
  }

  if (handshakePtr->validation_state != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
    set_security_error(ex, -1, 0, "Handshake state is not valid");
    return DDS::Security::VALIDATION_FAILED;
  }

  // This function is only ever called for a handshake final message, so handshaking is complete
  // Just create a shared secret handle in the stub
  handshakePtr->validation_state = DDS::Security::VALIDATION_OK;
  handshakePtr->secret_handle = get_next_handle();

  return DDS::Security::VALIDATION_OK;
}

AuthenticationBuiltInImpl::HandshakeData_Ptr AuthenticationBuiltInImpl::get_handshake_data(DDS::Security::HandshakeHandle handle)
{
  ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);

  // Mutex controls adding/removing handshakes, but not the contents of the handshakes
  HandshakeData_Ptr dataPtr(0);
  Handshake_Handle_Data::iterator iData = handshake_data_.find(handle);
  if (iData != handshake_data_.end()) {
    dataPtr = iData->second;
  }

  return dataPtr;
}

AuthenticationBuiltInImpl::IdentityData_Ptr AuthenticationBuiltInImpl::get_identity_data(DDS::Security::IdentityHandle handle)
{
  ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);

  // Mutex controls adding/removing identitiy data, but not the contents of the data
  IdentityData_Ptr dataPtr(0);
  Identity_Handle_Data::iterator iData = identity_data_.find(handle);
  if (iData != identity_data_.end()) {
    dataPtr = iData->second;
  }

  return dataPtr;
}

bool AuthenticationBuiltInImpl::is_handshake_initiator(const OpenDDS::DCPS::GUID_t& local, const OpenDDS::DCPS::GUID_t& remote)
{
  // Stub will just return true
  ACE_UNUSED_ARG(local);
  ACE_UNUSED_ARG(remote);

  return true;
}

bool AuthenticationBuiltInImpl::check_class_versions(const char* remote_class_id)
{
  if (NULL == remote_class_id) {
    return false;
    }
  bool class_matches = false;

  // Slow, but this is just for the stub
  std::string class_id_str(remote_class_id);

  // Class name is the text prior to the final ':'
  size_t colon_pos = class_id_str.find_last_of(':');
  if (std::string::npos != colon_pos && colon_pos > 0) {
    // Compare the class name vs the expected class name
    std::string remote_class_name  = class_id_str.substr(0, colon_pos);
    if (0 == Auth_Plugin_Name.compare(remote_class_name)) {
      // Major version is the text between the final : and a  .
      size_t major_start = colon_pos + 1;
      size_t period_pos = class_id_str.find_first_of('.', major_start);
      if (std::string::npos != period_pos && period_pos > major_start) {
        std::string major_version = class_id_str.substr(major_start, period_pos - major_start);
        if (0 == Auth_Plugin_Major_Version.compare(major_version)) {
          class_matches = true;
        }
      }
    }
  }

  return class_matches;
}

void AuthenticationBuiltInImpl::set_security_error(DDS::Security::SecurityException& ex, int code, int minor_code, const char* message)
{
  ex.code = code;
  ex.minor_code = minor_code;
  ex.message = message;
}

std::string AuthenticationBuiltInImpl::build_class_id(const std::string& message_ext)
{
  std::stringstream class_id_stream;
  class_id_stream << Auth_Plugin_Name
    << ":" << Auth_Plugin_Major_Version
    << "." << Auth_Plugin_Minor_Version
    << "+" << message_ext;

  return class_id_stream.str();
}

std::string AuthenticationBuiltInImpl::get_extension(const char* class_id)
{
  std::string ext_string("");

  std::string class_id_str(class_id);
  size_t extension_delim_pos = class_id_str.find_last_of('+');
  if (extension_delim_pos != std::string::npos) {
    size_t start_ext_pos = extension_delim_pos + 1;
    if (start_ext_pos < class_id_str.length()) {
    ext_string = class_id_str.substr(start_ext_pos);
    }
  }

  return ext_string;
}

uint64_t AuthenticationBuiltInImpl::get_next_handle()
{
  return next_handle_++;
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL


