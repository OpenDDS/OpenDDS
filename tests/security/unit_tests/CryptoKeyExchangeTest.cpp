
#include "dds/DCPS/security/CryptoBuiltInImpl.h"
#include "dds/DdsDcpsInfrastructureC.h"
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

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
