#include "DynamicDataTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/DynamicData.h>

#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

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
    0x00,0x00,0x00,0xc2, // +4=4 dheader
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
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x0c, // +4+4=168 nested_struct
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=180 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=198 swtr
  };

  SingleValueStruct expected = {
    E_UINT8,
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
    {12},
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

  ACE_CDR::Long l;
  ret = data.get_complex_value(nested_dd, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_int32_value(l, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.nested_struct.l, l);

  XTypes::DynamicData nested_dd2;
  ret = nested_dd.get_complex_value(nested_dd2, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_dd2.get_int32_value(l, random_id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.nested_struct.l, l);

  ACE_CDR::Char* str = 0;
  ret = data.get_string_value(str, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);

  ret = data.get_complex_value(nested_dd, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_string_value(str, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);

  ACE_CDR::WChar* wstr = 0;
  ret = data.get_wstring_value(wstr, 18);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);

  ret = data.get_complex_value(nested_dd, 18);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd.get_wstring_value(wstr, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);
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
      0x00,0x00,0x00,0x0e, // +4=4 dheader
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
      0x00,0x00,0x00,0x0e, // +4=4 dheader
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
      0x00,0x00,0x00,0x14, // +4=4 dheader
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
      0x00,0x00,0x00,0x14, // +4=4 dheader
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
      0x00,0x00,0x00,0x10, // +4=4 dheader
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
      0x00,0x00,0x00,0x14, // +4=4 dheader
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
      0x00,0x00,0x00,0x20, // +4=4 dheader
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
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0b, // +8=12 discriminator
      0x00,0x00,0x00,0x0c, 'a' // +5=17 char_8
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
      0x00,0x00,0x00,0x0e, // +4=4 dheader
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
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0d, // +8=12 discriminator
      0x00,0x00,0x00,0x0e, 0xff // +5=17 byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Octet byte;
    DDS::ReturnCode_t ret = data.get_byte_value(byte, 14);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Octet(255), byte);
    ret = data.get_byte_value(byte, 10);
    EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0e, // +8=12 discriminator
      0x00,0x00,0x00,0x0f, 0x01 // +5=17 bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Boolean bool_;
    DDS::ReturnCode_t ret = data.get_boolean_value(bool_, 15);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(true, bool_);
    ret = data.get_boolean_value(bool_, 10);
    EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0f, // +8=12 discriminator
      0x30,0x00,0x00,0x10, 0x00,0x00,0x00,0x04,'a','b','c','\0' // +12=24 str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Char* str = 0;
    DDS::ReturnCode_t ret = data.get_string_value(str, 16);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_STREQ("abc", str);
    ret = data.get_string_value(str, 10);
    EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  }
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x1c, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x10, // +8=12 discriminator
      0x40,0x00,0x00,0x11, 0x00,0x00,0x00,0x0c,
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // +8+12=32 wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::WChar* wstr = 0;
    DDS::ReturnCode_t ret = data.get_wstring_value(wstr, 17);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_STREQ(wstr, L"abc");
    ret = data.get_wstring_value(wstr, 10);
    EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  }
  {
    // Read default member
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x11, // +8=12 discriminator
      0x20,0x00,0x00,0x12, 0x00,0x00,0x00,0x09 // +8=20 my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicData data(&msg, xcdr2, dt);

    ACE_CDR::Long my_enum;
    DDS::ReturnCode_t ret = data.get_int32_value(my_enum, 18);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    EXPECT_EQ(ACE_CDR::Long(9), my_enum);
    ret = data.get_int32_value(my_enum, 11);
    EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  }
}

