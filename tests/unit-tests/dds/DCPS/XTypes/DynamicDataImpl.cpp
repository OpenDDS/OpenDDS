#ifndef OPENDDS_SAFETY_PROFILE

#include "../../../DynamicDataImplTypeSupportImpl.h"
#include "../../../../Utils/DataView.h"

#include <dds/DCPS/XTypes/TypeLookupService.h>
#include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#include <dds/DCPS/XTypes/DynamicDataImpl.h>

using namespace OpenDDS;
using namespace DynamicDataImpl;

const DCPS::Encoding xcdr2(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_BIG);
const DCPS::Encoding xcdr1(DCPS::Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);

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
void verify_single_value_struct(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  StructType input;
  set_single_value_struct(input);
  XTypes::DynamicDataImpl data(type);
  DDS::ReturnCode_t ret = data.set_int32_value(0, input.my_enum);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, input.int_32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, input.uint_32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int8_value(3, input.int_8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, input.uint_8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int16_value(5, input.int_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint16_value(6, input.uint_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int64_value(7, input.int_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint64_value(8, input.uint_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float32_value(9, input.float_32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float64_value(10, input.float_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  // Member type at the given Id does not match interface
  ret = data.set_char8_value(14, input.char_8);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_char8_value(12, input.char_8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#ifdef DDS_HAS_WCHAR
  ret = data.set_char16_value(13, input.char_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif
  ret = data.set_byte_value(14, input.byte);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_boolean_value(15, input._cxx_bool);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicTypeMember_var dtm;
  ret = type->get_member(dtm, 16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  // TODO: Use _var here and update DynamicDataImpl's set_complex_value to duplicate the
  // input pointer.
  XTypes::DynamicDataImpl* nested_data = new XTypes::DynamicDataImpl(md->type());
  ret = nested_data->set_int32_value(0, input.nested_struct.l);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret =  data.set_complex_value(16, nested_data);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_string_value(17, input.str);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#ifdef DDS_HAS_WCHAR
  ret = data.set_wstring_value(18, input.wstr);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif

  {
    ACE_Message_Block buffer(512);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

  // Rewrite a member (of type short)
  const DDS::MemberId rewrite_id = 5;
  ret = type->get_member(dtm, rewrite_id);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  XTypes::DynamicDataImpl* int16_dd = new XTypes::DynamicDataImpl(md->type());
  // Using incorrect interface
  ret = int16_dd->set_int32_value(XTypes::MEMBER_ID_INVALID, 10);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  // Use correct interface but wrong id (expect MEMBER_ID_INVALID)
  ret = int16_dd->set_int16_value(rewrite_id, input.int_16);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = int16_dd->set_int16_value(XTypes::MEMBER_ID_INVALID, input.int_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(rewrite_id, int16_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  {
    ACE_Message_Block buffer(512);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
}

template<typename StructType>
void verify_default_single_value_struct(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  StructType input;
  set_single_value_struct(input);
  XTypes::DynamicDataImpl data(type);
  // my_enum is not set
  // int_32 is not set
  DDS::ReturnCode_t ret = data.set_uint32_value(2, input.uint_32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int8_value(3, input.int_8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, input.uint_8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  // int_16 is not set
  ret = data.set_uint16_value(6, input.uint_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int64_value(7, input.int_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint64_value(8, input.uint_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float32_value(9, input.float_32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float64_value(10, input.float_64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  // char_8 is not set
#ifdef DDS_HAS_WCHAR
  ret = data.set_char16_value(13, input.char_16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif
  // byte is not set
  // bool is not set
  // nested_struct is not set
  ret = data.set_string_value(17, input.str);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#ifdef DDS_HAS_WCHAR
  ret = data.set_wstring_value(18, input.wstr);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif
  {
    ACE_Message_Block buffer(512);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
}

void verify_int32_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(10));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  // A new discriminator value doesn't select the existing member.
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  // Write a member that isn't selected by the existing discriminator.
  ret = data.set_uint32_value(2, CORBA::ULong(10));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  // Rewrite the selected member.
  ret = data.set_int32_value(1, CORBA::Long(11));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_int32_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT32);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x00 // +8=20 int_32
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Int32 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(1, CORBA::Long(11));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0b // +8=20 int_32
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Doesn't set anything. Default discriminator value selects the Int32 member.
    XTypes::DynamicDataImpl data(dt);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x00 // +8=20 int_32
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
}

void verify_uint32_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(11));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT8);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(10));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(11));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_uint32_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // +8=12 discriminator
      0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x00 // +8=20 uint_32
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the UInt32 member. Expect failure since default discriminator
    // will select the Int32 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_uint32_value(2, CORBA::ULong(11));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_int8_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int8_value(3, CORBA::Int8(0x7f));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT8);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(10));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int8_value(3, CORBA::Int8(12));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_int8_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x02, // +8=12 discriminator
      0x00,0x00,0x00,0x03, 0x00 // +5=17 int_8
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Int8 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int8_value(3, CORBA::Int8(-3));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_uint8_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, CORBA::UInt8(0xff));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT16);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(10));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, CORBA::UInt8(0xaa));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_uint8_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +8=12 discriminator
      0x00,0x00,0x00,0x04, 0x00 // +5=17 uint_8
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the UInt8 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_uint8_value(4, CORBA::UInt8(3));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_int16_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int16_value(5, CORBA::Short(9));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(10));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int16_value(5, CORBA::Short(100));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_int16_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT16);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x04, // +8=12 discriminator
      0x10,0x00,0x00,0x05, 0x00,0x00 // +6=18 int_16
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Int16 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int16_value(5, CORBA::Short(123));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_uint16_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint16_value(6, CORBA::UShort(5));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT64);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint64_value(8, CORBA::ULongLong(222));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint16_value(6, CORBA::UShort(99));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_uint16_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT16);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x05, // +8=12 discriminator
      0x10,0x00,0x00,0x06, 0x00,0x00 // +6=18 uint_16
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the UInt16 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_uint16_value(6, CORBA::UShort(121));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_int64_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int64_value(7, CORBA::LongLong(0xfe));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT16);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, CORBA::UInt8(7));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int64_value(7, CORBA::LongLong(0xbb));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_int64_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT64);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x06, // +8=12 discriminator
      0x30,0x00,0x00,0x07, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 int_64
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Int64 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int64_value(7, CORBA::LongLong(3456));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_uint64_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint64_value(8, CORBA::ULongLong(0xff));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_INT16);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, CORBA::UInt8(7));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint64_value(8, CORBA::ULongLong(0xcd));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_uint64_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT64);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x07, // +8=12 discriminator
      0x30,0x00,0x00,0x08, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 uint_64
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the UInt64 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_uint64_value(8, CORBA::ULongLong(3456));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_float32_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float32_value(9, CORBA::Float(1.0f));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT64);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint8_value(4, CORBA::UInt8(7));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_float32_value(9, CORBA::Float(2.0f));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_float32_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT32);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x08, // +8=12 discriminator
      0x20,0x00,0x00,0x09, 0x00,0x00,0x00,0x00 // +8=20 float_32
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Float32 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_float32_value(9, CORBA::Float(3.0f));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_float64_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT64);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_float64_value(10, CORBA::Double(1.0));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_CHAR8);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_char8_value(12, CORBA::Char('a'));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_float64_value(10, CORBA::Double(2.0));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_float64_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT64);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x09, // +8=12 discriminator
      0x30,0x00,0x00,0x0a, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 float_64
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Float64 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_float64_value(10, CORBA::Double(3.0));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_char8_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_CHAR8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_char8_value(12, CORBA::Char('a'));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(22));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_char8_value(12, CORBA::Char('b'));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_char8_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_CHAR8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0b, // +8=12 discriminator
      0x00,0x00,0x00,0x0c, '\0' // +5=17 char_8
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Char8 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_char8_value(12, CORBA::Char('d'));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

