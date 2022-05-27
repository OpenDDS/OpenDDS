#ifndef OPENDDS_SAFETY_PROFILE

#include "../../../DynamicDataTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>

#include <dds/DCPS/SafetyProfileStreams.h>

#include <gtest/gtest.h>

using namespace OpenDDS;

const DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);
const DCPS::Encoding xcdr1(DCPS::Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);

void set_float128_value(ACE_CDR::LongDouble& a)
{
#if ACE_BIG_ENDIAN
  unsigned char value[] = { 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
#else
  unsigned char value[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0xff,0x3f };
#endif

#if ACE_SIZEOF_LONG_DOUBLE == 16
  ACE_OS::memcpy((unsigned char*)&a, (unsigned char*)value, 16);
#else
  ACE_OS::memcpy((unsigned char*)a.ld, (unsigned char*)value, 16);
#endif
}

void check_float128(const ACE_CDR::LongDouble& a, const ACE_CDR::LongDouble& b)
{
#if ACE_SIZEOF_LONG_DOUBLE == 16
  EXPECT_EQ(0, ACE_OS::memcmp((const void*)&a, (const void*)&b, 16));
#else
  EXPECT_EQ(0, ACE_OS::memcmp((const void*)a.ld, (const void*)b.ld, 16));
#endif
}

template<typename SequenceTypeA, typename SequenceTypeB>
void check_primitive_sequences(const SequenceTypeA& a, const SequenceTypeB& b)
{
  EXPECT_EQ(a.length(), b.length());
  for (unsigned i = 0; i < a.length(); ++i) {
    EXPECT_EQ(a[i], b[i]);
  }
}

void check_float128_sequences(const Float128Seq& a, const DDS::Float128Seq& b)
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

template<typename StructType>
void set_single_value_struct(StructType& a)
{
  a.my_enum = E_UINT8;
  a.int_32 = 10;
  a.uint_32 = 11;
  a.int_8 = 5;
  a.uint_8 = 6;
  a.int_16 = 0x1111;
  a.uint_16 = 0x2222;
  a.int_64 = 0x7fffffffffffffff;
  a.uint_64 = 0xffffffffffffffff;
  a.float_32 = 1.0f;
  a.float_64 = 1.0;
  set_float128_value(a.float_128);
  a.char_8 = 'a';
  a.byte = 0xff;
  a._cxx_bool = true;
  a.nested_struct.l = 12;
  a.str = "abc";
#ifdef DDS_HAS_WCHAR
  a.char_16 = 0x0061;
  a.wstr = L"abc";
#endif
}

template<typename StructType>
void verify_single_value_struct(DDS::DynamicData_ptr data)
{
  StructType expected;
  set_single_value_struct(expected);

  ACE_CDR::ULong count = data->get_item_count();
  EXPECT_EQ(count, ACE_CDR::ULong(19));

  ACE_CDR::Long my_enum;
  XTypes::MemberId id = data->get_member_id_at_index(0);
  EXPECT_EQ(id, ACE_CDR::ULong(0));
  DDS::ReturnCode_t ret = data->get_int32_value(my_enum, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  DDS::DynamicData_var nested_dd;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  const XTypes::MemberId random_id = 111;
  ret = nested_dd->get_int32_value(my_enum, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  ACE_CDR::Long int_32;
  id = data->get_member_id_at_index(1);
  EXPECT_EQ(id, ACE_CDR::ULong(1));
  ret = data->get_int32_value(int_32, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int32_value(int_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  ACE_CDR::ULong uint_32;
  id = data->get_member_id_at_index(2);
  EXPECT_EQ(id, ACE_CDR::ULong(2));
  ret = data->get_uint32_value(uint_32, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_32, uint_32);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_uint32_value(uint_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_32, uint_32);

  ACE_CDR::Int8 int_8;
  id = data->get_member_id_at_index(3);
  EXPECT_EQ(id, ACE_CDR::ULong(3));
  ret = data->get_int8_value(int_8, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_8, int_8);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int8_value(int_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_8, int_8);

  ACE_CDR::UInt8 uint_8;
  id = data->get_member_id_at_index(4);
  EXPECT_EQ(id, ACE_CDR::ULong(4));
  ret = data->get_uint8_value(uint_8, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_8, uint_8);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_uint8_value(uint_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_8, uint_8);

  ACE_CDR::Int16 int_16;
  id = data->get_member_id_at_index(5);
  EXPECT_EQ(id, ACE_CDR::ULong(5));
  ret = data->get_int16_value(int_16, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_16, int_16);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int16_value(int_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_16, int_16);

  ACE_CDR::UInt16 uint_16;
  id = data->get_member_id_at_index(6);
  EXPECT_EQ(id, ACE_CDR::ULong(6));
  ret = data->get_uint16_value(uint_16, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_16, uint_16);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_uint16_value(uint_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_16, uint_16);

  ACE_CDR::Int64 int_64;
  id = data->get_member_id_at_index(7);
  EXPECT_EQ(id, ACE_CDR::ULong(7));
  ret = data->get_int64_value(int_64, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_64, int_64);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int64_value(int_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_64, int_64);

  ACE_CDR::UInt64 uint_64;
  id = data->get_member_id_at_index(8);
  EXPECT_EQ(id, ACE_CDR::ULong(8));
  ret = data->get_uint64_value(uint_64, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_64, uint_64);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_uint64_value(uint_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.uint_64, uint_64);

  ACE_CDR::Float float_32;
  id = data->get_member_id_at_index(9);
  EXPECT_EQ(id, ACE_CDR::ULong(9));
  ret = data->get_float32_value(float_32, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_32, float_32);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_float32_value(float_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_32, float_32);

  ACE_CDR::Double float_64;
  id = data->get_member_id_at_index(10);
  EXPECT_EQ(id, ACE_CDR::ULong(10));
  ret = data->get_float64_value(float_64, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_64, float_64);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_float64_value(float_64, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.float_64, float_64);

  ACE_CDR::LongDouble float_128;
  id = data->get_member_id_at_index(11);
  EXPECT_EQ(id, ACE_CDR::ULong(11));
  ret = data->get_float128_value(float_128, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  check_float128(expected.float_128, float_128);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_float128_value(float_128, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  check_float128(expected.float_128, float_128);

  ACE_CDR::Char char_8;
  id = data->get_member_id_at_index(12);
  EXPECT_EQ(id, ACE_CDR::ULong(12));
  ret = data->get_char8_value(char_8, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_8, char_8);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_char8_value(char_8, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_8, char_8);

#ifdef DDS_HAS_WCHAR
  ACE_CDR::WChar char_16;
  id = data->get_member_id_at_index(13);
  EXPECT_EQ(id, ACE_CDR::ULong(13));
  ret = data->get_char16_value(char_16, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_16, char_16);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_char16_value(char_16, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.char_16, char_16);
#endif

  ACE_CDR::Octet byte;
  id = data->get_member_id_at_index(14);
  EXPECT_EQ(id, ACE_CDR::ULong(14));
  ret = data->get_byte_value(byte, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.byte, byte);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_byte_value(byte, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.byte, byte);

  ACE_CDR::Boolean bool_;
  id = data->get_member_id_at_index(15);
  EXPECT_EQ(id, ACE_CDR::ULong(15));
  ret = data->get_boolean_value(bool_, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected._cxx_bool, bool_);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_boolean_value(bool_, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected._cxx_bool, bool_);

  ACE_CDR::Long l;
  id = data->get_member_id_at_index(16);
  EXPECT_EQ(id, ACE_CDR::ULong(16));
  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  count = nested_dd->get_item_count();
  EXPECT_EQ(count, ACE_CDR::ULong(1));
  ret = nested_dd->get_int32_value(l, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.nested_struct.l, l);

  DDS::DynamicData_var nested_dd2;
  ret = nested_dd->get_complex_value(nested_dd2, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_dd2->get_int32_value(l, random_id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.nested_struct.l, l);

  ACE_CDR::Char* str = 0;
  id = data->get_member_id_at_index(17);
  EXPECT_EQ(id, ACE_CDR::ULong(17));
  ret = data->get_string_value(str, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);
  CORBA::string_free(str);

  str = 0;
  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_string_value(str, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.str.in(), str);
  CORBA::string_free(str);

#ifdef DDS_HAS_WCHAR
  ACE_CDR::WChar* wstr = 0;
  id = data->get_member_id_at_index(18);
  EXPECT_EQ(id, ACE_CDR::ULong(18));
  ret = data->get_wstring_value(wstr, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);
  CORBA::wstring_free(wstr);

  wstr = 0;
  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_wstring_value(wstr, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(expected.wstr.in(), wstr);
  CORBA::wstring_free(wstr);
#endif

  // Reading members out-of-order.
  ret = data->get_int32_value(int_32, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int32_value(int_32, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.int_32, int_32);

  ret = data->get_int32_value(my_enum, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);

  nested_dd = 0;
  ret = data->get_complex_value(nested_dd, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = nested_dd->get_int32_value(my_enum, random_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(expected.my_enum, my_enum);
}

void verify_index_mapping(DDS::DynamicData_ptr data)
{
  ACE_CDR::ULong count = data->get_item_count();
  EXPECT_EQ(count, ACE_CDR::ULong(17));

  XTypes::MemberId id = data->get_member_id_at_index(1);
  EXPECT_EQ(id, ACE_CDR::ULong(1));
  id = data->get_member_id_at_index(2);
  EXPECT_EQ(id, ACE_CDR::ULong(3));
  id = data->get_member_id_at_index(3);
  EXPECT_EQ(id, ACE_CDR::ULong(4));
  id = data->get_member_id_at_index(4);
  EXPECT_EQ(id, ACE_CDR::ULong(6));
  id = data->get_member_id_at_index(5);
  EXPECT_EQ(id, ACE_CDR::ULong(7));
}

template<typename StructType>
void set_sequence_value_struct(StructType& a)
{
  a.my_enums.length(2);
  a.my_enums[0] = E_UINT32; a.my_enums[1] = E_INT8;
  a.int_32s.length(3);
  a.int_32s[0] = 3; a.int_32s[1] = 4; a.int_32s[2] = 5;
  a.uint_32s.length(2);
  a.uint_32s[0] = 10; a.uint_32s[1] = 11;
  a.int_8s.length(3);
  a.int_8s[0] = 12; a.int_8s[1] = 13; a.int_8s[2] = 14;
  a.uint_8s.length(2);
  a.uint_8s[0] = 15; a.uint_8s[1] = 16;
  a.int_16s.length(2);
  a.int_16s[0] = 1; a.int_16s[1] = 2;
  a.uint_16s.length(3);
  a.uint_16s[0] = 3; a.uint_16s[1] = 4; a.uint_16s[2] = 5;
  a.int_64s.length(2);
  a.int_64s[0] = 0x7ffffffffffffffe; a.int_64s[1] = 0x7fffffffffffffff;
  a.uint_64s.length(1);
  a.uint_64s[0] = 0xffffffffffffffff;
  a.float_32s.length(1);
  a.float_32s[0] = 1.0f;
  a.float_64s.length(1);
  a.float_64s[0] = 1.0;
  a.float_128s.length(1);
  set_float128_value(a.float_128s[0]);
  a.char_8s.length(2);
  a.char_8s[0] = 'a'; a.char_8s[1] = 'b';
  a.byte_s.length(2);
  a.byte_s[0] = 0xee; a.byte_s[1] = 0xff;
  a.bool_s.length(1);
  a.bool_s[0] = 1;
  a.str_s.length(1);
  a.str_s[0] = "abc";
#ifdef DDS_HAS_WCHAR
  a.char_16s.length(3);
  a.char_16s[0] = 'c'; a.char_16s[1] = 'd'; a.char_16s[2] = 'e';
  a.wstr_s.length(2);
  a.wstr_s[0] = L"def"; a.wstr_s[1] = L"ghi";
#endif
}

template<typename StructType>
void verify_sequence_value_struct(DDS::DynamicData_ptr data)
{
  StructType expected;
  set_sequence_value_struct(expected);

  DDS::Int32Seq my_enums;
  DDS::ReturnCode_t ret = data->get_int32_values(my_enums, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.my_enums, my_enums);

  DDS::DynamicData_var complex;
  ret = data->get_complex_value(complex, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  my_enums.length(0);
  ret = data->get_int32_values(my_enums, 0);
  check_primitive_sequences(expected.my_enums, my_enums);

  ACE_CDR::ULong count = complex->get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(2), count);
  XTypes::MemberId id = complex->get_member_id_at_index(0);
  EXPECT_EQ(XTypes::MemberId(0), id);
  ACE_CDR::Long some_enum;
  ret = complex->get_int32_value(some_enum, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.my_enums[0], some_enum);

  DDS::Int32Seq int_32s;
  ret = data->get_int32_values(int_32s, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_32s, int_32s);

  complex = 0;
  ret = data->get_complex_value(complex, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  count = complex->get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(3), count);
  id = complex->get_member_id_at_index(1);
  ACE_CDR::Long some_int32;
  ret = complex->get_int32_value(some_int32, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.int_32s[1], some_int32);

  DDS::UInt32Seq uint_32s;
  ret = data->get_uint32_values(uint_32s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_32s, uint_32s);
  ret = data->get_uint32_values(uint_32s, 3);
  EXPECT_EQ(DDS::RETCODE_ERROR, ret);

  DDS::Int8Seq int_8s;
  ret = data->get_int8_values(int_8s, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_8s, int_8s);

  DDS::UInt8Seq uint_8s;
  ret = data->get_uint8_values(uint_8s, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_8s, uint_8s);

  DDS::Int16Seq int_16s;
  ret = data->get_int16_values(int_16s, 5);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_16s, int_16s);

  DDS::UInt16Seq uint_16s;
  ret = data->get_uint16_values(uint_16s, 6);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_16s, uint_16s);

  DDS::Int64Seq int_64s;
  ret = data->get_int64_values(int_64s, 7);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.int_64s, int_64s);

  DDS::UInt64Seq uint_64s;
  ret = data->get_uint64_values(uint_64s, 8);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.uint_64s, uint_64s);

  DDS::Float32Seq float_32s;
  ret = data->get_float32_values(float_32s, 9);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.float_32s, float_32s);

  DDS::Float64Seq float_64s;
  ret = data->get_float64_values(float_64s, 10);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.float_64s, float_64s);

  DDS::Float128Seq float_128s;
  ret = data->get_float128_values(float_128s, 11);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_float128_sequences(expected.float_128s, float_128s);

  DDS::CharSeq char_8s;
  ret = data->get_char8_values(char_8s, 12);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.char_8s, char_8s);

#ifdef DDS_HAS_WCHAR
  DDS::WcharSeq char_16s;
  ret = data->get_char16_values(char_16s, 13);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.char_16s, char_16s);
#endif

  DDS::ByteSeq byte_s;
  ret = data->get_byte_values(byte_s, 14);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.byte_s, byte_s);

  DDS::BooleanSeq bool_s;
  ret = data->get_boolean_values(bool_s, 15);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_primitive_sequences(expected.bool_s, bool_s);

  DDS::StringSeq str_s;
  ret = data->get_string_values(str_s, 16);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_string_sequences(expected.str_s, str_s);

#ifdef DDS_HAS_WCHAR
  DDS::WstringSeq wstr_s;
  ret = data->get_wstring_values(wstr_s, 17);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  check_string_sequences(expected.wstr_s, wstr_s);

  complex = 0;
  ret = data->get_complex_value(complex, 17);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  count = complex->get_item_count();
  EXPECT_EQ(ACE_CDR::ULong(2), count);
  id = complex->get_member_id_at_index(1);
  ACE_CDR::WChar* some_wstr = 0;
  ret = complex->get_wstring_value(some_wstr, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_STREQ(expected.wstr_s[1].in(), some_wstr);
  CORBA::wstring_free(some_wstr);
#endif
}

void verify_int32_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Long int_32;
  DDS::ReturnCode_t ret = data->get_int32_value(int_32, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Long(10), int_32);

  const XTypes::MemberId disc_id = data->get_member_id_at_index(0);
  EXPECT_EQ(XTypes::DISCRIMINATOR_ID, disc_id);
  ACE_CDR::Long disc_val;
  ret = data->get_int32_value(disc_val, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(disc_val, E_INT32);
  ACE_CDR::LongLong wrong_disc_val;
  ret = data->get_int64_value(wrong_disc_val, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);

  DDS::DynamicData_var disc_data;
  ret = data->get_complex_value(disc_data, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  const XTypes::MemberId any_id = 100;
  ret = disc_data->get_int32_value(disc_val, any_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(disc_val, E_INT32);
}

void verify_uint32_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::ULong uint_32;
  DDS::ReturnCode_t ret = data->get_uint32_value(uint_32, 2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::ULong(11), uint_32);

  const XTypes::MemberId disc_id = data->get_member_id_at_index(0);
  EXPECT_EQ(XTypes::DISCRIMINATOR_ID, disc_id);
  ACE_CDR::Long disc_val;
  ret = data->get_int32_value(disc_val, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(disc_val, E_UINT32);
  ACE_CDR::Short wrong_disc_val;
  ret = data->get_int16_value(wrong_disc_val, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);

  DDS::DynamicData_var disc_data;
  ret = data->get_complex_value(disc_data, disc_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  const XTypes::MemberId any_id = 100;
  ret = disc_data->get_int32_value(disc_val, any_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(disc_val, E_UINT32);
}

void verify_int8_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Int8 int_8;
  DDS::ReturnCode_t ret = data->get_int8_value(int_8, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Int8(127), int_8);
}

void verify_uint8_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::UInt8 uint_8;
  DDS::ReturnCode_t ret = data->get_uint8_value(uint_8, 4);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::UInt8(255), uint_8);
}

void verify_int16_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Short int_16;
  DDS::ReturnCode_t ret = data->get_int16_value(int_16, 5);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Short(9), int_16);
}

void verify_uint16_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::UShort uint_16;
  DDS::ReturnCode_t ret = data->get_uint16_value(uint_16, 6);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::UShort(5), uint_16);
}

void verify_int64_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::LongLong int_64;
  DDS::ReturnCode_t ret = data->get_int64_value(int_64, 7);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::LongLong(254), int_64);
}

