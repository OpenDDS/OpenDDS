/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_CRYPTOBUILTINIMPL_H
#define OPENDDS_DCPS_SECURITY_CRYPTOBUILTINIMPL_H

#include "OpenDDS_Security_Export.h"
#include "CryptoBuiltInC.h"

#include <dds/DdsSecurityCoreC.h>
#include <dds/Versioned_Namespace.h>

#include <tao/LocalObject.h>

#include <ace/Thread_Mutex.h>

#include <map>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

class OpenDDS_Security_Export CryptoBuiltInImpl
  : public virtual DDS::Security::CryptoKeyFactory
  , public virtual DDS::Security::CryptoKeyExchange
  , public virtual DDS::Security::CryptoTransform
  , public virtual CORBA::LocalObject
{
public:
  CryptoBuiltInImpl();
  virtual ~CryptoBuiltInImpl();


private:
  // Local Object

  bool _is_a(const char*);
  const char* _interface_repository_id() const;
  bool marshal(TAO_OutputCDR&);


  // Key Factory

  virtual DDS::Security::ParticipantCryptoHandle register_local_participant(
    DDS::Security::IdentityHandle participant_identity,
    DDS::Security::PermissionsHandle participant_permissions,
    const DDS::PropertySeq& participant_properties,
    const DDS::Security::ParticipantSecurityAttributes& participant_security_attributes,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::ParticipantCryptoHandle register_matched_remote_participant(
    DDS::Security::ParticipantCryptoHandle local_participant_crypto_handle,
    DDS::Security::IdentityHandle remote_participant_identity,
    DDS::Security::PermissionsHandle remote_participant_permissions,
    DDS::Security::SharedSecretHandle* shared_secret,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::DatawriterCryptoHandle register_local_datawriter(
    DDS::Security::ParticipantCryptoHandle participant_crypto,
    const DDS::PropertySeq& datawriter_properties,
    const DDS::Security::EndpointSecurityAttributes& datawriter_security_attributes,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::DatareaderCryptoHandle register_matched_remote_datareader(
    DDS::Security::DatawriterCryptoHandle local_datawriter_crypto_handle,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    DDS::Security::SharedSecretHandle* shared_secret,
    bool relay_only,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::DatareaderCryptoHandle register_local_datareader(
    DDS::Security::ParticipantCryptoHandle participant_crypto,
    const DDS::PropertySeq& datareader_properties,
    const DDS::Security::EndpointSecurityAttributes& datareader_security_attributes,
    DDS::Security::SecurityException& ex);

  virtual DDS::Security::DatawriterCryptoHandle register_matched_remote_datawriter(
    DDS::Security::DatareaderCryptoHandle local_datareader_crypto_handle,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypt,
    DDS::Security::SharedSecretHandle* shared_secret,
    DDS::Security::SecurityException& ex);

  virtual bool unregister_participant(
    DDS::Security::ParticipantCryptoHandle participant_crypto_handle,
    DDS::Security::SecurityException& ex);

  virtual bool unregister_datawriter(
    DDS::Security::DatawriterCryptoHandle datawriter_crypto_handle,
    DDS::Security::SecurityException& ex);

  virtual bool unregister_datareader(
    DDS::Security::DatareaderCryptoHandle datareader_crypto_handle,
    DDS::Security::SecurityException& ex);


  // Key Exchange

  virtual bool create_local_participant_crypto_tokens(
    DDS::Security::ParticipantCryptoTokenSeq& local_participant_crypto_tokens,
    DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool have_local_participant_crypto_tokens(
    DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypto);

  virtual bool set_remote_participant_crypto_tokens(
    DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypto,
    const DDS::Security::ParticipantCryptoTokenSeq& remote_participant_tokens,
    DDS::Security::SecurityException& ex);

  virtual bool have_remote_participant_crypto_tokens(
    DDS::Security::ParticipantCryptoHandle local_participant_crypto,
    DDS::Security::ParticipantCryptoHandle remote_participant_crypto);

  virtual bool create_local_datawriter_crypto_tokens(
    DDS::Security::DatawriterCryptoTokenSeq& local_datawriter_crypto_tokens,
    DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool have_local_datawriter_crypto_tokens(
    DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle remote_datareader_crypto);

  virtual bool set_remote_datawriter_crypto_tokens(
    DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
    const DDS::Security::DatawriterCryptoTokenSeq& remote_datawriter_tokens,
    DDS::Security::SecurityException& ex);

  virtual bool have_remote_datawriter_crypto_tokens(
    DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto);

  virtual bool create_local_datareader_crypto_tokens(
    DDS::Security::DatareaderCryptoTokenSeq& local_datareader_crypto_tokens,
    DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool have_local_datareader_crypto_tokens(
    DDS::Security::DatareaderCryptoHandle local_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle remote_datawriter_crypto);

  virtual bool set_remote_datareader_crypto_tokens(
    DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle remote_datareader_crypto,
    const DDS::Security::DatareaderCryptoTokenSeq& remote_datareader_tokens,
    DDS::Security::SecurityException& ex);

  virtual bool have_remote_datareader_crypto_tokens(
    DDS::Security::DatawriterCryptoHandle local_datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle remote_datareader_crypto);

  virtual bool return_crypto_tokens(
    const DDS::Security::CryptoTokenSeq& crypto_tokens,
    DDS::Security::SecurityException& ex);


  // Transform

  virtual bool encode_serialized_payload(
    DDS::OctetSeq& encoded_buffer,
    DDS::OctetSeq& extra_inline_qos,
    const DDS::OctetSeq& plain_buffer,
    DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool encode_datawriter_submessage(
    DDS::OctetSeq& encoded_rtps_submessage,
    const DDS::OctetSeq& plain_rtps_submessage,
    DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    const DDS::Security::DatareaderCryptoHandleSeq& receiving_datareader_crypto_list,
    CORBA::Long& receiving_datareader_crypto_list_index,
    DDS::Security::SecurityException& ex);

  virtual bool encode_datareader_submessage(
    DDS::OctetSeq& encoded_rtps_submessage,
    const DDS::OctetSeq& plain_rtps_submessage,
    DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
    const DDS::Security::DatawriterCryptoHandleSeq& receiving_datawriter_crypto_list,
    DDS::Security::SecurityException& ex);

  virtual bool encode_rtps_message(
    DDS::OctetSeq& encoded_rtps_message,
    const DDS::OctetSeq& plain_rtps_message,
    DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    const DDS::Security::ParticipantCryptoHandleSeq& receiving_participant_crypto_list,
    CORBA::Long& receiving_participant_crypto_list_index,
    DDS::Security::SecurityException& ex);

  virtual bool decode_rtps_message(
    DDS::OctetSeq& plain_buffer,
    const DDS::OctetSeq& encoded_buffer,
    DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
    DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool preprocess_secure_submsg(
    DDS::Security::DatawriterCryptoHandle& datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle& datareader_crypto,
    DDS::Security::SecureSubmessageCategory_t& secure_submessage_category,
    const DDS::OctetSeq& encoded_rtps_submessage,
    DDS::Security::ParticipantCryptoHandle receiving_participant_crypto,
    DDS::Security::ParticipantCryptoHandle sending_participant_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool decode_datawriter_submessage(
    DDS::OctetSeq& plain_rtps_submessage,
    const DDS::OctetSeq& encoded_rtps_submessage,
    DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool decode_datareader_submessage(
    DDS::OctetSeq& plain_rtps_submessage,
    const DDS::OctetSeq& encoded_rtps_submessage,
    DDS::Security::DatawriterCryptoHandle receiving_datawriter_crypto,
    DDS::Security::DatareaderCryptoHandle sending_datareader_crypto,
    DDS::Security::SecurityException& ex);

  virtual bool decode_serialized_payload(
    DDS::OctetSeq& plain_buffer,
    const DDS::OctetSeq& encoded_buffer,
    const DDS::OctetSeq& inline_qos,
    DDS::Security::DatareaderCryptoHandle receiving_datareader_crypto,
    DDS::Security::DatawriterCryptoHandle sending_datawriter_crypto,
    DDS::Security::SecurityException& ex);

  CryptoBuiltInImpl(const CryptoBuiltInImpl&);
  CryptoBuiltInImpl& operator=(const CryptoBuiltInImpl&);

  DDS::Security::NativeCryptoHandle generate_handle();
  DDS::Security::NativeCryptoHandle generate_handle_i();

  ACE_Thread_Mutex mutex_;
  int next_handle_;

  typedef KeyMaterial_AES_GCM_GMAC KeyMaterial;
  typedef KeyMaterial_AES_GCM_GMAC_Seq KeySeq;
  typedef std::map<DDS::Security::NativeCryptoHandle, KeySeq> KeyTable_t;
  KeyTable_t keys_;

  /// Use CryptoHandles to Determine What Should Be Encrypted and Decrypted
  ///@{
  struct EncryptOpts {
    bool submessage_, payload_;
    EncryptOpts() : submessage_(false), payload_(false) {}
    EncryptOpts(const DDS::Security::EndpointSecurityAttributes& attribs)
      : submessage_(attribs.is_submessage_protected)
      , payload_(attribs.is_payload_protected)
    {}
  };
  typedef std::map<DDS::Security::NativeCryptoHandle, EncryptOpts> EncryptOptions_t;
  EncryptOptions_t encrypt_options_;
  ///@}

  struct EntityInfo {
    DDS::Security::SecureSubmessageCategory_t category_;
    DDS::Security::NativeCryptoHandle handle_;
    EntityInfo(DDS::Security::SecureSubmessageCategory_t c,
               DDS::Security::NativeCryptoHandle h)
      : category_(c), handle_(h) {}
  };
  std::multimap<DDS::Security::ParticipantCryptoHandle,
                EntityInfo> participant_to_entity_;

  typedef std::pair<DDS::Security::NativeCryptoHandle, DDS::Security::NativeCryptoHandle> HandlePair_t;
  typedef std::map<HandlePair_t, DDS::Security::NativeCryptoHandle> DerivedKeyIndex_t;
  DerivedKeyIndex_t derived_key_handles_;

  struct Session {
    SessionIdType id_;
    IV_SuffixType iv_suffix_;
    KeyOctetSeq key_;
    ACE_UINT64 counter_;

    KeyOctetSeq get_key(const KeyMaterial& master, const CryptoHeader& header);
    void create_key(const KeyMaterial& master);
    void derive_key(const KeyMaterial& master);
    void next_id(const KeyMaterial& master);
    void inc_iv();
  };
  typedef std::pair<DDS::Security::NativeCryptoHandle, unsigned int> KeyId_t;
  typedef std::map<KeyId_t, Session> SessionTable_t;
  SessionTable_t sessions_;

  void clear_endpoint_data(DDS::Security::NativeCryptoHandle handle);
  void clear_common_data(DDS::Security::NativeCryptoHandle handle);

  bool encode_submessage(DDS::OctetSeq& encoded_rtps_submessage,
                         const DDS::OctetSeq& plain_rtps_submessage,
                         DDS::Security::NativeCryptoHandle sender_handle,
                         DDS::Security::SecurityException& ex);

  bool encrypt(const KeyMaterial& master, Session& sess,
               const DDS::OctetSeq& plain,
               CryptoHeader& header, CryptoFooter& footer,
               DDS::OctetSeq& out, DDS::Security::SecurityException& ex);

  bool authtag(const KeyMaterial& master, Session& sess,
               const DDS::OctetSeq& plain,
               CryptoHeader& header, CryptoFooter& footer,
               DDS::Security::SecurityException& ex);

  void encauth_setup(const KeyMaterial& master, Session& sess,
                     const DDS::OctetSeq& plain, CryptoHeader& header);

  bool decode_submessage(DDS::OctetSeq& plain_rtps_submessage,
                         const DDS::OctetSeq& encoded_rtps_submessage,
                         DDS::Security::NativeCryptoHandle sender_handle,
                         DDS::Security::SecurityException& ex);

  bool decrypt(const KeyMaterial& master, Session& sess, const char* ciphertext,
               unsigned int n, const CryptoHeader& header,
               const CryptoFooter& footer, DDS::OctetSeq& out,
               DDS::Security::SecurityException& ex);

  bool verify(const KeyMaterial& master, Session& sess, const char* in,
              unsigned int n, const CryptoHeader& header,
              const CryptoFooter& footer, DDS::OctetSeq& out,
              DDS::Security::SecurityException& ex);
};

} // Security
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
