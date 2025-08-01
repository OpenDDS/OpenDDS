/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/JsonValueReader.h>

#include <gtest/gtest.h>

#if OPENDDS_HAS_JSON_VALUE_READER

using namespace rapidjson;
using namespace OpenDDS::DCPS;
using namespace OpenDDS::XTypes;

static const MemberId BOOL_MEMBER_ID = 0;
static const MemberId BYTE_MEMBER_ID = 1;
static const MemberId INT8_MEMBER_ID = 2;
static const MemberId UINT8_MEMBER_ID = 3;
static const MemberId INT16_MEMBER_ID = 4;
static const MemberId UINT16_MEMBER_ID = 5;
static const MemberId INT32_MEMBER_ID = 6;
static const MemberId UINT32_MEMBER_ID = 7;
static const MemberId INT64_MEMBER_ID = 8;
static const MemberId UINT64_MEMBER_ID = 9;
static const MemberId FLOAT32_MEMBER_ID = 10;
static const MemberId FLOAT64_MEMBER_ID = 11;
static const MemberId FLOAT128_MEMBER_ID = 12;
//static const MemberId FIXED_MEMBER_ID = 13;
static const MemberId CHAR8_MEMBER_ID = 14;
static const MemberId CHAR16_MEMBER_ID = 15;
static const MemberId STRING_MEMBER_ID = 16;
//static const MemberId WSTRING_MEMBER_ID = 17;
static const MemberId ENUM_MEMBER_ID = 18;
static const MemberId BITMASK_MEMBER_ID = 19;

static const ListMemberHelper::Pair member_pairs[] = {
  {"bool", BOOL_MEMBER_ID},
  {"byte", BYTE_MEMBER_ID},
  {"int8", INT8_MEMBER_ID},
  {"uint8", UINT8_MEMBER_ID},
  {"int16", INT16_MEMBER_ID},
  {"uint16", UINT16_MEMBER_ID},
  {"int32", INT32_MEMBER_ID},
  {"uint32", UINT32_MEMBER_ID},
  {"int64", INT64_MEMBER_ID},
  {"uint64", UINT64_MEMBER_ID},
  {"float32", FLOAT32_MEMBER_ID},
  {"float64", FLOAT64_MEMBER_ID},
  {"float128", FLOAT128_MEMBER_ID},
  {"char8", CHAR8_MEMBER_ID},
  {"char16", CHAR16_MEMBER_ID},
  {"string", STRING_MEMBER_ID},
  {"enum", ENUM_MEMBER_ID},
  {"bitmask", BITMASK_MEMBER_ID},
  {0, 0}
};

static const ListMemberHelper member_helper(member_pairs);

enum MyEnum {
  kValue1,
  kValue2
};

static const ListEnumHelper::Pair enum_pairs[] = {
  {"kValue1", kValue1},
  {"kValue2", kValue2},
  {0, 0}
};

static const ListEnumHelper enum_helper(enum_pairs);

static const MapBitmaskHelper::Pair bitmask_pairs[] = {
  {"FLAG_0", 0},
  {"FLAG_1", 1},
  {"FLAG_2", 2},
  {"FLAG_3", 3},
  {0, 0}
};

static const MapBitmaskHelper bitmask_helper(bitmask_pairs, 64, OpenDDS::XTypes::TK_UINT64);

TEST(dds_DCPS_JsonValueReader, struct_empty)
{
  const char json[] = "{}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_struct());
  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, array_empty)
{
  const char json[] = "[]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_array());
  EXPECT_TRUE(jvr.end_array());
}

TEST(dds_DCPS_JsonValueReader, array_read)
{
  const char json[] = "[5,6]";
  ACE_CDR::Short i[2] = {0, 0};
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_array());
  EXPECT_TRUE(jvr.read_int16_array(&i[0], 2));
  EXPECT_TRUE(jvr.end_array());
  EXPECT_EQ(i[0], 5);
  EXPECT_EQ(i[1], 6);
}

TEST(dds_DCPS_JsonValueReader, sequence_read)
{
  const char json[] = "[5,6]";
  ACE_CDR::Short i[2] = {0, 0};
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_sequence());
  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.read_int16_array(&i[0], 2));
  EXPECT_FALSE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.end_sequence());
  EXPECT_EQ(i[0], 5);
  EXPECT_EQ(i[1], 6);
}

