/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "gtest/gtest.h"
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DdsSecurityEntities.h"
#include <cstring>

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
    auth_request_message_token(DDS::Security::TokenNIL)
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
    return NULL;
  }


  MockParticipantData mp1;
  MockParticipantData mp2;
};

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


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
