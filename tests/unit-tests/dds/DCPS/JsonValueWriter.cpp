/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/JsonValueWriter.h>

#include <gtest/gtest.h>

#if OPENDDS_HAS_JSON_VALUE_WRITER

using namespace OpenDDS::DCPS;

typedef rapidjson::StringBuffer Buffer;
typedef rapidjson::Writer<Buffer> Writer;

class TestWriter : public JsonValueWriter<Writer>
{
public:
  explicit TestWriter(Writer& writer)
    : JsonValueWriter(writer)
    , elements_(0)
  {}
  virtual bool write_int16_array(const ACE_CDR::Short* x, ACE_CDR::ULong length)
  {
    elements_ += length;
    return JsonValueWriter<Writer>::write_int16_array(x, length);
  }
  size_t elements_;
};

TEST(dds_DCPS_JsonValueWriter, begin_struct)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_STREQ(buffer.GetString(), "{");
}

TEST(dds_DCPS_JsonValueWriter, end_struct)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_TRUE(jvw.end_struct());
  EXPECT_STREQ(buffer.GetString(), "{}");
}

TEST(dds_DCPS_JsonValueWriter, begin_struct_member)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_TRUE(jvw.begin_struct_member(MemberParam("aField")));
  EXPECT_STREQ(buffer.GetString(), "{\"aField\"");
}

TEST(dds_DCPS_JsonValueWriter, end_struct_member)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_TRUE(jvw.begin_struct_member(MemberParam("aField")));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_struct_member());
  EXPECT_STREQ(buffer.GetString(), "{\"aField\":5");
}

TEST(dds_DCPS_JsonValueWriter, begin_union)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_STREQ(buffer.GetString(), "{");
}

TEST(dds_DCPS_JsonValueWriter, end_union)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_TRUE(jvw.end_union());
  EXPECT_STREQ(buffer.GetString(), "{}");
}

TEST(dds_DCPS_JsonValueWriter, begin_discriminator)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_TRUE(jvw.begin_discriminator(MemberParam()));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_STREQ(buffer.GetString(), "{\"$discriminator\":5");
}

TEST(dds_DCPS_JsonValueWriter, end_discriminator)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_TRUE(jvw.begin_discriminator(MemberParam()));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_discriminator());
  EXPECT_STREQ(buffer.GetString(), "{\"$discriminator\":5");
}

TEST(dds_DCPS_JsonValueWriter, begin_union_member)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_TRUE(jvw.begin_union_member(MemberParam("aField")));
  EXPECT_STREQ(buffer.GetString(), "{\"aField\"");
}

TEST(dds_DCPS_JsonValueWriter, end_union_member)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_union());
  EXPECT_TRUE(jvw.begin_union_member(MemberParam("aField")));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_union_member());
  EXPECT_STREQ(buffer.GetString(), "{\"aField\":5");
}

TEST(dds_DCPS_JsonValueWriter, complete_struct)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_TRUE(jvw.begin_struct_member(MemberParam("aField")));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_struct_member());
  EXPECT_TRUE(jvw.begin_struct_member(MemberParam("bField")));
  EXPECT_TRUE(jvw.write_int16(6));
  EXPECT_TRUE(jvw.end_struct_member());
  EXPECT_TRUE(jvw.end_struct());
  EXPECT_STREQ(buffer.GetString(), "{\"aField\":5,\"bField\":6}");
}

TEST(dds_DCPS_JsonValueWriter, begin_array)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_array());
  EXPECT_STREQ(buffer.GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_array)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_array());
  EXPECT_TRUE(jvw.end_array());
  EXPECT_STREQ(buffer.GetString(), "[]");
}

TEST(dds_DCPS_JsonValueWriter, begin_sequence)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_STREQ(buffer.GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_sequence)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_TRUE(jvw.end_sequence());
  EXPECT_STREQ(buffer.GetString(), "[]");
}

TEST(dds_DCPS_JsonValueWriter, begin_element)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_TRUE(jvw.begin_element(0));
  EXPECT_STREQ(buffer.GetString(), "[");
}

TEST(dds_DCPS_JsonValueWriter, end_element)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_TRUE(jvw.begin_element(0));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_STREQ(buffer.GetString(), "[5");
}

