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
#include "dds/DCPS/LocalObject.h"
#include "ace/config-macros.h"
#include "ace/Guard_T.h"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdio>

#include "dds/DCPS/security/SSL/Utils.h"

// Temporary include for get macaddress for unique guids
#include <ace/OS_NS_netdb.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

// Supported message versions and IDs - using these for stub version
static const std::string Auth_Plugin_Name("DDS:Auth:PKI-DH");
static const std::string Auth_Plugin_Major_Version("1");
static const std::string Auth_Plugin_Minor_Version("0");

static const std::string Identity_Status_Token_Class_Id("DDS:Auth:PKI-DH:1.0");
static const std::string Auth_Peer_Cred_Token_Class_Id("DDS:Auth:PKI-DH:1.0");
static const std::string Auth_Request_Class_Ext("AuthReq");
static const std::string Handshake_Request_Class_Ext("Req");
static const std::string Handshake_Reply_Class_Ext("Reply");
static const std::string Handshake_Final_Class_Ext("Final");

// Until a real implementation is created, a stub sequence will be sent for data
static const DDS::OctetSeq Empty_Seq;

struct SharedSecret : DCPS::LocalObject<DDS::Security::SharedSecretHandle> {

  SharedSecret(DDS::OctetSeq challenge1,
               DDS::OctetSeq challenge2,
               DDS::OctetSeq sharedSecret)
    : challenge1_(challenge1)
    , challenge2_(challenge2)
    , shared_secret_(sharedSecret)
  {}

  DDS::OctetSeq* challenge1() { return new DDS::OctetSeq(challenge1_); }
  DDS::OctetSeq* challenge2() { return new DDS::OctetSeq(challenge1_); }
  DDS::OctetSeq* sharedSecret() { return new DDS::OctetSeq(shared_secret_); }

  DDS::OctetSeq challenge1_, challenge2_, shared_secret_;
};

AuthenticationBuiltInImpl::AuthenticationBuiltInImpl()
: listener_ptr_()
, local_auth_data_mutex_()
, identity_mutex_()
, handshake_mutex_()
, handle_mutex_()
, next_handle_(1ULL)
, local_credential_data_()
{
}