TEST(dds_DCPS_JsonValueReader, sequence_empty)
{
  const char json[] = "[]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_sequence());
  EXPECT_FALSE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.end_sequence());
}

TEST(dds_DCPS_JsonValueReader, struct_max)
{
  const char json[] =
    "{"
    "\"bool\":true,"
    "\"byte\":255,"
#if OPENDDS_HAS_EXPLICIT_INTS
    "\"int8\":127,"
    "\"uint8\":255,"
#endif
    "\"int16\":32767,"
    "\"uint16\":65535,"
    "\"int32\":2147483647,"
    "\"uint32\":4294967295,"
    "\"int64\":9223372036854775807,"
    "\"uint64\":18446744073709551615,"
    "\"float32\":1.25,"
    "\"float64\":1.25,"
    "\"float128\":1.25,"
    "\"char8\":\"a\","
    "\"char16\":\"a\","
    "\"string\":\"a string\","
    "\"enum\":\"kValue1\","
    "\"bitmask\":\"FLAG_0|FLAG_2|FLAG_3\""
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  MemberId member_id;
  ACE_CDR::Boolean bool_value;
  ACE_CDR::Octet byte_value;
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::Int8 int8_value;
  ACE_CDR::UInt8 uint8_value;
#endif
  ACE_CDR::Short int16_value;
  ACE_CDR::UShort uint16_value;
  ACE_CDR::Long int32_value;
  ACE_CDR::ULong uint32_value;
  ACE_CDR::LongLong int64_value;
  ACE_CDR::ULongLong uint64_value;
  ACE_CDR::Float float32_value;
  ACE_CDR::Double float64_value;
  ACE_CDR::LongDouble float128_value;
  ACE_CDR::Char char8_value;
  ACE_CDR::WChar char16_value;
  std::string string_value;
  MyEnum enum_value;
  ACE_CDR::ULongLong bitmask_value;

  EXPECT_TRUE(jvr.begin_struct());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, BOOL_MEMBER_ID);
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, true);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, BYTE_MEMBER_ID);
  EXPECT_TRUE(jvr.read_byte(byte_value));
  EXPECT_EQ(byte_value, 255);
  EXPECT_TRUE(jvr.end_struct_member());

#if OPENDDS_HAS_EXPLICIT_INTS
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, INT8_MEMBER_ID);
  EXPECT_TRUE(jvr.read_int8(int8_value));
  EXPECT_EQ(int8_value, 127);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, UINT8_MEMBER_ID);
  EXPECT_TRUE(jvr.read_uint8(uint8_value));
  EXPECT_EQ(uint8_value, 255);
  EXPECT_TRUE(jvr.end_struct_member());
