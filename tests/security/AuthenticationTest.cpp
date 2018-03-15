/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DdsSecurityEntities.h"
#include "dds/DCPS/security/SSL/Certificate.h"
#include "dds/DCPS/security/SSL/Utils.h"
#include <cstring>
#include <algorithm>

#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

using OpenDDS::DCPS::GUID_t;
using OpenDDS::Security::AuthenticationBuiltInImpl;
using DDS::DomainParticipantQos;
using DDS::Property_t;
using DDS::PropertySeq;
using DDS::BinaryProperty_t;
using DDS::BinaryPropertySeq;
using DDS::Security::SecurityException;
using DDS::Security::IdentityHandle;
using DDS::Security::IdentityStatusToken;
using DDS::Security::IdentityToken;
using DDS::Security::PermissionsCredentialToken;
using DDS::Security::PermissionsToken;
using DDS::Security::AuthRequestMessageToken;
using DDS::Security::HandshakeHandle;
using DDS::Security::HandshakeMessageToken;
using DDS::Security::ValidationResult_t;
using DDS::Security::DomainId_t;


struct MockParticipantData
{
  DomainParticipantQos qos;
  GUID_t guid;
  GUID_t guid_adjusted;
  DomainId_t domain_id;
  SecurityException ex;

  IdentityHandle id_handle;
  IdentityToken id_token;
  AuthRequestMessageToken auth_request_message_token;
  HandshakeHandle handshake_handle;
  HandshakeMessageToken handshake_message_token;

  DDS::ParticipantBuiltinTopicData data;

  static unsigned char next_guid_modifier;

  static GUID_t make_guid()
  {
    /* Each subsequent call should have a lexicographically-greater value than the previous */
    GUID_t result = { {0x01,0x03, /* Vendor ID */
                       0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, next_guid_modifier},
                      { {0x01, 0x02, next_guid_modifier}, 0xFF} };

    next_guid_modifier++;

    return result;
  }

  MockParticipantData() :
    guid(OpenDDS::DCPS::GUID_UNKNOWN),
    guid_adjusted(OpenDDS::DCPS::GUID_UNKNOWN),
    domain_id(),
    ex(),
    id_handle(DDS::HANDLE_NIL),
    id_token(DDS::Security::TokenNIL),
    auth_request_message_token(DDS::Security::TokenNIL),
    handshake_handle(DDS::HANDLE_NIL),
    handshake_message_token(DDS::Security::TokenNIL)
  {
    guid = make_guid();
  }

  void add_property(Property_t p) {
    PropertySeq& seq = qos.property.value;
    size_t len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }

  void add_binary_property(BinaryProperty_t p) {
    BinaryPropertySeq& seq = qos.property.binary_value;
    size_t len = seq.length();
    seq.length(len + 1);
    seq[len] = p;
  }
};

unsigned char MockParticipantData::next_guid_modifier = 0x01;


struct AuthenticationTest : public ::testing::Test
{

  virtual ~AuthenticationTest()
  {

  }

  AuthenticationTest() {
    init_mock_participant_1();
    init_mock_participant_2();
  }

  void init_mock_participant_1()
  {
    Property_t idca, pkey, pass, idcert;

    idca.name = "dds.sec.auth.identity_ca";
    idca.value = "file:certs/opendds_identity_ca_cert.pem";
    idca.propagate = false;

    pkey.name = "dds.sec.auth.private_key";
    pkey.value = "file:certs/mock_participant_1/opendds_participant_private_key.pem";
    pkey.propagate = false;

    pass.name = "dds.sec.auth.password";
    pass.value = "";
    pass.propagate = false;

    idcert.name = "dds.sec.auth.identity_certificate";
    idcert.value = "file:certs/mock_participant_1/opendds_participant_cert.pem";
    idcert.propagate = false;

    mp1.add_property(idca);
    mp1.add_property(pkey);
    mp1.add_property(pass);
    mp1.add_property(idcert);
  }

