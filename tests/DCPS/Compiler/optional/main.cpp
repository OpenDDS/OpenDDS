#include "optionalC.h"
#include "optionalTypeSupportImpl.h"
#include "tests/Utils/DataView.h"

#include "gtest/gtest.h"
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/FilterEvaluator.h>

#include <vector>
#include <ace/ACE.h>
#include <ace/Log_Msg.h>
#include <gtest/gtest.h>
#include <tests/Utils/DataView.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

const Encoding xcdr1(Encoding::KIND_XCDR1, ENDIAN_BIG);
const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);
const Encoding xcdr2_le(Encoding::KIND_XCDR2, ENDIAN_LITTLE);

template<typename Type>
void serializer_test(const OpenDDS::DCPS::Encoding& encoding, Type value, const DataView& expected_cdr) {
  ACE_Message_Block buffer(1024);
  OpenDDS::DCPS::Serializer serializer(&buffer, encoding);

  ASSERT_TRUE(serializer << value);

  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);

 // Type output;
 // ASSERT_TRUE(serializer >> output);
}

template<typename Type>
void baseline_checks(const Encoding& encoding, Type value, const DataView& expected_cdr,
  SerializedSizeBound bound = SerializedSizeBound())
{
  EXPECT_EQ(serialized_size(encoding, value), expected_cdr.size);

  serializer_test<Type>(encoding, value, expected_cdr);
}

TEST(OptionalTests, SerializationXCDR2Empty)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x04, // +4 = 4

    // bool_field
    0x00,

    // short_field
    0x00, // +1 = 5

    // int32_field
    0x00, // +1 = 6

    // int64_field
    0x00, // +1 = 7
  };

  optional::OptionalMembers empty;
  baseline_checks(xcdr2, empty, expected);
}

TEST(OptionalTests, SerializationXCDR2NotEmpty)
{
  const uint8_t expected[] = {
    // Delimeter
    0x00, 0x00, 0x00, 0x6, // +4 = 4

    // bool_field
    0x00,

    // short_field
    0x01, // +1 is_present = 5
    //0x00, // +1 padding? = 6
    0x7f, 0xff, // +2 = 8

    // int32_field
    0x00, // +1 is_present = 9
    0x00,
  };

  optional::OptionalMembers value;
  value.short_field(0x7fff);
  //value.int32_field(0x7fffffff);
  serializer_test(xcdr2, value, expected);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
