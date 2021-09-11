/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "AuthenticationBuiltInImpl.h"

#include "CommonUtilities.h"
#include "TokenReader.h"
#include "TokenWriter.h"
#include "SSL/Utils.h"

#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/Serializer.h"

#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

#include "ace/config-macros.h"
#include "ace/Guard_T.h"

#include <sstream>
#include <vector>
#include <algorithm>
#include <cstdio>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

using CommonUtilities::set_security_error;

static bool challenges_match(const DDS::OctetSeq& c1, const DDS::OctetSeq& c2);

static void extract_participant_guid_from_cpdata(const DDS::OctetSeq& cpdata, DCPS::GUID_t& dst);

static bool validate_topic_data_guid(const DDS::OctetSeq& cpdata,
                                     const std::vector<unsigned char>& subject_name_hash,
                                     DDS::Security::SecurityException& ex);

const std::string Auth_Plugin_Name("DDS:Auth:PKI-DH");
const std::string Auth_Plugin_Major_Version("1");
const std::string Auth_Plugin_Minor_Version("0");

const std::string Auth_Request_Class_Ext("AuthReq");
const std::string Handshake_Request_Class_Ext("Req");
const std::string Handshake_Reply_Class_Ext("Reply");
const std::string Handshake_Final_Class_Ext("Final");

const char* AuthenticationBuiltInImpl::PROPERTY_HANDSHAKE_DEBUG = "opendds.sec.auth.handshake_debug";

struct SharedSecret : DCPS::LocalObject<DDS::Security::SharedSecretHandle> {

  SharedSecret(DDS::OctetSeq challenge1,
               DDS::OctetSeq challenge2,
               DDS::OctetSeq sharedSecret)
    : challenge1_(challenge1)
    , challenge2_(challenge2)
    , shared_secret_(sharedSecret)
  {}

  DDS::OctetSeq* challenge1() { return new DDS::OctetSeq(challenge1_); }
  DDS::OctetSeq* challenge2() { return new DDS::OctetSeq(challenge2_); }
  DDS::OctetSeq* sharedSecret() { return new DDS::OctetSeq(shared_secret_); }

  DDS::OctetSeq challenge1_, challenge2_, shared_secret_;
};

AuthenticationBuiltInImpl::AuthenticationBuiltInImpl()
: listener_ptr_()
, identity_mutex_()
, handshake_mutex_()
, handle_mutex_()
, next_handle_(1)
{
}

