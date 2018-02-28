#include "CryptoBuiltInImpl.h"
#include "CommonUtilities.h"
#include "TokenWriter.h"

#include "dds/DdsDcpsInfrastructureC.h"

namespace OpenDDS {
namespace Security {

CryptoBuiltInImpl::CryptoBuiltInImpl()
  : handle_mutex_()
  , next_handle_(1)
{}

CryptoBuiltInImpl::~CryptoBuiltInImpl()
{}

bool CryptoBuiltInImpl::_is_a(const char* id)
{
  return CryptoKeyFactory::_is_a(id)
    || CryptoKeyExchange::_is_a(id)
    || CryptoTransform::_is_a(id);
}

const char* CryptoBuiltInImpl::_interface_repository_id() const
{
  return "";
}

bool CryptoBuiltInImpl::marshal(TAO_OutputCDR&)
{
  return false;
}

int CryptoBuiltInImpl::generate_handle()
{
  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);
  int new_handle = next_handle_++;

  if (new_handle == DDS::HANDLE_NIL) {
    new_handle = next_handle_++;
  }
  return new_handle;
}


// Key Factory

DDS::Security::ParticipantCryptoHandle CryptoBuiltInImpl::register_local_participant(
  DDS::Security::IdentityHandle participant_identity,
  DDS::Security::PermissionsHandle participant_permissions,
  const DDS::PropertySeq& participant_properties,
  const DDS::Security::ParticipantSecurityAttributes& participant_security_attributes,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(participant_properties);
  ACE_UNUSED_ARG(participant_security_attributes);

  // Conditions required for this to succeed
  // - Local Domain Participant identified by participant_identity must have been
  //   authenticated and granted access to DDS Domain
  // - Handles are not DDS::HANDLE_NIL
  // - No requirements for participant_properties.  It is used for optional configuration
  // - participant_security_attributes is used to generate the key material
  if (DDS::HANDLE_NIL == participant_identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local participant ID");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == participant_permissions) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local permissions");
    return DDS::HANDLE_NIL;
  }

  // The stub will just generate a new handle value each time this is called
  return generate_handle();
}

DDS::Security::ParticipantCryptoHandle CryptoBuiltInImpl::register_matched_remote_participant(
  DDS::Security::ParticipantCryptoHandle local_participant_crypto_handle,
  DDS::Security::IdentityHandle remote_participant_identity,
  DDS::Security::PermissionsHandle remote_participant_permissions,
  DDS::Security::SharedSecretHandle* shared_secret,
  DDS::Security::SecurityException& ex)
{
  // Conditions required for this to succeed
  // - local_participant_crypto_handle must have been returned by a prior
  //   call to register_local_participant
  // - remote_participant_identity must have been authenticated and granted
  //   access to DDS Domain
  // - Handles are not DDS::HANDLE_NIL
  if (DDS::HANDLE_NIL == local_participant_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local participant crypto handle");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_participant_identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote participant ID");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_participant_permissions) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote permissions ID");
    return DDS::HANDLE_NIL;
  }
  if (!shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret data");
    return DDS::HANDLE_NIL;
  }

  // The stub won't keep track of what has been registered, it
  // just returns a new handle
  return generate_handle();
}

DDS::Security::DatawriterCryptoHandle CryptoBuiltInImpl::register_local_datawriter(
  DDS::Security::ParticipantCryptoHandle participant_crypto,
  const DDS::PropertySeq& datawriter_properties,
  const DDS::Security::EndpointSecurityAttributes& datawriter_security_attributes,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(datawriter_properties);
  ACE_UNUSED_ARG(datawriter_security_attributes);

  // Conditions for success
  // - participant_crypto must be a registered local participant crypto handle

  // The stub always generates a new handle as long as the input handle isn't NIL
  if (DDS::HANDLE_NIL == participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }

  return generate_handle();
}

