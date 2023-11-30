/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifdef OPENDDS_SECURITY

#include "sec_doc.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/security/SSL/Certificate.h>
#include <dds/DCPS/security/SSL/SignedDocument.h>
#include <dds/DCPS/security/SSL/Utils.h>
#include <dds/DCPS/security/CommonUtilities.h>
#include <dds/DCPS/security/TokenReader.h>
#include <dds/DCPS/security/AuthenticationBuiltInImpl.h>
#include <dds/DCPS/security/OpenSSL_init.h>

#include <dds/DCPS/RTPS/RtpsCoreC.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <cstring>
#include <algorithm>
#include <cstdio>

#include <gtest/gtest.h>

using OpenDDS::DCPS::GUID_t;
using DDS::DomainParticipantQos;
using namespace DDS::Security;
using namespace OpenDDS::Security;
using namespace OpenDDS::Security::SSL;

/// @brief Contains all relevant data needed for a simulated participant
/// in the authentication process.
struct MockParticipantData
{
  AuthenticationBuiltInImpl auth;

  DomainParticipantQos qos;
  GUID_t guid;
  GUID_t guid_adjusted;
  GUID_t guid_adjusted_remote;
  DomainId_t domain_id;
  SecurityException ex;

  IdentityHandle id_handle;
  IdentityHandle id_handle_remote;
  IdentityToken id_token;
  IdentityToken id_token_remote;
  AuthRequestMessageToken auth_request_message_token;
  AuthRequestMessageToken auth_request_message_token_remote;
  HandshakeHandle handshake_handle;
  AuthenticatedPeerCredentialToken auth_peer_credential_token;

  DDS::OctetSeq mock_participant_builtin_topic_data;

  static unsigned char next_guid_modifier;

  static GUID_t make_guid()
  {
    // Each subsequent call should have a lexicographically-greater value than the previous
    GUID_t result = { {0x01,0x03, // Vendor ID
                       0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, next_guid_modifier},
                      { {0x01, 0x02, next_guid_modifier}, 0xFF} };

    next_guid_modifier++;

    return result;
  }

  MockParticipantData() :
    auth(),
    guid(OpenDDS::DCPS::GUID_UNKNOWN),
    guid_adjusted(OpenDDS::DCPS::GUID_UNKNOWN),
    guid_adjusted_remote(OpenDDS::DCPS::GUID_UNKNOWN),
    domain_id(),
    ex(),
    id_handle(DDS::HANDLE_NIL),
    id_handle_remote(DDS::HANDLE_NIL),
    handshake_handle(DDS::HANDLE_NIL)
  {
    guid = make_guid();
  }

  void add_property(Property_t p) {
    PropertySeq& seq = qos.property.value;
    const CORBA::ULong len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }

  void add_binary_property(BinaryProperty_t p) {
    BinaryPropertySeq& seq = qos.property.binary_value;
    const CORBA::ULong len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }
};

unsigned char MockParticipantData::next_guid_modifier = 0x01;


struct dds_DCPS_security_AuthenticationBuiltInImpl : public ::testing::Test
{

  virtual ~dds_DCPS_security_AuthenticationBuiltInImpl()
  {

  }

  dds_DCPS_security_AuthenticationBuiltInImpl() {
    init_mock_participant_1();
    init_mock_participant_2();
  }

  void init_mock_participant_1()
  {
    Property_t idca, pkey, pass, idcert, cperms;

    idca.name = "dds.sec.auth.identity_ca";
    idca.value = sec_doc_prop("certs/identity/identity_ca_cert.pem").c_str();
    idca.propagate = false;

    pkey.name = "dds.sec.auth.private_key";
    pkey.value = sec_doc_prop("certs/identity/test_participant_01_private_key.pem").c_str();
    pkey.propagate = false;

    pass.name = "dds.sec.auth.password";
    pass.value = "";
    pass.propagate = false;

    idcert.name = "dds.sec.auth.identity_certificate";
    idcert.value = sec_doc_prop("certs/identity/test_participant_01_cert.pem").c_str();
    idcert.propagate = false;

    cperms.name = "dds.sec.access.permissions";
    cperms.value = sec_doc_prop("permissions/permissions_test_participant_01_JoinDomain_signed.p7s").c_str();
    cperms.propagate = false;

    mp1.add_property(idca);
    mp1.add_property(pkey);
    mp1.add_property(pass);
    mp1.add_property(idcert);
    mp1.add_property(cperms);

    fake_topic_data_from_cert(static_cast<const char*>(idcert.value),
                              mp1.mock_participant_builtin_topic_data,
                              mp1.guid);
  }

