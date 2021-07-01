/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/JsonValueWriter.h"

#include "gtest/gtest.h"

#ifdef OPENDDS_RAPIDJSON
#ifndef OPENDDS_SAFETY_PROFILE

using namespace rapidjson;
using namespace OpenDDS::DCPS;

TEST(dds_DCPS_JsonValueWriter, begin_struct)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  EXPECT_STREQ(jvw.buffer().GetString(), "{");
}

TEST(dds_DCPS_JsonValueWriter, end_struct)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.end_struct();
  EXPECT_STREQ(jvw.buffer().GetString(), "{}");
}

TEST(dds_DCPS_JsonValueWriter, begin_struct_member)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.begin_struct_member("aField");
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"aField\"");
}

TEST(dds_DCPS_JsonValueWriter, end_struct_member)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.begin_struct_member("aField");
  jvw.write_int8(5);
  jvw.end_struct_member();
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"aField\":5");
}

TEST(dds_DCPS_JsonValueWriter, begin_union)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  EXPECT_STREQ(jvw.buffer().GetString(), "{");
}

TEST(dds_DCPS_JsonValueWriter, end_union)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  jvw.end_union();
  EXPECT_STREQ(jvw.buffer().GetString(), "{}");
}

TEST(dds_DCPS_JsonValueWriter, begin_discriminator)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  jvw.begin_discriminator();
  jvw.write_int8(5);
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"$discriminator\":5");
}

TEST(dds_DCPS_JsonValueWriter, end_discriminator)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  jvw.begin_discriminator();
  jvw.write_int8(5);
  jvw.end_discriminator();
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"$discriminator\":5");
}

TEST(dds_DCPS_JsonValueWriter, begin_union_member)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  jvw.begin_union_member("aField");
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"aField\"");
}

TEST(dds_DCPS_JsonValueWriter, end_union_member)
{
  JsonValueWriter<> jvw;
  jvw.begin_union();
  jvw.begin_union_member("aField");
  jvw.write_int8(5);
  jvw.end_union_member();
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"aField\":5");
}

TEST(dds_DCPS_JsonValueWriter, complete_struct)
{
  JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.begin_struct_member("aField");
  jvw.write_int8(5);
  jvw.end_struct_member();
  jvw.begin_struct_member("bField");
  jvw.write_int8(6);
  jvw.end_struct_member();
  jvw.end_struct();
  EXPECT_STREQ(jvw.buffer().GetString(), "{\"aField\":5,\"bField\":6}");
}

