
#include "dds/DCPS/security/CryptoBuiltInImpl.h"
#include "dds/DdsSecurityCoreC.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "gtest/gtest.h"


using namespace OpenDDS::Security;

// Test fixture to allow a repeatable scenario where multiple participants are
// registered for each test case without having to repeat the code
class CryptoKeyFactoryFixture : public ::testing::Test
{
public:

// Local participant IDs to use
  static const DDS::Security::IdentityHandle Auth_Only_Id_Handle      = 1;
  static const DDS::Security::PermissionsHandle Auth_Only_Perm_Handle = 2;
  static const DDS::Security::IdentityHandle Encrypt_Identity_Handle  = 3;
  static const DDS::Security::PermissionsHandle Encrypt_Perm_Handle   = 4;
  static const DDS::Security::IdentityHandle Remote_Id_Handle         = 5;
  static const DDS::Security::PermissionsHandle Remote_Perm_Handle    = 6;
  static const DDS::Security::SharedSecretHandle Secret_Handle        = 7;

  CryptoKeyFactoryFixture()
    : auth_only_handle_(DDS::HANDLE_NIL)
    , encryption_handle_(DDS::HANDLE_NIL)
    , remote_handle_(DDS::HANDLE_NIL)
  {
  }

  ~CryptoKeyFactoryFixture()
  {
  }

  virtual void SetUp()
  {
    DDS::PropertySeq part_props;
    DDS::Security::ParticipantSecurityAttributes sec_attributes;
    DDS::Security::SecurityException ex;

    // Register local particpant with authentiation only
    auth_only_handle_ = GetFactory().register_local_participant(
      Auth_Only_Id_Handle,
      Auth_Only_Perm_Handle,
      part_props,
      sec_attributes,
      ex);

    // Register local particpant with encryption
    encryption_handle_ = GetFactory().register_local_participant(
      Encrypt_Identity_Handle,
      Encrypt_Perm_Handle,
      part_props,
      sec_attributes,
      ex);

    ASSERT_FALSE(auth_only_handle_ == DDS::HANDLE_NIL) << "AuthOnly handle was Null";
    ASSERT_FALSE(encryption_handle_ == DDS::HANDLE_NIL) << "Encrypt handle was Null";

    // Register a remote participant with the Encryption handle
    remote_handle_ = GetFactory().register_matched_remote_participant(
      encryption_handle_,
      Remote_Id_Handle,
      Remote_Perm_Handle,
      Secret_Handle,
      ex);

    ASSERT_FALSE(remote_handle_ == DDS::HANDLE_NIL) << "Remote handle was Null";
  }

  virtual void TearDown()
  {
    // Let the destructor do the work here
  }

  DDS::Security::CryptoKeyFactory& GetFactory()
  {
    return factory_;
  }

  DDS::Security::ParticipantCryptoHandle GetAuthOnlyParticipant()
  {
    return auth_only_handle_;
  }

  DDS::Security::ParticipantCryptoHandle GetEncryptionParticipant()
  {
    return encryption_handle_;
  }

  DDS::Security::ParticipantCryptoHandle GetRemoteParticipant()
  {
    return remote_handle_;
  }
private:

  CryptoBuiltInImpl factory_;

  DDS::Security::ParticipantCryptoHandle auth_only_handle_;
  DDS::Security::ParticipantCryptoHandle encryption_handle_;
  DDS::Security::ParticipantCryptoHandle remote_handle_;
};

TEST(CryptoKeyFactoryBuiltInImplTest, NullInputHandles)
{
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;
  DDS::Security::IdentityHandle part_id_handle = 1;
  DDS::Security::PermissionsHandle part_perm = 2;
  DDS::PropertySeq part_props;
  DDS::Security::ParticipantSecurityAttributes sec_attributes;
  DDS::Security::SecurityException ex;

  // Test will Null ID handle, should return a NIL handle
  EXPECT_EQ(DDS::HANDLE_NIL, test_class->register_local_participant(
    DDS::HANDLE_NIL,
    part_perm,
    part_props,
    sec_attributes,
    ex));

  // Test will Null permissions handle, should return a NIL handle
  part_id_handle = 1;
  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_local_participant(
    part_id_handle,
    DDS::HANDLE_NIL,
    part_props,
    sec_attributes,
    ex));
}