#endif

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, INT16_MEMBER_ID);
  EXPECT_TRUE(jvr.read_int16(int16_value));
  EXPECT_EQ(int16_value, 32767);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, UINT16_MEMBER_ID);
  EXPECT_TRUE(jvr.read_uint16(uint16_value));
  EXPECT_EQ(uint16_value, 65535);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, INT32_MEMBER_ID);
  EXPECT_TRUE(jvr.read_int32(int32_value));
  EXPECT_EQ(int32_value, 2147483647);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, UINT32_MEMBER_ID);
  EXPECT_TRUE(jvr.read_uint32(uint32_value));
  EXPECT_EQ(uint32_value, 4294967295);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, INT64_MEMBER_ID);
  EXPECT_TRUE(jvr.read_int64(int64_value));
  EXPECT_EQ(int64_value, 9223372036854775807);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, UINT64_MEMBER_ID);
  EXPECT_TRUE(jvr.read_uint64(uint64_value));
  EXPECT_EQ(uint64_value, 18446744073709551615ull);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, FLOAT32_MEMBER_ID);
  EXPECT_TRUE(jvr.read_float32(float32_value));
  EXPECT_EQ(float32_value, 1.25);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, FLOAT64_MEMBER_ID);
  EXPECT_TRUE(jvr.read_float64(float64_value));
  EXPECT_EQ(float64_value, 1.25);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, FLOAT128_MEMBER_ID);
  EXPECT_TRUE(jvr.read_float128(float128_value));
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4244)
  EXPECT_EQ(float128_value, 1.25);
  GTEST_DISABLE_MSC_WARNINGS_POP_()
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, CHAR8_MEMBER_ID);
  EXPECT_TRUE(jvr.read_char8(char8_value));
  EXPECT_EQ(char8_value, 'a');
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, CHAR16_MEMBER_ID);
  EXPECT_TRUE(jvr.read_char16(char16_value));
  EXPECT_EQ(char16_value, 'a');
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, STRING_MEMBER_ID);
  EXPECT_TRUE(jvr.read_string(string_value));
  EXPECT_EQ(string_value, "a string");
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, ENUM_MEMBER_ID);
  EXPECT_TRUE(jvr.read_enum(enum_value, enum_helper));
  EXPECT_EQ(enum_value, kValue1);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, BITMASK_MEMBER_ID);
  EXPECT_TRUE(jvr.read_bitmask(bitmask_value, bitmask_helper));
  EXPECT_EQ(bitmask_value, 13ull);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, a_union)
{
  const char json[] = "{\"$discriminator\":5,\"5\":true}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  ACE_CDR::Long discriminator;
  ACE_CDR::Boolean bool_value;

  EXPECT_TRUE(jvr.begin_union());
  EXPECT_TRUE(jvr.begin_discriminator());
  EXPECT_TRUE(jvr.read_int32(discriminator));
  EXPECT_EQ(discriminator, 5);
  EXPECT_TRUE(jvr.end_discriminator());
  EXPECT_TRUE(jvr.begin_union_member());
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, true);
  EXPECT_TRUE(jvr.end_union_member());
  EXPECT_TRUE(jvr.end_union());
}

TEST(dds_DCPS_JsonValueReader, array_min)
{
  const char json[] = "[false,0,"
#if OPENDDS_HAS_EXPLICIT_INTS
    "-128,0,"
#endif
    "-32768,0,-2147483648,0,-9223372036854775808,0,-1.25,-1.25,-1.25,\"a\",\"a\",\"a string\",\"kValue2\",\"FLAG_2|FLAG_3\"]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  ACE_CDR::Boolean bool_value;
  ACE_CDR::Octet byte_value;
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::Int8 int8_value;
  ACE_CDR::UInt8 uint8_value;
#endif
  ACE_CDR::Short int16_value;
  ACE_CDR::UShort uint16_value;
  ACE_CDR::Long int32_value;
  ACE_CDR::ULong uint32_value;
  ACE_CDR::LongLong int64_value;
  ACE_CDR::ULongLong uint64_value;
  ACE_CDR::Float float32_value;
  ACE_CDR::Double float64_value;
  ACE_CDR::LongDouble float128_value;
  ACE_CDR::Char char8_value;
  ACE_CDR::WChar char16_value;
  std::string string_value;
  MyEnum enum_value;
  ACE_CDR::ULongLong bitmask_value;

  EXPECT_TRUE(jvr.begin_array());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, false);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_byte(byte_value));
  EXPECT_EQ(byte_value, 0);
  EXPECT_TRUE(jvr.end_element());

#if OPENDDS_HAS_EXPLICIT_INTS
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int8(int8_value));
  EXPECT_EQ(int8_value, -128);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint8(uint8_value));
  EXPECT_EQ(uint8_value, 0);
  EXPECT_TRUE(jvr.end_element());
#endif

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int16(int16_value));
  EXPECT_EQ(int16_value, -32768);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint16(uint16_value));
  EXPECT_EQ(uint16_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int32(int32_value));
  EXPECT_EQ(int32_value, -2147483647 - 1);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint32(uint32_value));
  EXPECT_EQ(uint32_value, 0U);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int64(int64_value));
  EXPECT_EQ(int64_value, -9223372036854775807 - 1);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint64(uint64_value));
  EXPECT_EQ(uint64_value, 0U);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float32(float32_value));
  EXPECT_EQ(float32_value, -1.25);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float64(float64_value));
  EXPECT_EQ(float64_value, -1.25);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float128(float128_value));
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4244)
  EXPECT_EQ(float128_value, -1.25);
  GTEST_DISABLE_MSC_WARNINGS_POP_()
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_char8(char8_value));
  EXPECT_EQ(char8_value, 'a');
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_char16(char16_value));
  EXPECT_EQ(char16_value, 'a');
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_string(string_value));
  EXPECT_EQ(string_value, "a string");
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_enum(enum_value, enum_helper));
  EXPECT_EQ(enum_value, kValue2);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_bitmask(bitmask_value, bitmask_helper));
  EXPECT_EQ(bitmask_value, 12ull);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.end_array());
}

