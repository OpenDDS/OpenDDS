#include "DynamicDataTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicType.h>
#include <dds/DCPS/XTypes/DynamicData.h>

#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

#include <iostream>

using namespace OpenDDS;

XTypes::TypeLookupService_rch tls = DCPS::make_rch<XTypes::TypeLookupService>();

TEST(Mutable, ReadSingleValueFromStruct)
{
  DCPS::Encoding encoding(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::SingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::SingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());
  tls->add(type_map.begin(), type_map.end());
  XTypes::DynamicType_rch dt = tls->complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0xbc, // +4=4 delimiter
    0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x03, // +4+4=12 my_enum
    0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, // +4+4=20 int_32
    0x20,0x00,0x00,0x03, 0x00,0x00,0x00,0x0b, // +4+4=28 uint_32
    0x00,0x00,0x00,0x04, 0x05, (0), (0), (0), // +4+1+(3)=36 int_8
    0x00,0x00,0x00,0x05, 0x06, (0), (0), (0), // +4+1+(3)=44 uint_8
    0x10,0x00,0x00,0x06, 0x11,0x11, (0), (0), // +4+2+(2)=52 int_16
    0x10,0x00,0x00,0x07, 0x22,0x22, (0), (0), // +4+2+(2)=60 uint_16
    0x30,0x00,0x00,0x08, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=72 int_64
    0x30,0x00,0x00,0x09, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=84 uint_64
    0x20,0x00,0x00,0x0a, 0x3f,0x80,0x00,0x00, // +4+4=92 float_32
    0x30,0x00,0x00,0x0b, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+8=104 float_64
    0x40,0x00,0x00,0x0c, 0x00,0x00,0x00,0x10,
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +4+4+16=128 float_128
    0x00,0x00,0x00,0x0d, 'a', (0), (0), (0),  // +4+1+(3)=136 char_8
    0x10,0x00,0x00,0x0e, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0f, 0xff, (0), (0), (0), // +4+1+(3)=152 byte
    0x00,0x00,0x00,0x10, 0x01, (0), (0), (0), // +4+1+(3)=160 bool
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=172 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0c,
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +4+4+12=192 swtr
  };

  SingleValueStruct expected {
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
  XTypes::DynamicData data(&msg, encoding, dt);

  ACE_CDR::Long my_enum;
  DDS::ReturnCode_t ret = data.get_int32_value(my_enum, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  ACE_CDR::Long int_32;
  ret = data.get_int32_value(int_32, 2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  ACE_CDR::ULong uint_32;
  ret = data.get_uint32_value(uint_32, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_32, uint_32);

  ACE_CDR::Int8 int_8;
  ret = data.get_int8_value(int_8, 4);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_8, int_8);

  ACE_CDR::UInt8 uint_8;
  ret = data.get_uint8_value(uint_8, 5);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_8, uint_8);

  ACE_CDR::Int16 int_16;
  ret = data.get_int16_value(int_16, 6);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_16, int_16);

  ACE_CDR::UInt16 uint_16;
  ret = data.get_uint16_value(uint_16, 7);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_16, uint_16);

  ACE_CDR::Int64 int_64;
  ret = data.get_int64_value(int_64, 8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_64, int_64);

  ACE_CDR::UInt64 uint_64;
  ret = data.get_uint64_value(uint_64, 9);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_64, uint_64);

  ACE_CDR::Float float_32;
  ret = data.get_float32_value(float_32, 10);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_32, float_32);

  ACE_CDR::Double float_64;
  ret = data.get_float64_value(float_64, 11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_64, float_64);

  ACE_CDR::LongDouble float_128;
  ret = data.get_float128_value(float_128, 12);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#if ACE_SIZEOF_LONG_DOUBLE == 16
  EXPECT_STREQ((const char*)&expected.float_128, (const char*)&float_128);
#else
  EXPECT_STREQ((const char*)expected.float_128.ld, (const char*)float_128.ld);
#endif

  ACE_CDR::Char char_8;
  ret = data.get_char8_value(char_8, 13);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_8, char_8);

  ACE_CDR::WChar char_16;
  ret = data.get_char16_value(char_16, 14);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_16, char_16);

  ACE_CDR::Octet byte;
  ret = data.get_byte_value(byte, 15);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.byte, byte);

  ACE_CDR::Boolean bool_;
  ret = data.get_boolean_value(bool_, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected._cxx_bool, bool_);

  DCPS::String str;
  ret = data.get_string_value(str, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str.c_str());

#ifdef DDS_HAS_WCHAR
  DCPS::WString wstr;
  ret = data.get_wstring_value(wstr, 18);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr.c_str());
#endif
}

TEST(Mutable, ReadSingleValueFromUnion)
{
}

TEST(Mutable, ReadSingleValueFromSequence)
{
}

TEST(Mutable, ReadSingleValueFromArray)
{
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