TEST(dds_DCPS_JsonValueWriter, begin_array)
{
  JsonValueWriter<> jvw;
  jvw.begin_array();
  EXPECT_STREQ(jvw.buffer().GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_array)
{
  JsonValueWriter<> jvw;
  jvw.begin_array();
  jvw.end_array();
  EXPECT_STREQ(jvw.buffer().GetString(), "[]");
}

TEST(dds_DCPS_JsonValueWriter, begin_sequence)
{
  JsonValueWriter<> jvw;
  jvw.begin_sequence();
  EXPECT_STREQ(jvw.buffer().GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_sequence)
{
  JsonValueWriter<> jvw;
  jvw.begin_sequence();
  jvw.end_sequence();
  EXPECT_STREQ(jvw.buffer().GetString(), "[]");
}

TEST(dds_DCPS_JsonValueWriter, begin_element)
{
  JsonValueWriter<> jvw;
  jvw.begin_sequence();
  jvw.begin_element(0);
  EXPECT_STREQ(jvw.buffer().GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_element)
{
  JsonValueWriter<> jvw;
  jvw.begin_sequence();
  jvw.begin_element(0);
  jvw.write_int8(5);
  jvw.end_element();
  EXPECT_STREQ(jvw.buffer().GetString(), "[5");
}

TEST(dds_DCPS_JsonValueWriter, complete_array)
{
  JsonValueWriter<> jvw;
  jvw.begin_array();
  jvw.begin_element(0);
  jvw.write_int8(5);
  jvw.end_element();
  jvw.begin_element(1);
  jvw.write_int8(6);
  jvw.end_element();
  jvw.end_array();
  EXPECT_STREQ(jvw.buffer().GetString(), "[5,6]");
}

TEST(dds_DCPS_JsonValueWriter, write_boolean)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_boolean(false);
    EXPECT_STREQ(jvw.buffer().GetString(), "false");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_boolean(true);
    EXPECT_STREQ(jvw.buffer().GetString(), "true");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_byte)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_byte(0);
    EXPECT_STREQ(jvw.buffer().GetString(), "0");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_byte(255);
    EXPECT_STREQ(jvw.buffer().GetString(), "255");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int8)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_int8(-128);
    EXPECT_STREQ(jvw.buffer().GetString(), "-128");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int8(127);
    EXPECT_STREQ(jvw.buffer().GetString(), "127");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint8)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_uint8(0);
    EXPECT_STREQ(jvw.buffer().GetString(), "0");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_uint8(255);
    EXPECT_STREQ(jvw.buffer().GetString(), "255");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int16)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_int16(-32768);
    EXPECT_STREQ(jvw.buffer().GetString(), "-32768");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int16(32767);
    EXPECT_STREQ(jvw.buffer().GetString(), "32767");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint16)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_uint16(0);
    EXPECT_STREQ(jvw.buffer().GetString(), "0");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_uint16(65535);
    EXPECT_STREQ(jvw.buffer().GetString(), "65535");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int32)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_int32(-2147483647 - 1);
    EXPECT_STREQ(jvw.buffer().GetString(), "-2147483648");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int32(2147483647);
    EXPECT_STREQ(jvw.buffer().GetString(), "2147483647");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint32)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_uint32(0);
    EXPECT_STREQ(jvw.buffer().GetString(), "0");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_uint32(4294967295);
    EXPECT_STREQ(jvw.buffer().GetString(), "4294967295");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int64)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_int64(-9223372036854775807 - 1);
    EXPECT_STREQ(jvw.buffer().GetString(), "-9223372036854775808");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int64(-9007199254740991);
    EXPECT_STREQ(jvw.buffer().GetString(), "-9007199254740991");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int64(9007199254740991);
    EXPECT_STREQ(jvw.buffer().GetString(), "9007199254740991");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_int64(9223372036854775807);
    EXPECT_STREQ(jvw.buffer().GetString(), "9223372036854775807");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint64)
{
  {
    JsonValueWriter<> jvw;
    jvw.write_uint64(0);
    EXPECT_STREQ(jvw.buffer().GetString(), "0");
  }
  {
    JsonValueWriter<> jvw;
    jvw.write_uint64(9007199254740991);
    EXPECT_STREQ(jvw.buffer().GetString(), "9007199254740991");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_float32)
{
  JsonValueWriter<> jvw;
  jvw.write_float32(1.2f);
  EXPECT_STREQ(jvw.buffer().GetString(), "1.2000000476837159");
}

TEST(dds_DCPS_JsonValueWriter, write_float64)
{
  JsonValueWriter<> jvw;
  jvw.write_float64(3.4);
  EXPECT_STREQ(jvw.buffer().GetString(), "3.4");
}

TEST(dds_DCPS_JsonValueWriter, write_float128)
{
  JsonValueWriter<> jvw;
  ACE_CDR::LongDouble x;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(x, 5.6);
  jvw.write_float128(x);
  EXPECT_STREQ(jvw.buffer().GetString(), "5.6");
}

TEST(dds_DCPS_JsonValueWriter, write_fixed)
{
  JsonValueWriter<> jvw;
  jvw.write_fixed(OpenDDS::FaceTypes::Fixed());
  EXPECT_STREQ(jvw.buffer().GetString(), "\"fixed\"");
}

TEST(dds_DCPS_JsonValueWriter, write_char8)
{
  JsonValueWriter<> jvw;
  jvw.write_char8('a');
  EXPECT_STREQ(jvw.buffer().GetString(), "97");
}

TEST(dds_DCPS_JsonValueWriter, write_char16)
{
  JsonValueWriter<> jvw;
  jvw.write_char16('a');
  EXPECT_STREQ(jvw.buffer().GetString(), "97");
}

TEST(dds_DCPS_JsonValueWriter, write_string)
{
  JsonValueWriter<> jvw;
  jvw.write_string("a string");
  EXPECT_STREQ(jvw.buffer().GetString(), "\"a string\"");
}

TEST(dds_DCPS_JsonValueWriter, write_enum)
{
  JsonValueWriter<> jvw;
  jvw.write_enum("label", 5);
  EXPECT_STREQ(jvw.buffer().GetString(), "\"label\"");
}

#endif
#endif