AuthenticationBuiltInImpl::~AuthenticationBuiltInImpl()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("AuthenticationBuiltInImpl::~AuthenticationBuiltInImpl local_participants_ %B handshake_data_ %B\n"),
               local_participants_.size(),
               handshake_data_.size()));
  }
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::validate_local_identity(
  ::DDS::Security::IdentityHandle & local_identity_handle,
  ::OpenDDS::DCPS::GUID_t & adjusted_participant_guid,
  ::DDS::Security::DomainId_t /*domain_id*/,
  const ::DDS::DomainParticipantQos & participant_qos,
  const ::OpenDDS::DCPS::GUID_t & candidate_participant_guid,
  ::DDS::Security::SecurityException & ex)
{
  DDS::Security::ValidationResult_t result = DDS::Security::VALIDATION_FAILED;

  LocalAuthCredentialData::shared_ptr credentials = DCPS::make_rch<LocalAuthCredentialData>();
  if (!credentials->load_credentials(participant_qos.property.value, ex)) {
    return result;
  }

  if (credentials->validate()) {
    if (candidate_participant_guid != DCPS::GUID_UNKNOWN) {

      int err = SSL::make_adjusted_guid(candidate_participant_guid,
                                        adjusted_participant_guid,
                                        credentials->get_participant_cert());
      if (!err) {
        local_identity_handle = get_next_handle();

        LocalParticipantData::shared_ptr local_participant = DCPS::make_rch<LocalParticipantData>();
        local_participant->participant_guid = adjusted_participant_guid;
        local_participant->credentials = credentials;
        for (unsigned i = 0; i < participant_qos.property.value.length(); ++i) {
          if (std::strcmp(PROPERTY_HANDSHAKE_DEBUG,
                          participant_qos.property.value[i].name.in()) == 0) {
            local_participant->handshake_debug = true;
          }
        }

        {
          ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);
          local_participants_[local_identity_handle] = local_participant;

          if (DCPS::security_debug.bookkeeping) {
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                       ACE_TEXT("AuthenticationBuiltInImpl::validate_local_identity local_participants_ (total %B)\n"),
                       local_participants_.size()));
          }
        }

        result = DDS::Security::VALIDATION_OK;

      } else {
        set_security_error(ex, -1, 0, "SSL::make_adjusted_guid failed");
      }

    } else {
      set_security_error(ex, -1, 0, "GUID_UNKNOWN passed in for candidate_participant_guid");
    }

  } else {
    set_security_error(ex, -1, 0, "local-credential-data failed validation");
  }

  return result;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_identity_token(
  ::DDS::Security::IdentityToken & identity_token,
  ::DDS::Security::IdentityHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  LocalParticipantData::shared_ptr local_data = get_local_participant(handle);
  if (local_data) {
    const LocalAuthCredentialData& local_credential_data = *(local_data->credentials);

    const SSL::Certificate& pcert = local_credential_data.get_participant_cert();
    const SSL::Certificate& cacert = local_credential_data.get_ca_cert();

    std::string tmp;

    OpenDDS::Security::TokenWriter identity_wrapper(identity_token, Identity_Status_Token_Class_Id);

    pcert.subject_name_to_str(tmp);
    identity_wrapper.add_property(dds_cert_sn, tmp.c_str());
    identity_wrapper.add_property(dds_cert_algo, pcert.keypair_algo());

    cacert.subject_name_to_str(tmp);
    identity_wrapper.add_property(dds_ca_sn, tmp.c_str());
    identity_wrapper.add_property(dds_ca_algo, cacert.keypair_algo());

    status = true;

  } else {
    set_security_error(ex, -1, 0, "Unknown Identity handle");
  }
  return status;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_identity_status_token(
  ::DDS::Security::IdentityStatusToken&,
  ::DDS::Security::IdentityHandle handle,
  ::DDS::Security::SecurityException & ex)
{
  ::CORBA::Boolean status = false;

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  // Populate a simple version of an IdentityStatusToken as long as the handle is known
  LocalParticipantData::shared_ptr local_data = get_local_participant(handle);
  if (local_data) {

    // TODO: Pending AuthenticationListener support (see security spec. 8.3.2.2).
    // This routine will most likely populate the IdentityStatusToken with
    // useful data once this has been completed. For now it's a glorified no-op!

    status = true;
  } else {
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
  ACE_UNUSED_ARG(permissions_token);

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  LocalParticipantData::shared_ptr local_data = get_local_participant(handle);
  if (! local_data) {
    set_security_error(ex, -1, 0, "Identity handle not recognized");
    return false;
  }

  return local_data->credentials->load_access_permissions(permissions_credential, ex);
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
  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  LocalParticipantData::shared_ptr local_data = get_local_participant(local_identity_handle);

  if (!local_data) {
    set_security_error(ex, -1, 0, "Local participant ID not found");
    return DDS::Security::VALIDATION_FAILED;
  }

  if (!check_class_versions(remote_identity_token.class_id)) {
    set_security_error(ex, -1, 0, "Remote class ID is not compatible");
    return DDS::Security::VALIDATION_FAILED;
  }

  // Make sure that a remote_participant_guid has not already been assigned a
  // remote-identity-handle before creating a new one.
  RemoteParticipantMap::iterator begin = local_data->validated_remotes.begin(),
    end = local_data->validated_remotes.end(),
    found = std::find_if(begin, end,
                         was_guid_validated(remote_participant_guid));

  if (found == end) {
    // Generate local token.
    DDS::OctetSeq nonce;
    int err = SSL::make_nonce_256(nonce);
    if (err) {
      set_security_error(ex, -1, 0, "Failed to generate 256-bit nonce value for future_challenge property");
      return DDS::Security::VALIDATION_FAILED;
    }

    TokenWriter auth_req_wrapper(local_auth_request_token, build_class_id(Auth_Request_Class_Ext));
    auth_req_wrapper.add_bin_property("future_challenge", nonce);

    // Retain all of the data needed for a handshake with the remote participant
    RemoteParticipantData::shared_ptr remote_participant = DCPS::make_rch<RemoteParticipantData>();
    remote_participant->participant_guid = remote_participant_guid;
    remote_participant->local_participant = local_identity_handle;
    remote_participant->local_auth_request = local_auth_request_token;

    remote_identity_handle = get_next_handle();
    found = local_data->validated_remotes.insert(std::make_pair(remote_identity_handle, remote_participant)).first;

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::validate_remote_identity validated_remotes (total %B)\n"),
                 local_data->validated_remotes.size()));
    }
  }

  // Update the remote token.
  found->second->remote_auth_request = remote_auth_request_token;

  // Set return values.
  remote_identity_handle = found->first;

  // Don't need to send the local token if we have a remote token.
  TokenReader remote_request(remote_auth_request_token);
  if (remote_request.is_nil()) {
    local_auth_request_token = found->second->local_auth_request;
  } else {
    local_auth_request_token = DDS::Security::Token();
  }

  if (is_handshake_initiator(local_data->participant_guid, remote_participant_guid)) {
    return DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST;
  } else {
    return DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  }
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::begin_handshake_request(
  ::DDS::Security::HandshakeHandle & handshake_handle,
  ::DDS::Security::HandshakeMessageToken & handshake_message,
  ::DDS::Security::IdentityHandle initiator_identity_handle,
  ::DDS::Security::IdentityHandle replier_identity_handle,
  const ::DDS::OctetSeq & serialized_local_participant_data,
  ::DDS::Security::SecurityException & ex)
{
  if (serialized_local_participant_data.length() == 0) {
    set_security_error(ex, -1, 0, "No participant data provided");
    return DDS::Security::VALIDATION_FAILED;
  }

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  HandshakeDataPair handshake_data = make_handshake_pair(initiator_identity_handle, replier_identity_handle);

  if (! handshake_data.first) {
    set_security_error(ex, -1, 0, "Unknown local participant");
    return DDS::Security::VALIDATION_FAILED;
  }

  if (! handshake_data.second) {
    set_security_error(ex, -1, 0, "Unknown remote participant");
    return DDS::Security::VALIDATION_FAILED;
  }

  const LocalParticipantData& local_data = *handshake_data.first;
  RemoteParticipantData& remote_data = *handshake_data.second;

  const LocalAuthCredentialData& local_credential_data = *local_data.credentials;

  SSL::DiffieHellman::unique_ptr diffie_hellman(new SSL::DiffieHellman(new SSL::ECDH_PRIME_256_V1_CEUM));

  OpenDDS::Security::TokenWriter message_out(handshake_message, build_class_id(Handshake_Request_Class_Ext));

  // Compute hash_c1 and store for later

  DDS::OctetSeq hash_c1;

  {
    CredentialHash hash(local_credential_data.get_participant_cert(),
                        *diffie_hellman,
                        serialized_local_participant_data,
                        local_credential_data.get_access_permissions());

    int err = hash(hash_c1);
    if (err) {
      set_security_error(ex, -1, 0, "Failed to generate credential-hash 'hash_c1'");
      return DDS::Security::VALIDATION_FAILED;
    }
  }

  message_out.add_bin_property("c.id", local_credential_data.get_participant_cert().original_bytes());
  message_out.add_bin_property("c.perm", local_credential_data.get_access_permissions());
  message_out.add_bin_property("c.pdata", serialized_local_participant_data);
  message_out.add_bin_property("c.dsign_algo", local_credential_data.get_participant_cert().dsign_algo());
  message_out.add_bin_property("c.kagree_algo", diffie_hellman->kagree_algo());
  if (local_data.handshake_debug) {
    message_out.add_bin_property("hash_c1", hash_c1);
  }

  DDS::OctetSeq dhpub;
  diffie_hellman->pub_key(dhpub);
  message_out.add_bin_property("dh1", dhpub);

  OpenDDS::Security::TokenReader auth_wrapper(remote_data.local_auth_request);
  if (auth_wrapper.is_nil()) {
    DDS::OctetSeq nonce;
    int err = SSL::make_nonce_256(nonce);
    if (! err) {
      message_out.add_bin_property("challenge1", nonce);

    } else {
      set_security_error(ex, -1, 0, "Failed to generate 256-bit nonce value for challenge1 property");
      return DDS::Security::VALIDATION_FAILED;
    }

  } else {
    const DDS::OctetSeq& challenge_data = auth_wrapper.get_bin_property_value("future_challenge");
    message_out.add_bin_property("challenge1", challenge_data);
  }

  remote_data.initiator_identity = initiator_identity_handle;
  remote_data.replier_identity = replier_identity_handle;
  remote_data.state = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  remote_data.request = handshake_message;
  remote_data.reply = DDS::Security::Token();
  remote_data.diffie_hellman = DCPS::move(diffie_hellman);
  remote_data.hash_c1 = hash_c1;

  if (handshake_handle == DDS::HANDLE_NIL) {
    handshake_handle = get_next_handle();
  }

  {
    ACE_Guard<ACE_Thread_Mutex> identity_data_guard(handshake_mutex_);
    handshake_data_[handshake_handle] = handshake_data;

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::begin_handshake_request handshake_data_ (total %B)\n"),
                 handshake_data_.size()));
    }
  }

  return DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
}

