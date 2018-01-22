/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDS_CRYPTO_KEY_EXHANGE_BUILT_IN_IMPL_H
#define DDS_CRYPTO_KEY_EXHANGE_BUILT_IN_IMPL_H

#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "tao/LocalObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {


/**
* @class CryptoKeyExchangeBuiltInImpl
*
* @brief Implements the DDS built-in version of the Crypto plugin
* CryptoKeyExchange interface for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interfaces this class is implementing.
*
*/
class DdsSecurity_Export CryptoKeyExchangeBuiltInImpl
  : public virtual DDS::Security::CryptoKeyExchange
  , public virtual CORBA::LocalObject
{
public:
  CryptoKeyExchangeBuiltInImpl();
  virtual ~CryptoKeyExchangeBuiltInImpl();

  virtual ::CORBA::Boolean create_local_participant_crypto_tokens(
    ::DDS::Security::ParticipantCryptoTokenSeq & local_participant_crypto_tokens,
    ::DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_remote_participant_crypto_tokens(
    ::DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    ::DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    const ::DDS::Security::ParticipantCryptoTokenSeq & remote_participant_tokens,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean create_local_datawriter_crypto_tokens(
    ::DDS::Security::DatawriterCryptoTokenSeq & local_datawriter_crypto_tokens,
    ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    ::DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_remote_datawriter_crypto_tokens(
    ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    ::DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
    const ::DDS::Security::DatawriterCryptoTokenSeq & remote_datawriter_tokens,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean create_local_datareader_crypto_tokens(
    ::DDS::Security::DatareaderCryptoTokenSeq & local_datareader_cryto_tokens,
    ::DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    ::DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean set_remote_datareader_crypto_tokens(
    ::DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    ::DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
    const ::DDS::Security::DatareaderCryptoTokenSeq & remote_datareader_tokens,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean return_crypto_tokens(
    const ::DDS::Security::CryptoTokenSeq & crypto_tokens,
    ::DDS::Security::SecurityException & ex);

private:

  CryptoKeyExchangeBuiltInImpl(const CryptoKeyExchangeBuiltInImpl& right);
  CryptoKeyExchangeBuiltInImpl& operator=(const CryptoKeyExchangeBuiltInImpl& right);
};

} // OpenDDS
} // Security

#endif  // DDS_CRYPTO_KEY_EXHANGE_BUILT_IN_IMPL_H