TEST(CryptoKeyFactoryBuiltInImplTest, TestRegisterLocal)
{
  // Provide a local identity and some permissions
  DDS::Security::IdentityHandle part_id_handle = 1;
  DDS::Security::PermissionsHandle part_perm = 1;
  DDS::PropertySeq part_props;
  DDS::Security::ParticipantSecurityAttributes sec_attributes;
  DDS::Security::SecurityException ex;

  // Create an instance of the factory and register a set of
  // participants and check the return data.
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;

  // RTPS not protected, returns a valid handle
  EXPECT_FALSE(DDS::HANDLE_NIL == test_class->register_local_participant(
    part_id_handle,
    part_perm,
    part_props,
    sec_attributes,
    ex));

  // Re-register same handle?
  // Disabled for stub as the stub does not actually track what's registered
  //EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_local_participant(
  //  part_id_handle,
  //  part_perm,
  //  part_props,
  //  sec_attributes,
  //  ex));

  // RTPS Protected - returns valid handle
  sec_attributes.is_rtps_protected = true;
  ++part_id_handle;
  ++part_perm;
  EXPECT_FALSE(DDS::HANDLE_NIL == test_class->register_local_participant(
    part_id_handle,
    part_perm,
    part_props,
    sec_attributes,
    ex));
}

TEST(CryptoKeyFactoryBuiltInImplTest, RegisterRemoteParticipant)
{
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;

  // Register a single local participant to support remote participants
  ::DDS::Security::IdentityHandle local_id_handle = 1;
  ::DDS::Security::IdentityHandle local_perm_handle = 2;
  ::DDS::PropertySeq part_props;
  ::DDS::Security::ParticipantSecurityAttributes sec_attributes;
  ::DDS::Security::SecurityException ex;

  ::DDS::Security::IdentityHandle local_handle = test_class->register_local_participant(
    local_id_handle,
    local_perm_handle,
    part_props,
    sec_attributes,
    ex);

  ::DDS::Security::IdentityHandle remote_id_handle = 4;
  ::DDS::Security::PermissionsHandle remote_perm_handle = 5;
  ::DDS::Security::SharedSecretHandle secret_handle = 6;

  // Register with combinations of Null handles
  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    DDS::HANDLE_NIL,
    remote_id_handle,
    remote_perm_handle,
    secret_handle,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    DDS::HANDLE_NIL,
    remote_perm_handle,
    secret_handle,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    remote_id_handle,
    DDS::HANDLE_NIL,
    secret_handle,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    remote_id_handle,
    remote_perm_handle,
    DDS::HANDLE_NIL,
    ex));

  // Register with valid handles
  EXPECT_FALSE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    remote_id_handle,
    remote_perm_handle,
    secret_handle,
    ex));

  // Disabled because the stub does not track what is registered
  //// Re-register same handle relationship
  //EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
  //  local_handle,
  //  remote_id_handle,
  //  remote_perm_handle,
  //  secret_handle,
  //  ex));
}

TEST_F(CryptoKeyFactoryFixture, RegisterLocalDataWriterRemoteReader)
{
  ::DDS::PropertySeq datawriter_properties;
  ::DDS::Security::EndpointSecurityAttributes datawriter_security_attributes;
  ::DDS::Security::SecurityException ex;

  // Register with a Null handle
  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_local_datawriter(
    DDS::HANDLE_NIL,
    datawriter_properties,
    datawriter_security_attributes,
    ex));

  // Disabled for stub because it doesn't track what's registered
  //// Register with an unknown handle
  //EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_local_datawriter(
  //  235606,
  //  datawriter_properties,
  //  datawriter_security_attributes,
  //  ex));

  // Register with a good handle
  ::DDS::Security::DatawriterCryptoHandle local_handle = GetFactory().register_local_datawriter(
    GetEncryptionParticipant(),
    datawriter_properties,
    datawriter_security_attributes,
    ex);
  EXPECT_FALSE(DDS::HANDLE_NIL == local_handle);

  // Register a Remote datareader with some bad handles
  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    DDS::HANDLE_NIL,
    GetRemoteParticipant(),
    Secret_Handle,
    false,
    ex));

 EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    DDS::HANDLE_NIL,
    Secret_Handle,
    false,
    ex));

 EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    GetRemoteParticipant(),
    DDS::HANDLE_NIL,
    false,
    ex));

 EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    GetRemoteParticipant(),
    Secret_Handle,
    false,
    ex));
}

