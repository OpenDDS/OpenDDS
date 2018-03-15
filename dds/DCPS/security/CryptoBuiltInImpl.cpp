/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "CryptoBuiltInImpl.h"
#include "CommonUtilities.h"
#include "TokenWriter.h"

#include "SSL/Utils.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsSecurityParamsC.h"

#include "dds/DCPS/GuidUtils.h"

#include <openssl/evp.h>

using namespace DDS::Security;

namespace OpenDDS {
namespace Security {

CryptoBuiltInImpl::CryptoBuiltInImpl()
  : mutex_()
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

NativeCryptoHandle CryptoBuiltInImpl::generate_handle()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  int new_handle = next_handle_++;

  if (new_handle == DDS::HANDLE_NIL) {
    new_handle = next_handle_++;
  }
  return static_cast<NativeCryptoHandle>(new_handle);
}


// Key Factory

namespace {

  KeyMaterial_AES_GCM_GMAC make_key(unsigned int key_id, bool encrypt)
  {
    KeyMaterial_AES_GCM_GMAC k;

    for (unsigned int i = 0; i < TransformKindIndex; ++i)
      k.transformation_kind[i] = 0;
    k.transformation_kind[TransformKindIndex] =
      encrypt ? CRYPTO_TRANSFORMATION_KIND_AES256_GCM
      : CRYPTO_TRANSFORMATION_KIND_AES256_GMAC;

    k.master_salt.length(0);

    for (unsigned int i = 0; i < sizeof k.sender_key_id; ++i) {
      k.sender_key_id[i] = key_id >> (8 * i);
    }

    k.master_sender_key.length(0);

    for (unsigned int i = 0; i < sizeof k.receiver_specific_key_id; ++i) {
      k.receiver_specific_key_id[i] = 0;
    }

    k.master_receiver_specific_key.length(0);
    return k;
  }

  template <typename T, typename TSeq>
  void push_back(TSeq& seq, const T& t)
  {
    const unsigned int i = seq.length();
    seq.length(i + 1);
    seq[i] = t;
  }
}

ParticipantCryptoHandle CryptoBuiltInImpl::register_local_participant(
  IdentityHandle participant_identity,
  PermissionsHandle participant_permissions,
  const DDS::PropertySeq&,
  const ParticipantSecurityAttributes& participant_security_attributes,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == participant_identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local participant ID");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == participant_permissions) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local permissions");
    return DDS::HANDLE_NIL;
  }

  const NativeCryptoHandle h = generate_handle();
  if (!participant_security_attributes.is_rtps_protected) {
    return h;
  }

  CommonUtilities::set_security_error(ex, -1, 0, "Unsupported configuration");
  return DDS::HANDLE_NIL;
}

ParticipantCryptoHandle CryptoBuiltInImpl::register_matched_remote_participant(
  ParticipantCryptoHandle local_participant_crypto_handle,
  IdentityHandle remote_participant_identity,
  PermissionsHandle /*remote_participant_permissions*/,
  SharedSecretHandle* shared_secret,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_participant_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local participant crypto handle");
    return DDS::HANDLE_NIL;
  }
  if (DDS::HANDLE_NIL == remote_participant_identity) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote participant ID");
    return DDS::HANDLE_NIL;
  }
  if (!shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret data");
    return DDS::HANDLE_NIL;
  }

  return generate_handle();
}

namespace {
  bool is_builtin_volatile(const DDS::PropertySeq& props)
  {
    for (unsigned int i = 0; i < props.length(); ++i) {
      if (0 == std::strcmp(props[i].name.in(),
                           "dds.sec.builtin_endpoint_name")) {
        return 0 == std::strcmp(props[i].value.in(),
                                "BuiltinParticipantVolatileMessageSecureWriter")
          || 0 == std::strcmp(props[i].value.in(),
                              "BuiltinParticipantVolatileMessageSecureReader");
      }
    }
    return false;
  }

