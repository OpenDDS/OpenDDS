#include "dds/DCPS/security/CryptoKeyFactoryBuiltInImpl.h"
#include "dds/DCPS/security/CommonUtilities.h"
#include "dds/DdsDcpsInfrastructureC.h"

namespace OpenDDS {
namespace Security {

CryptoKeyFactoryBuiltInImpl::CryptoKeyFactoryBuiltInImpl()
: handle_mutex_()
, next_handle_(1)
{

}

CryptoKeyFactoryBuiltInImpl::~CryptoKeyFactoryBuiltInImpl()
{

}

::DDS::Security::ParticipantCryptoHandle CryptoKeyFactoryBuiltInImpl::register_local_participant(
  ::DDS::Security::IdentityHandle participant_identity,
  ::DDS::Security::PermissionsHandle participant_permissions,
  const ::DDS::PropertySeq & participant_properties,
  const ::DDS::Security::ParticipantSecurityAttributes & participant_security_attributes,
  ::DDS::Security::SecurityException & ex)
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

::DDS::Security::ParticipantCryptoHandle CryptoKeyFactoryBuiltInImpl::register_matched_remote_participant(
  ::DDS::Security::ParticipantCryptoHandle local_participant_crypto_handle,
  ::DDS::Security::IdentityHandle remote_participant_identity,
  ::DDS::Security::PermissionsHandle remote_participant_permissions,
  ::DDS::Security::SharedSecretHandle shared_secret,
  ::DDS::Security::SecurityException & ex)
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
  if (DDS::HANDLE_NIL == shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret data");
    return DDS::HANDLE_NIL;
  }

  // The stub won't keep track of what has been registered, it
  // just returns a new handle
  return generate_handle(); 
}

::DDS::Security::DatawriterCryptoHandle CryptoKeyFactoryBuiltInImpl::register_local_datawriter(
  ::DDS::Security::ParticipantCryptoHandle participant_crypto,
  const ::DDS::PropertySeq & datawriter_properties,
  const ::DDS::Security::EndpointSecurityAttributes & datawriter_security_attributes,
  ::DDS::Security::SecurityException & ex)
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

::DDS::Security::DatareaderCryptoHandle CryptoKeyFactoryBuiltInImpl::register_matched_remote_datareader(
  ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto_handle,
  ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
  ::DDS::Security::SharedSecretHandle shared_secret,
  ::CORBA::Boolean relay_only,
  ::DDS::Security::SecurityException & ex)
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
  if (DDS::HANDLE_NIL == shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret Handle");
    return DDS::HANDLE_NIL;
  }

  // The stub always generates a new handle as long as the input handle isn't NIL
  return generate_handle();
}

::DDS::Security::DatareaderCryptoHandle CryptoKeyFactoryBuiltInImpl::register_local_datareader(
  ::DDS::Security::ParticipantCryptoHandle participant_crypto,
  const ::DDS::PropertySeq & datareader_properties,
  const ::DDS::Security::EndpointSecurityAttributes & datareader_security_attributes,
  ::DDS::Security::SecurityException & ex)
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

::DDS::Security::DatawriterCryptoHandle CryptoKeyFactoryBuiltInImpl::register_matched_remote_datawriter(
  ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto_handle,
  ::DDS::Security::ParticipantCryptoHandle remote_participant_crypt,
  ::DDS::Security::SharedSecretHandle shared_secret,
  ::DDS::Security::SecurityException & ex)
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
  if (DDS::HANDLE_NIL == shared_secret) {
    CommonUtilities::set_security_error(ex, -1, 0, "Invalid Shared Secret Handle");
    return DDS::HANDLE_NIL;
  }

  // The stub always generates a new handle as long as the input handle isn't NIL
  return generate_handle();
}

::CORBA::Boolean CryptoKeyFactoryBuiltInImpl::unregister_participant(
  ::DDS::Security::ParticipantCryptoHandle participant_crypto_handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(participant_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

::CORBA::Boolean CryptoKeyFactoryBuiltInImpl::unregister_datawriter(
  ::DDS::Security::DatawriterCryptoHandle datawriter_crypto_handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(datawriter_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

::CORBA::Boolean CryptoKeyFactoryBuiltInImpl::unregister_datareader(
  ::DDS::Security::DatareaderCryptoHandle datareader_crypto_handle,
  ::DDS::Security::SecurityException & ex)
{
  ACE_UNUSED_ARG(datareader_crypto_handle);
  ACE_UNUSED_ARG(ex);

  // The stub will always succeed here
  return true;
}

int CryptoKeyFactoryBuiltInImpl::generate_handle()
{
  ACE_Guard<ACE_Thread_Mutex> guard(handle_mutex_);
  int new_handle = next_handle_++;

  if (new_handle == DDS::HANDLE_NIL) {
    new_handle = next_handle_++;
  }
  return new_handle;
}

} // Security
} // OpenDDS