void extract_participant_guid_from_cpdata(const DDS::OctetSeq& cpdata, DCPS::GUID_t& dst)
{
  dst = DCPS::GUID_UNKNOWN;

  ACE_Message_Block buffer(reinterpret_cast<const char*>(cpdata.get_buffer()), cpdata.length());
  buffer.wr_ptr(cpdata.length());
  OpenDDS::DCPS::Serializer serializer(&buffer,
                                       DCPS::Encoding::KIND_XCDR1,
                                       DCPS::ENDIAN_BIG);
  RTPS::ParameterList params;

  if (serializer >> params) {
    for (unsigned int i = 0; i < params.length(); ++i) {
      const RTPS::Parameter& p = params[i];

      if (p._d() == RTPS::PID_PARTICIPANT_GUID) {
        dst = p.guid();
        break;
      }
    }

  } else {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: extract_participant_guid_from_cpdata, ")
               ACE_TEXT("failed to deserialize guid from cpdata.\n")));
  }

}

bool validate_topic_data_guid(const DDS::OctetSeq& cpdata,
                              const std::vector<unsigned char>& subject_name_hash,
                              DDS::Security::SecurityException& ex)
{
  if (cpdata.length() > 5u) { /* Enough to withstand the hash-comparison below */

    DCPS::GUID_t remote_participant_guid;
    extract_participant_guid_from_cpdata(cpdata, remote_participant_guid);

    const DCPS::GuidPrefix_t& prefix = remote_participant_guid.guidPrefix;

    /* Make sure first bit is set */

    if ((prefix[0] & 0x80) != 0x80) {
      set_security_error(ex, -1, 0, "Malformed participant_guid in 'c.pdata'; First bit must be set.");
      return false;
    }

    /* Check the following 47 bits match the subject-hash */

    /* First byte needs to remove the manually-set first-bit before comparison */
    if ((prefix[0] & 0x7F) != SSL::offset_1bit(&subject_name_hash[0], 0)) {
      set_security_error(ex, -1, 0, "First byte of participant_guid in 'c.pdata' does not match bits of subject-name hash in 'c.id'");
      return false;
    }
    for (size_t i = 1; i <= 5u; ++i) { /* Compare remaining 5 bytes */
      if (prefix[i] != SSL::offset_1bit(&subject_name_hash[0], i)) { /* Slide the hash to the right 1 so it aligns with the guid prefix */
        set_security_error(ex, -1, 0, "Bits 2 - 48 of 'c.pdata' participant_guid does not match first 47 bits of subject-name hash in 'c.id'");
        return false;
      }
    }

  } else {
    set_security_error(ex, -1, 0, "Data missing in 'c.pdata'");
    return false;
  }

  return true;
}