  bool is_volatile_placeholder(const KeyMaterial_AES_GCM_GMAC& keymat)
  {
    static const CryptoTransformKind placeholder =
      {DCPS::VENDORID_OCI[0], DCPS::VENDORID_OCI[1], 0, 1};
    return 0 == std::memcmp(placeholder, keymat.transformation_kind,
                            sizeof placeholder);
  }

  KeyMaterial_AES_GCM_GMAC make_volatile_placeholder()
  {
    // not an actual key, just used to identify the local datawriter/reader
    // crypto handle for a Built-In Participant Volatile Msg endpoint
    const KeyMaterial_AES_GCM_GMAC k = {
      {DCPS::VENDORID_OCI[0], DCPS::VENDORID_OCI[1], 0, 1},
      KeyOctetSeq(), {0, 0, 0, 0}, KeyOctetSeq(), {0, 0, 0, 0}, KeyOctetSeq()
    };
    return k;
  }

  void hkdf(KeyOctetSeq& result, const DDS::OctetSeq_var& prefix,
            const char (&cookie)[17], const DDS::OctetSeq_var& suffix,
            const DDS::OctetSeq_var& data)
  {
    char* cookie_buffer = const_cast<char*>(cookie); // OctetSeq has no const
    DDS::OctetSeq cookieSeq(16, 16,
                            reinterpret_cast<CORBA::Octet*>(cookie_buffer));
    std::vector<const DDS::OctetSeq*> input(3);
    input[0] = prefix.ptr();
    input[1] = &cookieSeq;
    input[2] = suffix.ptr();
    DDS::OctetSeq key;
    SSL::hash(input, key);

    EVP_PKEY* pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, 0, key.get_buffer(),
                                          key.length());
    EVP_MD_CTX* ctx = EVP_MD_CTX_create();
    const EVP_MD* md = EVP_get_digestbyname("SHA256");
    EVP_DigestInit_ex(ctx, md, 0);
    EVP_DigestSignInit(ctx, 0, md, 0, pkey);
    EVP_DigestSignUpdate(ctx, data->get_buffer(), data->length());
    size_t req = 0;
    EVP_DigestSignFinal(ctx, 0, &req);
    result.length(static_cast<unsigned int>(req));
    EVP_DigestSignFinal(ctx, result.get_buffer(), &req);
    EVP_MD_CTX_destroy(ctx);
    EVP_PKEY_free(pkey);
  }

  KeyMaterial_AES_GCM_GMAC
  make_volatile_key(const DDS::OctetSeq_var& challenge1,
                    const DDS::OctetSeq_var& challenge2,
                    const DDS::OctetSeq_var& sharedSec)
  {
    static const char KxSaltCookie[] = "keyexchange salt";
    static const char KxKeyCookie[] = "key exchange key";
    KeyMaterial_AES_GCM_GMAC k = {
      {0, 0, 0, CRYPTO_TRANSFORMATION_KIND_AES256_GCM},
      KeyOctetSeq(), {0, 0, 0, 0}, KeyOctetSeq(), {0, 0, 0, 0}, KeyOctetSeq()
    };
    hkdf(k.master_salt, challenge1, KxSaltCookie, challenge2, sharedSec);
    hkdf(k.master_sender_key, challenge2, KxKeyCookie, challenge1, sharedSec);
    return k;
  }
}

DatawriterCryptoHandle CryptoBuiltInImpl::register_local_datawriter(
  ParticipantCryptoHandle participant_crypto,
  const DDS::PropertySeq& properties,
  const EndpointSecurityAttributes& security_attributes,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }

  const NativeCryptoHandle h = generate_handle();
  const PluginEndpointSecurityAttributesMask plugin_attribs =
    security_attributes.plugin_endpoint_attributes;
  KeySeq keys;

  if (is_builtin_volatile(properties)) {
    push_back(keys, make_volatile_placeholder());

  } else {
    bool used_h = false;
    if (security_attributes.is_submessage_protected) {
      push_back(keys,
                make_key(h, plugin_attribs & FLAG_IS_SUBMESSAGE_ENCRYPTED));
      used_h = true;
    }
    if (security_attributes.is_payload_protected) {
      const unsigned int key_id = used_h ? generate_handle() : h;
      push_back(keys,
                make_key(key_id, plugin_attribs & FLAG_IS_PAYLOAD_ENCRYPTED));
    }
  }

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  keys_[h] = keys;
  participant_to_entity_.insert(std::make_pair(participant_crypto, h));

  return h;
}