void verify_uint64_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::ULongLong uint_64;
  DDS::ReturnCode_t ret = data->get_uint64_value(uint_64, 8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::ULongLong(255), uint_64);
}

void verify_float32_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Float float_32;
  DDS::ReturnCode_t ret = data->get_float32_value(float_32, 9);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Float(1.0f), float_32);
}

void verify_float64_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Double float_64;
  DDS::ReturnCode_t ret = data->get_float64_value(float_64, 10);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Double(1.0), float_64);
}

void verify_float128_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::LongDouble float_128;
  DDS::ReturnCode_t ret = data->get_float128_value(float_128, 11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ACE_CDR::LongDouble expected;
  set_float128_value(expected);
  check_float128(expected, float_128);
}

void verify_char8_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Char char_8;
  DDS::ReturnCode_t ret = data->get_char8_value(char_8, 12);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Char('a'), char_8);
}

#ifdef DDS_HAS_WCHAR
void verify_char16_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::WChar char_16;
  DDS::ReturnCode_t ret = data->get_char16_value(char_16, 13);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::WChar(L'a'), char_16);
}
#endif

void verify_byte_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Octet byte;
  DDS::ReturnCode_t ret = data->get_byte_value(byte, 14);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Octet(255), byte);
  ret = data->get_byte_value(byte, 10);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
}

