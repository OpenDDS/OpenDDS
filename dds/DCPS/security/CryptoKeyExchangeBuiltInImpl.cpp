#include "dds/DCPS/security/CryptoKeyExchangeBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DCPS/security/TokenWriter.h"
#include "dds/DdsDcpsInfrastructureC.h"

namespace OpenDDS {
namespace Security {

// Constants used during crypto operations
static const std::string Crypto_Token_Class_Id("DDS:Crypto:AES_GCM_GMAC");
static const DDS::OctetSeq Empty_Seq;

CryptoKeyExchangeBuiltInImpl::CryptoKeyExchangeBuiltInImpl()
{

}

CryptoKeyExchangeBuiltInImpl::~CryptoKeyExchangeBuiltInImpl()
{

}
::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::create_local_participant_crypto_tokens(
  ::DDS::Security::ParticipantCryptoTokenSeq & local_participant_crypto_tokens,
  ::DDS::Security::ParticipantCryptoHandle local_participant_crypto,
  ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  ::DDS::Security::SecurityException & ex)
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

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::set_remote_participant_crypto_tokens(
  ::DDS::Security::ParticipantCryptoHandle local_participant_crypto,
  ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  const ::DDS::Security::ParticipantCryptoTokenSeq & remote_participant_tokens,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(local_participant_crypto);
  ACE_UNUSED_ARG(remote_participant_crypto);
  ACE_UNUSED_ARG(remote_participant_tokens);
  ACE_UNUSED_ARG(ex);

  // This stub method just pretends to work
  ::CORBA::Boolean results = true;
  return results;
}

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::create_local_datawriter_crypto_tokens(
  ::DDS::Security::DatawriterCryptoTokenSeq & local_datawriter_crypto_tokens,
  ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
  ::DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
  ::DDS::Security::SecurityException & ex)
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

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::set_remote_datawriter_crypto_tokens(
  ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
  ::DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
  const ::DDS::Security::DatawriterCryptoTokenSeq & remote_datawriter_tokens,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(local_datareader_crypto);
  ACE_UNUSED_ARG(remote_datawriter_crypto);
  ACE_UNUSED_ARG(remote_datawriter_tokens);
  ACE_UNUSED_ARG(ex);

  // The stub implementation will always succeed here
  ::CORBA::Boolean results = true;
  return results;
}

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::create_local_datareader_crypto_tokens(
  ::DDS::Security::DatareaderCryptoTokenSeq & local_datareader_cryto_tokens,
  ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
  ::DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
  ::DDS::Security::SecurityException & ex)
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

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::set_remote_datareader_crypto_tokens(
  ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
  ::DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
  const ::DDS::Security::DatareaderCryptoTokenSeq & remote_datareader_tokens,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(local_datawriter_crypto);
  ACE_UNUSED_ARG(remote_datareader_crypto);
  ACE_UNUSED_ARG(remote_datareader_tokens);
  ACE_UNUSED_ARG(ex);

  // The stub implementation will always succeed here
  ::CORBA::Boolean results = true;
  return results;
}

::CORBA::Boolean CryptoKeyExchangeBuiltInImpl::return_crypto_tokens(
  const ::DDS::Security::CryptoTokenSeq & crypto_tokens,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(crypto_tokens);
  ACE_UNUSED_ARG(ex);

  // Can't modify the input sequence at all because it's a const reference

  // The stub implementation will always succeed here
  ::CORBA::Boolean results = true;
  return results;
}

} // Security
} // OpenDDS