TEST(dds_DCPS_JsonValueReader, sequence_zero)
{
  const char json[] = "[false,0,"
#if OPENDDS_HAS_EXPLICIT_INTS
    "0,0,"
#endif
    "0,0,0,0,0,0,0,0,0,\"\\u0000\",\"\\u0000\",\"\",\"kValue1\",\"FLAG_0|FLAG_2\"]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  ACE_CDR::Boolean bool_value;
  ACE_CDR::Octet byte_value;
#if OPENDDS_HAS_EXPLICIT_INTS
  ACE_CDR::Int8 int8_value;
  ACE_CDR::UInt8 uint8_value;
#endif
  ACE_CDR::Short int16_value;
  ACE_CDR::UShort uint16_value;
  ACE_CDR::Long int32_value;
  ACE_CDR::ULong uint32_value;
  ACE_CDR::LongLong int64_value;
  ACE_CDR::ULongLong uint64_value;
  ACE_CDR::Float float32_value;
  ACE_CDR::Double float64_value;
  ACE_CDR::LongDouble float128_value;
  ACE_CDR::Char char8_value;
  ACE_CDR::WChar char16_value;
  std::string string_value;
  MyEnum enum_value;
  ACE_CDR::ULongLong bitmask_value;

  EXPECT_TRUE(jvr.begin_sequence());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, false);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_byte(byte_value));
  EXPECT_EQ(byte_value, 0);
  EXPECT_TRUE(jvr.end_element());

#if OPENDDS_HAS_EXPLICIT_INTS
  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int8(int8_value));
  EXPECT_EQ(int8_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint8(uint8_value));
  EXPECT_EQ(uint8_value, 0);
  EXPECT_TRUE(jvr.end_element());
#endif

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int16(int16_value));
  EXPECT_EQ(int16_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint16(uint16_value));
  EXPECT_EQ(uint16_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int32(int32_value));
  EXPECT_EQ(int32_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint32(uint32_value));
  EXPECT_EQ(uint32_value, 0U);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_int64(int64_value));
  EXPECT_EQ(int64_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_uint64(uint64_value));
  EXPECT_EQ(uint64_value, 0U);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float32(float32_value));
  EXPECT_EQ(float32_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float64(float64_value));
  EXPECT_EQ(float64_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_float128(float128_value));
  GTEST_DISABLE_MSC_WARNINGS_PUSH_(4244)
  EXPECT_EQ(float128_value, 0.0);
  GTEST_DISABLE_MSC_WARNINGS_POP_()
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_char8(char8_value));
  EXPECT_EQ(char8_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_char16(char16_value));
  EXPECT_EQ(char16_value, 0);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_string(string_value));
  EXPECT_EQ(string_value, "");
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_enum(enum_value, enum_helper));
  EXPECT_EQ(enum_value, kValue1);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_element());
  EXPECT_TRUE(jvr.read_bitmask(bitmask_value, bitmask_helper));
  EXPECT_EQ(bitmask_value, 5ull);
  EXPECT_TRUE(jvr.end_element());

  EXPECT_FALSE(jvr.elements_remaining());

  EXPECT_TRUE(jvr.end_sequence());
}

struct MyStruct {
  bool value;
};

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

bool vread(ValueReader& reader, MyStruct& s)
{
  if (!reader.begin_struct(APPENDABLE)) return false;
  MemberId member_id;
  if (!reader.begin_struct_member(member_id, member_helper)) return false;
  if (member_id != BOOL_MEMBER_ID)  return false;
  if (!reader.read_boolean(s.value))  return false;
  if (!reader.end_struct_member()) return false;
  if (!reader.end_struct()) return false;
  return true;
}

