#ifdef OPENDDS_SECURITY

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/security/CryptoBuiltInImpl.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsSecurityCoreC.h"

#include "gtest/gtest.h"

using namespace OpenDDS::Security;
using namespace testing;

static const ::DDS::Security::ParticipantCryptoHandle Local_Participant = 1;
static const ::DDS::Security::ParticipantCryptoHandle Remote_Participant = 2;
static const ::DDS::Security::DatawriterCryptoHandle Local_Writer = 3;
static const ::DDS::Security::DatareaderCryptoHandle Local_Reader = 4;
static const ::DDS::Security::DatawriterCryptoHandle Remote_Writer = 5;
static const ::DDS::Security::DatareaderCryptoHandle Remote_Reader = 6;

class CryptoKeyExchangeTest : public Test
{
public:
  CryptoKeyExchangeTest()
  : test_class_()
  {
  }

  ~CryptoKeyExchangeTest()
  {
  }

  DDS::Security::CryptoKeyExchange& get_inst()
  {
    return test_class_;
  }

private:

  CryptoBuiltInImpl test_class_;
};

// In general the actual use of the internal interface isn't
// important, it is just being used to drive test cases, so
// don't report uninteresting calls


TEST_F(CryptoKeyExchangeTest, CreateLocalParticipantTokens)
{
  ::DDS::Security::CryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;

  // Test bad handles.  These should fail and not add any tokens
  EXPECT_FALSE(get_inst().create_local_participant_crypto_tokens(tokens, DDS::HANDLE_NIL, Remote_Participant, ex));
  EXPECT_EQ(0U, tokens.length());
  EXPECT_FALSE(get_inst().create_local_participant_crypto_tokens(tokens, Local_Participant, DDS::HANDLE_NIL, ex));
  EXPECT_EQ(0U, tokens.length());
}

TEST_F(CryptoKeyExchangeTest, SetRemoteParticipantTokens)
{
  // The function set_remote_participant_crypto_tokens has no required
  // behavior in the stub implementation, and so it should always succeed
  ::DDS::Security::ParticipantCryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;
  EXPECT_TRUE(get_inst().set_remote_participant_crypto_tokens(Local_Participant, Remote_Participant, tokens, ex));
}

TEST_F(CryptoKeyExchangeTest, CreateLocalDatawriterTokens)
{
  ::DDS::Security::CryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;

  // Test bad handles.  These should fail and not add any tokens
  EXPECT_FALSE(get_inst().create_local_datawriter_crypto_tokens(tokens, DDS::HANDLE_NIL, Remote_Reader, ex));
  EXPECT_EQ(0U, tokens.length());
  EXPECT_FALSE(get_inst().create_local_datawriter_crypto_tokens(tokens, Local_Writer, DDS::HANDLE_NIL, ex));
  EXPECT_EQ(0U, tokens.length());

  // Test with good handles and verify the output token.  The value
  // field of the token's property isn't checked in this test because
  // the stub doesn't do anything to it
  EXPECT_TRUE(get_inst().create_local_datawriter_crypto_tokens(tokens, Local_Writer, Remote_Reader, ex));
}

TEST_F(CryptoKeyExchangeTest, SetRemoteDataWriterTokens)
{
  // The function set_remote_participant_crypto_tokens has no required
  // behavior in the stub implementation, and so it should always succeed
  ::DDS::Security::ParticipantCryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;
  EXPECT_TRUE(get_inst().set_remote_datawriter_crypto_tokens(Local_Reader, Remote_Writer, tokens, ex));
}

