#include "optionalC.h"
#include "optionalTypeSupportImpl.h"

#include <tests/Utils/DataView.h>

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>
#include <dds/DCPS/JsonValueWriter.h>
#include <dds/DCPS/JsonValueReader.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <gtest/gtest.h>

#include <vector>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);

template<typename Type>
void serializer_test(const OpenDDS::DCPS::Encoding& encoding, Type value, const DataView& expected_cdr) {
  ACE_Message_Block buffer(1024);
  OpenDDS::DCPS::Serializer serializer(&buffer, encoding);

  ASSERT_TRUE(serializer << value);

  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);

  Type output;
  ASSERT_TRUE(serializer >> output);
}

template<typename Type>
void baseline_checks(const Encoding& encoding, Type value, const DataView& expected_cdr)
{
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);

  serializer_test<Type>(encoding, value, expected_cdr);
}

TEST(OptionalTests, SerializationXCDR2Empty)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x07, // +4 = 4

    // bool_field
    0x00, // +1 = 5

    // short_field
    0x00, // +1 = 6

    // int32_field
    0x00, // +1 = 7

    // int64_field
    0x00, // +1 = 8

    // str_field
    0x00, // +1 = 9

    0x00,

    0x00
  };

  optional::OptionalMembers empty{};
  baseline_checks(xcdr2, empty, expected);
}

TEST(OptionalTests, SerializationXCDR2NotEmpty)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x1a, // +4 = 4

    // bool_field
    0x00,

    // short_field
    0x01, // +1 is_present = 5
    0x7f, 0xff, // +2 = 8

    // int32_field
    0x00, // +1 is_present = 9
    0x00,

    // str_field
    0x01, // +1 = 10
    0x00, // ?
    0x00, 0x00, 0x00, 0x0c, // +4 = 14
    'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '\0', // +12 = 26
    0x00,

    // struct_field
    0x00
  };

  optional::OptionalMembers value{};
  value.short_field(0x7fff);
  value.str_field("Hello World");
  serializer_test(xcdr2, value, expected);
}

// TODO WIP will expand and cleanup
TEST(OptionalTests, JsonValueWriterReader) {
  typedef rapidjson::StringBuffer Buffer;
  typedef rapidjson::Writer<Buffer> Writer;


  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);

  optional::OptionalMembers expected{};
  expected.bool_field(true);
  expected.short_field(0x1234);
  expected.str_field("Hello World");

  vwrite(jvw, expected);

  rapidjson::StringStream ss(buffer.GetString());
  JsonValueReader<> jvr(ss);

  optional::OptionalMembers got{};
  vread(jvr, got);

  EXPECT_TRUE(expected.bool_field() == got.bool_field());
  EXPECT_TRUE(expected.short_field() == got.short_field());
  EXPECT_TRUE(expected.int32_field() == got.int32_field());
  EXPECT_TRUE(expected.int64_field() == got.int64_field());
  EXPECT_TRUE(expected.str_field() == got.str_field());
  EXPECT_TRUE(expected.seq_field() == got.seq_field());
  EXPECT_TRUE(expected.struct_field().has_value() == got.struct_field().has_value());
  if (expected.struct_field().has_value()) {
    EXPECT_TRUE(expected.struct_field().value().octect_field() == got.struct_field().value().octect_field());
  }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