void verify_bool_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Boolean bool_;
  DDS::ReturnCode_t ret = data->get_boolean_value(bool_, 15);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(true, bool_);
  ret = data->get_boolean_value(bool_, 10);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
}

void verify_string_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Char* str = 0;
  DDS::ReturnCode_t ret = data->get_string_value(str, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ("abc", str);
  CORBA::string_free(str);

  str = 0;
  ret = data->get_string_value(str, 10);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
}

#ifdef DDS_HAS_WCHAR
void verify_wstring_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::WChar* wstr = 0;
  DDS::ReturnCode_t ret = data->get_wstring_value(wstr, 17);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_STREQ(wstr, L"abc");
  CORBA::wstring_free(wstr);

  wstr = 0;
  ret = data->get_wstring_value(wstr, 10);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
}
#endif

void verify_enum_union(DDS::DynamicData_ptr data)
{
  ACE_CDR::Long my_enum;
  DDS::ReturnCode_t ret = data->get_int32_value(my_enum, 18);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  EXPECT_EQ(ACE_CDR::Long(9), my_enum);
  ret = data->get_int32_value(my_enum, 11);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
}

void verify_int32s_union(DDS::DynamicData_ptr data)
{
  DDS::Int32Seq int_32s;
  DDS::ReturnCode_t ret = data->get_int32_values(int_32s, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::ULong(2), int_32s.length());
  EXPECT_EQ(ACE_CDR::Long(10), int_32s[0]);
  EXPECT_EQ(ACE_CDR::Long(11), int_32s[1]);
}

void verify_uint32s_union(DDS::DynamicData_ptr data)
{
  DDS::UInt32Seq uint_32s;
  DDS::ReturnCode_t ret = data->get_uint32_values(uint_32s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::ULong(2), uint_32s.length());
  EXPECT_EQ(ACE_CDR::ULong(0xff), uint_32s[0]);
  EXPECT_EQ(ACE_CDR::ULong(0xffff), uint_32s[1]);
}

void verify_array_struct(DDS::DynamicData_ptr data)
{
  DDS::DynamicData_var array;
  DDS::ReturnCode_t ret = data->get_complex_value(array, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  XTypes::MemberId id = array->get_member_id_at_index(0);
  ret = array->get_int32_value(l, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::Long(0x12), l);
  id = array->get_member_id_at_index(1);
  ret = array->get_int32_value(l, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::Long(0x34), l);

  array = 0;
  ret = data->get_complex_value(array, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  id = array->get_member_id_at_index(0);
  ret = array->get_uint32_value(ul, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::ULong(0xff), ul);
  id = array->get_member_id_at_index(1);
  ret = array->get_uint32_value(ul, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::ULong(0xff), ul);

  array = 0;
  ret = data->get_complex_value(array, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Int8 i;
  id = array->get_member_id_at_index(0);
  ret = array->get_int8_value(i, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::Int8(0x01), i);
  id = array->get_member_id_at_index(1);
  ret = array->get_int8_value(i, id);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(ACE_CDR::Int8(0x02), i);
}

/////////////////////////// Mutable tests ///////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_ReadValueFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

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
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_single_value_struct<MutableSingleValueStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_StructWithOptionalMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::CompleteTypeObject cto = it->second.complete;
  // IDL compiler hasn't supported @optional yet,
  // so these 2 members are set to be optional manually.
  cto.struct_type.member_seq[2].common.member_flags |= XTypes::IS_OPTIONAL;
  cto.struct_type.member_seq[5].common.member_flags |= XTypes::IS_OPTIONAL;
  DDS::DynamicType_var dt = tls.complete_to_dynamic(cto, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0xb2, // +4=4 dheader
    0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +4+4=12 my_enum
    0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0a, // +4+4=20 int_32
    // Omitting uint_32
    0x00,0x00,0x00,0x03, 0x05, (0), (0), (0), // +4+1+(3)=28 int_8
    0x00,0x00,0x00,0x04, 0x06, (0), (0), (0), // +4+1+(3)=36 uint_8
    // Omitting int_16
    0x10,0x00,0x00,0x06, 0x22,0x22, (0), (0), // +4+2+(2)=44 uint_16
    0x30,0x00,0x00,0x07, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=56 int_64
    0x30,0x00,0x00,0x08, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=68 uint_64
    0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00, // +4+4=76 float_32
    0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+8=88 float_64
    0x40,0x00,0x00,0x0b, 0x00,0x00,0x00,0x10,
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +4+4+16=112 float_128
    0x00,0x00,0x00,0x0c, 'a', (0), (0), (0),  // +4+1+(3)=120 char_8
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=128 char_16
    0x00,0x00,0x00,0x0e, 0xff, (0), (0), (0), // +4+1+(3)=136 byte
    0x00,0x00,0x00,0x0f, 0x01, (0), (0), (0), // +4+1+(3)=144 bool
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x0c, // +4+4=152 nested_struct
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=164 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=182 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_index_mapping(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_ReadValueFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0a // +8=20 int_32
    };
    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32_union(&data);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // +8=12 discriminator
      0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0b // +8=20 uint_32
    };
    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32_union(&data);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x02, // +8=12 discriminator
      0x00,0x00,0x00,0x03, 0x7f // +5=17 int_8
    };
    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int8_union(&data);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +8=12 discriminator
      0x00,0x00,0x00,0x04, 0xff // +5=17 uint_8
    };
    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint8_union(&data);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x04, // +8=12 discriminator
      0x10,0x00,0x00,0x05, 0x00,0x09 // +6=18 int_16
    };
    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int16_union(&data);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x05, // +8=12 discriminator
      0x10,0x00,0x00,0x06, 0x00,0x05 // +6=18 uint_16
    };
    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint16_union(&data);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x06, // +8=12 discriminator
      0x30,0x00,0x00,0x07, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +12=24 int_64
    };
    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int64_union(&data);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x07, // +8=12 discriminator
      0x30,0x00,0x00,0x08, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +12=24 uint_64
    };
    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint64_union(&data);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x08, // +8=12 discriminator
      0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00 // +8=20 float_32
    };
    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float32_union(&data);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x09, // +8=12 discriminator
      0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 float_64
    };
    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float64_union(&data);
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
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float128_union(&data);
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0b, // +8=12 discriminator
      0x00,0x00,0x00,0x0c, 'a' // +5=17 char_8
    };
    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char8_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0c, // +8=12 discriminator
      0x10,0x00,0x00,0x0d, 0x00,0x61 // +6=18 char_16
    };
    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char16_union(&data);
  }