TEST(dds_DCPS_JsonValueWriter, complete_sequence)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_TRUE(jvw.begin_element(0));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.begin_element(1));
  EXPECT_TRUE(jvw.write_int16(6));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.end_sequence());
  EXPECT_STREQ(buffer.GetString(), "[5,6]");
}

TEST(dds_DCPS_JsonValueWriter, complete_sequence_write_array)
{
  Buffer buffer;
  Writer writer(buffer);
  const ACE_CDR::Short i[]= {5, 6};
  TestWriter jvw(writer);
  EXPECT_TRUE(jvw.begin_sequence());
  EXPECT_TRUE(jvw.write_int16_array(&i[0], 2));
  EXPECT_TRUE(jvw.end_sequence());
  EXPECT_STREQ(buffer.GetString(), "[5,6]");
  EXPECT_EQ(jvw.elements_, 2u);
}

TEST(dds_DCPS_JsonValueWriter, complete_array)
{
  Buffer buffer;
  Writer writer(buffer);
  TestWriter jvw(writer);
  EXPECT_TRUE(jvw.begin_array());
  EXPECT_TRUE(jvw.begin_element(0));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.begin_element(1));
  EXPECT_TRUE(jvw.write_int16(6));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.end_array());
  EXPECT_STREQ(buffer.GetString(), "[5,6]");
  EXPECT_EQ(jvw.elements_, 0u);
}

TEST(dds_DCPS_JsonValueWriter, complete_array_write_array)
{
  Buffer buffer;
  Writer writer(buffer);
  ACE_CDR::Short const i[2] = {5, 6};
  TestWriter jvw(writer);
  EXPECT_TRUE(jvw.begin_array());
  EXPECT_TRUE(jvw.write_int16_array(&i[0], 2));
  EXPECT_TRUE(jvw.end_array());
  EXPECT_STREQ(buffer.GetString(), "[5,6]");
  EXPECT_EQ(jvw.elements_, 2u);
}

TEST(dds_DCPS_JsonValueWriter, complete_struct_with_complete_array)
{
  Buffer buffer;
  Writer writer(buffer);
  TestWriter jvw(writer);
  EXPECT_TRUE(jvw.begin_struct());
  EXPECT_TRUE(jvw.begin_struct_member(MemberParam("a")));
  EXPECT_TRUE(jvw.begin_array());
  EXPECT_TRUE(jvw.begin_element(0));
  EXPECT_TRUE(jvw.write_int16(5));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.begin_element(1));
  EXPECT_TRUE(jvw.write_int16(6));
  EXPECT_TRUE(jvw.end_element());
  EXPECT_TRUE(jvw.end_array());
  EXPECT_TRUE(jvw.end_struct_member());
  EXPECT_TRUE(jvw.end_struct());
  EXPECT_STREQ(buffer.GetString(), "{\"a\":[5,6]}");
  EXPECT_EQ(jvw.elements_, 0u);
}

TEST(dds_DCPS_JsonValueWriter, write_boolean)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_boolean(false));
    EXPECT_STREQ(buffer.GetString(), "false");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_boolean(true));
    EXPECT_STREQ(buffer.GetString(), "true");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_byte)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_byte(0));
    EXPECT_STREQ(buffer.GetString(), "0");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_byte(255));
    EXPECT_STREQ(buffer.GetString(), "255");
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
TEST(dds_DCPS_JsonValueWriter, write_int8)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int8(-128));
    EXPECT_STREQ(buffer.GetString(), "-128");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int8(127));
    EXPECT_STREQ(buffer.GetString(), "127");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint8)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint8(0));
    EXPECT_STREQ(buffer.GetString(), "0");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint8(255));
    EXPECT_STREQ(buffer.GetString(), "255");
  }
}
#endif