static void make_reply_signature_sequence(const DDS::OctetSeq& hash_c2,
                                          const DDS::OctetSeq& challenge2,
                                          const DDS::OctetSeq& dh2,
                                          const DDS::OctetSeq& challenge1,
                                          const DDS::OctetSeq& dh1,
                                          const DDS::OctetSeq& hash_c1,
                                          DDS::BinaryPropertySeq& dst)
{
  DCPS::SequenceBackInsertIterator<DDS::BinaryPropertySeq> inserter(dst);

  {
    DDS::BinaryProperty_t p;
    p.name = "hash_c2";
    p.value = hash_c2;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "challenge2";
    p.value = challenge2;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "dh2";
    p.value = dh2;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "challenge1";
    p.value = challenge1;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "dh1";
    p.value = dh1;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "hash_c1";
    p.value = hash_c1;
    p.propagate = true;
    *inserter = p;
  }
}

static void make_final_signature_sequence(const DDS::OctetSeq& hash_c1,
                                          const DDS::OctetSeq& challenge1,
                                          const DDS::OctetSeq& dh1,
                                          const DDS::OctetSeq& challenge2,
                                          const DDS::OctetSeq& dh2,
                                          const DDS::OctetSeq& hash_c2,
                                          DDS::BinaryPropertySeq& dst)
{
  DCPS::SequenceBackInsertIterator<DDS::BinaryPropertySeq> inserter(dst);

  {
    DDS::BinaryProperty_t p;
    p.name = "hash_c1";
    p.value = hash_c1;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "challenge1";
    p.value = challenge1;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "dh1";
    p.value = dh1;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "challenge2";
    p.value = challenge2;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "dh2";
    p.value = dh2;
    p.propagate = true;
    *inserter = p;
  }

  {
    DDS::BinaryProperty_t p;
    p.name = "hash_c2";
    p.value = hash_c2;
    p.propagate = true;
    *inserter = p;
  }
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

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);

  // Copy the "in" part of the inout param
  const DDS::Security::HandshakeMessageToken request_token = handshake_message_out;
  handshake_message_out = DDS::Security::HandshakeMessageToken();

  DDS::OctetSeq challenge1, challenge2, dh2, cperm, hash_c1, hash_c2;

  SSL::Certificate::unique_ptr remote_cert(new SSL::Certificate);
  SSL::DiffieHellman::unique_ptr diffie_hellman;

  const DDS::Security::ValidationResult_t Failure = DDS::Security::VALIDATION_FAILED,
                                          Pending = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;

  if (serialized_local_participant_data.length() == 0) {
    set_security_error(ex, -1, 0, "No participant data provided");
    return Failure;
  }

  HandshakeDataPair handshake_data = make_handshake_pair(initiator_identity_handle, replier_identity_handle);

  if (! handshake_data.first) {
    set_security_error(ex, -1, 0, "Unknown local participant");
    return Failure;
  }

  if (! handshake_data.second) {
    set_security_error(ex, -1, 0, "Unknown remote participant");
    return Failure;
  }

  const LocalParticipantData& local_data = *handshake_data.first;
  RemoteParticipantData& remote_data = *handshake_data.second;

  DDS::Security::HandshakeMessageToken message_data_in(request_token);
  TokenReader message_in(message_data_in);
  if (message_in.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake_message_out is an inout param and must not be nil");
    return Failure;

  }
  challenge1 = message_in.get_bin_property_value("challenge1");

  TokenReader initiator_remote_auth_request(remote_data.remote_auth_request);
  if (! initiator_remote_auth_request.is_nil()) {
    const DDS::OctetSeq& future_challenge = initiator_remote_auth_request.get_bin_property_value("future_challenge");

    if (! challenges_match(challenge1, future_challenge)) {
      return Failure;
    }
  }

  const LocalAuthCredentialData& local_credential_data = *(local_data.credentials);

  const DDS::OctetSeq& cid = message_in.get_bin_property_value("c.id");
  if (cid.length() > 0) {

    remote_cert->deserialize(cid);
    if (X509_V_OK != remote_cert->validate(local_credential_data.get_ca_cert()))
    {
      set_security_error(ex, -1, 0, "Certificate validation failed");
      return Failure;
    }

  } else {
    set_security_error(ex, -1, 0, "Certificate validation failed due to empty 'c.id' property supplied");
    return Failure;
  }

  /* Validate participant_guid in c.pdata */

  const DDS::OctetSeq& cpdata = message_in.get_bin_property_value("c.pdata");

  std::vector<unsigned char> hash;
  if (0 != remote_cert->subject_name_digest(hash)) {
    set_security_error(ex, -1, 0, "Failed to generate subject-name digest from remote certificate.");
    return Failure;
  }

  if (! validate_topic_data_guid(cpdata, hash, ex)) {
    return Failure;
  }

  cperm = message_in.get_bin_property_value("c.perm");

  const DDS::OctetSeq& dh_algo = message_in.get_bin_property_value("c.kagree_algo");
  diffie_hellman.reset(SSL::DiffieHellman::factory(dh_algo));

  /* Compute hash_c1 and store for later */

  {
    CredentialHash hash(*remote_cert,
                        *diffie_hellman,
                        cpdata,
                        cperm);
    int err = hash(hash_c1);
    if (err) {
      set_security_error(ex, -1, 0, "Failed to compute hash_c1.");
      return Failure;
    }
  }

  /* Compute hash_c2 and store for later */

  {
    CredentialHash hash(local_credential_data.get_participant_cert(),
                        *diffie_hellman,
                        serialized_local_participant_data,
                        local_credential_data.get_access_permissions());
    int err = hash(hash_c2);
    if (err) {
      set_security_error(ex, -1, 0, "Failed to compute hash_c2.");
      return Failure;
    }
  }

  // TODO: Currently support for OCSP is optional in the security spec and
  // so it has been deferred to a post-beta release.
  // Add OCSP checks when "ocsp_status" property is given in message_in.
  // Most of this logic would probably be placed in the Certificate directly
  // or an OCSP abstraction that the Certificate uses.

  const DDS::OctetSeq& dh1 = message_in.get_bin_property_value("dh1");

  TokenWriter message_out(handshake_message_out, build_class_id(Handshake_Reply_Class_Ext));

  message_out.add_bin_property("c.id", local_credential_data.get_participant_cert().original_bytes());
  message_out.add_bin_property("c.perm", local_credential_data.get_access_permissions());
  message_out.add_bin_property("c.pdata", serialized_local_participant_data);
  message_out.add_bin_property("c.dsign_algo", local_credential_data.get_participant_cert().dsign_algo());
  message_out.add_bin_property("c.kagree_algo", diffie_hellman->kagree_algo());

  if (local_data.handshake_debug) {
    message_out.add_bin_property("hash_c2", hash_c2);
  }

  diffie_hellman->pub_key(dh2);
  message_out.add_bin_property("dh2", dh2);

  if (local_data.handshake_debug) {
    message_out.add_bin_property("hash_c1", hash_c1);
    message_out.add_bin_property("dh1", dh1);
  }

  message_out.add_bin_property("challenge1", challenge1);

  TokenReader initiator_local_auth_request(remote_data.local_auth_request);
  if (! initiator_local_auth_request.is_nil()) {
    const DDS::OctetSeq& future_challenge = initiator_local_auth_request.get_bin_property_value("future_challenge");
    message_out.add_bin_property("challenge2", future_challenge);
    challenge2 = future_challenge;

  } else {
    int err = SSL::make_nonce_256(challenge2);
    if (! err) {
      message_out.add_bin_property("challenge2", challenge2);

    } else {
      set_security_error(ex, -1, 0, "SSL::make_nonce_256 failed.");
      return Failure;
    }
  }

  DDS::BinaryPropertySeq sign_these;
  make_reply_signature_sequence(hash_c2,
                                challenge2,
                                dh2,
                                challenge1,
                                dh1,
                                hash_c1,
                                sign_these);

  DDS::OctetSeq signature;
  SSL::sign_serialized(sign_these, local_credential_data.get_participant_private_key(), signature);
  message_out.add_bin_property("signature", signature);

  remote_data.replier_identity = replier_identity_handle;
  remote_data.initiator_identity = initiator_identity_handle;
  remote_data.state = DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE;
  remote_data.diffie_hellman = DCPS::move(diffie_hellman);
  remote_data.certificate = DCPS::move(remote_cert);
  remote_data.c_perm = cperm;
  remote_data.reply = handshake_message_out;
  remote_data.request = request_token;
  remote_data.hash_c1 = hash_c1;
  remote_data.hash_c2 = hash_c2;

  if (handshake_handle == DDS::HANDLE_NIL) {
    handshake_handle = get_next_handle();
  }

  {
    ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);
    handshake_data_[handshake_handle] = handshake_data;

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::begin_handshake_reply handshake_data_ (total %B)\n"),
                 handshake_data_.size()));
    }
  }

  return Pending;
}

::DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::process_handshake(
  ::DDS::Security::HandshakeMessageToken & handshake_message_out,
  const ::DDS::Security::HandshakeMessageToken & handshake_message_in,
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  const std::string incoming_class_ext = get_extension(handshake_message_in.class_id);

  if (Handshake_Reply_Class_Ext == incoming_class_ext) {
    return process_handshake_reply(handshake_message_out, handshake_message_in, handshake_handle, ex);

  } else if (Handshake_Final_Class_Ext == incoming_class_ext) {
    return process_final_handshake(handshake_message_in, handshake_handle, ex);
  }

  return DDS::Security::VALIDATION_PENDING_RETRY;
}

::DDS::Security::SharedSecretHandle* AuthenticationBuiltInImpl::get_shared_secret(
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  using namespace DDS::Security;

  SharedSecretHandle* result = 0;

  ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

  HandshakeDataPair handshake_data = get_handshake_data(handshake_handle);
  if (handshake_data.first && handshake_data.second) {

    ValidationResult_t state = handshake_data.second->state;
    if (state == VALIDATION_OK || state == VALIDATION_OK_FINAL_MESSAGE) {
      SharedSecretHandle_var handle = handshake_data.second->shared_secret;
      result = handle._retn();

    } else {
      set_security_error(ex, -1, 0, "Validation state must be either VALIDATION_OK or VALIDATION_OK_FINAL_MESSAGE");
    }

  } else {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
  }

  return result;
}

