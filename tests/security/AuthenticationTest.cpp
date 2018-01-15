
#include "dds/DCPS/security/AuthenticationBuiltInImpl.h"
#include "gtest/gtest.h"


using namespace OpenDDS::Security;


static ::DDS::Security::IdentityHandle ValidateLocalParticipant(AuthenticationBuiltInImpl& test_class)
{
  // Just excercise the interface and return the handle/status
  // No GUID or QoS checking is needed at this time
  // THis also never fails in the stub, so don't bother checking
  ::DDS::Security::IdentityHandle local_handle = 0;
  ::OpenDDS::DCPS::GUID_t adjusted_participant_guid;
  ::OpenDDS::DCPS::GUID_t initial_guid;
  ::DDS::DomainParticipantQos qos;
  ::DDS::Security::SecurityException ex;

  test_class.validate_local_identity(
    local_handle,
    adjusted_participant_guid,
    1,
    qos,
    initial_guid,
    ex);

    return local_handle;
}
static void CallFunctionsWithInvalidLocalHandle(
  AuthenticationBuiltInImpl& test_class,
  DDS::Security::IdentityHandle handle)
{
  DDS::Security::SecurityException ex;

  // Can't get or set tokens without a validated handle
  DDS::Security::IdentityToken token;
  DDS::Security::IdentityStatusToken status_token;
  EXPECT_FALSE(test_class.get_identity_token(token, handle, ex));
  EXPECT_FALSE(test_class.get_identity_token(status_token, handle, ex));

  DDS::Security::PermissionsCredentialToken cred_token;
  DDS::Security::PermissionsToken perm_token;
  EXPECT_FALSE(test_class.set_permissions_credential_and_token(handle, cred_token, perm_token, ex));

  // Can't validate a remote paticipant without validating local first
  DDS::Security::IdentityHandle remote_handle_out = 0;
  DDS::Security::IdentityToken remote_token;
  DDS::Security::AuthRequestMessageToken local_auth_request_token;
  DDS::Security::AuthRequestMessageToken remote_auth_request_token;
  ::OpenDDS::DCPS::GUID_t remote_guid;
  ::DDS::Security::ValidationResult_t validate_result = 
    test_class.validate_remote_identity(
      remote_handle_out,
      local_auth_request_token,
      remote_auth_request_token,
      handle,
      remote_token,
      remote_guid,
      ex);
  EXPECT_EQ(::DDS::Security::VALIDATION_FAILED, validate_result);
}

TEST(AuthenticationTest, NoLocalIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  OpenDDS::Security::AuthenticationBuiltInImpl test_class;
  CallFunctionsWithInvalidLocalHandle(test_class, 0);
}

TEST(AuthenticationTest, WrongLocalIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  // In this test, there will be an identity registered, but the
  // wrong handle will be used
  OpenDDS::Security::AuthenticationBuiltInImpl test_class;
  ::DDS::Security::IdentityHandle local_handle = ValidateLocalParticipant(test_class);
  CallFunctionsWithInvalidLocalHandle(test_class, local_handle + 1);
}

TEST(AuthenticationTest, TestValidateRemoteIdentity)
{
  // This will just do some simple testing of calling various
  // functions of the API with an invalid local identity handle
  OpenDDS::Security::AuthenticationBuiltInImpl test_class;
  ::DDS::Security::SecurityException ex;
  ::DDS::Security::IdentityHandle handle = ValidateLocalParticipant(test_class);

  // Get tokens
  DDS::Security::IdentityToken token;
  EXPECT_TRUE(test_class.get_identity_token(token, handle, ex));
  DDS::Security::IdentityStatusToken status_token;
  EXPECT_TRUE(test_class.get_identity_token(status_token, handle, ex));

  DDS::Security::PermissionsCredentialToken cred_token;
  DDS::Security::PermissionsToken perm_token;
  EXPECT_TRUE(test_class.set_permissions_credential_and_token(handle, cred_token, perm_token, ex));

  // Can't validate a remote paticipant without validating local first
  DDS::Security::IdentityHandle remote_handle_out = 0;
  DDS::Security::IdentityToken remote_token;
  DDS::Security::AuthRequestMessageToken local_auth_request_token;
  DDS::Security::AuthRequestMessageToken remote_auth_request_token;
  ::OpenDDS::DCPS::GUID_t remote_guid;

  // The remote token needs to have a valid ID
  remote_token.class_id = "DDS:Auth:PKI-DH:1.0";

  ::DDS::Security::ValidationResult_t validate_result = 
    test_class.validate_remote_identity(
      remote_handle_out,
      local_auth_request_token,
      remote_auth_request_token,
      handle,
      remote_token,
      remote_guid,
      ex);
  EXPECT_EQ(::DDS::Security::VALIDATION_PENDING_HANDSHAKE_REQUEST, validate_result);
}



int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}