template<typename SequenceTypeA, typename SequenceTypeB>
void check_primitive_sequences(const SequenceTypeA& a, const SequenceTypeB& b)
{
  EXPECT_EQ(a.length(), b.length());
  for (unsigned i = 0; i < a.length(); ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

void check_float128(ACE_CDR::LongDouble a, ACE_CDR::LongDouble b)
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  EXPECT_STREQ((const char*)&a, (const char*)&b);
#else
  EXPECT_STREQ((const char*)a.ld, (const char*)b.ld);
#endif
}

void check_float128_sequences(const Float128Seq& a, const CORBA::LongDoubleSeq& b)
{
  EXPECT_EQ(a.length(), b.length());
  for (unsigned i = 0; i < a.length(); ++i) {
    check_float128(a[i], b[i]);
  }
}

template<typename StringSeqA, typename StringSeqB>
void check_string_sequences(const StringSeqA& a, const StringSeqB& b)
{
  EXPECT_EQ(a.length(), b.length());
  for (unsigned i = 0; i < a.length(); ++i) {
    EXPECT_STREQ(a[i], b[i]);
  }
}

TEST(Mutable, ReadValueFromSequence)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0x00,0x00,0x01,0x56, // +4=4 dheader
    0x40,0,0,0, 0,0,0,16, 0,0,0,12, 0,0,0,2, 0,0,0,1, 0,0,0,2, // +24=28 my_enums
    0x60,0,0,1, 0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +20=48 int_32s
    0x60,0,0,2, 0,0,0,2, 0,0,0,10, 0,0,0,11, // +16=64 uint_32s
    0x50,0,0,3, 0,0,0,3, 12,13,14,(0), // +12=76 int_8s
    0x50,0,0,4, 0,0,0,2, 15,16,(0),(0), // +12=88 uint_8s
    0x30,0,0,5, 0,0,0,2, 0,1,0,2, // +12=100 int_16s
    0x40,0,0,6, 0,0,0,10, 0,0,0,3, 0,3,0,4,0,5,(0),(0), // +20=120 uint_16s
    0x70,0,0,7, 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +24=144 int_64s
    0x70,0,0,8, 0,0,0,1, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +16=160 uint_64s
    0x60,0,0,9, 0,0,0,1, 0x3f,0x80,0x00,0x00, // +12=172 float_32s
    0x70,0,0,10, 0,0,0,1, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +16=188 float_64s
    0x40,0,0,11, 0,0,0,20, 0,0,0,1, 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // +28=216 float_128s
    0x40,0,0,12, 0,0,0,6, 0,0,0,2, 'a','b',(0),(0), // +16=232 char_8s
    0x40,0,0,13, 0,0,0,10, 0,0,0,3, 0,0x63,0,0x64,0,0x65,(0),(0), // +20=252 char_16s
    0x40,0,0,14, 0,0,0,6, 0,0,0,2, 0xee,0xff,(0),(0), // +16=268 byte_s
    0x40,0,0,15, 0,0,0,5, 0,0,0,1, 1,(0),(0),(0), // +16=284 bool_s
    0x40,0,0,16, 0,0,0,16, 0,0,0,12, 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +24=308 str_s
    0x40,0,0,17, 0,0,0,30, 0,0,0,26, 0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +38=346 wstr_s
  };

  SequenceStruct expected;
  expected.my_enums.length(2);
  expected.my_enums[0] = E_UINT32; expected.my_enums[1] = E_INT8;
  expected.int_32s.length(3);
  expected.int_32s[0] = 3; expected.int_32s[1] = 4; expected.int_32s[2] = 5;
  expected.uint_32s.length(2);
  expected.uint_32s[0] = 10; expected.uint_32s[1] = 11;
  expected.int_8s.length(3);
  expected.int_8s[0] = 12; expected.int_8s[1] = 13; expected.int_8s[2] = 14;
  expected.uint_8s.length(2);
  expected.uint_8s[0] = 15; expected.uint_8s[1] = 16;
  expected.int_16s.length(2);
  expected.int_16s[0] = 1; expected.int_16s[1] = 2;
  expected.uint_16s.length(3);
  expected.uint_16s[0] = 3; expected.uint_16s[1] = 4; expected.uint_16s[2] = 5;
  expected.int_64s.length(2);
  expected.int_64s[0] = 0x7ffffffffffffffe; expected.int_64s[1] = 0x7fffffffffffffff;
  expected.uint_64s.length(1);
  expected.uint_64s[0] = 0xffffffffffffffff;
  expected.float_32s.length(1);
  expected.float_32s[0] = 1.0f;
  expected.float_64s.length(1);
  expected.float_64s[0] = 1.0;
  expected.float_128s.length(1);
  expected.float_128s[0] = 1.0L;
  expected.char_8s.length(2);
  expected.char_8s[0] = 'a'; expected.char_8s[1] = 'b';
  expected.char_16s.length(3);
  expected.char_16s[0] = 'c'; expected.char_16s[1] = 'd'; expected.char_16s[2] = 'e';
  expected.byte_s.length(2);
  expected.byte_s[0] = 0xee; expected.byte_s[1] = 0xff;
  expected.bool_s.length(1);
  expected.bool_s[0] = 1;
  expected.str_s.length(1);
  expected.str_s[0] = "abc";
  expected.wstr_s.length(2);
  expected.wstr_s[0] = L"def"; expected.wstr_s[1] = L"ghi";

  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicData data(&msg, xcdr2, dt);

  CORBA::LongSeq my_enums;
  DDS::ReturnCode_t ret = data.get_int32_values(my_enums, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.my_enums, my_enums);

  XTypes::DynamicData complex;
  ret = data.get_complex_value(complex, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  my_enums.length(0);
  ret = data.get_int32_values(my_enums, 0);
  check_primitive_sequences(expected.my_enums, my_enums);

  ACE_CDR::ULong count = complex.get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(2), count);
  XTypes::MemberId id = complex.get_member_id_at_index(0);
  EXPECT_EQ(XTypes::MemberId(0), id);
  ACE_CDR::Long some_enum;
  ret = complex.get_int32_value(some_enum, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.my_enums[0], some_enum);

  CORBA::LongSeq int_32s;
  ret = data.get_int32_values(int_32s, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_32s, int_32s);

  ret = data.get_complex_value(complex, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  count = complex.get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(3), count);
  id = complex.get_member_id_at_index(1);
  ACE_CDR::Long some_int32;
  ret = complex.get_int32_value(some_int32, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.int_32s[1], some_int32);

  CORBA::ULongSeq uint_32s;
  ret = data.get_uint32_values(uint_32s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_32s, uint_32s);
  ret = data.get_uint32_values(uint_32s, 3);
  EXPECT_EQ(DDS::RETCODE_ERROR, ret);

  CORBA::Int8Seq int_8s;
  ret = data.get_int8_values(int_8s, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_8s, int_8s);

  CORBA::UInt8Seq uint_8s;
  ret = data.get_uint8_values(uint_8s, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_8s, uint_8s);

  CORBA::ShortSeq int_16s;
  ret = data.get_int16_values(int_16s, 5);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_16s, int_16s);

  CORBA::UShortSeq uint_16s;
  ret = data.get_uint16_values(uint_16s, 6);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_16s, uint_16s);

  CORBA::LongLongSeq int_64s;
  ret = data.get_int64_values(int_64s, 7);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_64s, int_64s);

  CORBA::ULongLongSeq uint_64s;
  ret = data.get_uint64_values(uint_64s, 8);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_64s, uint_64s);

  CORBA::FloatSeq float_32s;
  ret = data.get_float32_values(float_32s, 9);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.float_32s, float_32s);

  CORBA::DoubleSeq float_64s;
  ret = data.get_float64_values(float_64s, 10);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.float_64s, float_64s);

  CORBA::LongDoubleSeq float_128s;
  ret = data.get_float128_values(float_128s, 11);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_float128_sequences(expected.float_128s, float_128s);

  CORBA::CharSeq char_8s;
  ret = data.get_char8_values(char_8s, 12);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.char_8s, char_8s);

  CORBA::WCharSeq char_16s;
  ret = data.get_char16_values(char_16s, 13);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.char_16s, char_16s);

  CORBA::OctetSeq byte_s;
  ret = data.get_byte_values(byte_s, 14);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.byte_s, byte_s);

  CORBA::BooleanSeq bool_s;
  ret = data.get_boolean_values(bool_s, 15);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.bool_s, bool_s);

  CORBA::StringSeq str_s;
  ret = data.get_string_values(str_s, 16);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_string_sequences(expected.str_s, str_s);

  CORBA::WStringSeq wstr_s;
  ret = data.get_wstring_values(wstr_s, 17);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_string_sequences(expected.wstr_s, wstr_s);

  ret = data.get_complex_value(complex, 17);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  count = complex.get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(2), count);
  id = complex.get_member_id_at_index(1);
  ACE_CDR::WChar* some_wstr = 0;
  ret = complex.get_wstring_value(some_wstr, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_STREQ(expected.wstr_s[1].in(), some_wstr);
}

TEST(Appendable, SkipNestedStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char appendable_struct[] = {
    0x00,0x00,0x00,0x18,// +4=4 dheader
    0x00,0x00,0x00,0x0a, // +4=8 l
    0x00,0x00,0x00,0x14, // +4=12 nested_struct1
    0x00,0x0c,(0),(0), // +4=16 s
    0x00,0x00,0x00,0x15, // +4=20 nested_struct2
    0x0f,(0),(0),(0), // +4=24 c
    0x00,0x00,0x00,0x01 // +4=28 l2
  };
  AppendableStruct expected = { 10, {20}, 12, {21}, 15, 1 };

  ACE_Message_Block msg(128);
  msg.copy((const char*)appendable_struct, sizeof(appendable_struct));
  XTypes::DynamicData data(&msg, xcdr2, dt);

  ACE_CDR::Int16 s;
  DDS::ReturnCode_t ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  XTypes::DynamicData complex;
  ret = data.get_complex_value(complex, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = complex.get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.nested_struct2.l, l);

  // Issue(sonndinh): When skipping nested_struct2, it doesn't skip the 2 bytes padding ahead of it.
  ACE_CDR::Int8 c;
  DDS::ReturnCode_t ret = data.get_int8_value(c, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.c, c);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
