#include "DynamicDataTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/DynamicData.h>

#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

#include <iostream>

using namespace OpenDDS;

const DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);

TEST(Mutable, ReadValueFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0xbc, // +4=4 dheader
    0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +4+4=12 my_enum
    0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0a, // +4+4=20 int_32
    0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0b, // +4+4=28 uint_32
    0x00,0x00,0x00,0x03, 0x05, (0), (0), (0), // +4+1+(3)=36 int_8
    0x00,0x00,0x00,0x04, 0x06, (0), (0), (0), // +4+1+(3)=44 uint_8
    0x10,0x00,0x00,0x05, 0x11,0x11, (0), (0), // +4+2+(2)=52 int_16
    0x10,0x00,0x00,0x06, 0x22,0x22, (0), (0), // +4+2+(2)=60 uint_16
    0x30,0x00,0x00,0x07, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=72 int_64
    0x30,0x00,0x00,0x08, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=84 uint_64
    0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00, // +4+4=92 float_32
    0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+8=104 float_64
    0x40,0x00,0x00,0x0b, 0x00,0x00,0x00,0x10,
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +4+4+16=128 float_128
    0x00,0x00,0x00,0x0c, 'a', (0), (0), (0),  // +4+1+(3)=136 char_8
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0e, 0xff, (0), (0), (0), // +4+1+(3)=152 byte
    0x00,0x00,0x00,0x0f, 0x01, (0), (0), (0), // +4+1+(3)=160 bool
    0x30,0x00,0x00,0x10, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=172 str
    0x40,0x00,0x00,0x11, 0x00,0x00,0x00,0x0c,
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +4+4+12=192 swtr
  };

  SingleValueStruct expected = {
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

  DCPS::String str;
  ret = data.get_string_value(str, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str.c_str());

  ret = data.get_complex_value(nested_dd, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_string_value(str, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str.c_str());

#ifdef DDS_HAS_WCHAR
  DCPS::WString wstr;
  ret = data.get_wstring_value(wstr, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr.c_str());

  ret = data.get_complex_value(nested_dd, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_wstring_value(wstr, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr.c_str());
#endif
}

TEST(Mutable, ReadValueFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0a // +8=20 int_32
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
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // +8=12 discriminator
      0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0b // +8=20 uint_32
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
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x02, // +8=12 discriminator
      0x00,0x00,0x00,0x03, 0x7f // +5=17 int_8
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
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +8=12 discriminator
      0x00,0x00,0x00,0x04, 0xff // +5=17 uint_8
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
      0x00,0x00,0x00,0x12, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x04, // +8=12 discriminator
      0x10,0x00,0x00,0x05, 0x00,0x09 // +6=18 int_16
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
      0x00,0x00,0x00,0x12, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x05, // +8=12 discriminator
      0x10,0x00,0x00,0x06, 0x00,0x05 // +6=18 uint_16
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
      0x00,0x00,0x00,0x18, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x06, // +8=12 discriminator
      0x30,0x00,0x00,0x07, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +12=24 int_64
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
      0x00,0x00,0x00,0x18, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x07, // +8=12 discriminator
      0x30,0x00,0x00,0x08, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +12=24 uint_64
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
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x08, // +8=12 discriminator
      0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00 // +8=20 float_32
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
      0x00,0x00,0x00,0x18, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x09, // +8=12 discriminator
      0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 float_64
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
      0x00,0x00,0x00,0x18, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0a, // +8=12 discriminator
      0x40,0x00,0x00,0x0b, 0x00,0x00,0x00,0x10,
      0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0     // +4+4+16=36 float_128
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
      0x00,0x00,0x00,0x11, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0b, // +8=12 discriminator
      0x00,0x00,0x00,0x0c, 'a'     // +5=17 char_8
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
      0x00,0x00,0x00,0x12, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0c, // +8=12 discriminator
      0x10,0x00,0x00,0x0d, 0x00,0x61 // +6=18 char_16
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

TEST(Mutable, ReadValueFromSequence)
{
}

TEST(Mutable, ReadValueFromArray)
{
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