TEST_F(CryptoKeyExchangeTest, CreateLocalDataReaderTokens)
{
  ::DDS::Security::CryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;

  // Test bad handles.  These should fail and not add any tokens
  EXPECT_FALSE(get_inst().create_local_datareader_crypto_tokens(tokens, DDS::HANDLE_NIL, Remote_Writer, ex));
  EXPECT_EQ(0U, tokens.length());
  EXPECT_FALSE(get_inst().create_local_datareader_crypto_tokens(tokens, Local_Reader, DDS::HANDLE_NIL, ex));
  EXPECT_EQ(0U, tokens.length());

  // Test with good handles and verify the output token.  The value
  // field of the token's property isn't checked in this test because
  // the stub doesn't do anything to it
  EXPECT_TRUE(get_inst().create_local_datareader_crypto_tokens(tokens, Local_Reader, Remote_Writer, ex));
}

TEST_F(CryptoKeyExchangeTest, SetRemoteDataReaderTokens)
{
  // The function set_remote_participant_crypto_tokens has no required
  // behavior in the stub implementation, and so it should always succeed
  ::DDS::Security::ParticipantCryptoTokenSeq tokens;
  ::DDS::Security::SecurityException ex;
  EXPECT_TRUE(get_inst().set_remote_datareader_crypto_tokens(Local_Writer, Remote_Reader, tokens, ex));
}

struct FakeSharedSecret
  : OpenDDS::DCPS::LocalObject<DDS::Security::SharedSecretHandle> {

  DDS::OctetSeq* challenge1() { return new DDS::OctetSeq; }
  DDS::OctetSeq* challenge2() { return new DDS::OctetSeq; }
  DDS::OctetSeq* sharedSecret() { return new DDS::OctetSeq; }
};

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

  CryptoKeyFactoryFixture()
    : auth_only_handle_(DDS::HANDLE_NIL)
    , encryption_handle_(DDS::HANDLE_NIL)
    , remote_handle_(DDS::HANDLE_NIL)
    , secret_handle_(new FakeSharedSecret)
  {
  }

  ~CryptoKeyFactoryFixture()
  {
  }

  virtual void SetUp()
  {
    DDS::PropertySeq part_props;
    DDS::Security::ParticipantSecurityAttributes sec_attributes =
      {
        false, false, true, false, false,
        0, part_props
      };
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
      secret_handle_,
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

protected:
  DDS::Security::SharedSecretHandle_var secret_handle_;
};

TEST(CryptoKeyFactoryBuiltInImplTest, NullInputHandles)
{
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;
  DDS::Security::IdentityHandle part_id_handle = 1;
  DDS::Security::PermissionsHandle part_perm = 2;
  DDS::PropertySeq part_props;
  DDS::Security::ParticipantSecurityAttributes sec_attributes =
    {
      false, false, false, false, false,
      0, part_props
    };
  DDS::Security::SecurityException ex;

  // Test will Null ID handle, should return a NIL handle
  EXPECT_EQ(DDS::HANDLE_NIL, test_class->register_local_participant(
    DDS::HANDLE_NIL,
    part_perm,
    part_props,
    sec_attributes,
    ex));

  // Test will Null permissions handle, should return a NIL handle
  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_local_participant(
    part_id_handle,
    DDS::HANDLE_NIL,
    part_props,
    sec_attributes,
    ex));
}

