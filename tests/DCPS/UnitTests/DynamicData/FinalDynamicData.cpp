#include "DynamicDataTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/DynamicData.h>

#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

#include <iostream>

using namespace OpenDDS;

const DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);

TEST(Final, ReadValueFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SingleValueStructFinal_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SingleValueStructFinal_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x03, // +4=8 my_enum
    0x00,0x00,0x00,0x0a, // +4=12 int_32
    0x00,0x00,0x00,0x0b, // +4=16 uint_32
    0x05, // +1=17 int_8
    0x06, // +1=18 uint_8
    0x11,0x11, // +2 =20 int_16
    0x22,0x22, // +2 =22 uint_16
    (0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+8=32 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=40 uint_64
    0x3f,0x80,0x00,0x00, // +4=44 float_32
    0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +8=52 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=68 float_128
    'a',  // +1=69 char_8
    (0),0x00,0x61, // +(1)+2=72 char_16
    0xff, // +1=73 byte
    0x01, // +1=74 bool
    (0),(0),0x00,0x00,0x00,0x04, 'a','b','c','\0', // +(2)+8=84 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=96 swtr
  };

  SingleValueStructFinal expected = {
    UINT8,
    10,
    11,
    5,
    6,
    0x1111,
    0x2222,
    0x7fffffffffffffff,
    0xffffffffffffffff,
    1.0f,
    1.0,
    1.0L,
    'a',
    0x0061,
    0xff,
    true,
    "abc",
    L"abc"
  };

  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicData data(&msg, xcdr2, dt);

  // For reading value indirectly through a nested DynamicData object.
  XTypes::MemberId random_id = 111;
  XTypes::DynamicData nested_dd;

  ACE_CDR::Long my_enum;
  DDS::ReturnCode_t ret = data.get_int32_value(my_enum, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  ret = data.get_complex_value(nested_dd, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int32_value(my_enum, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  ACE_CDR::Long int_32;
  ret = data.get_int32_value(int_32, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  ret = data.get_complex_value(nested_dd, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int32_value(int_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  ACE_CDR::ULong uint_32;
  ret = data.get_uint32_value(uint_32, 2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_32, uint_32);

  ret = data.get_complex_value(nested_dd, 2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_uint32_value(uint_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_32, uint_32);

  ACE_CDR::Int8 int_8;
  ret = data.get_int8_value(int_8, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_8, int_8);

  ret = data.get_complex_value(nested_dd, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int8_value(int_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_8, int_8);

  ACE_CDR::UInt8 uint_8;
  ret = data.get_uint8_value(uint_8, 4);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_8, uint_8);

  ret = data.get_complex_value(nested_dd, 4);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_uint8_value(uint_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_8, uint_8);

  ACE_CDR::Int16 int_16;
  ret = data.get_int16_value(int_16, 5);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_16, int_16);

  ret = data.get_complex_value(nested_dd, 5);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int16_value(int_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_16, int_16);

  ACE_CDR::UInt16 uint_16;
  ret = data.get_uint16_value(uint_16, 6);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_16, uint_16);

  ret = data.get_complex_value(nested_dd, 6);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_uint16_value(uint_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_16, uint_16);

  ACE_CDR::Int64 int_64;
  ret = data.get_int64_value(int_64, 7);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_64, int_64);

  ret = data.get_complex_value(nested_dd, 7);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int64_value(int_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_64, int_64);

  ACE_CDR::UInt64 uint_64;
  ret = data.get_uint64_value(uint_64, 8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_64, uint_64);

  ret = data.get_complex_value(nested_dd, 8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_uint64_value(uint_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_64, uint_64);

  ACE_CDR::Float float_32;
  ret = data.get_float32_value(float_32, 9);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_32, float_32);

  ret = data.get_complex_value(nested_dd, 9);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_float32_value(float_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_32, float_32);

  ACE_CDR::Double float_64;
  ret = data.get_float64_value(float_64, 10);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_64, float_64);

  ret = data.get_complex_value(nested_dd, 10);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_float64_value(float_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_64, float_64);

  ACE_CDR::LongDouble float_128;
  ret = data.get_float128_value(float_128, 11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#if ACE_SIZEOF_LONG_DOUBLE == 16
  EXPECT_STREQ((const char*)&expected.float_128, (const char*)&float_128);
#else
  EXPECT_STREQ((const char*)expected.float_128.ld, (const char*)float_128.ld);
#endif

  ret = data.get_complex_value(nested_dd, 11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_float128_value(float_128, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  #if ACE_SIZEOF_LONG_DOUBLE == 16
  EXPECT_STREQ((const char*)&expected.float_128, (const char*)&float_128);
#else
  EXPECT_STREQ((const char*)expected.float_128.ld, (const char*)float_128.ld);
#endif

  ACE_CDR::Char char_8;
  ret = data.get_char8_value(char_8, 12);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_8, char_8);

  ret = data.get_complex_value(nested_dd, 12);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_char8_value(char_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_8, char_8);

  ACE_CDR::WChar char_16;
  ret = data.get_char16_value(char_16, 13);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_16, char_16);

  ret = data.get_complex_value(nested_dd, 13);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_char16_value(char_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_16, char_16);

  ACE_CDR::Octet byte;
  ret = data.get_byte_value(byte, 14);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.byte, byte);

  ret = data.get_complex_value(nested_dd, 14);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_byte_value(byte, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.byte, byte);

  ACE_CDR::Boolean bool_;
  ret = data.get_boolean_value(bool_, 15);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected._cxx_bool, bool_);

  ret = data.get_complex_value(nested_dd, 15);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_boolean_value(bool_, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected._cxx_bool, bool_);

  ACE_CDR::Char* str;
  ret = data.get_string_value(str, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);

  ret = data.get_complex_value(nested_dd, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_string_value(str, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);

  ACE_CDR::WChar* wstr;
  ret = data.get_wstring_value(wstr, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);

  ret = data.get_complex_value(nested_dd, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_wstring_value(wstr, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);
}

TEST(Final, ReadValueFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SingleValueUnionFinal_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SingleValueUnionFinal_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x00, // +4=4 discriminator
      0x00,0x00,0x00,0x0a  // +4=8 int_32
    };

    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Long int_32;
    DDS::ReturnCode_t ret = data.get_int32_value(int_32, 1);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Long(10), int_32);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x01, // +4=4 discriminator
      0x00,0x00,0x00,0x0b // +4=8 uint_32
    };

    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::ULong uint_32;
    DDS::ReturnCode_t ret = data.get_uint32_value(uint_32, 2);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::ULong(11), uint_32);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x02, // +4=4 discriminator
      0x7f // +1=5 int_8
    };

    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Int8 int_8;
    DDS::ReturnCode_t ret = data.get_int8_value(int_8, 3);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Int8(127), int_8);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x03, // +4=4 discriminator
      0xff // +1=5 uint_8
    };

    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::UInt8 uint_8;
    DDS::ReturnCode_t ret = data.get_uint8_value(uint_8, 4);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::UInt8(255), uint_8);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x04, // +4=4 discriminator
      0x00,0x09 // +2=6 int_16
    };

    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Short int_16;
    DDS::ReturnCode_t ret = data.get_int16_value(int_16, 5);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Short(9), int_16);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x05, // +4=4 discriminator
      0x00,0x05 // +2=6 uint_16
    };

    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::UShort uint_16;
    DDS::ReturnCode_t ret = data.get_uint16_value(uint_16, 6);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::UShort(5), uint_16);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x06, // +4=4 discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +8=12 int_64
    };

    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::LongLong int_64;
    DDS::ReturnCode_t ret = data.get_int64_value(int_64, 7);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::LongLong(254), int_64);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x07, // +4=4 discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +8=12 uint_64
    };

    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::ULongLong uint_64;
    DDS::ReturnCode_t ret = data.get_uint64_value(uint_64, 8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::ULongLong(255), uint_64);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x08, // +4=4 discriminator
      0x3f,0x80,0x00,0x00 // +4=8 float_32
    };

    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Float float_32;
    DDS::ReturnCode_t ret = data.get_float32_value(float_32, 9);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Float(1.0f), float_32);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x09, // +4=4 discriminator
      0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +8=12 float_64
    };

    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Double float_64;
    DDS::ReturnCode_t ret = data.get_float64_value(float_64, 10);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Double(1.0), float_64);
  }
  {
    unsigned char float128_union[] = {
      0x00,0x00,0x00,0x0a, // +4=4 discriminator
      0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0     // 16=20 float_128
    };

    msg.reset();
    msg.copy((const char*)float128_union, sizeof(float128_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::LongDouble float_128;
    DDS::ReturnCode_t ret = data.get_float128_value(float_128, 11);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_CDR::LongDouble expected = 1.0L;
#if ACE_SIZEOF_LONG_DOUBLE == 16
    EXPECT_STREQ((const char*)&expected, (const char*)&float_128);
#else
    EXPECT_STREQ((const char*)expected.ld, (const char*)float_128.ld);
#endif
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x0b, // +4=4 discriminator
      'a'     // +1=5 char_8
    };

    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Char char_8;
    DDS::ReturnCode_t ret = data.get_char8_value(char_8, 12);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Char('a'), char_8);
  }
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x0c, // +4=4 discriminator
      0x00,0x61 // +2=6 char_16
    };

    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::WChar char_16;
    DDS::ReturnCode_t ret = data.get_char16_value(char_16, 13);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::WChar(L'a'), char_16);
  }
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