#endif
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0d, // +8=12 discriminator
      0x00,0x00,0x00,0x0e, 0xff // +5=17 byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_byte_union(&data);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0e, // +8=12 discriminator
      0x00,0x00,0x00,0x0f, 0x01 // +5=17 bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_bool_union(&data);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0f, // +8=12 discriminator
      0x30,0x00,0x00,0x10, 0x00,0x00,0x00,0x04,'a','b','c','\0' // +12=24 str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_string_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x1c, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x10, // +8=12 discriminator
      0x40,0x00,0x00,0x11, 0x00,0x00,0x00,0x0c,
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // +8+12=32 wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_wstring_union(&data);
  }
#endif
  {
    // Read default member
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x11, // +8=12 discriminator
      0x20,0x00,0x00,0x12, 0x00,0x00,0x00,0x09 // +8=20 my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_enum_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_ReadSequenceFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

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
  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_sequence_value_struct<MutableSequenceStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_ReadSequenceFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableSequenceUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableSequenceUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32s_union[] = {
      0x00,0x00,0x00,0x1c, // dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // discriminator
      0x40,0x00,0x00,0x01, 0x00,0x00,0x00,0x0c,
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, 0x00,0x00,0x00,0x0b // int_32s
    };
    msg.copy((const char*)int32s_union, sizeof(int32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32s_union(&data);
  }
  {
    unsigned char uint32s_union[] = {
      0x00,0x00,0x00,0x1c, // dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // discriminator
      0x40,0x00,0x00,0x02, 0x00,0x00,0x00,0x0c,
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0xff, 0x00,0x00,0xff,0xff // uint_32s
    };
    msg.reset();
    msg.copy((const char*)uint32s_union, sizeof(uint32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32s_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_ReadValueFromArray)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char array_struct[] = {
    0x00,0x00,0x00,0x1e, // +4=4 dheader
    0x30,0x00,0x00,0x00, 0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +12=16 int_32a
    0x30,0x00,0x00,0x01, 0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +12=28 uint_32a
    0x10,0x00,0x00,0x02, 0x01, 0x02 // +6=34 int_8a
  };
  ACE_Message_Block msg(256);
  msg.copy((const char*)array_struct, sizeof(array_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
  verify_array_struct(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_SkipNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::MutableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::MutableStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char mutable_struct[] = {
    0x00,0x00,0x00,0x39, // +4=4 dheader
    0x00,0x00,0x00,0x00, 'a',(0),(0),(0), // +5+(3)=12 c
    /////////// outer (FinalNestedStructOuter) ///////////
    0x40,0x00,0x00,0x01, 0x00,0x00,0x00,0x0e, // +8=20 Emheader & nextint
    0x12,0x34,0x56,0x78, // +4=24 l
    0x00,0x00,0x00,0x04, 0x7f,0xff,0xff,0xff, // +8=32 inner.l
    0x43,0x21,(0),(0), // +2+(2)=36 s
    ////////////////////////////////////////////////////////
    0x10,0x00,0x00,0x02, 0x00,0x0a,(0),(0), // +6+(2)=44 s
    /////////// inner (FinalNestedUnionInner) ////////////
    0x30,0x00,0x00,0x03, // +4=48 Emheader
    0x00,0x00,0x00,0x01, // +4=52 discriminator
    0xff,0xff,0xff,0xff, // +4=56 ul
    ////////////////////////////////////////////////////////
    0x00,0x00,0x00,0x04, 0x11 // +5=61 i
  };
  MutableStruct expected;
  expected.c = 'a';
  expected.outer.l = 0x12345678;
  expected.outer.inner.l = 0x7fffffff;
  expected.outer.s = 0x4321;
  expected.s = 0x000a;
  expected.inner.ul(ACE_CDR::ULong(0xffffffff));
  expected.i = 0x11;

  ACE_Message_Block msg(128);
  msg.copy((const char*)mutable_struct, sizeof(mutable_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  DDS::DynamicData_var nested_level1;
  DDS::ReturnCode_t ret = data.get_complex_value(nested_level1, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = nested_level1->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.l, l);
  ACE_CDR::Short s;
  ret = nested_level1->get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.s, s);
  DDS::DynamicData_var nested_level2;
  ret = nested_level1->get_complex_value(nested_level2, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_level2->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.inner.l, l);

  ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  nested_level1 = 0;
  ret = data.get_complex_value(nested_level1, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  ret = nested_level1->get_uint32_value(ul, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.inner.ul(), ul);

  ACE_CDR::Int8 i;
  ret = data.get_int8_value(i, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.i, i);

  // Test the validity of inner DynamicData when the outer one no longer exists.
  DDS::DynamicData_var nested;
  {
    XTypes::DynamicDataImpl enclosing(&msg, xcdr2, dt);
    ret = enclosing.get_complex_value(nested, 1);
    EXPECT_EQ(DDS::RETCODE_OK, ret);
  }
  ACE_CDR::Long l_val;
  ret = nested->get_int32_value(l_val, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.l, l_val);
  ACE_CDR::Short s_val;
  ret = nested->get_int16_value(s_val, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.s, s_val);
}

/////////////////////////////// Appendable tests ////////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x60,  // +4=4 dheader
    0x00,0x00,0x00,0x03, // +4=8 my_enum
    0x00,0x00,0x00,0x0a, // +4=12 int_32
    0x00,0x00,0x00,0x0b, // +4=16 uint_32
    0x05, // +1=17 int_8
    0x06, // +1=18 uint_8
    0x11,0x11, // +2=20 int_16
    0x22,0x22, // +2=22 uint_16
    (0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+8=32 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=40 uint_64
    0x3f,0x80,0x00,0x00, // +4=44 float_32
    0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +8=52 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=68 float_128
    'a',  // +1=69 char_8
    (0),0x00,0x61, // +(1)+2=72 char_16
    0xff, // +1=73 byte
    0x01, // +1=74 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=80 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=88 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=100 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_single_value_struct<AppendableSingleValueStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromStructXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x03, // +4=4 my_enum
    0x00,0x00,0x00,0x0a, // +4=8 int_32
    0x00,0x00,0x00,0x0b, // +4=12 uint_32
    0x05, // +1=13 int_8
    0x06, // +1=14 uint_8
    0x11,0x11, // +2=16 int_16
    0x22,0x22, // +2=18 uint_16
    (0),(0),(0),(0),(0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(6)+8=32 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=40 uint_64
    0x3f,0x80,0x00,0x00, // +4=44 float_32
    (0),(0),(0),(0),0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +(4)+8=56 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=72 float_128
    'a',  // +1=73 char_8
    (0),0x00,0x61, // +(1)+2=76 char_16
    0xff, // +1=77 byte
    0x01, // +1=78 bool
    (0),(0), 0x00,0x00,0x00,0x0c, // +(2)+4=84 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=92 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=100 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  verify_single_value_struct<AppendableSingleValueStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_StructWithOptionalMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::CompleteTypeObject cto = it->second.complete;
  cto.struct_type.member_seq[2].common.member_flags |= XTypes::IS_OPTIONAL;
  cto.struct_type.member_seq[5].common.member_flags |= XTypes::IS_OPTIONAL;
  DDS::DynamicType_var dt = tls.complete_to_dynamic(cto, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x5c,  // +4=4 dheader
    0x00,0x00,0x00,0x03, // +4=8 my_enum
    0x00,0x00,0x00,0x0a, // +4=12 int_32
    0x00, // +1=13 Omitting uint_32
    0x05, // +1=14 int_8
    0x06, // +1=15 uint_8
    0x00, // +1=16 Omitting int_16
    0x22,0x22, // +2=18 uint_16
    (0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+8=28 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=36 uint_64
    0x3f,0x80,0x00,0x00, // +4=40 float_32
    0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +8=48 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=664 float_128
    'a',  // +1=65 char_8
    (0),0x00,0x61, // +(1)+2=68 char_16
    0xff, // +1=69 byte
    0x01, // +1=70 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=76 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=84 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=96 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_index_mapping(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x08, // dheader
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x0a // int_32
    };
    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32_union(&data);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x08, // dheader
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x0b // uint_32
    };
    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32_union(&data);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x05, // dheader
      0x00,0x00,0x00,0x02, // discriminator
      0x7f // int_8
    };
    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int8_union(&data);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x05, // dheader
      0x00,0x00,0x00,0x03, // discriminator
      0xff // uint_8
    };
    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint8_union(&data);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x06, // dheader
      0x00,0x00,0x00,0x04, // discriminator
      0x00,0x09 // int_16
    };
    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int16_union(&data);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x06, // dheader
      0x00,0x00,0x00,0x05, // discriminator
      0x00,0x05 // uint_16
    };
    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint16_union(&data);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x0c, // dheader
      0x00,0x00,0x00,0x06, // discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// int_64
    };
    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int64_union(&data);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x0c, // dheader
      0x00,0x00,0x00,0x07, // discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// uint_64
    };
    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint64_union(&data);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x08, // dheader
      0x00,0x00,0x00,0x08, // discriminator
      0x3f,0x80,0x00,0x00 // float_32
    };
    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float32_union(&data);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x0c, // dheader
      0x00,0x00,0x00,0x09, // discriminator
      0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // float_64
    };
    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float64_union(&data);
  }
  {
    unsigned char float128_union[] = {
      0x00,0x00,0x00,0x14, // dheader
      0x00,0x00,0x00,0x0a, // discriminator
      0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0 // float_128
    };
    msg.reset();
    msg.copy((const char*)float128_union, sizeof(float128_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float128_union(&data);
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x05, // dheader
      0x00,0x00,0x00,0x0b, // discriminator
      'a'     // char_8
    };
    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char8_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x06, // dheader
      0x00,0x00,0x00,0x0c, // discriminator
      0x00,0x61 // char_16
    };
    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char16_union(&data);
  }
#endif
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x05, // dheader
      0x00,0x00,0x00,0x0d, // discriminator
      0xff // byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_byte_union(&data);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x05, // dheader
      0x00,0x00,0x00,0x0e, // discriminator
      0x01 // bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_bool_union(&data);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x0c, // dheader
      0x00,0x00,0x00,0x0f, // discriminator
      0x00,0x00,0x00,0x04,'a','b','c','\0' // str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_string_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x10, // dheader
      0x00,0x00,0x00,0x10, // discriminator
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_wstring_union(&data);
  }
