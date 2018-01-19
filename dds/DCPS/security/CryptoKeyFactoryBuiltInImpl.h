/*
 *
 *
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */



#ifndef DDS_CRYPTO_KEY_FACTORY_BUILTIN_IMPL_H
#define DDS_CRYPTO_KEY_FACTORY_BUILTIN_IMPL_H

#include "dds/DCPS/security/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "tao/LocalObject.h"
#include "ace/Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
* @class CryptoKeyFactoryBuiltInImpl
*
* @brief Implements the CryptoKeyFactory interface in the built in Cryptographic
* plugin for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/
class DdsSecurity_Export  CryptoKeyFactoryBuiltInImpl
	: public virtual DDS::Security::CryptoKeyFactory
  , public virtual CORBA::LocalObject
{
  
public:

  CryptoKeyFactoryBuiltInImpl();

  virtual ~CryptoKeyFactoryBuiltInImpl();

  virtual ::DDS::Security::ParticipantCryptoHandle register_local_participant(
    ::DDS::Security::IdentityHandle participant_identity,
    ::DDS::Security::PermissionsHandle participant_permissions,
    const ::DDS::PropertySeq & participant_properties,
    const ::DDS::Security::ParticipantSecurityAttributes & participant_security_attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::ParticipantCryptoHandle register_matched_remote_participant(
    ::DDS::Security::ParticipantCryptoHandle local_participant_crypto_handle,
    ::DDS::Security::IdentityHandle remote_participant_identity,
    ::DDS::Security::PermissionsHandle remote_participant_permissions,
    ::DDS::Security::SharedSecretHandle shared_secret,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::DatawriterCryptoHandle register_local_datawriter(
    ::DDS::Security::ParticipantCryptoHandle participant_crypto,
    const ::DDS::PropertySeq & datawriter_properties,
    const ::DDS::Security::EndpointSecurityAttributes & datawriter_security_attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::DatareaderCryptoHandle register_matched_remote_datareader(
    ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto_handle,
    ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    ::DDS::Security::SharedSecretHandle shared_secret,
    ::CORBA::Boolean relay_only,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::DatareaderCryptoHandle register_local_datareader(
    ::DDS::Security::ParticipantCryptoHandle participant_crypto,
    const ::DDS::PropertySeq & datareader_properties,
    const ::DDS::Security::EndpointSecurityAttributes & datareader_security_attributes,
    ::DDS::Security::SecurityException & ex);

  virtual ::DDS::Security::DatawriterCryptoHandle register_matched_remote_datawriter(
    ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto_handle,
    ::DDS::Security::ParticipantCryptoHandle remote_participant_crypt,
    ::DDS::Security::SharedSecretHandle shared_secret,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean unregister_participant(
    ::DDS::Security::ParticipantCryptoHandle participant_crypto_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean unregister_datawriter(
    ::DDS::Security::DatawriterCryptoHandle datawriter_crypto_handle,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean unregister_datareader(
    ::DDS::Security::DatareaderCryptoHandle datareader_crypto_handle,
    ::DDS::Security::SecurityException & ex);

private:

  CryptoKeyFactoryBuiltInImpl(const CryptoKeyFactoryBuiltInImpl& right);
  CryptoKeyFactoryBuiltInImpl& operator=(const CryptoKeyFactoryBuiltInImpl& right);

  int generate_handle();

  ACE_Thread_Mutex handle_mutex_;
  int next_handle_;
};

} // Security
} // OpenDDS

#endif // DDS_CRYPTO_KEY_FACTORY_BUILTIN_IMPL_H