::CORBA::Boolean AuthenticationBuiltInImpl::get_authenticated_peer_credential_token(
  ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  using namespace DDS::Security;
  ::CORBA::Boolean result = false;

  ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

  HandshakeDataPair handshake_data = get_handshake_data(handshake_handle);
  if (handshake_data.first && handshake_data.second) {
    ValidationResult_t state = handshake_data.second->state;
    if (state == VALIDATION_OK || state == VALIDATION_OK_FINAL_MESSAGE) {
      OpenDDS::Security::TokenWriter peer_token(peer_credential_token, Auth_Peer_Cred_Token_Class_Id);

      DDS::BinaryPropertySeq& props = peer_credential_token.binary_properties;
      props.length(2);

      DDS::BinaryProperty_t p1;
      p1.name = "c.id";
      p1.value = handshake_data.second->certificate->original_bytes();
      p1.propagate = true;

      DDS::BinaryProperty_t p2;
      p2.name = "c.perm";
      p2.value = handshake_data.second->c_perm;
      p2.propagate = true;

      props[0] = p1;
      props[1] = p2;

      result = true;

    } else {
      set_security_error(ex, -1, 0, "Validation state must be either VALIDATION_OK or VALIDATION_OK_FINAL_MESSAGE");
    }

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

  if (!listener) {
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
  // Nothing to do here yet
  ACE_UNUSED_ARG(token);
  ACE_UNUSED_ARG(ex);
  return true;
}


::CORBA::Boolean AuthenticationBuiltInImpl::return_identity_status_token(
  const ::DDS::Security::IdentityStatusToken & token,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here yet
  ACE_UNUSED_ARG(token);
  ACE_UNUSED_ARG(ex);
  return true;
}


::CORBA::Boolean AuthenticationBuiltInImpl::return_authenticated_peer_credential_token(
  const ::DDS::Security::AuthenticatedPeerCredentialToken & peer_credential_token,
  ::DDS::Security::SecurityException & ex)
{
  // Nothing to do here yet
  ACE_UNUSED_ARG(peer_credential_token);
  ACE_UNUSED_ARG(ex);
  return true;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_handshake_handle(
  ::DDS::Security::HandshakeHandle handshake_handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_Guard<ACE_Thread_Mutex> guard(handshake_mutex_);

  HandshakeDataMap::iterator found = handshake_data_.find(handshake_handle);
  if (found != handshake_data_.end()) {
    handshake_data_.erase(found);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::return_handshake_handle handshake_data_ (total %B)\n"),
                 handshake_data_.size()));
    }

    return true;
  }

  set_security_error(ex, -1, 0, "Handshake handle not recognized");
  return false;
}

::CORBA::Boolean AuthenticationBuiltInImpl::return_identity_handle(
  ::DDS::Security::IdentityHandle identity_handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_Guard<ACE_Thread_Mutex> guard(identity_mutex_);

  LocalParticipantMap::iterator local = local_participants_.find(identity_handle);

  if (local != local_participants_.end()) {

    {
      ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

      for (HandshakeDataMap::iterator it = handshake_data_.begin(); it != handshake_data_.end(); /* increment in loop*/) {
        if (it->second.first == local->second) {
          handshake_data_.erase(it++);
        } else {
          ++it;
        }
      }
    }

    local_participants_.erase(local);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::return_identity_handle local_participants_ (total %B)\n"),
                 local_participants_.size()));
    }

    return true;
  }

  local = std::find_if(local_participants_.begin(), local_participants_.end(),
                       local_has_remote_handle(identity_handle));

  if (local != local_participants_.end()) {

    const RemoteParticipantMap::iterator remote = local->second->validated_remotes.find(identity_handle);

    {
      ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

      for (HandshakeDataMap::iterator it = handshake_data_.begin(); it != handshake_data_.end(); /* increment in loop*/) {
        if (it->second.second == remote->second) {
          handshake_data_.erase(it++);
        } else {
          ++it;
        }
      }
    }

    local->second->validated_remotes.erase(remote);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("AuthenticationBuiltInImpl::return_identity_handle validated_remotes (total %B)\n"),
                 local->second->validated_remotes.size()));
    }

    return true;
  }

  set_security_error(ex, -1, 0, "Identity handle not recognized");
  return false;
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
  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);
  ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

  DDS::OctetSeq challenge1, hash_c2;
  SSL::Certificate::unique_ptr remote_cert(new SSL::Certificate);

  const DDS::Security::ValidationResult_t Failure = DDS::Security::VALIDATION_FAILED;
  const DDS::Security::ValidationResult_t FinalMessage = DDS::Security::VALIDATION_OK_FINAL_MESSAGE;

  HandshakeDataPair handshake_data = get_handshake_data(handshake_handle);
  if (!handshake_data.first || !handshake_data.second) {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
    return Failure;
  }

  LocalParticipantData& local_data = *(handshake_data.first);
  RemoteParticipantData& remote_data = *(handshake_data.second);

  if (remote_data.state != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
    set_security_error(ex, -1, 0, "Handshake state is not valid");
    return Failure;
  }

  TokenReader message_in(handshake_message_in);
  if (message_in.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake_message_in must not be nil");
    return Failure;
  }

  const DDS::OctetSeq& challenge2 = message_in.get_bin_property_value("challenge2");

  TokenReader auth_wrapper(remote_data.remote_auth_request);
  if (! auth_wrapper.is_nil()) {
    const DDS::OctetSeq& future_challenge = auth_wrapper.get_bin_property_value("future_challenge");

    if (! challenges_match(challenge2, future_challenge)) {
      set_security_error(ex, -1, 0, "challenge2 does not match future_challenge");
      return Failure;
    }
  }

  const LocalAuthCredentialData& local_credential_data = *(local_data.credentials);

  const DDS::OctetSeq& cid = message_in.get_bin_property_value("c.id");
  if (cid.length() > 0) {

      remote_cert->deserialize(cid);

    if (X509_V_OK != remote_cert->validate(local_credential_data.get_ca_cert()))
    {
      set_security_error(ex, -1, 0, "Certificate validation failed");
      return Failure;
    }

  } else {
    set_security_error(ex, -1, 0, "Certificate validation failed due to empty 'c.id' property supplied");
    return Failure;
  }

  /* Check that challenge1 on message_in matches the one sent in HandshakeRequestMessageToken */

  TokenReader handshake_request_token(remote_data.request);
  if (handshake_request_token.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake-request-token is nil");
    return Failure;

  } else {
    challenge1 = handshake_request_token.get_bin_property_value("challenge1");
    const DDS::OctetSeq& challenge1_reply =  message_in.get_bin_property_value("challenge1");

    if (! challenges_match(challenge1, challenge1_reply)) {
      set_security_error(ex, -1, 0, "handshake-request challenge1 value does not match local challenge1");
      return Failure;
    }
  }

  /* Validate participant_guid in c.pdata */

  const DDS::OctetSeq& cpdata = message_in.get_bin_property_value("c.pdata");

  std::vector<unsigned char> hash;
  if (0 != remote_cert->subject_name_digest(hash)) {
    set_security_error(ex, -1, 0, "Failed to generate subject-name digest from remote certificate.");
    return Failure;
  }

  if (! validate_topic_data_guid(cpdata, hash, ex)) {
    return Failure;
  }

  /* Compute/Store the Diffie-Hellman shared-secret */

  const DDS::OctetSeq& dh2 = message_in.get_bin_property_value("dh2");
  if (0 != remote_data.diffie_hellman->gen_shared_secret(dh2)) {
    set_security_error(ex, -1, 0, "Failed to generate shared secret from dh1 and dh2");
    return Failure;
  }

  const DDS::OctetSeq& cperm = message_in.get_bin_property_value("c.perm");

  /* Compute hash_c2 and store for later (hash_c1 was already computed in request) */

  {
    CredentialHash hash(*remote_cert,
                        *remote_data.diffie_hellman,
                        cpdata,
                        cperm);
    int err = hash(hash_c2);
    if (err) {
      set_security_error(ex, -1, 0, "Computing hash_c2 failed");
      return Failure;
    }
  }

  /* Validate Signature field */
  const DDS::OctetSeq& dh1 = handshake_request_token.get_bin_property_value("dh1");

  DDS::BinaryPropertySeq verify_these;
  make_reply_signature_sequence(hash_c2,
                                challenge2,
                                dh2,
                                challenge1,
                                dh1,
                                remote_data.hash_c1,
                                verify_these);

  const DDS::OctetSeq& remote_signature = message_in.get_bin_property_value("signature");

  int err = SSL::verify_serialized(verify_these, *remote_cert, remote_signature);
  if (err) {
    set_security_error(ex, -1, 0, "Remote 'signature' field failed signature verification");
    return Failure;
  }

  OpenDDS::Security::TokenWriter final_msg(handshake_message_out, build_class_id(Handshake_Final_Class_Ext));

  if (local_data.handshake_debug) {
    final_msg.add_bin_property("hash_c1", remote_data.hash_c1);
    final_msg.add_bin_property("hash_c2", hash_c2);
    final_msg.add_bin_property("dh1", dh1);
    final_msg.add_bin_property("dh2", dh2);
  }

  final_msg.add_bin_property("challenge1", challenge1);
  final_msg.add_bin_property("challenge2", challenge2);

  DDS::BinaryPropertySeq sign_these;
  make_final_signature_sequence(remote_data.hash_c1,
                                challenge1,
                                dh1,
                                challenge2,
                                dh2,
                                hash_c2,
                                sign_these);

  DDS::OctetSeq tmp;
  SSL::sign_serialized(sign_these, local_credential_data.get_participant_private_key(), tmp);
  final_msg.add_bin_property("signature", tmp);

  remote_data.certificate = DCPS::move(remote_cert);
  remote_data.state = FinalMessage;
  remote_data.c_perm = message_in.get_bin_property_value("c.perm");
  remote_data.hash_c2 = hash_c2;
  remote_data.shared_secret = new SharedSecret(challenge1,
                                               challenge2,
                                               remote_data.diffie_hellman->get_shared_secret());
  return FinalMessage;
}

