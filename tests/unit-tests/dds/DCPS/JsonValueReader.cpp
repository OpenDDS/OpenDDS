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

TEST(JsonValueReader, struct_empty)
{
  const char json[] = "{}";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_struct());
  EXPECT_TRUE(jvr.end_struct());
}

TEST(JsonValueReader, array_empty)
{
  const char json[] = "[]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_array());
  EXPECT_TRUE(jvr.end_array());
}

TEST(JsonValueReader, sequence_empty)
{
  const char json[] = "[]";
  StringStream ss(json);
  JsonValueReader<> jvr(ss);
  EXPECT_TRUE(jvr.begin_sequence());
  EXPECT_FALSE(jvr.elements_remaining());
  EXPECT_TRUE(jvr.end_sequence());
}

TEST(JsonValueReader, struct_max)
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
    "\"char8\":97,"
    "\"char16\":97,"
    "\"string\":\"a string\","
    "\"enum\":\"kValue1\""
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
  EXPECT_EQ(float128_value, 1.25);
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

  EXPECT_TRUE(jvr.end_struct());
}

TEST(JsonValueReader, a_union)
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

TEST(JsonValueReader, array_min)
{
  const char json[] = "[false,0,"
#if OPENDDS_HAS_EXPLICIT_INTS
    "-128,0,"
#endif
    "-32768,0,-2147483648,0,-9223372036854775808,0,-1.25,-1.25,-1.25,97,97,\"a string\",\"kValue2\"]";
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
  EXPECT_EQ(float128_value, -1.25);
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

  EXPECT_TRUE(jvr.end_array());
}

TEST(JsonValueReader, sequence_zero)
{
  const char json[] = "[false,0,"
#if OPENDDS_HAS_EXPLICIT_INTS
    "0,0,"
#endif
    "0,0,0,0,0,0,0,0,0,0,0,\"\",\"kValue1\"]";
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
  EXPECT_EQ(float128_value, 0.0);
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
  if (!reader.begin_struct()) return false;
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

TEST(JsonValueReader, from_json)
{
  const char json[] = "{\"bool\":true}";
  StringStream ss(json);
  MyStruct s;
  EXPECT_TRUE(from_json(s, ss));
}

#endif
