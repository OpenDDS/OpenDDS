/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/PrinterValueWriter.h>

#include <gtest/gtest.h>

#ifndef OPENDDS_SAFETY_PROFILE

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_PrinterValueWriter, begin_struct)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, end_struct)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.end_struct());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, begin_struct_member)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("aField")));
  EXPECT_STREQ(pvw.str().c_str(), "    aField: ");
}

TEST(dds_DCPS_PrinterValueWriter, end_struct_member)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("aField")));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_STREQ(pvw.str().c_str(), "    aField: 5");
}

TEST(dds_DCPS_PrinterValueWriter, begin_union)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, end_union)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_TRUE(pvw.end_union());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, begin_discriminator)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_TRUE(pvw.begin_discriminator(MemberParam()));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_STREQ(pvw.str().c_str(), "    $discriminator: 5");
}

TEST(dds_DCPS_PrinterValueWriter, end_discriminator)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_TRUE(pvw.begin_discriminator(MemberParam()));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_discriminator());
  EXPECT_STREQ(pvw.str().c_str(), "    $discriminator: 5");
}

TEST(dds_DCPS_PrinterValueWriter, begin_union_member)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_TRUE(pvw.begin_union_member(MemberParam("aField")));
  EXPECT_STREQ(pvw.str().c_str(), "    aField: ");
}

TEST(dds_DCPS_PrinterValueWriter, end_union_member)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_union());
  EXPECT_TRUE(pvw.begin_union_member(MemberParam("aField")));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_union_member());
  EXPECT_STREQ(pvw.str().c_str(), "    aField: 5");
}

TEST(dds_DCPS_PrinterValueWriter, complete_struct)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("aField")));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("bField")));
  EXPECT_TRUE(pvw.write_int16(6));
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_TRUE(pvw.end_struct());
  EXPECT_STREQ(pvw.str().c_str(), "    aField: 5\n    bField: 6");
}

TEST(dds_DCPS_PrinterValueWriter, begin_array)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_array());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, end_array)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_array());
  EXPECT_TRUE(pvw.end_array());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, begin_sequence)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, end_sequence)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_TRUE(pvw.end_sequence());
  EXPECT_STREQ(pvw.str().c_str(), "");
}

TEST(dds_DCPS_PrinterValueWriter, begin_element)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_TRUE(pvw.begin_element(0));
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: ");
}

TEST(dds_DCPS_PrinterValueWriter, end_element)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_TRUE(pvw.begin_element(0));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: 5");
}

TEST(dds_DCPS_PrinterValueWriter, complete_sequence)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_TRUE(pvw.begin_element(0));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.begin_element(1));
  EXPECT_TRUE(pvw.write_int16(6));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.end_sequence());
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: 5\n    [1]: 6");
}

TEST(dds_DCPS_PrinterValueWriter, complete_sequence_write_array)
{
  const ACE_CDR::Short i[]= {5, 6};
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_sequence());
  EXPECT_TRUE(pvw.write_int16_array(&i[0], 2));
  EXPECT_TRUE(pvw.end_sequence());
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: 5\n    [1]: 6");
}

TEST(dds_DCPS_PrinterValueWriter, complete_array)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_array());
  EXPECT_TRUE(pvw.begin_element(0));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.begin_element(1));
  EXPECT_TRUE(pvw.write_int16(6));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.end_array());
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: 5\n    [1]: 6");
}

TEST(dds_DCPS_PrinterValueWriter, complete_array_write_array)
{
  ACE_CDR::Short const i[2] = {5, 6};
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_array());
  EXPECT_TRUE(pvw.write_int16_array(&i[0], 2));
  EXPECT_TRUE(pvw.end_array());
  EXPECT_STREQ(pvw.str().c_str(), "    [0]: 5\n    [1]: 6");
}

TEST(dds_DCPS_PrinterValueWriter, complete_struct_with_complete_array)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("a")));
  EXPECT_TRUE(pvw.begin_array());
  EXPECT_TRUE(pvw.begin_element(0));
  EXPECT_TRUE(pvw.write_int16(5));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.begin_element(1));
  EXPECT_TRUE(pvw.write_int16(6));
  EXPECT_TRUE(pvw.end_element());
  EXPECT_TRUE(pvw.end_array());
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_TRUE(pvw.end_struct());
  // lint.pl ignores whitespace_before_newline_in_string on next line
  EXPECT_STREQ(pvw.str().c_str(), "    a: \n        [0]: 5\n        [1]: 6");
}

TEST(dds_DCPS_PrinterValueWriter, write_boolean)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_boolean(false));
    EXPECT_STREQ(pvw.str().c_str(), "false");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_boolean(true));
    EXPECT_STREQ(pvw.str().c_str(), "true");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_byte)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_byte(0));
    EXPECT_STREQ(pvw.str().c_str(), "0x0");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_byte(255));
    EXPECT_STREQ(pvw.str().c_str(), "0xff");
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
TEST(dds_DCPS_PrinterValueWriter, write_int8)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int8(-128));
    EXPECT_STREQ(pvw.str().c_str(), "-128");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int8(127));
    EXPECT_STREQ(pvw.str().c_str(), "127");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_uint8)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint8(0));
    EXPECT_STREQ(pvw.str().c_str(), "0");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint8(255));
    EXPECT_STREQ(pvw.str().c_str(), "255");
  }
}
#endif