#endif
  {
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x08, // dheader
      0x00,0x00,0x00,0x11, // discriminator
      0x00,0x00,0x00,0x09 // my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_enum_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromUnionXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x0a // int_32
    };
    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int32_union(&data);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x0b // uint_32
    };
    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint32_union(&data);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x02, // discriminator
      0x7f // int_8
    };
    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int8_union(&data);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x03, // discriminator
      0xff // uint_8
    };
    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint8_union(&data);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x04, // discriminator
      0x00,0x09 // int_16
    };
    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int16_union(&data);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x05, // discriminator
      0x00,0x05 // uint_16
    };
    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint16_union(&data);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x06, // discriminator
      (0),(0),(0),(0), 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// int_64
    };
    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int64_union(&data);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x07, // discriminator
      (0),(0),(0),(0), 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// uint_64
    };
    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint64_union(&data);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x08, // discriminator
      0x3f,0x80,0x00,0x00 // float_32
    };
    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float32_union(&data);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x09, // discriminator
      (0),(0),(0),(0), 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // float_64
    };
    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float64_union(&data);
  }
  {
    unsigned char float128_union[] = {
      0x00,0x00,0x00,0x0a, // discriminator
      (0),(0),(0),(0), 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0 // float_128
    };
    msg.reset();
    msg.copy((const char*)float128_union, sizeof(float128_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float128_union(&data);
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x0b, // discriminator
      'a'     // char_8
    };
    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_char8_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x0c, // discriminator
      0x00,0x61 // char_16
    };
    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_char16_union(&data);
  }
#endif
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x0d, // discriminator
      0xff // byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_byte_union(&data);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x0e, // discriminator
      0x01 // bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_bool_union(&data);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x0f, // discriminator
      0x00,0x00,0x00,0x04,'a','b','c','\0' // str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_string_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x10, // discriminator
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_wstring_union(&data);
  }