  void init_mock_participant_2()
  {
    Property_t idca, pkey, pass, idcert;

    idca.name = "dds.sec.auth.identity_ca";
    idca.value = "file:certs/opendds_identity_ca_cert.pem";
    idca.propagate = false;

    pkey.name = "dds.sec.auth.private_key";
    pkey.value = "file:certs/mock_participant_2/opendds_participant_private_key.pem";
    pkey.propagate = false;

    pass.name = "dds.sec.auth.password";
    pass.value = "";
    pass.propagate = false;

    idcert.name = "dds.sec.auth.identity_certificate";
    idcert.value = "file:certs/mock_participant_2/opendds_participant_cert.pem";
    idcert.propagate = false;

    mp2.add_property(idca);
    mp2.add_property(pkey);
    mp2.add_property(pass);
    mp2.add_property(idcert);
  }


  static std::string value_of(const std::string& key, const PropertySeq& s)
  {
    size_t n = s.length();
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(s[i].name, key.c_str()) == 0)
          return static_cast<const char*>(s[i].value);
    }
    return "";
  }

  static const DDS::OctetSeq& value_of(const std::string& key, const BinaryPropertySeq& s)
  {
    size_t n = s.length();
    for (size_t i = 0; i < n; ++i) {
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

  static void fake_topic_data_from_cert(const std::string& path, DDS::OctetSeq& dst)
  {
    using namespace OpenDDS::DCPS;
    /* Fill this with data directly from the cert */

    OpenDDS::Security::SSL::Certificate cert(path);

    GUID_t guid = MockParticipantData::make_guid();
    OpenDDS::Security::SSL::make_adjusted_guid(guid, guid, cert);

    ACE_Message_Block buffer(1024);
    //buffer.wr_ptr(cpdata.length());
    OpenDDS::DCPS::Serializer serializer(&buffer, OpenDDS::DCPS::Serializer::SWAP_BE, OpenDDS::DCPS::Serializer::ALIGN_CDR);

    OpenDDS::RTPS::ParameterList params;
    OpenDDS::RTPS::Parameter p;
    p.guid(guid);
    p._d(OpenDDS::RTPS::PID_PARTICIPANT_GUID);
    add_param(params, p);

    ASSERT_TRUE(serializer << params);

    dst.length(buffer.length());
    buffer.copy(reinterpret_cast<char*>(dst.get_buffer()), dst.length());

  }

  static DDS::OctetSeq EmptySequence;

  MockParticipantData mp1;
  MockParticipantData mp2;
};

DDS::OctetSeq AuthenticationTest::EmptySequence = DDS::OctetSeq();

TEST_F(AuthenticationTest, ValidateLocalIdentity_Success)
{
  AuthenticationBuiltInImpl auth;
  IdentityHandle h;
  GUID_t adjusted;
  ValidationResult_t r = auth.validate_local_identity(h, adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK);
}

TEST_F(AuthenticationTest, GetIdentityToken_Success)
{
  /* From this cmd:  openssl x509 -noout -subject -in certs/opendds_participant_cert.pem */
  std::string cert_sn("C = US, ST = MO, O = Object Computing (Test Identity CA), CN = Object Computing (Test Identity CA), emailAddress = info@objectcomputing.com");

  /* Same thing but with certs/opendds_identity_ca_cert.pem */
  std::string ca_sn("C = US, ST = MO, L = Saint Louis, O = Object Computing (Test Identity CA), CN = Object Computing (Test Iden CA), emailAddress = info@objectcomputing.com");

  AuthenticationBuiltInImpl auth;

  auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);

  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  ASSERT_EQ(cert_sn, value_of("dds.cert.sn", mp1.id_token.properties));
  ASSERT_EQ(ca_sn, value_of("dds.ca.sn", mp1.id_token.properties));

  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.cert.algo", mp1.id_token.properties));
  ASSERT_EQ(std::string("RSA-2048"), value_of("dds.ca.algo", mp1.id_token.properties));
}

TEST_F(AuthenticationTest, ValidateRemoteIdentity_UsingLocalAuthRequestToken_PendingHandshakeRequest)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  /* Local participant */
  ValidationResult_t r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  /* Remote participant */
  r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  /* Leave the remote-auth-request-token set to TokenNil to force local-auth-request */
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  /* Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in */
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  /*  Given the hash of the subject names in the local/remote keys, the first 11 bytes of
   *  the adjusted GUIDs are shown below. Since byte 2 in the remote GUID is lexicographically
   *  greater than byte 2 in the local GUID we have: remote > local and hence the
   *  VALIDATION_PENDING_HANDSHAKE_REQUEST is returned. Otherwise per the spec we would get
   *  VALIDATION_PENDING_HANDSHAKE_MESSAGE.
   *
   *  (gdb) x/11bx &local
   *    0x6e9268:       0x80    0x07    0xbb    0xb3    0x1d    0x43    0xbd    0x8d
   *    0x6e9270:       0x42    0x60    0x62
   *
   *  (gdb) x/11bx &remote
   *    0x6cc8e0:       0x80    0xa0    0x5c    0xda    0xc4    0x66    0xd3    0x6c
   *    0x6cc8e8:       0x0c    0xd8    0xfc
   *
   */
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);
}

