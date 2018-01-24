/*
 *
 *
 * Distributed under the DDS License.
 * See: http://www.DDS.org/license.html
 */
#ifndef DDS_CRYPTO_TRANSFORM_BUILTIN_IMPL_H
#define DDS_CRYPTO_TRANSFORM_BUILTIN_IMPL_H

#include "dds/DdsSecurityCoreC.h"
#include "dds/Versioned_Namespace.h"
#include "tao/LocalObject.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

/**
* @class CryptoTransformBuiltInImpl
*
* @brief Implements the CryptoTransform interface in the built in Cryptographic
* plugin for the DDS Security Specification
*
* See the DDS security specification, OMG formal/17-09-20, for a description of
* the interface this class is implementing.
*
*/
class DdsSecurity_Export  CryptoTransformBuiltInImpl
	: public virtual DDS::Security::CryptoTransform
  , public virtual CORBA::LocalObject
{
public:

  virtual ::CORBA::Boolean encode_serialized_payload(
    ::DDS::OctetSeq & encoded_buffer,
    ::DDS::OctetSeq & extra_inline_qos,
    const ::DDS::OctetSeq & plain_buffer,
    ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean encode_datawriter_submessage(
    ::DDS::OctetSeq & encoded_rtps_submessage,
    const ::DDS::OctetSeq & plain_rtps_submessage,
    ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    const ::DDS::Security::DatareaderCryptoHandleSeq & receiving_datareader_crypto_list,
    ::CORBA::Long & receiving_datareader_crypto_list_index,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean encode_datareader_submessage(
    ::DDS::OctetSeq & encoded_rtps_submessage,
    const ::DDS::OctetSeq & plain_rtps_submessage,
    ::DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
    const ::DDS::Security::DatawriterCryptoHandleSeq & receiving_datawriter_crypto_list,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean encode_rtps_message(
    ::DDS::OctetSeq & encoded_rtps_message,
    const ::DDS::OctetSeq & plain_rtps_message,
    ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    const ::DDS::Security::ParticipantCryptoHandleSeq & receiving_participant_crypto_list,
    ::CORBA::Long & receiving_participant_crypto_list_index,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean decode_rtps_message(
    ::DDS::OctetSeq & plain_buffer,
    const ::DDS::OctetSeq & encoded_buffer,
    ::DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
    ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean preprocess_secure_submsg(
    ::DDS::Security::DatawriterCryptoHandle & datawriter_crypto,
    ::DDS::Security::DatareaderCryptoHandle & datareader_crypto,
    ::DDS::Security::SecureSubmessageCategory_t & secure_submessage_category,
    const ::DDS::OctetSeq & encoded_rtps_submessage,
    ::DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
    ::DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean decode_datawriter_submessage(
    ::DDS::OctetSeq & plain_rtps_submessage,
    const ::DDS::OctetSeq & encoded_rtps_submessage,
    ::DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
    ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    const ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean decode_datareader_submessage(
    ::DDS::OctetSeq & plain_rtps_message,
    const ::DDS::OctetSeq & encoded_rtps_message,
    ::DDS::Security::DatawriterCryptoHandle receiving_datawriter_crypto,
    ::DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
    ::DDS::Security::SecurityException & ex);

  virtual ::CORBA::Boolean decode_serialized_payload(
    ::DDS::OctetSeq & plain_buffer,
    const ::DDS::OctetSeq & encoded_buffer,
    const ::DDS::OctetSeq & inline_qos,
    ::DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
    ::DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    ::DDS::Security::SecurityException & ex);
};

} // Security 
} // OpenDDS

#endif // DDS_CRYPTO_TRANSFORM_BUILTIN_IMPL_H