DDS::Security::DatareaderCryptoHandle CryptoBuiltInImpl::register_matched_remote_datareader(
  DDS::Security::DatawriterCryptoHandle local_datawriter_crypto_handle,
  DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  DDS::Security::SharedSecretHandle* shared_secret,
  bool relay_only,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(relay_only);

  // Conditions for success
  // - local_datawriter_crypto_handle must be a registered local participant crypto handle
  // - remote_participant_crypto must be a registered remote participant crypto handle
  if (DDS::HANDLE_NIL == local_datawriter_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Remote Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  if (!shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret Handle");
    return DDS::HANDLE_NIL;
  }

  // The stub always generates a new handle as long as the input handle isn't NIL
  return generate_handle();
}

DDS::Security::DatareaderCryptoHandle CryptoBuiltInImpl::register_local_datareader(
  DDS::Security::ParticipantCryptoHandle participant_crypto,
  const DDS::PropertySeq& datareader_properties,
  const DDS::Security::EndpointSecurityAttributes& datareader_security_attributes,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(datareader_properties);
  ACE_UNUSED_ARG(datareader_security_attributes);

  // Conditions for success
  // - participant_crypto must be a registered local participant crypto handle
  if (DDS::HANDLE_NIL == participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  // The stub always generates a new handle as long as the input handle isn't NIL
  return generate_handle();
}

DDS::Security::DatawriterCryptoHandle CryptoBuiltInImpl::register_matched_remote_datawriter(
  DDS::Security::DatareaderCryptoHandle local_datareader_crypto_handle,
  DDS::Security::ParticipantCryptoHandle remote_participant_crypt,
  DDS::Security::SharedSecretHandle* shared_secret,
  DDS::Security::SecurityException& ex)
{
  // Conditions for success
  // - local_datawriter_crypto_handle must be a registered local participant crypto handle
  // - remote_participant_crypto must be a registered remote participant crypto handle
  if (DDS::HANDLE_NIL == local_datareader_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local DataWriter Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_participant_crypt) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Remote Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  if (!shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret Handle");
    return DDS::HANDLE_NIL;
  }

  // The stub always generates a new handle as long as the input handle isn't NIL
  return generate_handle();
}

bool CryptoBuiltInImpl::unregister_participant(
  DDS::Security::ParticipantCryptoHandle participant_crypto_handle,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(participant_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

bool CryptoBuiltInImpl::unregister_datawriter(
  DDS::Security::DatawriterCryptoHandle datawriter_crypto_handle,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(datawriter_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

bool CryptoBuiltInImpl::unregister_datareader(
  DDS::Security::DatareaderCryptoHandle datareader_crypto_handle,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(datareader_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}


// Key Exchange

// Constants used during crypto operations
namespace
{
  const std::string Crypto_Token_Class_Id("DDS:Crypto:AES_GCM_GMAC");
  const DDS::OctetSeq Empty_Seq;
}

bool CryptoBuiltInImpl::create_local_participant_crypto_tokens(
  DDS::Security::ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
  DDS::Security::ParticipantCryptoHandle local_participant_crypto,
  DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  DDS::Security::SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local participant handle");
    return false;
  }
  if (DDS::HANDLE_NIL == remote_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote participant handle");
    return false;
  }

  // The input is a sequence of tokens, but the spec appears to indicate
  // that the plugin will only be returning a single token in the sequence.
  // Any existing contents of the sequence will be destroyed
  local_participant_crypto_tokens.length(1);
  if (1 != local_participant_crypto_tokens.length()) {
    CommonUtilities::set_security_error(ex, -1, 0, "Unable to allocate space for token");
    return false;
  }

  // Stub implementation will just fill in data in the new token
  TokenWriter crypto_token(local_participant_crypto_tokens[0], Crypto_Token_Class_Id, 0, 1);
  crypto_token.set_bin_property(0, "dds.cryp.keymat", Empty_Seq, true);
  return true;
}

bool CryptoBuiltInImpl::set_remote_participant_crypto_tokens(
  DDS::Security::ParticipantCryptoHandle local_participant_crypto,
  DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  const DDS::Security::ParticipantCryptoTokenSeq& remote_participant_tokens,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(local_participant_crypto);
  ACE_UNUSED_ARG(remote_participant_crypto);
  ACE_UNUSED_ARG(remote_participant_tokens);
  ACE_UNUSED_ARG(ex);

  // This stub method just pretends to work
  bool results = true;
  return results;
}

bool CryptoBuiltInImpl::create_local_datawriter_crypto_tokens(
  DDS::Security::DatawriterCryptoTokenSeq& local_datawriter_crypto_tokens,
  DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
  DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
  DDS::Security::SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local writer handle");
    return false;
  }
  if (DDS::HANDLE_NIL == remote_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote reader handle");
    return false;
  }

  // The input is a sequence of tokens, but the spec appears to indicate
  // that the plugin will only be returning a single token in the sequence.
  // Any existing contents of the sequence will be destroyed
  local_datawriter_crypto_tokens.length(1);
  if (1 != local_datawriter_crypto_tokens.length()) {
    CommonUtilities::set_security_error(ex, -1, 0, "Unable to allocate space for token");
    return false;
  }

  // Stub implementation will just fill in data in the new token
  TokenWriter crypto_token(local_datawriter_crypto_tokens[0], Crypto_Token_Class_Id, 0, 1);
  crypto_token.set_bin_property(0, "dds.cryp.keymat", Empty_Seq, true);
  return true;
}

bool CryptoBuiltInImpl::set_remote_datawriter_crypto_tokens(
  DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
  DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
  const DDS::Security::DatawriterCryptoTokenSeq& remote_datawriter_tokens,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(local_datareader_crypto);
  ACE_UNUSED_ARG(remote_datawriter_crypto);
  ACE_UNUSED_ARG(remote_datawriter_tokens);
  ACE_UNUSED_ARG(ex);

  // The stub implementation will always succeed here
  bool results = true;
  return results;
}

bool CryptoBuiltInImpl::create_local_datareader_crypto_tokens(
  DDS::Security::DatareaderCryptoTokenSeq& local_datareader_cryto_tokens,
  DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
  DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
  DDS::Security::SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local writer handle");
    return false;
  }
  if (DDS::HANDLE_NIL == remote_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote reader handle");
    return false;
  }

  // The input is a sequence of tokens, but the spec appears to indicate
  // that the plugin will only be returning a single token in the sequence.
  // Any existing contents of the sequence will be destroyed
  local_datareader_cryto_tokens.length(1);
  if (1 != local_datareader_cryto_tokens.length()) {
    CommonUtilities::set_security_error(ex, -1, 0, "Unable to allocate space for token");
    return false;
  }

  // Stub implementation will just fill in data in the new token
  TokenWriter crypto_token(local_datareader_cryto_tokens[0], Crypto_Token_Class_Id, 0, 1);
  crypto_token.set_bin_property(0, "dds.cryp.keymat", Empty_Seq, true);
  return true;
}

bool CryptoBuiltInImpl::set_remote_datareader_crypto_tokens(
  DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
  DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
  const DDS::Security::DatareaderCryptoTokenSeq& remote_datareader_tokens,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(local_datawriter_crypto);
  ACE_UNUSED_ARG(remote_datareader_crypto);
  ACE_UNUSED_ARG(remote_datareader_tokens);
  ACE_UNUSED_ARG(ex);

  // The stub implementation will always succeed here
  bool results = true;
  return results;
}

bool CryptoBuiltInImpl::return_crypto_tokens(
  const DDS::Security::CryptoTokenSeq& crypto_tokens,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(crypto_tokens);
  ACE_UNUSED_ARG(ex);

  // Can't modify the input sequence at all because it's a const reference

  // The stub implementation will always succeed here
  bool results = true;
  return results;
}


// Transform

bool CryptoBuiltInImpl::encode_serialized_payload(
  DDS::OctetSeq& encoded_buffer,
  DDS::OctetSeq& extra_inline_qos,
  const DDS::OctetSeq& plain_buffer,
  DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(extra_inline_qos);

  if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid datawriter handle");
    return false;
  }

  // Simple implementation wraps the plain_buffer back into the output
  // and adds no extra_inline_qos
  DDS::OctetSeq transformed_buffer(plain_buffer);
  encoded_buffer.swap(transformed_buffer);
  return true;
}

bool CryptoBuiltInImpl::encode_datawriter_submessage(
  DDS::OctetSeq& encoded_rtps_submessage,
  const DDS::OctetSeq& plain_rtps_submessage,
  DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  const DDS::Security::DatareaderCryptoHandleSeq& receiving_datareader_crypto_list,
  CORBA::Long& receiving_datareader_crypto_list_index,
  DDS::Security::SecurityException& ex)
{
  // Verify the input handles are valid before doing the transformation
  if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid datawriter handle");
    return false;
  }

  DDS::Security::DatareaderCryptoHandle reader_handle = DDS::HANDLE_NIL;
  if (receiving_datareader_crypto_list_index >= 0) {
    // Need to make this unsigned to get prevent warnings when comparing to length()
    CORBA::ULong index = static_cast<CORBA::ULong>(receiving_datareader_crypto_list_index);
    if (index < receiving_datareader_crypto_list.length()) {
      reader_handle = receiving_datareader_crypto_list[receiving_datareader_crypto_list_index];
    }
  }

  if (DDS::HANDLE_NIL == reader_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid datareader handle");
    return false;
  }

  // Simple implementation wraps the plain_buffer back into the output
  // and adds no extra_inline_qos
  DDS::OctetSeq transformed_buffer(plain_rtps_submessage);
  encoded_rtps_submessage.swap(transformed_buffer);

  // Advance the counter to indicate this reader has been handled
  ++receiving_datareader_crypto_list_index;

  return true;
}

bool CryptoBuiltInImpl::encode_datareader_submessage(
  DDS::OctetSeq& encoded_rtps_submessage,
  const DDS::OctetSeq& plain_rtps_submessage,
  DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
  const DDS::Security::DatawriterCryptoHandleSeq& receiving_datawriter_crypto_list,
  DDS::Security::SecurityException& ex)
{
  // Perform sanity checking on input data
  if (DDS::HANDLE_NIL == sending_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid DataReader handle");
    return false;
  }
  if (0 == receiving_datawriter_crypto_list.length()) {
    CommonUtilities::set_security_error(ex, -1, 0, "No Datawriters specified");
    return false;
  }

  // For the stub, just copy the plain message into the encoded message
  DDS::OctetSeq transformed_buffer(plain_rtps_submessage);
  encoded_rtps_submessage.swap(transformed_buffer);

  return true;
}

bool CryptoBuiltInImpl::encode_rtps_message(
  DDS::OctetSeq& encoded_rtps_message,
  const DDS::OctetSeq& plain_rtps_message,
  DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  const DDS::Security::ParticipantCryptoHandleSeq& receiving_participant_crypto_list,
  CORBA::Long& receiving_participant_crypto_list_index,
  DDS::Security::SecurityException& ex)
{
  // Perform sanity checking on input data
  if (DDS::HANDLE_NIL == sending_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid DataReader handle");
    return false;
  }
  if (0 == receiving_participant_crypto_list.length()) {
    CommonUtilities::set_security_error(ex, -1, 0, "No Datawriters specified");
    return false;
  }

  DDS::Security::ParticipantCryptoHandle dest_handle = DDS::HANDLE_NIL;
  if (receiving_participant_crypto_list_index >= 0) {
    // Need to make this unsigned to get prevent warnings when comparing to length()
    CORBA::ULong index = static_cast<CORBA::ULong>(receiving_participant_crypto_list_index);
    if (index < receiving_participant_crypto_list.length()) {
      dest_handle = receiving_participant_crypto_list[receiving_participant_crypto_list_index];
    }
  }

  if (DDS::HANDLE_NIL == dest_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid receiver handle");
    return false;
  }

  // Simple implementation wraps the plain_buffer back into the output
  // and adds no extra_inline_qos
  DDS::OctetSeq transformed_buffer(plain_rtps_message);
  encoded_rtps_message.swap(transformed_buffer);

  // Advance the counter to indicate this reader has been handled
  ++receiving_participant_crypto_list_index;

  return true;
}

bool CryptoBuiltInImpl::decode_rtps_message(
  DDS::OctetSeq& plain_buffer,
  const DDS::OctetSeq& encoded_buffer,
  DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
  DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  DDS::Security::SecurityException& ex)
{
  // Perform sanity checking on input data
  if (DDS::HANDLE_NIL == receiving_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Receiving Participant handle");
    return false;
  }
  if (DDS::HANDLE_NIL == sending_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "No Sending Participant handle");
    return false;
  }

  // For the stub, just supply the input as the output
  DDS::OctetSeq transformed_buffer(encoded_buffer);
  plain_buffer.swap(transformed_buffer);

  return true;
}

bool CryptoBuiltInImpl::preprocess_secure_submsg(
  DDS::Security::DatawriterCryptoHandle& datawriter_crypto,
  DDS::Security::DatareaderCryptoHandle& datareader_crypto,
  DDS::Security::SecureSubmessageCategory_t& secure_submessage_category,
  const DDS::OctetSeq& encoded_rtps_submessage,
  DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
  DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(encoded_rtps_submessage);

  if (DDS::HANDLE_NIL == receiving_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Receiving Participant");
    return false;
  }
  if (DDS::HANDLE_NIL == sending_participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Sending Participant");
    return false;
  }

  // For now, just set some simple values, but this won't
  // be very useful as a stub
  secure_submessage_category = DDS::Security::DATAWRITER_SUBMESSAGE;
  datawriter_crypto = 1;
  datareader_crypto = 2;

  //enum SecureSubmessageCategory_t
  //{
  //  INFO_SUBMESSAGE,
  //  DATAWRITER_SUBMESSAGE,
  //  DATAREADER_SUBMESSAGE
  //};

  // Determine submessage type

  // If DATAWRITER_SUBMESSAGE:
  //  Set datawriter_crypto to local datawriter crypto handle
  //  Set datareader_crypto to remote crypto handle linked to datawriter_crypto
  // else is DATAREADER_SUBMESSAGE:
  //  Set datareader_crypto to local datareader crypto handle
  //  Set datawriter_crypto to remote crypto handle linked to datareader_crypto

  // else Fail
  // Set datawriter_crypto to be either the local writer attached to the
  // external reader
  return true;
}

bool CryptoBuiltInImpl::decode_datawriter_submessage(
  DDS::OctetSeq& plain_rtps_submessage,
  const DDS::OctetSeq& encoded_rtps_submessage,
  DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
  DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  const DDS::Security::SecurityException& ex)
{
  // Error ex is marked as const in this function call
  ACE_UNUSED_ARG(ex);

  if (DDS::HANDLE_NIL == receiving_datareader_crypto) {
    //CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
    return false;
  }
  if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
    //CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
    return false;
  }

  // For the stub, just supply the input as the output
  DDS::OctetSeq transformed_buffer(encoded_rtps_submessage);
  plain_rtps_submessage.swap(transformed_buffer);

  return true;
}