TEST_F(AuthenticationTest, ValidateRemoteIdentity_LocalAuthRequestTokenNil_PendingHandshakeMessage)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  /* Local participant: notice how it is mp2 this time */
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  /* Remote participant: mp1 this time */
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  /* First, generate an auth-request-token for the "remote" participant mp1. TokenNil is passed in this time
   * so it will force-generate a local token associated with mp1. The results for this don't matter that much
   * because the next call to validate_remote_identity is where the magic (in this test) happens. */
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  r = auth.validate_remote_identity(mp1.id_handle,
                                    mp2.auth_request_message_token,
                                    mp1.auth_request_message_token,
                                    mp2.id_handle,
                                    mp1.id_token,
                                    mp1.guid_adjusted,
                                    ex);

  /* Make sure mp2 auth-request-token is TokenNil per the spec */
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  /* Expected since now lexicographical GUID comparison yields: local > remote (in contrast to the test above) */
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);
}

TEST_F(AuthenticationTest, BeginHandshakeRequest_UsingLocalAuthRequestToken_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  /* Local participant */
  ValidationResult_t r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  /* Remote participant */
  r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  /* Leave the remote-auth-request-token set to TokenNil to force local-auth-request */
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  /* Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in */
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  /* Arbitrary... This isn't used in begin_handshake_request anyway! */
  DDS::OctetSeq pretend_topic_data;
  pretend_topic_data.length(1);
  pretend_topic_data[0] = 'A';

  /* Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier */
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   mp1.handshake_message_token,
                                   mp1.id_handle,
                                   mp2.id_handle,
                                   pretend_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);

  /* Since many of the out-params are randomly-generated it is only
   * practical to check the lengths of the resultant parameters */
  const DDS::BinaryPropertySeq& bprops = mp1.handshake_message_token.binary_properties;
  const DDS::PropertySeq& props = mp1.handshake_message_token.properties;
  ASSERT_EQ(8u, bprops.length());
  ASSERT_EQ(0u, props.length());
  ASSERT_EQ(256u, value_of("dh1", bprops).length());
  {
    std::string expected("DH+MODP-2048-256");
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

  /* The operation shall check the content of the local_auth_request_token associated
   * with the remote_identity_handle. If the value is _not_ TokenNIL then "challenge1"
   * should be equal to the local_auth_request_token "future_challenge" property.
   *
   * Above the local-auth-request was forced, so the remote_identity_handle must map to
   * a non-nil local_auth_request_token. As such, "future_challenge" and "challenge1"
   * should be equivalent. */
  {
    const DDS::OctetSeq& prop1 = value_of("challenge1", bprops);
    const DDS::OctetSeq& prop2 = value_of("future_challenge", mp1.auth_request_message_token.binary_properties);
    ASSERT_EQ(0, std::memcmp(prop1.get_buffer(),
                             prop2.get_buffer(),
                             std::min(prop1.length(),
                                      prop2.length())));
  }

}

struct BeginHandshakeReplyTest : public AuthenticationTest
{
  BeginHandshakeReplyTest()
  {

  }

};