bool challenges_match(const DDS::OctetSeq& c1, const DDS::OctetSeq& c2)
{
  if ((c1.length()) < 1 || (c2.length() < 1)) {
    return false;
  }
  if (c1.length() != c2.length()) {
    return false;
  }

  if (0 != std::memcmp(c1.get_buffer(), c2.get_buffer(), c2.length())) {
    return false;
  }

  return true;
}

DDS::Security::ValidationResult_t AuthenticationBuiltInImpl::process_final_handshake(
  const DDS::Security::HandshakeMessageToken & handshake_message_in, /* Final message token */
  DDS::Security::HandshakeHandle handshake_handle,
  DDS::Security::SecurityException & ex)
{
  const DDS::Security::ValidationResult_t Failure = DDS::Security::VALIDATION_FAILED;
  const DDS::Security::ValidationResult_t ValidationOkay = DDS::Security::VALIDATION_OK;

  ACE_Guard<ACE_Thread_Mutex> identity_data_guard(identity_mutex_);
  ACE_Guard<ACE_Thread_Mutex> handshake_data_guard(handshake_mutex_);

  HandshakeDataPair handshake_data = get_handshake_data(handshake_handle);
  if (!handshake_data.first || !handshake_data.second) {
    set_security_error(ex, -1, 0, "Unknown handshake handle");
    return Failure;
  }

  RemoteParticipantData& remote_data = *(handshake_data.second);

  if (remote_data.state != DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE) {
    set_security_error(ex, -1, 0, "Handshake state is not valid");
    return Failure;
  }

  /* Check challenge1 and challenge2 match what was sent with the reply-message-token */

  TokenReader handshake_final_token(handshake_message_in);
  if (handshake_final_token.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake-final-token is nil");
    return Failure;
  }

  TokenReader handshake_reply_token(remote_data.reply);
  if (handshake_reply_token.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake-reply-token is nil");
    return Failure;
  }

  /* Per the spec, dh1 is optional in all _but_ the request token so it's grabbed from the request */
  TokenReader handshake_request_token(remote_data.request);
  if (handshake_reply_token.is_nil()) {
    set_security_error(ex, -1, 0, "Handshake-reply-token is nil");
    return Failure;
  }

  const DDS::OctetSeq& dh1 = handshake_request_token.get_bin_property_value("dh1");
  const DDS::OctetSeq& dh2 = handshake_reply_token.get_bin_property_value("dh2");

  const DDS::OctetSeq& challenge1_reply = handshake_reply_token.get_bin_property_value("challenge1");
  const DDS::OctetSeq& challenge2_reply = handshake_reply_token.get_bin_property_value("challenge2");

  const DDS::OctetSeq& challenge1_final = handshake_final_token.get_bin_property_value("challenge1");
  const DDS::OctetSeq& challenge2_final = handshake_final_token.get_bin_property_value("challenge2");

  if (! challenges_match(challenge1_reply, challenge1_final) || ! challenges_match(challenge2_reply, challenge2_final)) {
      return Failure;
  }

  /* Validate Signature field */

  const SSL::Certificate::unique_ptr& remote_cert = remote_data.certificate;

  DDS::BinaryPropertySeq verify_these;
  make_final_signature_sequence(remote_data.hash_c1,
                                challenge1_reply,
                                dh1,
                                challenge2_reply,
                                dh2,
                                remote_data.hash_c2,
                                verify_these);

  const DDS::OctetSeq& remote_signature = handshake_final_token.get_bin_property_value("signature");

  int err = SSL::verify_serialized(verify_these, *remote_cert, remote_signature);
  if (err) {
    set_security_error(ex, -1, 0, "Remote 'signature' field failed signature verification");
    return Failure;
  }

  /* Compute/Store the Diffie-Hellman shared-secret */

  if (0 != remote_data.diffie_hellman->gen_shared_secret(dh1)) {
    set_security_error(ex, -1, 0, "Failed to generate shared secret from dh2 and dh1");
    return Failure;
  }

  remote_data.state = DDS::Security::VALIDATION_OK;
  remote_data.shared_secret = new SharedSecret(challenge1_reply,
                                                 challenge2_reply,
                                                 remote_data.diffie_hellman->get_shared_secret());

  return ValidationOkay;
}