template <>
void set_default(MyStruct& value)
{
  value.value = false;
}

}
}
OPENDDS_END_VERSIONED_NAMESPACE_DECL

TEST(dds_DCPS_JsonValueReader, from_json)
{
  const char json[] = "{\"bool\":true}";
  StringStream ss(json);
  MyStruct s;
  EXPECT_TRUE(from_json(s, ss));
  EXPECT_TRUE(s.value);
}

void check_members(JsonValueReader<>& jvr)
{
  MemberId member_id;
  ACE_CDR::Boolean bool_value;
  ACE_CDR::Octet byte_value;
  ACE_CDR::Short int16_value;
  ACE_CDR::UShort uint16_value;

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, BOOL_MEMBER_ID);
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, true);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, BYTE_MEMBER_ID);
  EXPECT_TRUE(jvr.read_byte(byte_value));
  EXPECT_EQ(byte_value, 255);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, INT16_MEMBER_ID);
  EXPECT_TRUE(jvr.read_int16(int16_value));
  EXPECT_EQ(int16_value, 32767);
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper));
  EXPECT_EQ(member_id, UINT16_MEMBER_ID);
  EXPECT_TRUE(jvr.read_uint16(uint16_value));
  EXPECT_EQ(uint16_value, 65535);
  EXPECT_TRUE(jvr.end_struct_member());
}

#define SKIP_VALUE "{\"nested1\":[1,2,3,4,5],\"nested2\":2.17}"

TEST(dds_DCPS_JsonValueReader, skip_unknown_first)
{
  const char json[] = "{"
    "  \"unknown\": " SKIP_VALUE ","
    "  \"bool\": true,"
    "  \"byte\": 255,"
    "  \"int16\": 32767,"
    "  \"uint16\": 65535"
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_struct());
  check_members(jvr);
  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, skip_unknown_middle)
{
  const char json[] = "{"
    "  \"bool\": true,"
    "  \"byte\": 255,"
    "  \"unknown\": " SKIP_VALUE ","
    "  \"int16\": 32767,"
    "  \"uint16\": 65535"
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_struct());
  check_members(jvr);
  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, skip_unknown_last)
{
  const char json[] = "{"
    "  \"bool\": true,"
    "  \"byte\": 255,"
    "  \"int16\": 32767,"
    "  \"uint16\": 65535,"
    "  \"unknown\": " SKIP_VALUE
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_struct());
  check_members(jvr);
  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, optional_members)
{
  const char json[] =
    "{"
    "\"bool\":true,"
    "\"byte\":10,"
    "\"int16\":null,"
    "\"uint16\":11,"
    "\"int32\":12,"
    "\"nested_struct\":null,"
    "\"uint32\":13,"
    "\"array\":null,"
    "\"int64\":14,"
    "\"sequence\":null,"
    "\"uint64\":15,"
    "\"nested_union\":null,"
    "\"char8\":\"a\","
    "\"nested_union2\":{\"$discriminator\":3,\"3\":null}"
    "}";

  const ListMemberHelper::Pair member_pairs_local[] = {
    {"bool", 0},
    {"byte", 1},
    {"int16", 2},
    {"uint16", 3},
    {"int32", 4},
    {"nested_struct", 5},
    {"uint32", 6},
    {"array", 7},
    {"int64", 8},
    {"sequence", 9},
    {"uint64", 10},
    {"nested_union", 11},
    {"char8", 12},
    {"nested_union2", 13},
    {0, 0}
  };
  const ListMemberHelper member_helper_local(member_pairs_local);

  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  MemberId member_id;
  ACE_CDR::Boolean bool_value;
  ACE_CDR::Octet byte_value;
  ACE_CDR::UShort uint16_value;
  ACE_CDR::Long int32_value;
  ACE_CDR::ULong uint32_value;
  ACE_CDR::LongLong int64_value;
  ACE_CDR::ULongLong uint64_value;
  ACE_CDR::Char char8_value;
  ACE_CDR::Long disc_value;

  EXPECT_TRUE(jvr.begin_struct());

  // bool: non-optional
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.read_boolean(bool_value));
  EXPECT_EQ(bool_value, true);
  EXPECT_TRUE(jvr.end_struct_member());

  // byte: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_byte(byte_value));
  EXPECT_EQ(byte_value, 10);
  EXPECT_TRUE(jvr.end_struct_member());

  // int16: no value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_struct_member());

  // uint16: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_uint16(uint16_value));
  EXPECT_EQ(uint16_value, 11);
  EXPECT_TRUE(jvr.end_struct_member());

  // int32: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_int32(int32_value));
  EXPECT_EQ(int32_value, 12);
  EXPECT_TRUE(jvr.end_struct_member());

  // nested_struct: no value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_struct_member());

  // uint32: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_uint32(uint32_value));
  EXPECT_EQ(uint32_value, 13ul);
  EXPECT_TRUE(jvr.end_struct_member());

  // array: no value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_struct_member());

  // int64: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_int64(int64_value));
  EXPECT_EQ(int64_value, 14);
  EXPECT_TRUE(jvr.end_struct_member());

  // sequence: no value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_struct_member());

  // uint64: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_uint64(uint64_value));
  EXPECT_EQ(uint64_value, 15ull);
  EXPECT_TRUE(jvr.end_struct_member());

  // nested_union: no value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_struct_member());

  // char8: has value
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.read_char8(char8_value));
  EXPECT_EQ(char8_value, 'a');
  EXPECT_TRUE(jvr.end_struct_member());

  // nested_union2: has value with an absent branch
  EXPECT_TRUE(jvr.begin_struct_member(member_id, member_helper_local));
  EXPECT_TRUE(jvr.member_has_value());
  EXPECT_TRUE(jvr.begin_union());
  EXPECT_TRUE(jvr.begin_discriminator());
  EXPECT_TRUE(jvr.read_int32(disc_value));
  EXPECT_EQ(disc_value, 3);
  EXPECT_TRUE(jvr.end_discriminator());
  EXPECT_TRUE(jvr.begin_union_member());
  EXPECT_FALSE(jvr.member_has_value());
  EXPECT_TRUE(jvr.end_union_member());
  EXPECT_TRUE(jvr.end_union());
  EXPECT_TRUE(jvr.end_struct_member());

  EXPECT_TRUE(jvr.end_struct());
}