TEST_F(BeginHandshakeReplyTest, BeginHandshakeReply_PendingHandshakeMessage_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  DDS::OctetSeq pretend_topic_data;
  fake_topic_data_from_cert("file:certs/mock_participant_1/opendds_participant_cert.pem", pretend_topic_data);

  /* Local participant: notice how it is mp2 this time */
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  /* Remote participant: mp1 this time */
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  /* Both sides of the handshake validate remote identity */
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  /* Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in */
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  /* Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier */
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   mp1.handshake_message_token,
                                   mp1.id_handle,
                                   mp2.id_handle,
                                   pretend_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);

  r = auth.validate_remote_identity(mp1.id_handle,
                                    mp2.auth_request_message_token,
                                    mp1.auth_request_message_token,
                                    mp2.id_handle,
                                    mp1.id_token,
                                    mp1.guid_adjusted,
                                    ex);

  /* Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in */
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  /* mp2 is the one performing the handhsake reply */
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  r = auth.begin_handshake_reply(mp2.handshake_handle,
                                 mp1.handshake_message_token, /* Token received from mp1 after it called begin_handhsake_request */
                                 mp1.id_handle,
                                 mp2.id_handle,
                                 pretend_topic_data,
                                 ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);
}

TEST_F(AuthenticationTest, BeginHandshakeRequest_BeginHandshakeReply_ProcessHandshake_Success)
{
  AuthenticationBuiltInImpl auth;
  SecurityException ex;

  DDS::OctetSeq pretend_topic_data;
  fake_topic_data_from_cert("file:certs/mock_participant_1/opendds_participant_cert.pem", pretend_topic_data);

  /* Local participant: notice how it is mp2 this time */
  ValidationResult_t r = auth.validate_local_identity(mp2.id_handle, mp2.guid_adjusted, mp2.domain_id, mp2.qos, mp2.guid, mp2.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp2.id_token, mp2.id_handle, mp2.ex));

  /* Remote participant: mp1 this time */
  r = auth.validate_local_identity(mp1.id_handle, mp1.guid_adjusted, mp1.domain_id, mp1.qos, mp1.guid, mp1.ex);
  ASSERT_EQ(DDS::Security::VALIDATION_OK, r);
  ASSERT_EQ(true, auth.get_identity_token(mp1.id_token, mp1.id_handle, mp1.ex));

  /* Both sides of the handshake validate remote identity */
  r = auth.validate_remote_identity(mp2.id_handle,
                                    mp1.auth_request_message_token,
                                    mp2.auth_request_message_token,
                                    mp1.id_handle,
                                    mp2.id_token,
                                    mp2.guid_adjusted,
                                    ex);

  /* Expected: Local auth request token non-nil because a nil remote-auth-request-token was passed-in */
  ASSERT_EQ(1u, mp1.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0, strcmp(mp1.auth_request_message_token.binary_properties[0].name, "future_challenge"));
  ASSERT_EQ(32u /* 256bit nonce = 32 bytes */, mp1.auth_request_message_token.binary_properties[0].value.length());

  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, r);

  /* Since mp1 received VALIDATION_PENDING_HANDSHAKE_REQUEST: mp1 = initiator, mp2 = replier */
  r = auth.begin_handshake_request(mp1.handshake_handle,
                                   mp1.handshake_message_token,
                                   mp1.id_handle,
                                   mp2.id_handle,
                                   pretend_topic_data,
                                   ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);

  r = auth.validate_remote_identity(mp1.id_handle,
                                    mp2.auth_request_message_token,
                                    mp1.auth_request_message_token,
                                    mp2.id_handle,
                                    mp1.id_token,
                                    mp1.guid_adjusted,
                                    ex);

  /* Make sure mp2 auth-request-token is TokenNil because a non-nil (received over the network) remote-auth-request-token was passed in */
  ASSERT_EQ(0u, mp2.auth_request_message_token.binary_properties.length());
  ASSERT_EQ(0u, mp2.auth_request_message_token.properties.length());
  ASSERT_EQ(0, strcmp(mp2.auth_request_message_token.class_id, ""));

  /* mp2 is the one performing the handhsake reply */
  ASSERT_EQ(DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE, r);

  r = auth.begin_handshake_reply(mp2.handshake_handle,
                                 mp1.handshake_message_token, /* Token received from mp1 after it called begin_handhsake_request */
                                 mp1.id_handle,
                                 mp2.id_handle,
                                 pretend_topic_data,
                                 ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_PENDING_HANDSHAKE_MESSAGE);

  /* Process handshake on mp1 */
  DDS::Security::HandshakeMessageToken mp1_process_handshake_token;
  r = auth.process_handshake(mp1_process_handshake_token,
                             mp2.handshake_message_token,
                             mp1.handshake_handle,
                             ex);

  ASSERT_EQ(r, DDS::Security::VALIDATION_OK_FINAL_MESSAGE);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