  void init_mock_participant_2()
  {
    Property_t idca, pkey, pass, idcert, cperms;

    idca.name = "dds.sec.auth.identity_ca";
    idca.value = sec_doc_prop("certs/identity/identity_ca_cert.pem").c_str();
    idca.propagate = false;

    pkey.name = "dds.sec.auth.private_key";
    pkey.value = sec_doc_prop("certs/identity/test_participant_02_private_key.pem").c_str();
    pkey.propagate = false;

    pass.name = "dds.sec.auth.password";
    pass.value = "";
    pass.propagate = false;

    idcert.name = "dds.sec.auth.identity_certificate";
    idcert.value = sec_doc_prop("certs/identity/test_participant_02_cert.pem").c_str();
    idcert.propagate = false;

    cperms.name = "dds.sec.access.permissions";
    cperms.value = sec_doc_prop("permissions/permissions_test_participant_02_JoinDomain_signed.p7s").c_str();
    cperms.propagate = false;

    mp2.add_property(idca);
    mp2.add_property(pkey);
    mp2.add_property(pass);
    mp2.add_property(idcert);
    mp2.add_property(cperms);

    fake_topic_data_from_cert(static_cast<const char*>(idcert.value),
                              mp2.mock_participant_builtin_topic_data,
                              mp2.guid);
  }


  static std::string value_of(const std::string& key, const PropertySeq& s)
  {
    const CORBA::ULong n = s.length();
    for (CORBA::ULong i = 0; i < n; ++i) {
        if (strcmp(s[i].name, key.c_str()) == 0)
          return static_cast<const char*>(s[i].value);
    }
    return "";
  }

  static const DDS::OctetSeq& value_of(const std::string& key, const BinaryPropertySeq& s)
  {
    const CORBA::ULong n = s.length();
    for (CORBA::ULong i = 0; i < n; ++i) {
        if (strcmp(s[i].name, key.c_str()) == 0)
          return s[i].value;
    }
    return EmptySequence;
  }

  static void add_param(OpenDDS::RTPS::ParameterList& param_list, const OpenDDS::RTPS::Parameter& param) {
    const CORBA::ULong length = param_list.length();
    param_list.length(length + 1);
    param_list[length] = param;
  }

  static void fake_topic_data_from_cert(const std::string& path, DDS::OctetSeq& dst, GUID_t guid)
  {
    using namespace OpenDDS::DCPS;
    // Fill this with data directly from the cert

    OpenDDS::Security::SSL::Certificate cert(path);

    OpenDDS::Security::SSL::make_adjusted_guid(guid, guid, cert);

    ACE_Message_Block buffer(1024);
    //buffer.wr_ptr(cpdata.length());
    OpenDDS::DCPS::Serializer serializer(&buffer, Encoding::KIND_XCDR1, ENDIAN_BIG);

    OpenDDS::RTPS::ParameterList params;
    OpenDDS::RTPS::Parameter p;
    p.guid(guid);
    p._d(OpenDDS::RTPS::PID_PARTICIPANT_GUID);
    add_param(params, p);

    ASSERT_TRUE(serializer << params);

    dst.length(static_cast<CORBA::ULong>(buffer.length()));
    std::memcpy(dst.get_buffer(), buffer.rd_ptr(), dst.length());
  }

  static DDS::OctetSeq EmptySequence;

  MockParticipantData mp1;
  MockParticipantData mp2;
};