TEST_F(CryptoKeyFactoryFixture, RegisterDataReaderAndRemoteWriter)
{
  ::DDS::PropertySeq datareader_properties;
  ::DDS::Security::EndpointSecurityAttributes datareader_security_attributes;
  ::DDS::Security::SecurityException ex;

  // Register with a Null handle
  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_local_datareader(
    DDS::HANDLE_NIL,
    datareader_properties,
    datareader_security_attributes,
    ex));

  // Register with a good handle
  ::DDS::Security::DatareaderCryptoHandle local_handle = GetFactory().register_local_datareader(
    Auth_Only_Id_Handle,
    datareader_properties,
    datareader_security_attributes,
    ex);
  EXPECT_FALSE(DDS::HANDLE_NIL == local_handle);

  // Register a remote data writer with some bad handles
  // These should all return HandleNIL
  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    DDS::HANDLE_NIL,
    GetRemoteParticipant(),
    Secret_Handle,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    DDS::HANDLE_NIL,
    Secret_Handle,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    GetRemoteParticipant(),
    DDS::HANDLE_NIL,
    ex));

  // Register using valid handles, this will return a valid output handle
  EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    GetRemoteParticipant(),
    Secret_Handle,
    ex));
}

TEST_F(CryptoKeyFactoryFixture, UnregisterParticipants)
{
  ::DDS::Security::SecurityException ex;

  EXPECT_TRUE(GetFactory().unregister_participant(GetEncryptionParticipant(), ex));

  // These are disabled for the stub
  // Already unregistered handle
  //EXPECT_FALSE(GetFactory().unregister_participant(GetEncryptionParticipant(), ex));
  // Unknown handle
  //EXPECT_FALSE(GetFactory().unregister_participant(44, ex));
  // Known handle
}

TEST_F(CryptoKeyFactoryFixture, UnregisterDataWriter)
{
  ::DDS::PropertySeq datawriter_properties;
  ::DDS::Security::EndpointSecurityAttributes datawriter_security_attributes;
  ::DDS::Security::SecurityException ex;

  // Register a writer and then unregister it
  ::DDS::Security::DatawriterCryptoHandle local_handle = GetFactory().register_local_datawriter(
    Auth_Only_Id_Handle,
    datawriter_properties,
    datawriter_security_attributes,
    ex);
  EXPECT_FALSE(DDS::HANDLE_NIL == local_handle);
  EXPECT_TRUE(GetFactory().unregister_datawriter(local_handle, ex));

  // These are disabled for the stub
  //// Try again
  //EXPECT_FALSE(GetFactory().unregister_datawriter(local_handle, ex));
  //// Unregister a non-existant writer
  //EXPECT_FALSE(GetFactory().unregister_datawriter(4322, ex));
}

TEST_F(CryptoKeyFactoryFixture, UnRegisterDataReader)
{
  ::DDS::PropertySeq datareader_properties;
  ::DDS::Security::EndpointSecurityAttributes datareader_security_attributes;
  ::DDS::Security::SecurityException ex;

  // Register a writer and then unregister it
  ::DDS::Security::DatareaderCryptoHandle local_handle = GetFactory().register_local_datareader(
    Auth_Only_Id_Handle,
    datareader_properties,
    datareader_security_attributes,
    ex);
  EXPECT_FALSE(DDS::HANDLE_NIL == local_handle);
  EXPECT_TRUE(GetFactory().unregister_datareader(local_handle, ex));

  // These are disabled for the stub
  //// Try again
  // EXPECT_FALSE(GetFactory().unregister_datareader(local_handle, ex));
  //// Unregister a non-existant reader
  //EXPECT_FALSE(GetFactory().unregister_datareader(4322, ex));
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