TEST_F(CryptoKeyFactoryFixture, TestRegisterLocal)
{
  // Provide a local identity and some permissions
  DDS::Security::IdentityHandle part_id_handle = 1;
  DDS::Security::PermissionsHandle part_perm = 1;
  DDS::PropertySeq part_props;
  DDS::Security::ParticipantSecurityAttributes sec_attributes =
    {
      false, false, false, false, false,
      0, part_props
    };
  DDS::Security::SecurityException ex;

  // Create an instance of the factory and register a set of
  // participants and check the return data.
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;

  // RTPS not protected, returns a nil handle
  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_local_participant(
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

TEST_F(CryptoKeyFactoryFixture, RegisterRemoteParticipant)
{
  DDS::Security::CryptoKeyFactory_var test_class = new CryptoBuiltInImpl;

  // Register a single local participant to support remote participants
  ::DDS::Security::IdentityHandle local_id_handle = 1;
  ::DDS::Security::IdentityHandle local_perm_handle = 2;
  ::DDS::PropertySeq part_props;
  ::DDS::Security::ParticipantSecurityAttributes sec_attributes =
      {
        false, false, false, false, false,
        0, part_props
      };
  ::DDS::Security::SecurityException ex;

  ::DDS::Security::IdentityHandle local_handle = test_class->register_local_participant(
    local_id_handle,
    local_perm_handle,
    part_props,
    sec_attributes,
    ex);

  ::DDS::Security::IdentityHandle remote_id_handle = 4;
  ::DDS::Security::PermissionsHandle remote_perm_handle = 5;

  // Register with combinations of Null handles
  EXPECT_FALSE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    DDS::HANDLE_NIL,
    remote_id_handle,
    remote_perm_handle,
    secret_handle_,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    DDS::HANDLE_NIL,
    remote_perm_handle,
    secret_handle_,
    ex));

  // Not currently checking permissions handle since spec contradicts itself
  // regarding how to treat the return value of validate_remote_permissions():
  // Table 27 'is_access_protected' says it's fine for the handle to be NIL
  // 8.5.1.7.2 says that the handle passed to Crypto Key Factory may not be NIL
  // EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
  //   local_handle,
  //   remote_id_handle,
  //   DDS::HANDLE_NIL,
  //   secret_handle_,
  //   ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    remote_id_handle,
    remote_perm_handle,
    0,
    ex));

  // Register with valid handles
  EXPECT_FALSE(DDS::HANDLE_NIL == test_class->register_matched_remote_participant(
    local_handle,
    remote_id_handle,
    remote_perm_handle,
    secret_handle_,
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
  ::DDS::Security::EndpointSecurityAttributes datawriter_security_attributes =
      {
        {false, false, false, false},
        false, false, false, 0, datawriter_properties
      };
  ::DDS::Security::SecurityException ex;

  // Register with a Null handle
  EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_local_datawriter(
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
    secret_handle_,
    false,
    ex));

 EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    DDS::HANDLE_NIL,
    secret_handle_,
    false,
    ex));

 EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    GetRemoteParticipant(),
    0,
    false,
    ex));

 EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datareader(
    local_handle,
    GetRemoteParticipant(),
    secret_handle_,
    false,
    ex));
}