AuthenticationBuiltInImpl::LocalParticipantData::shared_ptr
AuthenticationBuiltInImpl::get_local_participant(DDS::Security::IdentityHandle handle)
{
  LocalParticipantMap::iterator found = local_participants_.find(handle);
  if (found != local_participants_.end()) {
    return found->second;
  }

  return LocalParticipantData::shared_ptr();
}

AuthenticationBuiltInImpl::LocalParticipantData::~LocalParticipantData()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("LocalParticipantData::~LocalParticipantData validated_remotes %B\n"),
               validated_remotes.size()));
  }
}

AuthenticationBuiltInImpl::HandshakeDataPair
AuthenticationBuiltInImpl::get_handshake_data(DDS::Security::HandshakeHandle handle)
{
  HandshakeDataMap::iterator found = handshake_data_.find(handle);
  if (found != handshake_data_.end()) {
    return found->second;
  }

  return HandshakeDataPair();
}

AuthenticationBuiltInImpl::HandshakeDataPair
AuthenticationBuiltInImpl::make_handshake_pair(DDS::Security::IdentityHandle h1,
                                               DDS::Security::IdentityHandle h2)
{
  DDS::Security::IdentityHandle other = DDS::HANDLE_NIL;

  LocalParticipantMap::iterator found_local = local_participants_.find(h1);
  if (found_local != local_participants_.end()) {
    other = h2;

  } else {
    found_local = local_participants_.find(h2);
    if (found_local != local_participants_.end()) {
      other = h1;

    } else {
      return HandshakeDataPair();
    }
  }

  RemoteParticipantMap::iterator found_remote = found_local->second->validated_remotes.find(other);
  if (found_remote != found_local->second->validated_remotes.end()) {
    return std::make_pair(found_local->second, found_remote->second);
  }

  return HandshakeDataPair();
}

bool AuthenticationBuiltInImpl::is_handshake_initiator(
  const OpenDDS::DCPS::GUID_t& local, const OpenDDS::DCPS::GUID_t& remote)
{
  const unsigned char* local_ = reinterpret_cast<const unsigned char*>(&local);
  const unsigned char* remote_ = reinterpret_cast<const unsigned char*>(&remote);

  using DCPS::SecurityDebug;
  if (DCPS::security_debug.force_auth_role != SecurityDebug::FORCE_AUTH_ROLE_NORMAL) {
    return DCPS::security_debug.force_auth_role == SecurityDebug::FORCE_AUTH_ROLE_LEADER;
  }

  /* if remote > local, pending request; else pending handshake message */
  return std::lexicographical_compare(local_, local_ + sizeof(local),
                                      remote_, remote_ + sizeof(remote));

}

bool AuthenticationBuiltInImpl::check_class_versions(const char* remote_class_id)
{
  if (!remote_class_id) {
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

std::string AuthenticationBuiltInImpl::build_class_id(const std::string& message_ext)
{
  std::string class_id_stream = Auth_Plugin_Name +
    + ":" + Auth_Plugin_Major_Version
    + "." + Auth_Plugin_Minor_Version
    + "+" + message_ext;

  return class_id_stream;
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

CORBA::Long AuthenticationBuiltInImpl::get_next_handle()
{
  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);
  return CommonUtilities::increment_handle(next_handle_);
}

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