TEST(dds_DCPS_PrinterValueWriter, write_int16)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int16(-32768));
    EXPECT_STREQ(pvw.str().c_str(), "-32768");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int16(32767));
    EXPECT_STREQ(pvw.str().c_str(), "32767");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_uint16)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint16(0));
    EXPECT_STREQ(pvw.str().c_str(), "0");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint16(65535));
    EXPECT_STREQ(pvw.str().c_str(), "65535");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_int32)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int32(-2147483647 - 1));
    EXPECT_STREQ(pvw.str().c_str(), "-2147483648");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int32(2147483647));
    EXPECT_STREQ(pvw.str().c_str(), "2147483647");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_uint32)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint32(0));
    EXPECT_STREQ(pvw.str().c_str(), "0");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint32(4294967295U));
    EXPECT_STREQ(pvw.str().c_str(), "4294967295");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_int64)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int64(-9223372036854775807 - 1));
    EXPECT_STREQ(pvw.str().c_str(), "-9223372036854775808");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int64(-9007199254740991));
    EXPECT_STREQ(pvw.str().c_str(), "-9007199254740991");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int64(9007199254740991));
    EXPECT_STREQ(pvw.str().c_str(), "9007199254740991");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_int64(9223372036854775807));
    EXPECT_STREQ(pvw.str().c_str(), "9223372036854775807");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_uint64)
{
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint64(0));
    EXPECT_STREQ(pvw.str().c_str(), "0");
  }
  {
    PrinterValueWriter pvw;
    EXPECT_TRUE(pvw.write_uint64(9007199254740991));
    EXPECT_STREQ(pvw.str().c_str(), "9007199254740991");
  }
}

TEST(dds_DCPS_PrinterValueWriter, write_float32)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_float32(1.2f));
  EXPECT_STREQ(pvw.str().c_str(), "1.2");
}

TEST(dds_DCPS_PrinterValueWriter, write_float64)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_float64(3.4));
  EXPECT_STREQ(pvw.str().c_str(), "3.4");
}

TEST(dds_DCPS_PrinterValueWriter, write_float128)
{
  PrinterValueWriter pvw;
  ACE_CDR::LongDouble x;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(x, 5.6);
  EXPECT_TRUE(pvw.write_float128(x));
  EXPECT_STREQ(pvw.str().c_str(), "5.6");
}

TEST(dds_DCPS_PrinterValueWriter, write_fixed)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_fixed(ACE_CDR::Fixed::from_string("1.0")));
  EXPECT_STREQ(pvw.str().c_str(), "1.0");
}

TEST(dds_DCPS_PrinterValueWriter, write_char8)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_char8('a'));
  EXPECT_STREQ(pvw.str().c_str(), "a");
}

TEST(dds_DCPS_PrinterValueWriter, write_char8_null)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_char8(0));
  EXPECT_STREQ(pvw.str().c_str(), "\\x00");
}

TEST(dds_DCPS_PrinterValueWriter, write_char16)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_char16('a'));
  EXPECT_STREQ(pvw.str().c_str(), "a");
}

TEST(dds_DCPS_PrinterValueWriter, write_char16_null)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.write_char16(0));
  EXPECT_STREQ(pvw.str().c_str(), "\\x0000");
}

TEST(dds_DCPS_PrinterValueWriter, write_string)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(static_cast<ValueWriter&>(pvw).write_string("a string"));
  EXPECT_STREQ(pvw.str().c_str(), "a string");
}

TEST(dds_DCPS_PrinterValueWriter, write_enum)
{
  PrinterValueWriter pvw;
  const ListEnumHelper::Pair enum_pairs[] = {
    {"label_1", 5},
    {"label_2", 6},
    {0, 0}
  };
  const ListEnumHelper enum_helper(enum_pairs);
  EXPECT_TRUE(pvw.write_enum(5, enum_helper));
  EXPECT_STREQ(pvw.str().c_str(), "label_1 (5)");
}

TEST(dds_DCPS_PrinterValueWriter, write_bitmask)
{
  PrinterValueWriter pvw;
  const MapBitmaskHelper::Pair bitmask_pairs[] = {
    {"flag0", 0},
    {"flag1", 1},
    {"flag2", 2},
    {"flag3", 3},
    {"flag4", 4},
    {"flag5", 5},
    {"flag6", 6},
    {0, 0}
  };
  const MapBitmaskHelper bitmask_helper(bitmask_pairs, 10, OpenDDS::XTypes::TK_UINT16);
  EXPECT_TRUE(pvw.write_bitmask(1 << 2 | 1 << 4 | 1 << 6, bitmask_helper));
  EXPECT_STREQ(pvw.str().c_str(), "flag2|flag4|flag6");
}

TEST(dds_DCPS_PrinterValueWriter, write_absent_value)
{
  PrinterValueWriter pvw;
  EXPECT_TRUE(pvw.begin_struct());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("Int32Field")));
  EXPECT_TRUE(pvw.write_int32(10));
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_TRUE(pvw.begin_struct_member(MemberParam("NullField")));
  EXPECT_TRUE(pvw.write_absent_value());
  EXPECT_TRUE(pvw.end_struct_member());
  EXPECT_TRUE(pvw.end_struct());
  EXPECT_STREQ(pvw.str().c_str(), "    Int32Field: 10\n    NullField: null");
}

#endif