DatareaderCryptoHandle CryptoBuiltInImpl::register_matched_remote_datareader(
  DatawriterCryptoHandle local_datawriter_crypto_handle,
  ParticipantCryptoHandle remote_participant_crypto,
  SharedSecretHandle* shared_secret,
  bool /*relay_only*/,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_datawriter_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local DataWriter Crypto Handle");
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

  const DatareaderCryptoHandle h = generate_handle();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!keys_.count(local_datawriter_crypto_handle)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local DataWriter Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  const KeySeq& dw_keys = keys_[local_datawriter_crypto_handle];

  if (dw_keys.length() == 1 && is_volatile_placeholder(dw_keys[0])) {
    // Create a key from SharedSecret and track it as if Key Exchange happened
    KeySeq dr_keys(1);
    dr_keys.length(1);
    dr_keys[0] = make_volatile_key(shared_secret->challenge1(),
                                   shared_secret->challenge2(),
                                   shared_secret->sharedSecret());
    keys_[h] = dr_keys;
  }

  participant_to_entity_.insert(std::make_pair(remote_participant_crypto, h));
  return h;
}

DatareaderCryptoHandle CryptoBuiltInImpl::register_local_datareader(
  ParticipantCryptoHandle participant_crypto,
  const DDS::PropertySeq& properties,
  const EndpointSecurityAttributes& security_attributes,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == participant_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Participant Crypto Handle");
    return DDS::HANDLE_NIL;
  }

  const NativeCryptoHandle h = generate_handle();
  const PluginEndpointSecurityAttributesMask plugin_attribs =
    security_attributes.plugin_endpoint_attributes;
  KeySeq keys;

  if (is_builtin_volatile(properties)) {
    push_back(keys, make_volatile_placeholder());

  } else {
    if (security_attributes.is_submessage_protected) {
      push_back(keys,
                make_key(h, plugin_attribs & FLAG_IS_SUBMESSAGE_ENCRYPTED));
    }
  }

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  keys_[h] = keys;
  participant_to_entity_.insert(std::make_pair(participant_crypto, h));

  return h;
}

DatawriterCryptoHandle CryptoBuiltInImpl::register_matched_remote_datawriter(
  DatareaderCryptoHandle local_datareader_crypto_handle,
  ParticipantCryptoHandle remote_participant_crypto,
  SharedSecretHandle* shared_secret,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_datareader_crypto_handle) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local DataWriter Crypto Handle");
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

  const DatareaderCryptoHandle h = generate_handle();
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  if (!keys_.count(local_datareader_crypto_handle)) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Local DataReader Crypto Handle");
    return DDS::HANDLE_NIL;
  }
  const KeySeq& dr_keys = keys_[local_datareader_crypto_handle];

  if (dr_keys.length() == 1 && is_volatile_placeholder(dr_keys[0])) {
    // Create a key from SharedSecret and track it as if Key Exchange happened
    KeySeq dw_keys(1);
    dw_keys.length(1);
    dw_keys[0] = make_volatile_key(shared_secret->challenge1(),
                                   shared_secret->challenge2(),
                                   shared_secret->sharedSecret());
    keys_[h] = dw_keys;
  }

  participant_to_entity_.insert(std::make_pair(remote_participant_crypto, h));
  return h;
}