#ifdef DDS_HAS_WCHAR
void verify_char16_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_CHAR16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_char16_value(13, CORBA::WChar(0x0061));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int16_value(5, CORBA::Short(34));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_char16_value(13, CORBA::WChar(0x0062));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_char16_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_CHAR16);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0c, // +8=12 discriminator
      0x10,0x00,0x00,0x0d, 0x00,0x00 // +6=18 char_16
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Char16 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_char16_value(13, CORBA::WChar(0x0063));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}
#endif

void verify_byte_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_BYTE);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(0xff));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_byte_value(14, CORBA::Octet(0xff));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int16_value(5, CORBA::Short(34));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_byte_value(14, CORBA::Octet(0xab));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_byte_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_BYTE);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0d, // +8=12 discriminator
      0x00,0x00,0x00,0x0e, 0x00 // +5=17 byte_
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Byte member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_byte_value(14, CORBA::Octet(0xaa));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_bool_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_BOOL);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(0xff));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_boolean_value(15, CORBA::Boolean(true));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint16_value(6, CORBA::UShort(56));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_boolean_value(15, CORBA::Boolean(false));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_bool_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_BOOL);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0e, // +8=12 discriminator
      0x00,0x00,0x00,0x0f, 0x00 // +5=17 bool_
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the Boolean member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_boolean_value(15, CORBA::Boolean(true));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