TEST(dds_DCPS_JsonValueWriter, write_int16)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int16(-32768));
    EXPECT_STREQ(buffer.GetString(), "-32768");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int16(32767));
    EXPECT_STREQ(buffer.GetString(), "32767");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint16)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint16(0));
    EXPECT_STREQ(buffer.GetString(), "0");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint16(65535));
    EXPECT_STREQ(buffer.GetString(), "65535");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int32)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int32(-2147483647 - 1));
    EXPECT_STREQ(buffer.GetString(), "-2147483648");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int32(2147483647));
    EXPECT_STREQ(buffer.GetString(), "2147483647");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint32)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint32(0));
    EXPECT_STREQ(buffer.GetString(), "0");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint32(4294967295));
    EXPECT_STREQ(buffer.GetString(), "4294967295");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_int64)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int64(-9223372036854775807 - 1));
    EXPECT_STREQ(buffer.GetString(), "-9223372036854775808");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int64(-9007199254740991));
    EXPECT_STREQ(buffer.GetString(), "-9007199254740991");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int64(9007199254740991));
    EXPECT_STREQ(buffer.GetString(), "9007199254740991");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_int64(9223372036854775807));
    EXPECT_STREQ(buffer.GetString(), "9223372036854775807");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_uint64)
{
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint64(0));
    EXPECT_STREQ(buffer.GetString(), "0");
  }
  {
    Buffer buffer;
    Writer writer(buffer);
    JsonValueWriter<Writer> jvw(writer);
    EXPECT_TRUE(jvw.write_uint64(9007199254740991));
    EXPECT_STREQ(buffer.GetString(), "9007199254740991");
  }
}

TEST(dds_DCPS_JsonValueWriter, write_float32)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_float32(1.2f));
  EXPECT_STREQ(buffer.GetString(), "1.2000000476837158");
}

TEST(dds_DCPS_JsonValueWriter, write_float64)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_float64(3.4));
  EXPECT_STREQ(buffer.GetString(), "3.4");
}

TEST(dds_DCPS_JsonValueWriter, write_float128)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  ACE_CDR::LongDouble x;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(x, 5.6);
  EXPECT_TRUE(jvw.write_float128(x));
  EXPECT_STREQ(buffer.GetString(), "5.6");
}

TEST(dds_DCPS_JsonValueWriter, write_fixed)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_fixed(ACE_CDR::Fixed::from_integer(ACE_CDR::LongLong(1))));
  EXPECT_STREQ(buffer.GetString(), "\"1\"");
}

TEST(dds_DCPS_JsonValueWriter, write_char8)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_char8('a'));
  EXPECT_STREQ(buffer.GetString(), "\"a\"");
}

TEST(dds_DCPS_JsonValueWriter, write_char8_null)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_char8(0));
  EXPECT_STREQ(buffer.GetString(), "\"\\u0000\"");
}

TEST(dds_DCPS_JsonValueWriter, write_char16)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_char16('a'));
  EXPECT_STREQ(buffer.GetString(), "\"a\"");
}

TEST(dds_DCPS_JsonValueWriter, write_char16_null)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(jvw.write_char16(0));
  EXPECT_STREQ(buffer.GetString(), "\"\\u0000\"");
}

TEST(dds_DCPS_JsonValueWriter, write_string)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  EXPECT_TRUE(static_cast<ValueWriter&>(jvw).write_string("a string"));
  EXPECT_STREQ(buffer.GetString(), "\"a string\"");
}

TEST(dds_DCPS_JsonValueWriter, write_enum)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
  const ListEnumHelper::Pair enum_pairs[] = {
    {"label_1", 5},
    {"label_2", 6},
    {0, 0}
  };
  const ListEnumHelper enum_helper(enum_pairs);
  EXPECT_TRUE(jvw.write_enum(5, enum_helper));
  EXPECT_STREQ(buffer.GetString(), "\"label_1\"");
}

TEST(dds_DCPS_JsonValueWriter, write_bitmask)
{
  Buffer buffer;
  Writer writer(buffer);
  JsonValueWriter<Writer> jvw(writer);
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
  const MapBitmaskHelper bitmask_helper(bitmask_pairs, 7, OpenDDS::XTypes::TK_UINT8);
  EXPECT_TRUE(jvw.write_bitmask(1 << 1 | 1 << 3 | 1 << 5, bitmask_helper));
  EXPECT_STREQ(buffer.GetString(), "\"flag1|flag3|flag5\"");
}

#endif