bool CryptoBuiltInImpl::unregister_participant(
  ParticipantCryptoHandle participant_crypto_handle,
  SecurityException& ex)
{
  ACE_UNUSED_ARG(participant_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

bool CryptoBuiltInImpl::unregister_datawriter(
  DatawriterCryptoHandle datawriter_crypto_handle,
  SecurityException& ex)
{
  ACE_UNUSED_ARG(datawriter_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

bool CryptoBuiltInImpl::unregister_datareader(
  DatareaderCryptoHandle datareader_crypto_handle,
  SecurityException& ex)
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
  ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
  ParticipantCryptoHandle local_participant_crypto,
  ParticipantCryptoHandle remote_participant_crypto,
  SecurityException& ex)
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
  ParticipantCryptoHandle local_participant_crypto,
  ParticipantCryptoHandle remote_participant_crypto,
  const ParticipantCryptoTokenSeq& remote_participant_tokens,
  SecurityException& ex)
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
  DatawriterCryptoTokenSeq& local_datawriter_crypto_tokens,
  DatawriterCryptoHandle local_datawriter_crypto,
  DatareaderCryptoHandle remote_datareader_crypto,
  SecurityException& ex)
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
  DatareaderCryptoHandle local_datareader_crypto,
  DatawriterCryptoHandle remote_datawriter_crypto,
  const DatawriterCryptoTokenSeq& remote_datawriter_tokens,
  SecurityException& ex)
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
  DatareaderCryptoTokenSeq& local_datareader_cryto_tokens,
  DatareaderCryptoHandle local_datareader_crypto,
  DatawriterCryptoHandle remote_datawriter_crypto,
  SecurityException& ex)
{
  if (DDS::HANDLE_NIL == local_datareader_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid local reader handle");
    return false;
  }
  if (DDS::HANDLE_NIL == remote_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid remote writer handle");
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
  DatawriterCryptoHandle local_datawriter_crypto,
  DatareaderCryptoHandle remote_datareader_crypto,
  const DatareaderCryptoTokenSeq& remote_datareader_tokens,
  SecurityException& ex)
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
  const CryptoTokenSeq& crypto_tokens,
  SecurityException& ex)
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
  DatawriterCryptoHandle sending_datawriter_crypto,
  SecurityException& ex)
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
  DatawriterCryptoHandle sending_datawriter_crypto,
  const DatareaderCryptoHandleSeq& receiving_datareader_crypto_list,
  CORBA::Long& receiving_datareader_crypto_list_index,
  SecurityException& ex)
{
  // Verify the input handles are valid before doing the transformation
  if (DDS::HANDLE_NIL == sending_datawriter_crypto) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid datawriter handle");
    return false;
  }

  DatareaderCryptoHandle reader_handle = DDS::HANDLE_NIL;
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
  DatareaderCryptoHandle sending_datareader_crypto,
  const DatawriterCryptoHandleSeq& receiving_datawriter_crypto_list,
  SecurityException& ex)
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
  ParticipantCryptoHandle sending_participant_crypto,
  const ParticipantCryptoHandleSeq& receiving_participant_crypto_list,
  CORBA::Long& receiving_participant_crypto_list_index,
  SecurityException& ex)
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

  ParticipantCryptoHandle dest_handle = DDS::HANDLE_NIL;
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
  ParticipantCryptoHandle receiving_participant_crypto,
  ParticipantCryptoHandle sending_participant_crypto,
  SecurityException& ex)
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
  DatawriterCryptoHandle& datawriter_crypto,
  DatareaderCryptoHandle& datareader_crypto,
  SecureSubmessageCategory_t& secure_submessage_category,
  const DDS::OctetSeq& encoded_rtps_submessage,
  ParticipantCryptoHandle receiving_participant_crypto,
  ParticipantCryptoHandle sending_participant_crypto,
  SecurityException& ex)
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
  secure_submessage_category = DATAWRITER_SUBMESSAGE;
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
  DatareaderCryptoHandle receiving_datareader_crypto,
  DatawriterCryptoHandle sending_datawriter_crypto,
  const SecurityException& ex)
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
  DatawriterCryptoHandle receiving_datawriter_crypto,
  DatareaderCryptoHandle sending_datareader_crypto,
  SecurityException& ex)
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
  DatareaderCryptoHandle receiving_datareader_crypto,
  DatawriterCryptoHandle sending_datawriter_crypto,
  SecurityException& ex)
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