void verify_string_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_STRING8);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::ULong(0xff));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_string_value(16, "abc");
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT32);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint16_value(6, CORBA::UShort(56));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_string_value(16, "def");
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_string_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_STRING8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x15, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0f, // +8=12 discriminator
      0x40,0x00,0x00,0x10, 0x00,0x00,0x00,0x05, 0x00,0x00,0x00,0x01,'\0' // +13=25 str
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the String8 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_string_value(16, "hello");
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

#ifdef DDS_HAS_WCHAR
void verify_wstring_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_STRING16);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(1, CORBA::Long(1234));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_wstring_value(17, L"abc");
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT64);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::UInt32(4321));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_wstring_value(17, L"def");
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_wstring_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_STRING16);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x10, // +8=12 discriminator
      0x20,0x00,0x00,0x11, 0x00,0x00,0x00,0x00 // +8=20 wstr
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the String16 member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_wstring_value(17, L"hello");
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}
#endif

void verify_enum_union(DDS::DynamicType_var dt, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, CORBA::Long(17));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_char8_value(12, CORBA::Char('a'));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(18, CORBA::Long(9));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  {
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, E_FLOAT64);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_uint32_value(2, CORBA::UInt32(4321));
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_value(18, CORBA::Long(10));
  EXPECT_EQ(ret, DDS::RETCODE_OK);
}

void verify_default_enum_union_mutable(DDS::DynamicType_var dt)
{
  {
    // Only set the discriminator.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(XTypes::DISCRIMINATOR_ID, CORBA::Long(17));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x11, // +8=12 discriminator
      0x20,0x00,0x00,0x12, 0x00,0x00,0x00,0x00 // +8=20 my_enum
    };
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
  {
    // Only set the SomeEnum member.
    XTypes::DynamicDataImpl data(dt);
    DDS::ReturnCode_t ret = data.set_int32_value(18, CORBA::Long(6));
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ACE_Message_Block buffer(64);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_FALSE(ser << data);
  }
}

/////////////////////////// Mutable tests ///////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_WriteMutableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_WriteMutableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  const unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0xaa, // +4=4 dheader
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
    //    0x40,0x00,0x00,0x0b, 0x00,0x00,0x00,0x10,
    //    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +4+4+16=128 float_128
    0x00,0x00,0x00,0x0c, 'a', (0), (0), (0),  // +4+1+(3)=136 char_8
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0e, 0xff, (0), (0), (0), // +4+1+(3)=152 byte
    0x00,0x00,0x00,0x0f, 0x01, (0), (0), (0), // +4+1+(3)=160 bool
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x0c, // +4+4=168 nested_struct
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=180 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=198 swtr
  };
  verify_single_value_struct<WriteMutableSingleValueStruct>(dt, single_value_struct);

  // Test write when some members take default values.
  const unsigned char default_single_value[] = {
    0x00,0x00,0x00,0xaa, // +4=4 dheader
    0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +4+4=12 my_enum (default)
    0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x00, // +4+4=20 int_32 (default)
    0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0b, // +4+4=28 uint_32
    0x00,0x00,0x00,0x03, 0x05, (0), (0), (0), // +4+1+(3)=36 int_8
    0x00,0x00,0x00,0x04, 0x06, (0), (0), (0), // +4+1+(3)=44 uint_8
    0x10,0x00,0x00,0x05, 0x00,0x00, (0), (0), // +4+2+(2)=52 int_16 (default)
    0x10,0x00,0x00,0x06, 0x22,0x22, (0), (0), // +4+2+(2)=60 uint_16
    0x30,0x00,0x00,0x07, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=72 int_64
    0x30,0x00,0x00,0x08, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +4+8=84 uint_64
    0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00, // +4+4=92 float_32
    0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +4+8=104 float_64
    //    0x40,0x00,0x00,0x0b, 0x00,0x00,0x00,0x10,
    //    0x3f,0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,    // +4+4+16=128 float_128
    0x00,0x00,0x00,0x0c, '\0', (0), (0), (0),  // +4+1+(3)=136 char_8 (default)
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0e, 0x00, (0), (0), (0), // +4+1+(3)=152 byte (default)
    0x00,0x00,0x00,0x0f, 0x00, (0), (0), (0), // +4+1+(3)=160 bool (default)
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x00, // +4+4=168 nested_struct (default)
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=180 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=198 swtr
  };
  verify_default_single_value_struct<WriteMutableSingleValueStruct>(dt, default_single_value);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_WriteMutableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_WriteMutableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +8=12 discriminator
      0x20,0x00,0x00,0x01, 0x00,0x00,0x00,0x0a // +8=20 int_32
    };
    verify_int32_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x01, // +8=12 discriminator
      0x20,0x00,0x00,0x02, 0x00,0x00,0x00,0x0b // +8=20 uint_32
    };
    verify_uint32_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x02, // +8=12 discriminator
      0x00,0x00,0x00,0x03, 0x7f // +5=17 int_8
    };
    verify_int8_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x03, // +8=12 discriminator
      0x00,0x00,0x00,0x04, 0xff // +5=17 uint_8
    };
    verify_uint8_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x04, // +8=12 discriminator
      0x10,0x00,0x00,0x05, 0x00,0x09 // +6=18 int_16
    };
    verify_int16_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x05, // +8=12 discriminator
      0x10,0x00,0x00,0x06, 0x00,0x05 // +6=18 uint_16
    };
    verify_uint16_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x06, // +8=12 discriminator
      0x30,0x00,0x00,0x07, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xfe// +12=24 int_64
    };
    verify_int64_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x07, // +8=12 discriminator
      0x30,0x00,0x00,0x08, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff// +12=24 uint_64
    };
    verify_uint64_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x08, // +8=12 discriminator
      0x20,0x00,0x00,0x09, 0x3f,0x80,0x00,0x00 // +8=20 float_32
    };
    verify_float32_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x09, // +8=12 discriminator
      0x30,0x00,0x00,0x0a, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00 // +12=24 float_64
    };
    verify_float64_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0b, // +8=12 discriminator
      0x00,0x00,0x00,0x0c, 'a' // +5=17 char_8
    };
    verify_char8_union(dt, expected_cdr);
  }