TEST(dds_DCPS_JsonValueReader, map_string_key)
{
  static const char json[] = "{"
    "  \"bool\": 1,"
    "  \"byte\": 255,"
    "  \"int16\": 32767,"
    "  \"uint16\": 65535"
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_map(TK_STRING8, TK_INT32));
  std::string k;
  DDS::Int32 v;

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_string(k));
  EXPECT_EQ("bool", k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_int32(v));
  EXPECT_EQ(1, v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_string(k));
  EXPECT_EQ("byte", k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_int32(v));
  EXPECT_EQ(255, v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_string(k));
  EXPECT_EQ("int16", k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_int32(v));
  EXPECT_EQ(32767, v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_string(k));
  EXPECT_EQ("uint16", k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_int32(v));
  EXPECT_EQ(65535, v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_FALSE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.end_map());
}

TEST(dds_DCPS_JsonValueReader, map_int_key)
{
  static const char json[] = "{"
    "  \"1\": \"bool\","
    "  \"255\": \"byte\","
    "  \"32767\": \"int16\","
    "  \"65535\": \"uint16\""
    "}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_map(TK_INT32, TK_STRING8));
  DDS::Int32 k;
  std::string v;

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_int32(k));
  EXPECT_EQ(1, k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_string(v));
  EXPECT_EQ("bool", v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_int32(k));
  EXPECT_EQ(255, k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_string(v));
  EXPECT_EQ("byte", v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_int32(k));
  EXPECT_EQ(32767, k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_string(v));
  EXPECT_EQ("int16", v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_TRUE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.begin_key());
  EXPECT_TRUE(jvr.read_int32(k));
  EXPECT_EQ(65535, k);
  EXPECT_TRUE(jvr.end_key());
  EXPECT_TRUE(jvr.begin_value());
  EXPECT_TRUE(jvr.read_string(v));
  EXPECT_EQ("uint16", v);
  EXPECT_TRUE(jvr.end_value());

  EXPECT_FALSE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.end_map());
}

#endif
