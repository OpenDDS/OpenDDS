#include "dds/DCPS/security/CryptoBuiltInImpl.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "gtest/gtest.h"


using namespace OpenDDS::Security;
using namespace testing;

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
  DDS::OctetSeq inline_qos;
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatawriterCryptoHandle handle = 1;
  EXPECT_TRUE(get_inst().encode_serialized_payload(output, inline_qos, get_buffer(), handle, ex));
  EXPECT_EQ(get_buffer(), output);
}

TEST_F(CryptoTransformTest, encode_serialized_payload_WithData)
{
  DDS::OctetSeq inline_qos;
  DDS::OctetSeq output;
  DDS::Security::SecurityException ex;
  DDS::Security::DatawriterCryptoHandle handle = 1;

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
  ::DDS::OctetSeq output;
  ::DDS::Security::SecurityException ex;
  ::DDS::Security::DatawriterCryptoHandle handle = 1;

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


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