TEST_F(CryptoKeyFactoryFixture, RegisterDataReaderAndRemoteWriter)
{
  ::DDS::PropertySeq datareader_properties;
  ::DDS::Security::EndpointSecurityAttributes datareader_security_attributes =
      {
        {false, false, false, false},
        false, false, false, 0, datareader_properties
      };
  ::DDS::Security::SecurityException ex;

  // Register with a Null handle
  EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_local_datareader(
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
    secret_handle_,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    DDS::HANDLE_NIL,
    secret_handle_,
    ex));

  EXPECT_TRUE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    GetRemoteParticipant(),
    0,
    ex));

  // Register using valid handles, this will return a valid output handle
  EXPECT_FALSE(DDS::HANDLE_NIL == GetFactory().register_matched_remote_datawriter(
    local_handle,
    GetRemoteParticipant(),
    secret_handle_,
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
  ::DDS::Security::EndpointSecurityAttributes datawriter_security_attributes =
      {
        {false, false, false, false},
        false, false, false, 0, datawriter_properties
      };
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
  ::DDS::Security::EndpointSecurityAttributes datareader_security_attributes =
      {
        {false, false, false, false},
        false, false, false, 0, datareader_properties
      };
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

class CryptoTransformTest : public Test
{
public:
  CryptoTransformTest()
    : test_class_()
    , test_buffer_()
    , readers_()
    , writers_()
    , participants_()
  {
  }

  ~CryptoTransformTest()
  {
  }

  void init_buffer(CORBA::ULong length, CORBA::Octet value)
  {
    // Fill up a buffer with some data
    test_buffer_.length(length);
    for (CORBA::ULong index = 0; index < length; ++index) {
      CORBA::Octet new_value = static_cast<CORBA::Octet>(value * (index + 1));
      test_buffer_[index] = value;
      value = new_value;
    }
  }

  void init_readers(CORBA::ULong length)
  {
    init_seq(readers_, length);
  }

  void init_writers(CORBA::ULong length)
  {
    init_seq(writers_, length);
  }

  void init_participants(CORBA::ULong length)
  {
    init_seq(participants_, length);
  }

  DDS::Security::CryptoTransform& get_inst()
  {
    return test_class_;
  }

  DDS::OctetSeq& get_buffer()
  {
    return test_buffer_;
  }

  DDS::Security::DatareaderCryptoHandleSeq& get_readers()
  {
    return readers_;
  }

  DDS::Security::DatawriterCryptoHandleSeq& get_writers()
  {
    return writers_;
  }

  DDS::Security::ParticipantCryptoHandleSeq& get_participants()
  {
    return participants_;
  }

protected:

  template<class T>
  void init_seq(T& seq, CORBA::ULong length)
  {
    seq.length(length);
    for (CORBA::ULong ndx = 0; ndx < length; ++ndx) {
      seq[ndx] = ndx + 1;
    }
  }

  CryptoBuiltInImpl test_class_;
  DDS::OctetSeq test_buffer_;
  DDS::Security::DatareaderCryptoHandleSeq readers_;
  DDS::Security::DatawriterCryptoHandleSeq writers_;
  DDS::Security::ParticipantCryptoHandleSeq participants_;

  struct SharedSecret : DDS::Security::SharedSecretHandle {
    DDS::OctetSeq* challenge1() { return 0; }
    DDS::OctetSeq* challenge2() { return 0; }
    DDS::OctetSeq* sharedSecret() { return 0; }
  } shared_secret_;
};


TEST_F(CryptoTransformTest, encode_serialized_payload_NullHandle)
{
  DDS::OctetSeq inline_qos;
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatawriterCryptoHandle handle = DDS::HANDLE_NIL;
  EXPECT_FALSE(get_inst().encode_serialized_payload(output, inline_qos, get_buffer(), handle, ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_serialized_payload_EmptyBuffer)
{
  using namespace DDS::Security;
  CryptoKeyFactory& kef = dynamic_cast<CryptoKeyFactory&>(get_inst());

  DDS::OctetSeq inline_qos;
  DDS::OctetSeq output;
  DDS::PropertySeq no_properties;
  EndpointSecurityAttributes esa = {{false, false, false, false}, false, false, false, 0, no_properties};
  SecurityException ex;
  const DatawriterCryptoHandle handle = kef.register_local_datawriter(0, no_properties, esa, ex);

  EXPECT_TRUE(get_inst().encode_serialized_payload(output, inline_qos, get_buffer(), handle, ex));
  EXPECT_EQ(get_buffer(), output);
}

TEST_F(CryptoTransformTest, encode_serialized_payload_WithData)
{
  using namespace DDS::Security;
  CryptoKeyFactory& kef = dynamic_cast<CryptoKeyFactory&>(get_inst());

  DDS::OctetSeq inline_qos;
  DDS::OctetSeq output;
  DDS::PropertySeq no_properties;
  EndpointSecurityAttributes esa = {{false, false, false, false}, false, false, false, 0, no_properties};
  SecurityException ex;
  const DatawriterCryptoHandle handle = kef.register_local_datawriter(0, no_properties, esa, ex);

  // Create a buffer of non-zero size with non-zero data
  init_buffer(50U, 3);

  EXPECT_TRUE(get_inst().encode_serialized_payload(output, inline_qos, get_buffer(), handle, ex));
  EXPECT_EQ(get_buffer(), output);
}

TEST_F(CryptoTransformTest, encode_datawriter_submessage_NullSendingHandle)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatawriterCryptoHandle handle = DDS::HANDLE_NIL;
  CORBA::Long index = 0;

  // Provide a valid list of reader handles
  init_readers(1);

  EXPECT_FALSE(get_inst().encode_datawriter_submessage(
    output, get_buffer(), handle, get_readers(), index, ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_datawriter_submessage_NegativeIndex)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatawriterCryptoHandle handle = 1;
  CORBA::Long index = -1;

  // Provide a valid list of reader handles
  init_readers(3);

  EXPECT_FALSE(get_inst().encode_datawriter_submessage(output, get_buffer(), handle, get_readers(), index, ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_datawriter_submessage_NilReaderAtN)
{
  ::DDS::OctetSeq output;
  ::DDS::Security::SecurityException ex;
  ::DDS::Security::DatawriterCryptoHandle handle = 1;

  // Provide a valid list of reader handles
  const ::CORBA::Long NUM_READERS = 10;
  init_readers(NUM_READERS);

  // Test each position to see if a NIL reader is handled properly
  DDS::Security::DatareaderCryptoHandleSeq& readers = get_readers();
  for (::CORBA::Long n = 0; n < NUM_READERS; ++n) {
    // Temporarily replace the value at N with a NIL handle
    DDS::Security::DatareaderCryptoHandle temp = readers[n];
    readers[n] = DDS::HANDLE_NIL;
    ::CORBA::Long index = n;

    EXPECT_FALSE(get_inst().encode_datawriter_submessage(
      output, get_buffer(), handle, readers, index, ex));
    EXPECT_EQ(0U, output.length());
    EXPECT_EQ(n, index);

    // Restore the handle before next iteration
    readers[n] = temp;
  }
}

TEST_F(CryptoTransformTest, encode_datawriter_submessage_MultiPass)
{
  using namespace DDS::Security;
  CryptoKeyFactory& kef = dynamic_cast<CryptoKeyFactory&>(get_inst());

  DDS::OctetSeq output;
  DDS::PropertySeq no_properties;
  EndpointSecurityAttributes esa = {{false, false, false, false}, false, false, false, 0, no_properties};
  SecurityException ex;
  const DatawriterCryptoHandle handle = kef.register_local_datawriter(0, no_properties, esa, ex);

  // Provide a valid list of reader handles
  const ::CORBA::Long NUM_READERS = 10;
  init_readers(NUM_READERS);

  // Create a non-zero buffer to encode
  init_buffer(48, 7);

  // Test each position
  for (::CORBA::Long n = 0; n <= NUM_READERS; ++n) {
    ::CORBA::Long index = n;

    if (index < NUM_READERS) {
      EXPECT_TRUE(get_inst().encode_datawriter_submessage(
        output, get_buffer(), handle, get_readers(), index, ex));
      EXPECT_EQ(get_buffer(), output);
      EXPECT_LT(n, index);
    } else {
      // Out of bounds test
      EXPECT_FALSE(get_inst().encode_datawriter_submessage(
        output, get_buffer(), handle, get_readers(), index, ex));
      EXPECT_EQ(n, index);
    }
  }
}

TEST_F(CryptoTransformTest, encode_datareader_submessage_NullHandle)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatareaderCryptoHandle handle = DDS::HANDLE_NIL;

  // Provide a valid list of reader handles
  init_writers(1);

  EXPECT_FALSE(get_inst().encode_datareader_submessage(
    output, get_buffer(), handle, get_writers(), ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_datareader_submessage_EmptyBuffer)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatareaderCryptoHandle handle = 1;

  // Provide a valid list of reader handles
  init_writers(1);

  EXPECT_TRUE(get_inst().encode_datareader_submessage(
    output, get_buffer(), handle, get_writers(), ex));
  EXPECT_EQ(get_buffer(), output);
}

TEST_F(CryptoTransformTest, encode_datareader_submessage_GoodBuffer)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatareaderCryptoHandle handle = 1;

  // Provide a valid list of reader handles
  init_writers(5);

  // Use a populated buffer for this test
  init_buffer(256, 127);

  EXPECT_TRUE(get_inst().encode_datareader_submessage(
    output, get_buffer(), handle, get_writers(), ex));
  EXPECT_EQ(get_buffer(), output);
}

TEST_F(CryptoTransformTest, encode_rtps_message_NullSendingHandle)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::ParticipantCryptoHandle handle = DDS::HANDLE_NIL;
  CORBA::Long index = 0;

  // Provide a valid list of participant handles
  init_participants(1);

  EXPECT_FALSE(get_inst().encode_rtps_message(
    output, get_buffer(), handle, get_participants(), index, ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_rtps_message_NoParticipants)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::ParticipantCryptoHandle handle = 1;
  CORBA::Long index = 0;

  EXPECT_FALSE(get_inst().encode_rtps_message(
    output, get_buffer(), handle, get_participants(), index, ex));
  EXPECT_EQ(0U, output.length());
}

TEST_F(CryptoTransformTest, encode_rtps_message_NegativeIndex)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::ParticipantCryptoHandle handle = 1;
  CORBA::Long index = -1;

  // Provide a valid list of participant handles
  init_participants(1);

  EXPECT_FALSE(get_inst().encode_rtps_message(
    output, get_buffer(), handle, get_participants(), index, ex));
  EXPECT_EQ(0U, output.length());
}

// Multiple receiver handles is not currently supported in RTPS Message encoding
//
//TEST_F(CryptoTransformTest, encode_rtps_message_NilReaderAtN)
//{
//  ::DDS::OctetSeq output;
//  ::DDS::Security::SecurityException ex;
//  ::DDS::Security::ParticipantCryptoHandle handle = 1;
//
//  // Provide a valid list of participant handles
//  const ::CORBA::Long NUM_PARTS = 10;
//  init_participants(NUM_PARTS);
//
//  // Test each position to see if a NIL reader is handled properly
//  DDS::Security::ParticipantCryptoHandleSeq& participants = get_participants();
//  for (::CORBA::Long n = 0; n < NUM_PARTS; ++n) {
//    // Temporarily replace the value at N with a NIL handle
//    DDS::Security::ParticipantCryptoHandle temp = participants[n];
//    participants[n] = DDS::HANDLE_NIL;
//    ::CORBA::Long index = n;
//
//    EXPECT_FALSE(get_inst().encode_rtps_message(
//      output, get_buffer(), handle, participants, index, ex));
//    EXPECT_EQ(0U, output.length());
//    EXPECT_EQ(n, index);
//
//    // Restore the handle before next iteration
//    participants[n] = temp;
//  }
//}
//
//TEST_F(CryptoTransformTest, encode_rtps_message_MultiPass)
//{
//  ::DDS::OctetSeq output;
//  ::DDS::Security::SecurityException ex;
//  ::DDS::Security::ParticipantCryptoHandle handle = 1;
//
//  // Provide a valid list of participant handles
//  const ::CORBA::Long NUM_PARTS = 10;
//  init_participants(NUM_PARTS);
//
//  // Test each position to see if a NIL reader is handled properly
//  for (::CORBA::Long n = 0; n < NUM_PARTS; ++n) {
//    ::CORBA::Long index = n;
//    if (index < NUM_PARTS) {
//      EXPECT_TRUE(get_inst().encode_rtps_message(
//        output, get_buffer(), handle, get_participants(), index, ex));
//      EXPECT_EQ(get_buffer(), output);
//      EXPECT_EQ((n + 1), index);
//    } else {
//      // Out of bounds test
//      EXPECT_FALSE(get_inst().encode_rtps_message(
//        output, get_buffer(), handle, get_participants(), index, ex));
//      EXPECT_EQ(n, index);
//    }
//  }
//}

TEST_F(CryptoTransformTest, decode_rtps_message_NilHandles)
{
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;

  EXPECT_FALSE(get_inst().decode_rtps_message(
    output, get_buffer(), DDS::HANDLE_NIL, 1, ex));
  EXPECT_FALSE(get_inst().decode_rtps_message(
    output, get_buffer(), 1, DDS::HANDLE_NIL, ex));
  EXPECT_FALSE(get_inst().decode_rtps_message(
    output, get_buffer(), DDS::HANDLE_NIL, DDS::HANDLE_NIL, ex));
}

TEST_F(CryptoTransformTest, preprocess_secure_submsg_NilHandles)
{
  ::DDS::Security::DatawriterCryptoHandle datawriter_crypto = DDS::HANDLE_NIL;
  ::DDS::Security::DatawriterCryptoHandle datareader_crypto = DDS::HANDLE_NIL;
  ::DDS::Security::SecureSubmessageCategory_t submsgcat = DDS::Security::INFO_SUBMESSAGE;
  ::DDS::Security::SecurityException ex;

  // Good recv handle, bad send handle
  EXPECT_FALSE(get_inst().preprocess_secure_submsg(
    datawriter_crypto, datareader_crypto, submsgcat, get_buffer(), 1, DDS::HANDLE_NIL, ex));

  // Good send handle, bad recv handle
  EXPECT_FALSE(get_inst().preprocess_secure_submsg(
    datawriter_crypto, datareader_crypto, submsgcat, get_buffer(), DDS::HANDLE_NIL, 1, ex));

  // Both bad handles
  EXPECT_FALSE(get_inst().preprocess_secure_submsg(
    datawriter_crypto, datareader_crypto, submsgcat, get_buffer(), DDS::HANDLE_NIL, DDS::HANDLE_NIL, ex));
}

//TEST_F(CryptoTransformTest, preprocess_secure_submsg_Success)
//{
//  ::DDS::Security::DatawriterCryptoHandle datawriter_crypto = DDS::HANDLE_NIL;
//  ::DDS::Security::DatawriterCryptoHandle datareader_crypto = DDS::HANDLE_NIL;
//  ::DDS::Security::SecureSubmessageCategory_t submsgcat = DDS::Security::INFO_SUBMESSAGE;
//  ::DDS::Security::SecurityException ex;
//  ::DDS::Security::ParticipantCryptoHandle send_handle = 2;
//  ::DDS::Security::ParticipantCryptoHandle recv_handle = 1;
//
//  // The stub is hard-coded, functionality has not been implemented yet
//  EXPECT_TRUE(get_inst().preprocess_secure_submsg(
//    datawriter_crypto, datareader_crypto, submsgcat, get_buffer(), recv_handle, send_handle, ex));
//}

TEST_F(CryptoTransformTest, decode_datawriter_submessage_NilHandles)
{
  ::DDS::OctetSeq output;
  ::DDS::Security::SecurityException ex;

  // Good recv handle, bad send handle
  EXPECT_FALSE(get_inst().decode_datawriter_submessage(
    output, get_buffer(), 1, DDS::HANDLE_NIL, ex));

  //// Good send handle, bad recv handle
  //EXPECT_FALSE(get_inst().decode_datawriter_submessage(
  //  output, get_buffer(), DDS::HANDLE_NIL, 1, ex));

  // Both bad handles
  EXPECT_FALSE(get_inst().decode_datawriter_submessage(
    output, get_buffer(), DDS::HANDLE_NIL, DDS::HANDLE_NIL, ex));
}

//TEST_F(CryptoTransformTest, decode_datawriter_submessage_Success)
//{
//  ::DDS::OctetSeq output;
//  ::DDS::Security::SecurityException ex;
//
//  init_buffer(19, 2);
//
//  // Good recv handle, bad send handle
//  EXPECT_TRUE(get_inst().decode_datawriter_submessage(
//    output, get_buffer(), 1, 2, ex));
//  EXPECT_EQ(output, get_buffer());
//}

TEST_F(CryptoTransformTest, decode_datareader_submessage_NilHandles)
{
  ::DDS::OctetSeq output;
  ::DDS::Security::SecurityException ex;

  // Good recv handle, bad send handle
  EXPECT_FALSE(get_inst().decode_datawriter_submessage(
    output, get_buffer(), 1, DDS::HANDLE_NIL, ex));

  // Good send handle, bad recv handle
  //EXPECT_FALSE(get_inst().decode_datawriter_submessage(
  //  output, get_buffer(), DDS::HANDLE_NIL, 1, ex));

  // Both bad handles
  EXPECT_FALSE(get_inst().decode_datareader_submessage(
    output, get_buffer(), DDS::HANDLE_NIL, DDS::HANDLE_NIL, ex));
}

TEST_F(CryptoTransformTest, decode_datareader_submessage_Success)
{
  ::DDS::OctetSeq output;
  ::DDS::Security::SecurityException ex;

  init_buffer(19, 2);

  // EXPECT_TRUE(get_inst().decode_datareader_submessage(
  //   output, get_buffer(), 1, 2, ex));
  // EXPECT_EQ(output, get_buffer());
}

TEST_F(CryptoTransformTest, decode_serialized_payload_NilHandles)
{
  ::DDS::OctetSeq output;
  ::DDS::OctetSeq inline_qos;
  ::DDS::Security::SecurityException ex;

  // Good recv handle, bad send handle
  EXPECT_FALSE(get_inst().decode_serialized_payload(
    output, get_buffer(), inline_qos, 1, DDS::HANDLE_NIL, ex));

  // Good send handle, bad recv handle
  // Current implementation allows this, see note in CryptoBuiltInImpl.h
//  EXPECT_FALSE(get_inst().decode_serialized_payload(
//    output, get_buffer(), inline_qos, DDS::HANDLE_NIL, 1, ex));

  // Both bad handles
  EXPECT_FALSE(get_inst().decode_serialized_payload(
    output, get_buffer(), inline_qos, DDS::HANDLE_NIL, DDS::HANDLE_NIL, ex));
}

TEST_F(CryptoTransformTest, decode_serialized_payload_Success)
{
  using namespace DDS::Security;
  CryptoKeyFactory& kef = dynamic_cast<CryptoKeyFactory&>(get_inst());
  CryptoKeyExchange& kex = dynamic_cast<CryptoKeyExchange&>(get_inst());

  DDS::PropertySeq no_properties;
  EndpointSecurityAttributes esa = {{false, false, false, false}, true, false, false, 0, no_properties};
  SecurityException ex;
  const DatareaderCryptoHandle drch = kef.register_local_datareader(0, no_properties, esa, ex);
  const ParticipantCryptoHandle rpch = kef.register_matched_remote_participant(0, 1, 2, &shared_secret_, ex);
  const DatawriterCryptoHandle dwch = kef.register_matched_remote_datawriter(drch, rpch, &shared_secret_, ex);

  const DatawriterCryptoHandle peer_dwch = kef.register_local_datawriter(0, no_properties, esa, ex);
  DatawriterCryptoTokenSeq dwct;
  kex.create_local_datawriter_crypto_tokens(dwct, peer_dwch, 99, ex);
  kex.set_remote_datawriter_crypto_tokens(drch, dwch, dwct, ex);

  init_buffer(294, 17);
  DDS::OctetSeq output;
  DDS::OctetSeq inline_qos;
  EXPECT_TRUE(get_inst().decode_serialized_payload(output, get_buffer(), inline_qos, drch, dwch, ex));
  EXPECT_EQ(get_buffer(), output);
}

#endif