DDS::OctetSeq dds_DCPS_security_AuthenticationBuiltInImpl::EmptySequence = DDS::OctetSeq();

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, ValidateLocalIdentity_MultipleParticipants_NoClash)
{
  AuthenticationBuiltInImpl auth;
  auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);

  auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex);
  auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex);

  OpenDDS::Security::TokenReader r1(mp1.id_token),  r2(mp2.id_token);

  const char* subject1 = r1.get_property_value("dds.cert.sn");
  ASSERT_TRUE(subject1);
  const char* subject2 = r2.get_property_value("dds.cert.sn");
  ASSERT_TRUE(subject2);

  ASSERT_NE(0, std::strcmp(subject1, subject2));
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, ValidateLocalIdentity_Success)
{
  AuthenticationBuiltInImpl auth;
  IdentityHandle h;
  GUID_t adjusted;
  ValidationResult_t r = auth.validate_local_identity(h, adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, GetIdentityToken_Success)
{
  // From this cmd:  openssl x509 -noout -subject -in certs/identity/test_participant_01_cert.pem
  std::string cert_sn("CN = Ozzie Ozmann, O = Internet Widgits Pty Ltd, ST = Some-State, C = AU");

  // Same thing but with certs/identity/identity_ca_cert.pem
  std::string ca_sn("C = US, ST = MO, L = Saint Louis, O = Object Computing (Test Identity CA), CN = Object Computing (Test Identity CA), emailAddress = info@objectcomputing.com");

  AuthenticationBuiltInImpl auth;

  auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);

  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  ASSERT_EQ(cert_sn, value_of("dds.cert.sn", mp1.id_token.properties));
  ASSERT_EQ(ca_sn, value_of("dds.ca.sn", mp1.id_token.properties));

  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.cert.algo", mp1.id_token.properties));
  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.ca.algo", mp1.id_token.properties));
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, ValidateRemoteIdentity_UsingLocalAuthRequestToken_PendingHandshakeRequest)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  // Local participant
  ValidationResult_t r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Remote participant
  r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Leave the remote-auth-request-token set to TokenNil to force local-auth-request
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, ValidateRemoteIdentity_LocalAuthRequestTokenNil_PendingHandshakeMessage)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // First, generate an auth-request-token for the "remote" participant mp1. TokenNil is passed in this time
  // so it will force-generate a local token associated with mp1. The results for this don't matter that much
  // because the next call to validate_remote_identity is where the magic (in this test) happens.
  // Both sides of the handshake validate remote identity
  r = auth.validate_remote_identity(mp1.id_handle_remote,
                                    mp1.auth_request_message_token,
                                    mp1.auth_request_message_token_remote,
                                    mp1.id_handle,
                                    mp1.id_token_remote,
                                    mp1.guid_adjusted_remote,
                                    ex);

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp2.id_token_remote = mp1.id_token;
  mp2.guid_adjusted_remote = mp1.guid_adjusted;
  mp2.auth_request_message_token_remote = mp1.auth_request_message_token;

  r = auth.validate_remote_identity(mp2.id_handle_remote,
                                    mp2.auth_request_message_token,
                                    mp2.auth_request_message_token_remote,
                                    mp2.id_handle,
                                    mp2.id_token_remote,
                                    mp2.guid_adjusted_remote,
                                    ex);

  // Make sure mp2 auth-request-token is TokenNil per the spec
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // Expected since now lexicographical GUID comparison yields: local > remote (in contrast to the test above)
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, BeginHandshakeRequest_UsingLocalAuthRequestToken_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  // Local participant
  ValidationResult_t r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Remote participant
  r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Leave the remote-auth-request-token set to TokenNil to force local-auth-request
  r = auth.validate_remote_identity(mp1.id_handle_remote,
                                    mp1.auth_request_message_token,
                                    mp1.auth_request_message_token_remote,
                                    mp1.id_handle,
                                    mp1.id_token_remote,
                                    mp1.guid_adjusted_remote,
                                    ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  // Arbitrary... This isn't used in begin_handshake_request anyway!
  DDS::OctetSeq pretend_topic_data;
  pretend_topic_data.length(1);
  pretend_topic_data[0] = 'A';

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   request_token,
                                   mp1.id_handle,
                                   mp1.id_handle_remote,
                                   pretend_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  // Since many of the out-params are randomly-generated it is only
  // practical to check the lengths of the resultant parameters
  const DDS::BinaryPropertySeq& bprops = request_token.binary_properties;
  const DDS::PropertySeq& props = request_token.properties;
  ASSERT_EQ(7u, bprops.length());
  ASSERT_EQ(0u, props.length());
  ASSERT_EQ(65u, value_of("dh1", bprops).length());
  {
    std::string expected("ECDH+prime256v1-CEUM");
    const DDS::OctetSeq& prop = value_of("c.kagree_algo", bprops);
    ASSERT_EQ(0, std::memcmp(expected.c_str(),
                              prop.get_buffer(),
                              std::min(CORBA::ULong(expected.length()),
                                       prop.length())));
  }
  {
    std::string expected("RSASSA-PSS-SHA256");
    const DDS::OctetSeq& prop = value_of("c.dsign_algo", bprops);
    ASSERT_EQ(0, std::memcmp(expected.c_str(),
                              prop.get_buffer(),
                              std::min(CORBA::ULong(expected.length()),
                                       prop.length())));
  }

  ASSERT_EQ(32u, value_of("challenge1", bprops).length());

  // The operation shall check the content of the local_auth_request_token associated
  // with the remote_identity_handle. If the value is _not_ TokenNIL then "challenge1"
  // should be equal to the local_auth_request_token "future_challenge" property.
  //
  // Above the local-auth-request was forced, so the remote_identity_handle must map to
  // a non-nil local_auth_request_token. As such, "future_challenge" and "challenge1"
  // should be equivalent.
  {
    const DDS::OctetSeq& prop1 = value_of("challenge1", bprops);
    const DDS::OctetSeq& prop2 = value_of("future_challenge", mp1.auth_request_message_token.binary_properties);
    ASSERT_EQ(0, std::memcmp(prop1.get_buffer(),
                             prop2.get_buffer(),
                             std::min(prop1.length(),
                                      prop2.length())));
  }

}