#endif
  {
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x11, // discriminator
      0x00,0x00,0x00,0x09 // my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_enum_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadSequenceFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0x00,0x00,0x00,0xe8, // dheader
    0,0,0,12, 0,0,0,2, 0,0,0,1, 0,0,0,2, // +16=16 my_enums
    0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +16=32 int_32s
    0,0,0,2, 0,0,0,10, 0,0,0,11, // +12=44 uint_32s
    0,0,0,3, 12,13,14, // +7=51 int_8s
    (0), 0,0,0,2, 15,16, // +(1)+6=58 uint_8s
    (0),(0), 0,0,0,2, 0,1,0,2, // +(2)+8=68 int_16s
    0,0,0,3, 0,3,0,4,0,5, // +10=78 uint_16s
    (0),(0), 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+20=100 int_64s
    0,0,0,1, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +12=112 uint_64s
    0,0,0,1, 0x3f,0x80,0x00,0x00, // +8=120 float_32s
    0,0,0,1, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +12=132 float_64s
    0,0,0,1, 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // +20=152 float_128s
    0,0,0,2, 'a','b', // +6=158 char_8s
    (0),(0), 0,0,0,3, 0,0x63,0,0x64,0,0x65, // +(2)+10=170 char_16s
    (0),(0), 0,0,0,2, 0xee,0xff, // +(2)+6=178 byte_s
    (0),(0), 0,0,0,1, 1, // +(2)+5=185 bool_s
    (0),(0),(0), 0,0,0,12, 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +(3)+16=204 str_s
    0,0,0,26, 0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +16+(2)+10=232 wstr_s
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_sequence_value_struct<AppendableSequenceStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadSequenceFromStructXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0,0,0,2, 0,0,0,1, 0,0,0,2, // +12=12 my_enums
    0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +16=28 int_32s
    0,0,0,2, 0,0,0,10, 0,0,0,11, // +12=40 uint_32s
    0,0,0,3, 12,13,14, // +7=47 int_8s
    (0), 0,0,0,2, 15,16, // +(1)+6=54 uint_8s
    (0),(0), 0,0,0,2, 0,1,0,2, // +(2)+8=64 int_16s
    0,0,0,3, 0,3,0,4,0,5, // +10=74 uint_16s
    (0),(0), 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+20=96 int_64s
    0,0,0,1, (0),(0),(0),(0), 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+(4)+8=112 uint_64s
    0,0,0,1, 0x3f,0x80,0x00,0x00, // +8=120 float_32s
    0,0,0,1, (0),(0),(0),(0), 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+(4)+8=136 float_64s
    0,0,0,1, (0),(0),(0),(0), 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // +4+(4)+16=160 float_128s
    0,0,0,2, 'a','b', // +6=166 char_8s
    (0),(0), 0,0,0,3, 0,0x63,0,0x64,0,0x65, // +(2)+10=178 char_16s
    (0),(0), 0,0,0,2, 0xee,0xff, // +(2)+6=186 byte_s
    (0),(0), 0,0,0,1, 1, // +(2)+5=193 bool_s
    (0),(0),(0), 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +(3)+12=208 str_s
    0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +14+(2)+10=234 wstr_s
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  verify_sequence_value_struct<AppendableSequenceStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadSequenceFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSequenceUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSequenceUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32s_union[] = {
      0x00,0x00,0x00,0x10, // dheader
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, 0x00,0x00,0x00,0x0b // int_32s
    };
    msg.copy((const char*)int32s_union, sizeof(int32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32s_union(&data);
  }
  {
    unsigned char uint32s_union[] = {
      0x00,0x00,0x00,0x10, // dheader
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0xff, 0x00,0x00,0xff,0xff // uint_32s
    };
    msg.reset();
    msg.copy((const char*)uint32s_union, sizeof(uint32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32s_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadSequenceFromUnionXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableSequenceUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableSequenceUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32s_union[] = {
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, 0x00,0x00,0x00,0x0b // int_32s
    };
    msg.copy((const char*)int32s_union, sizeof(int32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int32s_union(&data);
  }
  {
    unsigned char uint32s_union[] = {
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0xff, 0x00,0x00,0xff,0xff // uint_32s
    };
    msg.reset();
    msg.copy((const char*)uint32s_union, sizeof(uint32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint32s_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromArray)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char array_struct[] = {
    0x00,0x00,0x00,0x12, // +4=4 dheader
    0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +8=12 int_32a
    0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +8=20 uint_32a
    0x01, 0x02 // +2=22 int_8a
  };
  ACE_Message_Block msg(256);
  msg.copy((const char*)array_struct, sizeof(array_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
  verify_array_struct(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_ReadValueFromArrayXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char array_struct[] = {
    0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +8=8 int_32a
    0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +8=16 uint_32a
    0x01, 0x02 // +2=18 int_8a
  };
  ACE_Message_Block msg(256);
  msg.copy((const char*)array_struct, sizeof(array_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
  verify_array_struct(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_SkipNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char appendable_struct[] = {
    0x00,0x00,0x00,0x35,// +4=4 dheader
    'a',(0),(0),(0), // +1+(3)=8 c
    /////////// outer (MutableNestedStructOuter) ///////////
    0x00,0x00,0x00,0x15, // +4=12 deadher
    0x00,0x00,0x00,0x00, 0xff,(0),(0),(0), // +5+(3)=20 o
    0x20,0x00,0x00,0x01, 0x7f,0xff,0xff,0xff, // +8=28 inner.l
    0x00,0x00,0x00,0x02, 0x01, // +5=33 b
    ////////////////////////////////////////////////////////
    (0),0x00,0x0a, // +3=36 s
    /////////// inner (MutableNestedUnionInner) ////////////
    0x00,0x00,0x00,0x10, // +4=40 dheader
    0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // +8=48 discriminator
    0x20,0x00,0x00,0x01, 0xff,0xff,0xff,0xff, // +8=56 ul
    ////////////////////////////////////////////////////////
    0x11 // +1=57 i
  };
  AppendableStruct expected;
  expected.c = 'a';
  expected.outer.o = 0xff;
  expected.outer.inner.l = 0x7fffffff;
  expected.outer.b = true;
  expected.s = 0x000a;
  expected.inner.ul(ACE_CDR::ULong(0xffffffff));
  expected.i = 0x11;

  ACE_Message_Block msg(128);
  msg.copy((const char*)appendable_struct, sizeof(appendable_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  DDS::DynamicData_var nested_level1;
  DDS::ReturnCode_t ret = data.get_complex_value(nested_level1, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Octet o;
  ret = nested_level1->get_byte_value(o, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.o, o);
  ACE_CDR::Boolean b;
  ret = nested_level1->get_boolean_value(b, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.b, b);
  DDS::DynamicData_var nested_level2;
  ret = nested_level1->get_complex_value(nested_level2, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = nested_level2->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.inner.l, l);

  ACE_CDR::Int16 s;
  ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  nested_level1 = 0;
  ret = data.get_complex_value(nested_level1, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  ret = nested_level1->get_uint32_value(ul, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.inner.ul(), ul);

  ACE_CDR::Int8 i;
  ret = data.get_int8_value(i, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.i, i);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_SkipNestedMembersXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::AppendableStructXCDR1_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::AppendableStructXCDR1_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char appendable_struct[] = {
    'a',(0),(0),(0), // +4=4 c
    /////////// outer (AppendableNestedStructOuterXCDR1) ///////////
    0x12,0x34,0x56,0x78, // +4=8 ul
    0x7f,0xff,0xff,0xff, // +4=12 inner.l
    0x43,0x21, // +2=14 us
    ////////////////////////////////////////////////////////
    0x00,0x0a, // +2=16 s
    /////////// inner (AppendableNestedUnionInner) ////////////
    0x00,0x00,0x00,0x01, // +4=20 discriminator
    0xff,0xff,0xff,0xff, // +4=24 ul
    ////////////////////////////////////////////////////////
    0x11 // +1=25 i
  };

  AppendableStructXCDR1 expected;
  expected.c = 'a';
  expected.outer.ul = 0x12345678;
  expected.outer.inner.l = 0x7fffffff;
  expected.outer.us = 0x4321;
  expected.s = 0x000a;
  expected.inner.ul(ACE_CDR::ULong(0xffffffff));
  expected.i = 0x11;

  ACE_Message_Block msg(128);
  msg.copy((const char*)appendable_struct, sizeof(appendable_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  DDS::DynamicData_var nested_level1;
  DDS::ReturnCode_t ret = data.get_complex_value(nested_level1, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  ret = nested_level1->get_uint32_value(ul, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.ul, ul);
  ACE_CDR::UShort us;
  ret = nested_level1->get_uint16_value(us, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.us, us);
  DDS::DynamicData_var nested_level2;
  ret = nested_level1->get_complex_value(nested_level2, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = nested_level2->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.inner.l, l);

  ACE_CDR::Short s;
  ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  nested_level1 = 0;
  ret = data.get_complex_value(nested_level1, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_level1->get_uint32_value(ul, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.inner.ul(), ul);

  ACE_CDR::Int8 i;
  ret = data.get_int8_value(i, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.i, i);
}

/////////////////////////////// Final tests /////////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x03, // +4=4 my_enum
    0x00,0x00,0x00,0x0a, // +4=8 int_32
    0x00,0x00,0x00,0x0b, // +4=12 uint_32
    0x05, // +1=13 int_8
    0x06, // +1=14 uint_8
    0x11,0x11, // +2 =16 int_16
    0x22,0x22, // +2 =18 uint_16
    (0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+8=28 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=36 uint_64
    0x3f,0x80,0x00,0x00, // +4=40 float_32
    0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +8=48 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=64 float_128
    'a',  // +1=65 char_8
    (0),0x00,0x61, // +(1)+2=68 char_16
    0xff, // +1=69 byte
    0x01, // +1=70 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=76 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=84 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=96 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_single_value_struct<FinalSingleValueStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromStructXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x03, // +4=4 my_enum
    0x00,0x00,0x00,0x0a, // +4=8 int_32
    0x00,0x00,0x00,0x0b, // +4=12 uint_32
    0x05, // +1=13 int_8
    0x06, // +1=14 uint_8
    0x11,0x11, // +2 =16 int_16
    0x22,0x22, // +2 =18 uint_16
    (0),(0),(0),(0),(0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(6)+8=32 int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +8=40 uint_64
    0x3f,0x80,0x00,0x00, // +4=44 float_32
    (0),(0),(0),(0),0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +(4)+8=56 float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +16=72 float_128
    'a',  // +1=73 char_8
    (0),0x00,0x61, // +(1)+2=76 char_16
    0xff, // +1=77 byte
    0x01, // +1=78 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=84 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=92 str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // +12=104 swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  verify_single_value_struct<FinalSingleValueStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_StructWithOptionalMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  XTypes::CompleteTypeObject cto = it->second.complete;
  cto.struct_type.member_seq[2].common.member_flags |= XTypes::IS_OPTIONAL;
  cto.struct_type.member_seq[5].common.member_flags |= XTypes::IS_OPTIONAL;
  DDS::DynamicType_var dt = tls.complete_to_dynamic(cto, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x03, // my_enum
    0x00,0x00,0x00,0x0a, // int_32
    0x00, // Omitting uint_32
    0x05, // int_8
    0x06, // uint_8
    0x00, // Omitting int_16
    0x22,0x22, // uint_16
    (0),(0),0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // int_64
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // uint_64
    0x3f,0x80,0x00,0x00, // float_32
    0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // float_64
    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // float_128
    'a',  // char_8
    (0),0x00,0x61, // char_16
    0xff, // byte
    0x01, // bool
    (0), (0), 0x00,0x00,0x00,0x0c, // nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // str
    0x00,0x00,0x00,0x08, 0,0x61,0,0x62,0,0x63,0,0, // swtr
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)single_value_struct, sizeof(single_value_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_index_mapping(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x00, // +4=4 discriminator
      0x00,0x00,0x00,0x0a  // +4=8 int_32
    };
    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32_union(&data);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x01, // +4=4 discriminator
      0x00,0x00,0x00,0x0b // +4=8 uint_32
    };
    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32_union(&data);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x02, // +4=4 discriminator
      0x7f // +1=5 int_8
    };
    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int8_union(&data);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x03, // +4=4 discriminator
      0xff // +1=5 uint_8
    };
    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint8_union(&data);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x04, // +4=4 discriminator
      0x00,0x09 // +2=6 int_16
    };
    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int16_union(&data);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x05, // +4=4 discriminator
      0x00,0x05 // +2=6 uint_16
    };
    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint16_union(&data);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x06, // +4=4 discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +8=12 int_64
    };
    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int64_union(&data);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x07, // +4=4 discriminator
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +8=12 uint_64
    };
    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint64_union(&data);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x08, // +4=4 discriminator
      0x3f,0x80,0x00,0x00 // +4=8 float_32
    };
    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float32_union(&data);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x09, // +4=4 discriminator
      0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +8=12 float_64
    };
    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float64_union(&data);
  }
  {
    unsigned char float128_union[] = {
      0x00,0x00,0x00,0x0a, // +4=4 discriminator
      0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0     // 16=20 float_128
    };
    msg.reset();
    msg.copy((const char*)float128_union, sizeof(float128_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_float128_union(&data);
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x0b, // +4=4 discriminator
      'a'     // +1=5 char_8
    };
    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char8_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x0c, // +4=4 discriminator
      0x00,0x61 // +2=6 char_16
    };
    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_char16_union(&data);
  }
#endif
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x0d, // discriminator
      0xff // byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_byte_union(&data);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x0e, // discriminator
      0x01 // bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_bool_union(&data);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x0f, // discriminator
      0x00,0x00,0x00,0x04,'a','b','c','\0' // str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_string_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x10, // discriminator
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_wstring_union(&data);
  }
#endif
  {
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x11, // discriminator
      0x00,0x00,0x00,0x09 // my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_enum_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromUnionXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32_union[] = {
      0x00,0x00,0x00,0x00, // +4=4 discriminator
      0x00,0x00,0x00,0x0a  // +4=8 int_32
    };
    msg.copy((const char*)int32_union, sizeof(int32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int32_union(&data);
  }
  {
    unsigned char uint32_union[] = {
      0x00,0x00,0x00,0x01, // +4=4 discriminator
      0x00,0x00,0x00,0x0b // +4=8 uint_32
    };
    msg.reset();
    msg.copy((const char*)uint32_union, sizeof(uint32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint32_union(&data);
  }
  {
    unsigned char int8_union[] = {
      0x00,0x00,0x00,0x02, // +4=4 discriminator
      0x7f // +1=5 int_8
    };
    msg.reset();
    msg.copy((const char*)int8_union, sizeof(int8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int8_union(&data);
  }
  {
    unsigned char uint8_union[] = {
      0x00,0x00,0x00,0x03, // +4=4 discriminator
      0xff // +1=5 uint_8
    };
    msg.reset();
    msg.copy((const char*)uint8_union, sizeof(uint8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint8_union(&data);
  }
  {
    unsigned char int16_union[] = {
      0x00,0x00,0x00,0x04, // +4=4 discriminator
      0x00,0x09 // +2=6 int_16
    };
    msg.reset();
    msg.copy((const char*)int16_union, sizeof(int16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int16_union(&data);
  }
  {
    unsigned char uint16_union[] = {
      0x00,0x00,0x00,0x05, // +4=4 discriminator
      0x00,0x05 // +2=6 uint_16
    };
    msg.reset();
    msg.copy((const char*)uint16_union, sizeof(uint16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint16_union(&data);
  }
  {
    unsigned char int64_union[] = {
      0x00,0x00,0x00,0x06, // +4=4 discriminator
      (0),(0),(0),(0),0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +(4)+8=16 int_64
    };
    msg.reset();
    msg.copy((const char*)int64_union, sizeof(int64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int64_union(&data);
  }
  {
    unsigned char uint64_union[] = {
      0x00,0x00,0x00,0x07, // +4=4 discriminator
      (0),(0),(0),(0),0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +(4)+8=16 uint_64
    };
    msg.reset();
    msg.copy((const char*)uint64_union, sizeof(uint64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint64_union(&data);
  }
  {
    unsigned char float32_union[] = {
      0x00,0x00,0x00,0x08, // +4=4 discriminator
      0x3f,0x80,0x00,0x00 // +4=8 float_32
    };
    msg.reset();
    msg.copy((const char*)float32_union, sizeof(float32_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float32_union(&data);
  }
  {
    unsigned char float64_union[] = {
      0x00,0x00,0x00,0x09, // +4=4 discriminator
      (0),(0),(0),(0),0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +(4)+8=16 float_64
    };
    msg.reset();
    msg.copy((const char*)float64_union, sizeof(float64_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float64_union(&data);
  }
  {
    unsigned char float128_union[] = {
      0x00,0x00,0x00,0x0a, // +4=4 discriminator
      (0),(0),(0),(0),0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0     // +(4)+16=24 float_128
    };
    msg.reset();
    msg.copy((const char*)float128_union, sizeof(float128_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_float128_union(&data);
  }
  {
    unsigned char char8_union[] = {
      0x00,0x00,0x00,0x0b, // +4=4 discriminator
      'a'     // +1=5 char_8
    };
    msg.reset();
    msg.copy((const char*)char8_union, sizeof(char8_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_char8_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char char16_union[] = {
      0x00,0x00,0x00,0x0c, // +4=4 discriminator
      0x00,0x61 // +2=6 char_16
    };
    msg.reset();
    msg.copy((const char*)char16_union, sizeof(char16_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_char16_union(&data);
  }
#endif
  {
    unsigned char byte_union[] = {
      0x00,0x00,0x00,0x0d, // discriminator
      0xff // byte_
    };
    msg.reset();
    msg.copy((const char*)byte_union, sizeof(byte_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_byte_union(&data);
  }
  {
    unsigned char bool_union[] = {
      0x00,0x00,0x00,0x0e, // discriminator
      0x01 // bool_
    };
    msg.reset();
    msg.copy((const char*)bool_union, sizeof(bool_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_bool_union(&data);
  }
  {
    unsigned char str_union[] = {
      0x00,0x00,0x00,0x0f, // discriminator
      0x00,0x00,0x00,0x04,'a','b','c','\0' // str
    };
    msg.reset();
    msg.copy((const char*)str_union, sizeof(str_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_string_union(&data);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char wstr_union[] = {
      0x00,0x00,0x00,0x10, // discriminator
      0x00,0x00,0x00,0x08, 0x00,0x61,0x00,0x62,0x00,0x63,0x00,0x00 // wstr
    };
    msg.reset();
    msg.copy((const char*)wstr_union, sizeof(wstr_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_wstring_union(&data);
  }
#endif
  {
    unsigned char enum_union[] = {
      0x00,0x00,0x00,0x11, // discriminator
      0x00,0x00,0x00,0x09 // my_enum
    };
    msg.reset();
    msg.copy((const char*)enum_union, sizeof(enum_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_enum_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadSequenceFromStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0,0,0,12, 0,0,0,2, 0,0,0,1, 0,0,0,2, // +16=16 my_enums
    0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +16=32 int_32s
    0,0,0,2, 0,0,0,10, 0,0,0,11, // +12=44 uint_32s
    0,0,0,3, 12,13,14, // +7=51 int_8s
    (0), 0,0,0,2, 15,16, // +(1)+6=58 uint_8s
    (0),(0), 0,0,0,2, 0,1,0,2, // +(2)+8=68 int_16s
    0,0,0,3, 0,3,0,4,0,5, // +10=78 uint_16s
    (0),(0), 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+20=100 int_64s
    0,0,0,1, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +12=112 uint_64s
    0,0,0,1, 0x3f,0x80,0x00,0x00, // +8=120 float_32s
    0,0,0,1, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +12=132 float_64s
    0,0,0,1, 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // +20=152 float_128s
    0,0,0,2, 'a','b', // +6=158 char_8s
    (0),(0), 0,0,0,3, 0,0x63,0,0x64,0,0x65, // +(2)+10=170 char_16s
    (0),(0), 0,0,0,2, 0xee,0xff, // +(2)+6=178 byte_s
    (0),(0), 0,0,0,1, 1, // +(2)+5=185 bool_s
    (0),(0),(0), 0,0,0,12, 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +(3)+16=204 str_s
    0,0,0,26, 0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +16+(2)+10=232 wstr_s
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  verify_sequence_value_struct<FinalSequenceStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadSequenceFromStructXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0,0,0,2, 0,0,0,1, 0,0,0,2, // +12=12 my_enums
    0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +16=28 int_32s
    0,0,0,2, 0,0,0,10, 0,0,0,11, // +12=40 uint_32s
    0,0,0,3, 12,13,14, // +7=47 int_8s
    (0), 0,0,0,2, 15,16, // +(1)+6=54 uint_8s
    (0),(0), 0,0,0,2, 0,1,0,2, // +(2)+8=64 int_16s
    0,0,0,3, 0,3,0,4,0,5, // +10=74 uint_16s
    (0),(0), 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +(2)+20=96 int_64s
    0,0,0,1, (0),(0),(0),(0), 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+(4)+8=112 uint_64s
    0,0,0,1, 0x3f,0x80,0x00,0x00, // +8=120 float_32s
    0,0,0,1, (0),(0),(0),(0), 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+(4)+8=136 float_64s
    0,0,0,1, (0),(0),(0),(0), 0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // +4+(4)+16=160 float_128s
    0,0,0,2, 'a','b', // +6=166 char_8s
    (0),(0), 0,0,0,3, 0,0x63,0,0x64,0,0x65, // +(2)+10=178 char_16s
    (0),(0), 0,0,0,2, 0xee,0xff, // +(2)+6=186 byte_s
    (0),(0), 0,0,0,1, 1, // +(2)+5=193 bool_s
    (0),(0),(0), 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +(3)+12=208 str_s
    0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +14+(2)+10=234 wstr_s
  };
  ACE_Message_Block msg(1024);
  msg.copy((const char*)sequence_struct, sizeof(sequence_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  verify_sequence_value_struct<FinalSequenceStruct>(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadSequenceFromUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSequenceUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSequenceUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32s_union[] = {
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, 0x00,0x00,0x00,0x0b // int_32s
    };
    msg.copy((const char*)int32s_union, sizeof(int32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_int32s_union(&data);
  }
  {
    unsigned char uint32s_union[] = {
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0xff, 0x00,0x00,0xff,0xff // uint_32s
    };
    msg.reset();
    msg.copy((const char*)uint32s_union, sizeof(uint32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
    verify_uint32s_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadSequenceFromUnionXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalSequenceUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalSequenceUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  ACE_Message_Block msg(256);
  {
    unsigned char int32s_union[] = {
      0x00,0x00,0x00,0x00, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0x0a, 0x00,0x00,0x00,0x0b // int_32s
    };
    msg.copy((const char*)int32s_union, sizeof(int32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_int32s_union(&data);
  }
  {
    unsigned char uint32s_union[] = {
      0x00,0x00,0x00,0x01, // discriminator
      0x00,0x00,0x00,0x02, 0x00,0x00,0x00,0xff, 0x00,0x00,0xff,0xff // uint_32s
    };
    msg.reset();
    msg.copy((const char*)uint32s_union, sizeof(uint32s_union));
    XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
    verify_uint32s_union(&data);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromArray)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char array_struct[] = {
    0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +8=8 int_32a
    0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +8=16 uint_32a
    0x01, 0x02 // +2=18 int_8a
  };
  ACE_Message_Block msg(256);
  msg.copy((const char*)array_struct, sizeof(array_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);
  verify_array_struct(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_ReadValueFromArrayXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char array_struct[] = {
    0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +8=8 int_32a
    0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +8=16 uint_32a
    0x01, 0x02 // +2=18 int_8a
  };
  ACE_Message_Block msg(256);
  msg.copy((const char*)array_struct, sizeof(array_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);
  verify_array_struct(&data);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_SkipNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char final_struct[] = {
    'a',(0),(0),(0), // +4=4 c
    /////////// outer (AppendableNestedStructOuter) ///////////
    0x00,0x00,0x00,0x12, // +4=8 dheader
    0x12,0x34,0x56,0x78, // +4=12 ul
    0x00,0x00,0x00,0x08, 0x20,0x00,0x00,0x00, 0x7f,0xff,0xff,0xff, // +12=24 inner.l
    0x43,0x21, // +2=26 us
    ////////////////////////////////////////////////////////
    0x00,0x0a, // +2=28 s
    /////////// inner (AppendableNestedUnionInner) ////////////
    0x00,0x00,0x00,0x08, // +4=32 dheader
    0x00,0x00,0x00,0x01, // +4=36 discriminator
    0xff,0xff,0xff,0xff, // +4=40 ul
    ////////////////////////////////////////////////////////
    0x11 // +1=41 i
  };
  FinalStruct expected;
  expected.c = 'a';
  expected.outer.ul = 0x12345678;
  expected.outer.inner.l = 0x7fffffff;
  expected.outer.us = 0x4321;
  expected.s = 0x000a;
  expected.inner.ul(ACE_CDR::ULong(0xffffffff));
  expected.i = 0x11;

  ACE_Message_Block msg(128);
  msg.copy((const char*)final_struct, sizeof(final_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr2, dt);

  DDS::DynamicData_var nested_level1;
  DDS::ReturnCode_t ret = data.get_complex_value(nested_level1, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  ret = nested_level1->get_uint32_value(ul, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.ul, ul);
  ACE_CDR::UShort us;
  ret = nested_level1->get_uint16_value(us, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.us, us);
  DDS::DynamicData_var nested_level2;
  ret = nested_level1->get_complex_value(nested_level2, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = nested_level2->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.inner.l, l);

  ACE_CDR::Short s;
  ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  nested_level1 = 0;
  ret = data.get_complex_value(nested_level1, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_level1->get_uint32_value(ul, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.inner.ul(), ul);

  ACE_CDR::Int8 i;
  ret = data.get_int8_value(i, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.i, i);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_SkipNestedMembersXCDR1)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::FinalStructXCDR1_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::FinalStructXCDR1_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char final_struct[] = {
    'a',(0),(0),(0), // +4=4 c
    /////////// outer (FinalNestedStructOuterXCDR1) ///////////
    0x12,0x34,0x56,0x78, // +4=8 ul
    0x7f,0xff,0xff,0xff, // +4=12 inner.l
    0x43,0x21, // +2=14 us
    ////////////////////////////////////////////////////////
    0x00,0x0a, // +2=16 s
    /////////// inner (FinalNestedUnionInner) ////////////
    0x00,0x00,0x00,0x01, // +4=20 discriminator
    0xff,0xff,0xff,0xff, // +4=24 ul
    ////////////////////////////////////////////////////////
    0x11 // +1=25 i
  };

  FinalStructXCDR1 expected;
  expected.c = 'a';
  expected.outer.ul = 0x12345678;
  expected.outer.inner.l = 0x7fffffff;
  expected.outer.us = 0x4321;
  expected.s = 0x000a;
  expected.inner.ul(ACE_CDR::ULong(0xffffffff));
  expected.i = 0x11;

  ACE_Message_Block msg(128);
  msg.copy((const char*)final_struct, sizeof(final_struct));
  XTypes::DynamicDataImpl data(&msg, xcdr1, dt);

  DDS::DynamicData_var nested_level1;
  DDS::ReturnCode_t ret = data.get_complex_value(nested_level1, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::ULong ul;
  ret = nested_level1->get_uint32_value(ul, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.ul, ul);
  ACE_CDR::UShort us;
  ret = nested_level1->get_uint16_value(us, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.us, us);
  DDS::DynamicData_var nested_level2;
  ret = nested_level1->get_complex_value(nested_level2, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ACE_CDR::Long l;
  ret = nested_level2->get_int32_value(l, 0);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.outer.inner.l, l);

  ACE_CDR::Short s;
  ret = data.get_int16_value(s, 2);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.s, s);

  nested_level1 = 0;
  ret = data.get_complex_value(nested_level1, 3);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  ret = nested_level1->get_uint32_value(ul, 1);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.inner.ul(), ul);

  ACE_CDR::Int8 i;
  ret = data.get_int8_value(i, 4);
  EXPECT_EQ(DDS::RETCODE_OK, ret);
  EXPECT_EQ(expected.i, i);
}

#endif // OPENDDS_SAFETY_PROFILE