#ifdef DDS_HAS_WCHAR
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0e, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0c, // +8=12 discriminator
      0x10,0x00,0x00,0x0d, 0x00,0x61 // +6=18 char_16
    };
    verify_char16_union(dt, expected_cdr);
  }
#endif
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0d, // +8=12 discriminator
      0x00,0x00,0x00,0x0e, 0xff // +5=17 byte_
    };
    verify_byte_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x0d, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0e, // +8=12 discriminator
      0x00,0x00,0x00,0x0f, 0x01 // +5=17 bool_
    };
    verify_bool_union(dt, expected_cdr);
  }
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x14, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x0f, // +8=12 discriminator
      0x30,0x00,0x00,0x10, 0x00,0x00,0x00,0x04,'a','b','c','\0' // +12=24 str
    };
    verify_string_union(dt, expected_cdr);
  }
#ifdef DDS_HAS_WCHAR
  {
    // Serialization of wide string doesn't include termination NUL
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x1a, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x10, // +8=12 discriminator
      0x40,0x00,0x00,0x11, 0x00,0x00,0x00,0x0a,
      0x00,0x00,0x00,0x06, 0x00,0x61,0x00,0x62,0x00,0x63 // +18=30 wstr
    };
    verify_wstring_union(dt, expected_cdr);
  }
#endif
  {
    unsigned char expected_cdr[] = {
      0x00,0x00,0x00,0x10, // +4=4 dheader
      0x20,0x00,0x00,0x00, 0x00,0x00,0x00,0x11, // +8=12 discriminator
      0x20,0x00,0x00,0x12, 0x00,0x00,0x00,0x09 // +8=20 my_enum
    };
    verify_enum_union(dt, expected_cdr);
  }
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToUnionDefault)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_WriteMutableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_WriteMutableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  verify_default_int32_union_mutable(dt);
  verify_default_uint32_union_mutable(dt);
  verify_default_int8_union_mutable(dt);
  verify_default_uint8_union_mutable(dt);
  verify_default_int16_union_mutable(dt);
  verify_default_uint16_union_mutable(dt);
  verify_default_int64_union_mutable(dt);
  verify_default_uint64_union_mutable(dt);
  verify_default_float32_union_mutable(dt);
  verify_default_float64_union_mutable(dt);
  verify_default_char8_union_mutable(dt);
#ifdef DDS_HAS_WCHAR
  verify_default_char16_union_mutable(dt);
#endif
  verify_default_byte_union_mutable(dt);
  verify_default_bool_union_mutable(dt);
  verify_default_string_union_mutable(dt);
  verify_default_wstring_union_mutable(dt);
  verify_default_enum_union_mutable(dt);
}

#endif // OPENDDS_SAFETY_PROFILE