struct dds_DCPS_security_AuthenticationBuiltInImpl_BeginHandshakeReplyTest : public dds_DCPS_security_AuthenticationBuiltInImpl
{
  dds_DCPS_security_AuthenticationBuiltInImpl_BeginHandshakeReplyTest()
  {

  }

};

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl_BeginHandshakeReplyTest, BeginHandshakeReply_PendingHandshakeMessage_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Both sides of the handshake validate remote identity
  r = auth.validate_remote_identity(mp1.id_handle_remote,
                                    mp1.auth_request_message_token,
                                    mp1.auth_request_message_token_remote,
                                    mp1.id_handle,
                                    mp1.id_token_remote,
                                    mp1.guid_adjusted_remote,
                                    ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   request_token,
                                   mp1.id_handle,
                                   mp1.id_handle_remote,
                                   mp1.mock_participant_builtin_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  r = auth.validate_remote_identity(mp1.id_handle,
                                    mp2.auth_request_message_token,
                                    mp1.auth_request_message_token,
                                    mp2.id_handle,
                                    mp1.id_token,
                                    mp1.guid_adjusted,
                                    ex);

  // Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token);

  r = auth.begin_handshake_reply(mp2.handshake_handle,
                                 reply_token, // Token received from mp1 after it called begin_handshake_request
                                 mp1.id_handle,
                                 mp2.id_handle,
                                 mp2.mock_participant_builtin_topic_data,
                                 ex);
  const DDS::Security::HandshakeHandle mp2_handshake_handle = mp2.handshake_handle;

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2_handshake_handle, DDS::HANDLE_NIL);

  // Suppose the initiator didn't get the reply and resends the request.

  r = auth.begin_handshake_reply(mp2.handshake_handle,
                                 reply_token, // Token received from mp1 after it called begin_handshake_request
                                 mp1.id_handle,
                                 mp2.id_handle,
                                 mp2.mock_participant_builtin_topic_data,
                                 ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_EQ(mp2_handshake_handle, mp2.handshake_handle);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, BeginHandshakeRequest_BeginHandshakeReply_ProcessHandshake_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  IdentityHandle mp1_remote_mp2;
  // Both sides of the handshake validate remote identity
  r = auth.validate_remote_identity(mp1_remote_mp2,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   request_token,
                                   mp1.id_handle,
                                   mp1_remote_mp2,
                                   mp1.mock_participant_builtin_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  IdentityHandle mp2_remote_mp1;
  r = auth.validate_remote_identity(mp2_remote_mp1,
                                    mp2.auth_request_message_token,
                                    mp1.auth_request_message_token,
                                    mp2.id_handle,
                                    mp1.id_token,
                                    mp1.guid_adjusted,
                                    ex);

  // Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token); // Request was an in-out param

  r = auth.begin_handshake_reply(mp2.handshake_handle,
                                 reply_token,
                                 mp2_remote_mp1,
                                 mp2.id_handle,
                                 mp2.mock_participant_builtin_topic_data,
                                 ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2.handshake_handle, DDS::HANDLE_NIL);

  // Process handshake on mp1
  DDS::Security::HandshakeMessageToken final_token;
  r = auth.process_handshake(final_token,
                             reply_token,
                             mp1.handshake_handle,
                             ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);

  // Process handshake on mp2
  DDS::Security::HandshakeMessageToken unused_token;
  r = auth.process_handshake(unused_token,
                             final_token,
                             mp2.handshake_handle,
                             ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, SeparateAuthImpls_BeginHandshakeRequest_BeginHandshakeReply_ProcessHandshake_Success)
{
  // The goal here is not only to test having two separate Auth impl objects, but to make sure the info available to each side is cleanly separated
  // With the obvious exception of the data that is passed in messages
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = mp2.auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp2.auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = mp1.auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp1.auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Both sides of the handshake validate remote identity
  r = mp1.auth.validate_remote_identity(mp1.id_handle_remote,
                                     mp1.auth_request_message_token,
                                     mp1.auth_request_message_token_remote,
                                     mp1.id_handle,
                                     mp1.id_token_remote,
                                     mp1.guid_adjusted_remote,
                                     ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = mp1.auth.begin_handshake_request(mp1.handshake_handle,
                                       request_token,
                                       mp1.id_handle,
                                       mp1.id_handle_remote,
                                       mp1.mock_participant_builtin_topic_data,
                                       ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp2.id_token_remote = mp1.id_token;
  mp2.guid_adjusted_remote = mp1.guid_adjusted;
  mp2.auth_request_message_token_remote = mp1.auth_request_message_token;

  r = mp2.auth.validate_remote_identity(mp2.id_handle_remote,
                                     mp2.auth_request_message_token,
                                     mp2.auth_request_message_token_remote,
                                     mp2.id_handle,
                                     mp2.id_token_remote,
                                     mp2.guid_adjusted_remote,
                                     ex);

  // Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token); // Request was an in-out param

  r = mp2.auth.begin_handshake_reply(mp2.handshake_handle,
                                     reply_token,
                                     mp2.id_handle_remote,
                                     mp2.id_handle,
                                     mp2.mock_participant_builtin_topic_data,
                                     ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2.handshake_handle, DDS::HANDLE_NIL);

  // Process handshake on mp1
  DDS::Security::HandshakeMessageToken final_token;
  r = mp1.auth.process_handshake(final_token,
                              reply_token,
                              mp1.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);

  // Process handshake on mp2
  DDS::Security::HandshakeMessageToken unused_token;
  r = mp2.auth.process_handshake(unused_token,
                              final_token,
                              mp2.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, SeparateAuthImpls_FullHandshake_NoAuthTokenTransfer_Success)
{
  // The goal here is not only to test having two separate Auth impl objects, but to make sure the info available to each side is cleanly separated
  // With the obvious exception of the data that is passed in messages
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = mp2.auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp2.auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = mp1.auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp1.auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Both sides of the handshake validate remote identity
  r = mp1.auth.validate_remote_identity(mp1.id_handle_remote,
                                     mp1.auth_request_message_token,
                                     mp1.auth_request_message_token_remote,
                                     mp1.id_handle,
                                     mp1.id_token_remote,
                                     mp1.guid_adjusted_remote,
                                     ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = mp1.auth.begin_handshake_request(mp1.handshake_handle,
                                       request_token,
                                       mp1.id_handle,
                                       mp1.id_handle_remote,
                                       mp1.mock_participant_builtin_topic_data,
                                       ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp2.id_token_remote = mp1.id_token;
  mp2.guid_adjusted_remote = mp1.guid_adjusted;

  r = mp2.auth.validate_remote_identity(mp2.id_handle_remote,
                                     mp2.auth_request_message_token,
                                     mp2.auth_request_message_token_remote,
                                     mp2.id_handle,
                                     mp2.id_token_remote,
                                     mp2.guid_adjusted_remote,
                                     ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp2.auth_request_message_token.binary_properties[0].value.length());

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token); // Request was an in-out param

  r = mp2.auth.begin_handshake_reply(mp2.handshake_handle,
                                     reply_token,
                                     mp2.id_handle_remote,
                                     mp2.id_handle,
                                     mp2.mock_participant_builtin_topic_data,
                                     ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2.handshake_handle, DDS::HANDLE_NIL);

  // Process handshake on mp1
  DDS::Security::HandshakeMessageToken final_token;
  r = mp1.auth.process_handshake(final_token,
                              reply_token,
                              mp1.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);

  // Process handshake on mp2
  DDS::Security::HandshakeMessageToken unused_token;
  r = mp2.auth.process_handshake(unused_token,
                              final_token,
                              mp2.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, GetAuthenticationPeerCredentialToken_InitiatorAndReplier_Success)
{
  // The goal here is not only to test having two separate Auth impl objects, but to make sure the info available to each side is cleanly separated
  // With the obvious exception of the data that is passed in messages
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = mp2.auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp2.auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = mp1.auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp1.auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Both sides of the handshake validate remote identity
  r = mp1.auth.validate_remote_identity(mp1.id_handle_remote,
                                     mp1.auth_request_message_token,
                                     mp1.auth_request_message_token_remote,
                                     mp1.id_handle,
                                     mp1.id_token_remote,
                                     mp1.guid_adjusted_remote,
                                     ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = mp1.auth.begin_handshake_request(mp1.handshake_handle,
                                       request_token,
                                       mp1.id_handle,
                                       mp1.id_handle_remote,
                                       mp1.mock_participant_builtin_topic_data,
                                       ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp2.id_token_remote = mp1.id_token;
  mp2.guid_adjusted_remote = mp1.guid_adjusted;
  mp2.auth_request_message_token_remote = mp1.auth_request_message_token;

  r = mp2.auth.validate_remote_identity(mp2.id_handle_remote,
                                     mp2.auth_request_message_token,
                                     mp2.auth_request_message_token_remote,
                                     mp2.id_handle,
                                     mp2.id_token_remote,
                                     mp2.guid_adjusted_remote,
                                     ex);

  // Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token); // Request was an in-out param

  r = mp2.auth.begin_handshake_reply(mp2.handshake_handle,
                                     reply_token,
                                     mp2.id_handle_remote,
                                     mp2.id_handle,
                                     mp2.mock_participant_builtin_topic_data,
                                     ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2.handshake_handle, DDS::HANDLE_NIL);

  // Process handshake on mp1
  DDS::Security::HandshakeMessageToken final_token;
  r = mp1.auth.process_handshake(final_token,
                              reply_token,
                              mp1.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);

  // Process handshake on mp2
  DDS::Security::HandshakeMessageToken unused_token;
  r = mp2.auth.process_handshake(unused_token,
                              final_token,
                              mp2.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);


  ASSERT_TRUE(mp1.auth.get_authenticated_peer_credential_token(mp1.auth_peer_credential_token,
                                                               mp1.handshake_handle, ex));

  ASSERT_EQ(Certificate(TokenReader(mp1.auth_peer_credential_token).get_bin_property_value("c.id")),
            Certificate(TokenReader(reply_token).get_bin_property_value("c.id")));

  ASSERT_EQ(SignedDocument(TokenReader(mp1.auth_peer_credential_token).get_bin_property_value("c.perm")),
            SignedDocument(TokenReader(reply_token).get_bin_property_value("c.perm")));


  ASSERT_TRUE(mp2.auth.get_authenticated_peer_credential_token(mp2.auth_peer_credential_token,
                                                               mp2.handshake_handle, ex));

  ASSERT_EQ(Certificate(TokenReader(mp2.auth_peer_credential_token).get_bin_property_value("c.id")),
            Certificate(TokenReader(request_token).get_bin_property_value("c.id")));

  ASSERT_EQ(SignedDocument(TokenReader(mp2.auth_peer_credential_token).get_bin_property_value("c.perm")),
            SignedDocument(TokenReader(request_token).get_bin_property_value("c.perm")));

}

TEST_F(dds_DCPS_security_AuthenticationBuiltInImpl, GetSharedSecret_InitiatorAndReplier_Match)
{
  // The goal here is not only to test having two separate Auth impl objects, but to make sure the info available to each side is cleanly separated
  // With the obvious exception of the data that is passed in messages
  SecurityException ex;

  // Local participant: notice how it is mp2 this time
  ValidationResult_t r = mp2.auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp2.auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  // Remote participant: mp1 this time
  r = mp1.auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, mp1.auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp1.id_token_remote = mp2.id_token;
  mp1.guid_adjusted_remote = mp2.guid_adjusted;

  // Both sides of the handshake validate remote identity
  r = mp1.auth.validate_remote_identity(mp1.id_handle_remote,
                                     mp1.auth_request_message_token,
                                     mp1.auth_request_message_token_remote,
                                     mp1.id_handle,
                                     mp1.id_token_remote,
                                     mp1.guid_adjusted_remote,
                                     ex);

  // Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  DDS::Security::HandshakeMessageToken request_token;

  // Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier
  r = mp1.auth.begin_handshake_request(mp1.handshake_handle,
                                       request_token,
                                       mp1.id_handle,
                                       mp1.id_handle_remote,
                                       mp1.mock_participant_builtin_topic_data,
                                       ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp1.handshake_handle, DDS::HANDLE_NIL);

  // Pretend like we're passing messages here ... tokens and guids are transfered OK
  mp2.id_token_remote = mp1.id_token;
  mp2.guid_adjusted_remote = mp1.guid_adjusted;
  mp2.auth_request_message_token_remote = mp1.auth_request_message_token;

  r = mp2.auth.validate_remote_identity(mp2.id_handle_remote,
                                     mp2.auth_request_message_token,
                                     mp2.auth_request_message_token_remote,
                                     mp2.id_handle,
                                     mp2.id_token_remote,
                                     mp2.guid_adjusted_remote,
                                     ex);

  // Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  // mp2 is the one performing the handhsake reply
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  DDS::Security::HandshakeMessageToken reply_token(request_token); // Request was an in-out param

  r = mp2.auth.begin_handshake_reply(mp2.handshake_handle,
                                     reply_token,
                                     mp2.id_handle_remote,
                                     mp2.id_handle,
                                     mp2.mock_participant_builtin_topic_data,
                                     ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
  ASSERT_NE(mp2.handshake_handle, DDS::HANDLE_NIL);

  // Process handshake on mp1
  DDS::Security::HandshakeMessageToken final_token;
  r = mp1.auth.process_handshake(final_token,
                              reply_token,
                              mp1.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);

  // Process handshake on mp2
  DDS::Security::HandshakeMessageToken unused_token;
  r = mp2.auth.process_handshake(unused_token,
                              final_token,
                              mp2.handshake_handle,
                              ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);

  SharedSecretHandle_var secret1 = mp1.auth.get_shared_secret(mp1.handshake_handle, ex);
  SharedSecretHandle_var secret2 = mp2.auth.get_shared_secret(mp2.handshake_handle, ex);
  ASSERT_NE((void*)0, secret1);
  ASSERT_NE((void*)0, secret2);
  DDS::OctetSeq_var secret1_data = secret1->sharedSecret();
  DDS::OctetSeq_var secret2_data = secret2->sharedSecret();
  ASSERT_NE(0u, secret1_data->length());
  ASSERT_EQ(secret1_data.in(), secret2_data.in());
}

#endif