AuthenticationBuiltInImpl::~AuthenticationBuiltInImpl()
{

}

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

  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_FAILED;

  ACE_Guard<ACE_Thread_Mutex> guard(local_auth_data_mutex_);

  local_credential_data_.load(participant_qos.property.value);

  if (local_credential_data_.validate()) {
    if (candidate_participant_guid != DCPS::GUID_UNKNOWN) {

      int err = SSL::make_adjusted_guid(candidate_participant_guid,
                                        adjusted_participant_guid,
                                        local_credential_data_.get_participant_cert());
      if (! err) {
        local_identity_handle = get_next_handle();

        IdentityData_Ptr local_identity = DCPS::make_rch<IdentityData>();
        local_identity->participant_guid = adjusted_participant_guid;

        {
          ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);
          identity_data_[local_identity_handle] = local_identity;
        }

        result = DDS::Security::VALIDATION_OK;
      }
    } else {
        fprintf(stderr,
                "AuthenticationBuiltInImpl::validate_local_identity: "
                "Error, GUID_UNKNOWN passed in for candidate_participant_guid\n");
    }
  }

  return result;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_identity_token(
  ::DDS::Security::IdentityToken & identity_token,
  ::DDS::Security::IdentityHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  IdentityData_Ptr local_data = get_identity_data(handle);
  if (local_data) {
    const SSL::Certificate& pcert = local_credential_data_.get_participant_cert();
    const SSL::Certificate& cacert = local_credential_data_.get_ca_cert();

    std::string tmp;

    OpenDDS::Security::TokenWriter identity_wrapper(identity_token, "DDS:Auth:PKI-DH:1.0", 4, 0);

    pcert.subject_name_to_str(tmp);
    identity_wrapper.set_property(0, "dds.cert.sn", tmp.c_str(), true);

    pcert.algorithm(tmp);
    identity_wrapper.set_property(1, "dds.cert.algo", tmp.c_str(), true);

    cacert.subject_name_to_str(tmp);
    identity_wrapper.set_property(2, "dds.ca.sn", tmp.c_str(), true);

    cacert.algorithm(tmp);
    identity_wrapper.set_property(3, "dds.ca.algo", tmp.c_str(), true);

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
    OpenDDS::Security::TokenWriter identity_stat_wrapper(identity_status_token, Identity_Status_Token_Class_Id, 1, 0);
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
  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_OK;

  IdentityData_Ptr local_data = get_identity_data(local_identity_handle);
  if (local_data) {
    if (check_class_versions(remote_identity_token.class_id)) {

      TokenReader remote_request(remote_auth_request_token);
      if (remote_request.is_nil()) {

        DDS::OctetSeq nonce;
        int err = SSL::make_nonce_256(nonce);
        if (! err) {
          TokenWriter auth_req_wrapper(local_auth_request_token,
                                       build_class_id(Auth_Request_Class_Ext),
                                       0,
                                       1);

          auth_req_wrapper.set_bin_property(0, "future_challenge", nonce, true);

        } else {
          result = DDS::Security::VALIDATION_FAILED;
        }

      } else {
        local_auth_request_token = DDS::Security::TokenNIL;
      }

      if (result == DDS::Security::VALIDATION_OK) {

        /* Retain all of the data needed for a handshake with the remote participant */
        IdentityData_Ptr newIdentityData = DCPS::make_rch<IdentityData>();
        newIdentityData->participant_guid = remote_participant_guid;
        newIdentityData->local_handle = local_identity_handle;
        newIdentityData->local_auth_request = local_auth_request_token;
        newIdentityData->remote_auth_request = remote_auth_request_token;

        remote_identity_handle = get_next_handle();
        {
          ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);
          identity_data_[remote_identity_handle] = newIdentityData;
        }

        if (is_handshake_initiator(local_data->participant_guid, remote_participant_guid)) {
          result = DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST;

        } else {
          result = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
        }
      }

    } else {
      set_security_error(ex, -1, 0, "Remote class ID is not compatible");
      result = DDS::Security::VALIDATION_FAILED;
    }

  } else {
    set_security_error(ex, -1, 0, "Local participant ID not found");
    result = DDS::Security::VALIDATION_FAILED;
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
  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_FAILED;

  if (serialized_local_participant_data.length() != 0) {

      IdentityData_Ptr replier_data = get_identity_data(replier_identity_handle);
      if (replier_data) {
        if (replier_data->local_handle == initiator_identity_handle) {

          OpenDDS::Security::TokenWriter handshake_wrapper(handshake_message,
                                                           build_class_id(Handshake_Request_Class_Ext),
                                                           0, 7);
          unsigned int prop_index = 0;
          DDS::OctetSeq tmp;

          local_credential_data_.get_participant_cert().serialize(tmp);
          handshake_wrapper.set_bin_property(prop_index++, "c.id", tmp, true);

          handshake_wrapper.set_bin_property(prop_index++, "c.perm", Empty_Seq, true); /* TODO */
          handshake_wrapper.set_bin_property(prop_index++, "c.pdata", serialized_local_participant_data, true);
          handshake_wrapper.set_bin_property(prop_index++, "c.dsign_algo", "RSASSA-PSS-SHA256", true);
          handshake_wrapper.set_bin_property(prop_index++, "c.kagree_algo", "DH+MODP-2048-256", true);

          local_credential_data_.get_diffie_hellman_key().pub_key(tmp);
          handshake_wrapper.set_bin_property(prop_index++, "dh1", tmp, true);

          OpenDDS::Security::TokenReader auth_wrapper(replier_data->local_auth_request);
          if (auth_wrapper.is_nil()) {
            DDS::OctetSeq nonce;
            int err = SSL::make_nonce_256(nonce);
            if (! err) {
              handshake_wrapper.set_bin_property(prop_index++, "challenge1", nonce, true);

            } else {
              return DDS::Security::VALIDATION_FAILED;
            }

          } else {
            const DDS::OctetSeq& challenge_data = auth_wrapper.get_bin_property_value("future_challenge");
            handshake_wrapper.set_bin_property(prop_index++, "challenge1", challenge_data, true);

          }

          HandshakeData_Ptr newHandshakeData = DCPS::make_rch<HandshakeData>();
          newHandshakeData->local_identity_handle = initiator_identity_handle;
          newHandshakeData->remote_identity_handle = replier_identity_handle;
          newHandshakeData->local_initiator = true;
          newHandshakeData->validation_state = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
          newHandshakeData->request_token = handshake_message;
          newHandshakeData->reply_token = DDS::Security::TokenNIL;

          handshake_handle = get_next_handle();
          {
            ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);
            handshake_data_[handshake_handle] = newHandshakeData;
          }

          result = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;

        } else {
            set_security_error(ex, -1, 0, "Participants are not matched");
        }

      } else {
        set_security_error(ex, -1, 0, "Unknown remote participant");
      }

  } else {
      set_security_error(ex, -1, 0, "No participant data provided");
  }

  return result;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::begin_handshake_reply(
  ::DDS::Security::HandshakeHandle & handshake_handle,
  ::DDS::Security::HandshakeMessageToken & handshake_message_out,
  ::DDS::Security::IdentityHandle initiator_identity_handle,
  ::DDS::Security::IdentityHandle replier_identity_handle,
  const ::DDS::OctetSeq & serialized_local_participant_data,
  ::DDS::Security::SecurityException & ex)
{
  using OpenDDS::Security::TokenWriter;
  using OpenDDS::Security::TokenReader;

  const DDS::Security::ValidationResult_t Failure = DDS::Security::VALIDATION_FAILED,
                                          Pending = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;

  if (serialized_local_participant_data.length() == 0) {
    set_security_error(ex, -1, 0, "No participant data provided");
    return Failure;
  }

  IdentityData_Ptr initiator_id_data = get_identity_data(initiator_identity_handle);
  if (!initiator_id_data) {
    set_security_error(ex, -1, 0, "Unknown remote participant");
    return Failure;
  }
  if (initiator_id_data->local_handle != replier_identity_handle) {
    set_security_error(ex, -1, 0, "Participants are not matched");
    return Failure;
  }

  TokenReader message_in(handshake_message_out);
  if (message_in.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake_message_out is an inout param and must not be nil");
    return Failure;

  } else {
    const DDS::OctetSeq& challenge1 = message_in.get_bin_property_value("challenge1");

    TokenReader initiator_remote_auth_request(initiator_id_data->remote_auth_request);
    if (! initiator_remote_auth_request.is_nil()) {
      const DDS::OctetSeq& future_challenge = initiator_remote_auth_request.get_bin_property_value("future_challenge");

      if ((challenge1.length()) < 1 || (future_challenge.length() < 1)) {
        return Failure;
      }
      if (challenge1.length() != future_challenge.length()) {
        return Failure;
      }

      if (0 != std::memcmp(challenge1.get_buffer(), future_challenge.get_buffer(), future_challenge.length())) {
        return Failure;
      }
    }

    SSL::Certificate cert;

    const DDS::OctetSeq& cid = message_in.get_bin_property_value("c.id");
    if (cid.length() > 0) {

      cert.deserialize(cid);
      if (X509_V_OK != cert.validate(local_credential_data_.get_ca_cert()))
      {
        set_security_error(ex, -1, 0, "Certificate validation failed");
        return Failure;
      }

    } else {
      set_security_error(ex, -1, 0, "Certificate validation failed due to empty 'c.id' property supplied");
      return Failure;
    }

    /* Verify first bit of "c.pdata" is set to 1 and check the following 47 bits match the
     * SHA-256 hash of SubjectName appearing in the IdentityCredential */

    const DDS::OctetSeq& cpdata = message_in.get_bin_property_value("c.pdata");
    if (cpdata.length() > 5u) { /* Enough to withstand the hash-comparison below */

      /* Make sure first bit is set */
      if (! cpdata[0] & 0x80) {
        set_security_error(ex, -1, 0, "Malformed ParticipantBuiltinTopicData in 'c.pdata'; First bit must be set.");
        return Failure;
      }

      std::vector<unsigned char> hash;
      if (0 != cert.subject_name_digest(hash)) {
        return Failure;
      }

      for (size_t i = 0; i <= 5u; ++i) {
        if (hash[i] != SSL::offset_1bit(cpdata.get_buffer(), i)) {
          set_security_error(ex, -1, 0, "Bits 2 - 48 of 'c.pdata' does not match first 47 bits of subject-name hash in 'c.id'");
          return Failure;
        }
      }

    } else {
      set_security_error(ex, -1, 0, "Empty ParticipantBuiltinTopicData in 'c.pdata'");
      return Failure;
    }
  }

  /* TODO Add OCSP checks when "ocsp_status" property is given in message_in.
   * Most of this logic would probably be placed in the Certificate directly
   * or an OCSP abstraction that the Certificate uses. */


  /* Add routines to get property by index and modify them inline */


  // Populate a stub handshake reply message.
  TokenWriter message_out(handshake_message_out,
                          build_class_id(Handshake_Reply_Class_Ext), 0, 13);
  int prop_index = 0;
  message_out.set_bin_property(prop_index++, "c.id", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "c.perm", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "c.pdata", serialized_local_participant_data, true);
  message_out.set_bin_property(prop_index++, "c.dsign_algo", "RSASSA-PSS-SHA256", true);
  message_out.set_bin_property(prop_index++, "c.kagree_algo", "DH+MODP-2048-256", true);
  message_out.set_bin_property(prop_index++, "hash_c2", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "dh2", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "hash_c1", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "dh1", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "challenge1", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "challenge2", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "ocsp_status", Empty_Seq, true);
  message_out.set_bin_property(prop_index++, "signature", Empty_Seq, true);


  TokenReader initiator_local_auth_request(initiator_id_data->local_auth_request);
  if (! initiator_local_auth_request.is_nil()) {
    const DDS::OctetSeq& future_challenge = initiator_local_auth_request.get_bin_property_value("future_challenge");
    message_out.set_bin_property(prop_index++, "challenge2", future_challenge, true);

  } else {
    DDS::OctetSeq nonce;
    int err = SSL::make_nonce_256(nonce);
    if (! err) {
        message_out.set_bin_property(prop_index++, "challenge2", nonce, true);

    } else {
      return Failure;
    }
  }


  HandshakeData_Ptr newHandshakeData = DCPS::make_rch<HandshakeData>();
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

::DDS::Security::SharedSecretHandle* AuthenticationBuiltInImpl::get_shared_secret(
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  // Return a non-zero value if the handshake handle is valid
  HandshakeData_Ptr handshakeData = get_handshake_data(handshake_handle);
  if (handshakeData) {
    DDS::Security::SharedSecretHandle_var handle = handshakeData->secret_handle;
    return handle._retn();
  } else {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
    return 0;
  }
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
    identity_data_.erase(iData);
    results = true;

  } else {
    set_security_error(ex, -1, 0, "Handshake handle not recognized");
  }
  return results;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_sharedsecret_handle(
  ::DDS::Security::SharedSecretHandle* sharedsecret_handle,
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
  handshakePtr->secret_handle = new SharedSecret(Empty_Seq, Empty_Seq, Empty_Seq);

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
  handshakePtr->secret_handle = new SharedSecret(Empty_Seq, Empty_Seq, Empty_Seq);

  return DDS::Security::VALIDATION_OK;
}

AuthenticationBuiltInImpl::HandshakeData_Ptr AuthenticationBuiltInImpl::get_handshake_data(DDS::Security::HandshakeHandle handle)
{
  ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);

  // Mutex controls adding/removing handshakes, but not the contents of the handshakes
  HandshakeData_Ptr dataPtr;
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
  IdentityData_Ptr dataPtr;
  Identity_Handle_Data::iterator iData = identity_data_.find(handle);
  if (iData != identity_data_.end()) {
    dataPtr = iData->second;
  }

  return dataPtr;
}

bool AuthenticationBuiltInImpl::is_handshake_initiator(const OpenDDS::DCPS::GUID_t& local, const OpenDDS::DCPS::GUID_t& remote)
{
  const unsigned char* local_ = reinterpret_cast<const unsigned char*>(&local);
  const unsigned char* remote_ = reinterpret_cast<const unsigned char*>(&remote);

  /* if remote > local, pending request; else pending handshake message */
  return std::lexicographical_compare(local_, local_ + sizeof(local),
                                      remote_, remote_ + sizeof(remote));

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