bool CryptoBuiltInImpl::decode_datareader_submessage(
  DDS::OctetSeq& plain_rtps_message,
  const DDS::OctetSeq& encoded_rtps_message,
  DDS::Security::DatawriterCryptoHandle receiving_datawriter_crypto,
  DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
  DDS::Security::SecurityException& ex)
{
  if (DDS::HANDLE_NIL == sending_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
    return false;
  }
  if (DDS::HANDLE_NIL == receiving_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
    return false;
  }

  // For the stub, just supply the input as the output
  DDS::OctetSeq transformed_buffer(encoded_rtps_message);
  plain_rtps_message.swap(transformed_buffer);

  return true;
}

bool CryptoBuiltInImpl::decode_serialized_payload(
  DDS::OctetSeq& plain_buffer,
  const DDS::OctetSeq& encoded_buffer,
  const DDS::OctetSeq& inline_qos,
  DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
  DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
  DDS::Security::SecurityException& ex)
{
  ACE_UNUSED_ARG(inline_qos);

  if (DDS::HANDLE_NIL == receiving_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datareader handle");
    return false;
  }
  if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Datawriter handle");
    return false;
  }

  // For the stub, just supply the input as the output
  DDS::OctetSeq transformed_buffer(encoded_buffer);
  plain_buffer.swap(transformed_buffer);

  return true;
}

}
}
