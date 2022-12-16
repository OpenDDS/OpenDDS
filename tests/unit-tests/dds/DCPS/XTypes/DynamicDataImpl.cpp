#ifndef OPENDDS_SAFETY_PROFILE

#include <DynamicDataImplTypeSupportImpl.h>

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
  // Set int_32 but use wrong Id
  ret = data.set_int32_value(2, input.int_32);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  // Set int_32 but use wrong interface
  ret = data.set_uint32_value(1, static_cast<CORBA::ULong>(input.int_32));
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
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
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
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
  DDS::DynamicData_var nested_data = new XTypes::DynamicDataImpl(md->type());
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
  DDS::DynamicData_var int16_dd = new XTypes::DynamicDataImpl(md->type());
  // Using incorrect interface
  ret = int16_dd->set_int32_value(XTypes::MEMBER_ID_INVALID, 10);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  // Use correct interface but wrong id (expect MEMBER_ID_INVALID)
  ret = int16_dd->set_int16_value(rewrite_id, input.int_16);
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
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
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  // Write a member that isn't selected by the existing discriminator.
  ret = data.set_uint32_value(2, CORBA::ULong(10));
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
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
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
  ret = data.set_int32_value(1, CORBA::Long(10));
  EXPECT_EQ(ret, DDS::RETCODE_ERROR);
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

template<typename SequenceTypeA, typename SequenceTypeB>
void set_sequences(SequenceTypeA& target, const SequenceTypeB& source)
{
  target.length(source.length());
  for (unsigned i = 0; i < source.length(); ++i) {
    target[i] = source[i];
  }
}

template<typename StructType>
void verify_sequence_value_struct(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  StructType input;
  set_sequence_value_struct(input);
  XTypes::DynamicDataImpl data(type);

  /// my_enums
  DDS::Int32Seq my_enums;
  set_sequences(my_enums, input.my_enums);
  DDS::ReturnCode_t ret = data.set_int32_values(0, my_enums);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_int32_values(2, my_enums);
  EXPECT_NE(ret, DDS::RETCODE_OK);

  /// int_32s
  DDS::Int32Seq int_32s;
  set_sequences(int_32s, input.int_32s);
  ret = data.set_int32_values(3, int_32s);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = data.set_int32_values(1, int_32s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// uint_32s
  DDS::UInt32Seq uint_32s;
  set_sequences(uint_32s, input.uint_32s);
  ret = data.set_uint32_values(4, uint_32s);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret =  data.set_uint32_values(2, uint_32s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// int_8s
  DDS::Int8Seq int_8s;
  set_sequences(int_8s, input.int_8s);
  ret = data.set_int8_values(3, int_8s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// uint_8s
  DDS::UInt8Seq uint_8s;
  set_sequences(uint_8s, input.uint_8s);
  ret = data.set_uint8_values(4, uint_8s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// int_16s
  DDS::Int16Seq int_16s;
  set_sequences(int_16s, input.int_16s);
  ret = data.set_int16_values(5, int_16s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// uint_16s
  DDS::UInt16Seq uint_16s;
  set_sequences(uint_16s, input.uint_16s);
  ret = data.set_uint16_values(6, uint_16s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// int_64s
  DDS::Int64Seq int_64s;
  set_sequences(int_64s, input.int_64s);
  ret = data.set_int64_values(7, int_64s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// uint_64s
  DDS::UInt64Seq uint_64s;
  set_sequences(uint_64s, input.uint_64s);
  ret = data.set_uint64_values(8, uint_64s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// float_32s
  DDS::Float32Seq float_32s;
  set_sequences(float_32s, input.float_32s);
  ret = data.set_float32_values(9, float_32s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// float_64s
  DDS::Float64Seq float_64s;
  set_sequences(float_64s, input.float_64s);
  ret = data.set_float64_values(10, float_64s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// char_8s
  DDS::CharSeq char_8s;
  set_sequences(char_8s, input.char_8s);
  ret = data.set_char8_values(12, char_8s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

#ifdef DDS_HAS_WCHAR
  /// char_16s
  DDS::WcharSeq char_16s;
  set_sequences(char_16s, input.char_16s);
  ret = data.set_char16_values(13, char_16s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif

  /// byte_s
  DDS::ByteSeq byte_s;
  set_sequences(byte_s, input.byte_s);
  ret = data.set_byte_values(14, byte_s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// bool_s
  DDS::BooleanSeq bool_s;
  set_sequences(bool_s, input.bool_s);
  ret = data.set_boolean_values(15, bool_s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  /// str_s
  DDS::StringSeq str_s;
  set_sequences(str_s, input.str_s);
  ret = data.set_string_values(16, str_s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

#ifdef DDS_HAS_WCHAR
  /// wstr_s
  DDS::WstringSeq wstr_s;
  set_sequences(wstr_s, input.wstr_s);
  ret = data.set_wstring_values(17, wstr_s);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
#endif
  {
    ACE_Message_Block buffer(512);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }

  // Rewrite some members.
  {
    // Set elements of my_enums individually
    DDS::DynamicTypeMember_var dtm;
    ret = type->get_member(dtm, 0);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    DDS::MemberDescriptor_var md;
    ret = dtm->get_descriptor(md);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    DDS::DynamicData_var enums_dd = new XTypes::DynamicDataImpl(md->type());
    DDS::MemberId id = enums_dd->get_member_id_at_index(0);
    EXPECT_EQ(id, DDS::MemberId(0));
    ret = enums_dd->set_int32_value(id, E_UINT32);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    id = enums_dd->get_member_id_at_index(1);
    EXPECT_EQ(id, DDS::MemberId(1));
    ret = enums_dd->set_int32_value(id, E_INT8);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ret = data.set_complex_value(0, enums_dd);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
  }
  {
    // Set elements of uint_32s individually
    DDS::DynamicTypeMember_var dtm;
    ret = type->get_member(dtm, 2);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    DDS::MemberDescriptor_var md;
    ret = dtm->get_descriptor(md);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    DDS::DynamicData_var uint32s_dd = new XTypes::DynamicDataImpl(md->type());
    DDS::MemberId id = uint32s_dd->get_member_id_at_index(0);
    EXPECT_EQ(id, DDS::MemberId(0));
    ret = uint32s_dd->set_uint32_value(id, uint_32s[0]);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    id = uint32s_dd->get_member_id_at_index(1);
    EXPECT_EQ(id, DDS::MemberId(1));
    ret = uint32s_dd->set_uint32_value(id, uint_32s[1]);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
    ret = data.set_complex_value(2, uint32s_dd);
    EXPECT_EQ(ret, DDS::RETCODE_OK);
  }
  {
    ACE_Message_Block buffer(512);
    DCPS::Serializer ser(&buffer, xcdr2);
    ASSERT_TRUE(ser << data);
    EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
  }
}

template<typename StructType>
void verify_sequence_value_struct_default(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(type);
  ACE_Message_Block buffer(512);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
}

void verify_array_struct(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  XTypes::DynamicDataImpl data(type);

  // int_32a
  DDS::DynamicTypeMember_var dtm;
  DDS::ReturnCode_t ret = type->get_member(dtm, 0);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var longarr_dd = new XTypes::DynamicDataImpl(md->type());
  DDS::MemberId id = longarr_dd->get_member_id_at_index(0);
  EXPECT_EQ(id, DDS::MemberId(0));
  ret = longarr_dd->set_int32_value(id, 0x12);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  id = longarr_dd->get_member_id_at_index(1);
  EXPECT_EQ(id, DDS::MemberId(1));
  ret = longarr_dd->set_int32_value(id, 0x34);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(0, longarr_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // uint_32a
  dtm = 0;
  ret = type->get_member(dtm, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  md = 0;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var ulongarr_dd = new XTypes::DynamicDataImpl(md->type());
  id = ulongarr_dd->get_member_id_at_index(0);
  EXPECT_EQ(id, DDS::MemberId(0));
  ret = ulongarr_dd->set_uint32_value(id, 0xff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  id = ulongarr_dd->get_member_id_at_index(1);
  EXPECT_EQ(id, DDS::MemberId(1));
  ret = ulongarr_dd->set_uint32_value(id, 0xff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(1, ulongarr_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // int_8a
  dtm = 0;
  ret = type->get_member(dtm, 2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  md = 0;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var int8arr_dd = new XTypes::DynamicDataImpl(md->type());
  id = int8arr_dd->get_member_id_at_index(0);
  EXPECT_EQ(id, DDS::MemberId(0));
  ret = int8arr_dd->set_int8_value(id, 0x01);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  id = int8arr_dd->get_member_id_at_index(1);
  EXPECT_EQ(id, DDS::MemberId(1));
  ret = int8arr_dd->set_int8_value(id, 0x02);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(2, int8arr_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  ACE_Message_Block buffer(128);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
}

void verify_array_struct_default(DDS::DynamicType_var type, const DataView& expected_cdr)
{
  // All array members take default values.
  XTypes::DynamicDataImpl data(type);
  ACE_Message_Block buffer(128);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, expected_cdr, buffer);
}

/////////////////////////// Mutable tests ///////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSingleValueStruct_xtag>();
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
    0x00,0x00,0x00,0x0c, 'a', (0), (0), (0),  // +4+1+(3)=136 char_8
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0e, 0xff, (0), (0), (0), // +4+1+(3)=152 byte
    0x00,0x00,0x00,0x0f, 0x01, (0), (0), (0), // +4+1+(3)=160 bool
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x0c, // +4+4=168 nested_struct
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=180 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=198 swtr
  };
  verify_single_value_struct<MutableSingleValueStruct>(dt, single_value_struct);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToStructDefault)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

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
    0x00,0x00,0x00,0x0c, '\0', (0), (0), (0),  // +4+1+(3)=136 char_8 (default)
    0x10,0x00,0x00,0x0d, 0x00,0x61, (0), (0), // +4+2+(2)=144 char_16
    0x00,0x00,0x00,0x0e, 0x00, (0), (0), (0), // +4+1+(3)=152 byte (default)
    0x00,0x00,0x00,0x0f, 0x00, (0), (0), (0), // +4+1+(3)=160 bool (default)
    0x20,0x00,0x00,0x10, 0x00,0x00,0x00,0x00, // +4+4=168 nested_struct (default)
    0x30,0x00,0x00,0x11, 0x00,0x00,0x00,0x04, 'a','b','c','\0', // +4+8=180 str
    0x40,0x00,0x00,0x12, 0x00,0x00,0x00,0x0a,
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +4+4+10=198 swtr
  };
  verify_default_single_value_struct<MutableSingleValueStruct>(dt, default_single_value);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToUnion)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSingleValueUnion_xtag>();
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

// TODO: Add a test case for optional members

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToUnionDefault)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSingleValueUnion_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSingleValueUnion_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  // Ideally, there would be a similar set of verifying functions for appendable and final.
  // But they will be different only in the serialization part. So these should be good
  // enough to test that default values are written correctly.
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

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteSequenceToStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char sequence_struct[] = {
    0x00,0x00,0x01,0x56, // +4=4 dheader
    0x40,0,0,0, 0,0,0,16, 0,0,0,12, 0,0,0,2, 0,0,0,1, 0,0,0,2, // +24=28 my_enums
    0x40,0,0,1, 0,0,0,16, 0,0,0,3, 0,0,0,3, 0,0,0,4, 0,0,0,5, // +24=52 int_32s
    0x40,0,0,2, 0,0,0,12, 0,0,0,2, 0,0,0,10, 0,0,0,11, // +20=72 uint_32s
    0x40,0,0,3, 0,0,0,7,  0,0,0,3, 12,13,14,(0), // +16=88 int_8s
    0x40,0,0,4, 0,0,0,6,  0,0,0,2, 15,16,(0),(0), // +16=104 uint_8s
    0x30,0,0,5, 0,0,0,2,  0,1,0,2, // +12=116 int_16s
    0x40,0,0,6, 0,0,0,10, 0,0,0,3, 0,3,0,4,0,5,(0),(0), // +20=136 uint_16s
    0x40,0,0,7, 0,0,0,20, 0,0,0,2, 0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xfe,
    0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +28=164 int_64s
    0x40,0,0,8, 0,0,0,12, 0,0,0,1, 0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff, // +20=184 uint_64s
    0x30,0,0,9, 0,0,0,1, 0x3f,0x80,0x00,0x00, // +12=196 float_32s
    0x40,0,0,10,0,0,0,12, 0,0,0,1, 0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00, // +20=216 float_64s
    0x40,0,0,12, 0,0,0,6, 0,0,0,2, 'a','b',(0),(0), // +16=232 char_8s
    0x40,0,0,13, 0,0,0,10, 0,0,0,3, 0,0x63,0,0x64,0,0x65,(0),(0), // +20=252 char_16s
    0x40,0,0,14, 0,0,0,6, 0,0,0,2, 0xee,0xff,(0),(0), // +16=268 byte_s
    0x40,0,0,15, 0,0,0,5, 0,0,0,1, 1,(0),(0),(0), // +16=284 bool_s
    0x40,0,0,16, 0,0,0,16, 0,0,0,12, 0,0,0,1, 0,0,0,4, 'a','b','c','\0', // +24=308 str_s
    0x40,0,0,17, 0,0,0,30, 0,0,0,26, 0,0,0,2, 0,0,0,6, 0,0x64,0,0x65,0,0x66,(0),(0),
    0,0,0,6, 0,0x67,0,0x68,0,0x69 // +38=346 wstr_s
  };
  verify_sequence_value_struct<MutableSequenceStruct>(dt, sequence_struct);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteSequenceToStructDefault)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableSequenceStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableSequenceStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  // Struct will all sequence members taking their default values.
  unsigned char sequence_struct[] = {
    0x00,0x00,0x00,0x94, // +4=4 dheader
    0x30,0,0,0, 0,0,0,4, 0,0,0,0, // +12=16 my_enums
    0x20,0,0,1, 0,0,0,0, // +8=24 int_32s
    0x20,0,0,2, 0,0,0,0, // +8=32 uint_32s
    0x20,0,0,3, 0,0,0,0, // +8=40 int_8s
    0x20,0,0,4, 0,0,0,0, // +8=48 uint_8s
    0x20,0,0,5, 0,0,0,0, // +8=56int_16s
    0x20,0,0,6, 0,0,0,0, // +8=64 uint_16s
    0x20,0,0,7, 0,0,0,0, // +8=72 int_64s
    0x20,0,0,8, 0,0,0,0, // +8=80 uint_64s
    0x20,0,0,9, 0,0,0,0, // +8=88 float_32s
    0x20,0,0,10, 0,0,0,0, // +8=96 float_64s
    0x20,0,0,12, 0,0,0,0, // +8=104 char_8s
    0x20,0,0,13, 0,0,0,0, // +8=112 char_16s
    0x20,0,0,14, 0,0,0,0, // +8=120 byte_s
    0x20,0,0,15, 0,0,0,0, // +8=128 bool_s
    0x30,0,0,16, 0,0,0,4, 0,0,0,0, // +12=140 str_s
    0x30,0,0,17, 0,0,0,4, 0,0,0,0 // +12=152 wstr_s
  };
  verify_sequence_value_struct_default<MutableSequenceStruct>(dt, sequence_struct);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteSequenceToUnion)
{
  // TODO: Write to union with members are sequence of basic types
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteSequenceUnionDefault)
{
  // TODO: Sequence members of a union take default value.
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToArray)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char expected_cdr[] = {
    0x00,0x00,0x00,0x1e, // +4=4 dheader
    0x30,0x00,0x00,0x00, 0x00,0x00,0x00,0x12, 0x00,0x00,0x00,0x34, // +12=16 int_32a
    0x30,0x00,0x00,0x01, 0x00,0x00,0x00,0xff, 0x00,0x00,0x00,0xff,  // +12=28 uint_32a
    0x10,0x00,0x00,0x02, 0x01, 0x02 // +6=34 int_8a
  };
  verify_array_struct(dt, expected_cdr);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteValueToArrayDefault)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableArrayStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableArrayStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char expected_cdr[] = {
    0x00,0x00,0x00,0x1e, // +4=4 dheader
    0x30,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, // +12=16 int_32a
    0x30,0x00,0x00,0x01, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,  // +12=28 uint_32a
    0x10,0x00,0x00,0x02, 0x00, 0x00 // +6=34 int_8a
  };
  verify_array_struct_default(dt, expected_cdr);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Mutable_WriteStructWithNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_MutableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_MutableStruct_xtag>();
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

  // i
  XTypes::DynamicDataImpl data(dt);
  DDS::ReturnCode_t ret = data.set_int8_value(4, 0x11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // inner
  DDS::DynamicTypeMember_var dtm;
  ret = dt->get_member(dtm, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var inner_dd = new XTypes::DynamicDataImpl(md->type());
  ret = inner_dd->set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = inner_dd->set_int32_value(0, 10);
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = inner_dd->set_char8_value(2, 'a');
  EXPECT_NE(ret, DDS::RETCODE_OK);
  ret = inner_dd->set_uint32_value(1, 0xffffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(3, inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // s
  ret = data.set_int16_value(2, 0x0a);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // outer
  dtm = 0;
  ret = dt->get_member(dtm, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  md = 0;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_dd = new XTypes::DynamicDataImpl(md->type());
  //    outer.l
  ret = outer_dd->set_int32_value(0, 0x12345678);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.s
  ret = outer_dd->set_int16_value(2, 0x4321);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.inner
  DDS::DynamicTypeMember_var dtm2;
  ret = md->type()->get_member(dtm2, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md2;
  ret = dtm2->get_descriptor(md2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_inner_dd = new XTypes::DynamicDataImpl(md2->type());
  ret = outer_inner_dd->set_int32_value(0, 0x7fffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = outer_dd->set_complex_value(1, outer_inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(1, outer_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // c
  ret = data.set_char8_value(0, 'a');
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  ACE_Message_Block buffer(128);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, mutable_struct, buffer);
}

/////////////////////////// Appendable tests ///////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_WriteValueToStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_AppendableSingleValueStruct_xtag>();
  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());
  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());

  unsigned char single_value_struct[] = {
    0x00,0x00,0x00,0x4e,  // +4=4 dheader
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
    'a',  // +1=53 char_8
    (0),0x00,0x61, // +(1)+2=56 char_16
    0xff, // +1=57 byte
    0x01, // +1=58 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=64 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=72 str
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +10=82 wstr
  };
  verify_single_value_struct<AppendableSingleValueStruct>(dt, single_value_struct);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_WriteStructWithNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_AppendableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_AppendableStruct_xtag>();
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

  XTypes::DynamicDataImpl data(dt);
  // c
  DDS::ReturnCode_t ret = data.set_char8_value(0, 'a');
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // outer
  DDS::DynamicTypeMember_var dtm;
  ret = dt->get_member(dtm, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_dd = new XTypes::DynamicDataImpl(md->type());
  //    outer.o
  ret = outer_dd->set_byte_value(0, 0xff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.inner
  DDS::DynamicTypeMember_var dtm2;
  ret = md->type()->get_member(dtm2, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md2;
  ret = dtm2->get_descriptor(md2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_inner_dd = new XTypes::DynamicDataImpl(md2->type());
  ret = outer_inner_dd->set_int32_value(0, 0x7fffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = outer_dd->set_complex_value(1, outer_inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.b
  ret = outer_dd->set_boolean_value(2, true);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(1, outer_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // s
  ret = data.set_int16_value(2, 0x000a);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // inner
  dtm = 0;
  ret = dt->get_member(dtm, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  md = 0;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var inner_dd = new XTypes::DynamicDataImpl(md->type());
  ret = inner_dd->set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = inner_dd->set_uint32_value(1, 0xffffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(3, inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // i
  ret = data.set_int8_value(4, 0x11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  ACE_Message_Block buffer(128);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, appendable_struct, buffer);
}

/////////////////////////// Final tests ///////////////////////////
TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_WriteValueToStruct)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_FinalSingleValueStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_FinalSingleValueStruct_xtag>();
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
    'a',  // +1=49 char_8
    (0),0x00,0x61, // +(1)+2=52 char_16
    0xff, // +1=53 byte
    0x01, // +1=54 bool
    (0), (0), 0x00,0x00,0x00,0x0c, // +(2)+4=60 nested_struct
    0x00,0x00,0x00,0x04, 'a','b','c','\0', // +8=68 str
    0x00,0x00,0x00,0x06, 0,0x61,0,0x62,0,0x63 // +10=78 wstr
  };
  verify_single_value_struct<FinalSingleValueStruct>(dt, single_value_struct);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_WriteStructWithNestedMembers)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_FinalStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_FinalStruct_xtag>();
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

  XTypes::DynamicDataImpl data(dt);
  // c
  DDS::ReturnCode_t ret = data.set_char8_value(0, 'a');
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // outer
  DDS::DynamicTypeMember_var dtm;
  ret = dt->get_member(dtm, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_dd = new XTypes::DynamicDataImpl(md->type());
  //    outer.ul
  ret = outer_dd->set_uint32_value(0, 0x12345678);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.inner
  DDS::DynamicTypeMember_var dtm2;
  ret = md->type()->get_member(dtm2, 1);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::MemberDescriptor_var md2;
  ret = dtm2->get_descriptor(md2);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var outer_inner_dd = new XTypes::DynamicDataImpl(md2->type());
  ret = outer_inner_dd->set_int32_value(0, 0x7fffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = outer_dd->set_complex_value(1, outer_inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  //    outer.us
  ret = outer_dd->set_uint16_value(2, 0x4321);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(1, outer_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // s
  ret = data.set_int16_value(2, 0x000a);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // inner
  dtm = 0;
  ret = dt->get_member(dtm, 3);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  md = 0;
  ret = dtm->get_descriptor(md);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  DDS::DynamicData_var inner_dd = new XTypes::DynamicDataImpl(md->type());
  ret = inner_dd->set_int32_value(XTypes::DISCRIMINATOR_ID, E_UINT32);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = inner_dd->set_uint32_value(1, 0xffffffff);
  EXPECT_EQ(ret, DDS::RETCODE_OK);
  ret = data.set_complex_value(3, inner_dd);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  // i
  ret = data.set_int8_value(4, 0x11);
  EXPECT_EQ(ret, DDS::RETCODE_OK);

  ACE_Message_Block buffer(128);
  DCPS::Serializer ser(&buffer, xcdr2);
  ASSERT_TRUE(ser << data);
  EXPECT_PRED_FORMAT2(assert_DataView, final_struct, buffer);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Final_WriteKeyOnly)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_FinalStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_FinalStruct_xtag>();
  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());

  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
  EXPECT_TRUE(dt);
  XTypes::DynamicDataImpl data(dt);
  static const ACE_CDR::Int8 expected_value = 42;
  EXPECT_EQ(DDS::RETCODE_OK, data.set_int8_value(4, expected_value));

  static const DCPS::Encoding xcdr2_noswap(DCPS::Encoding::KIND_XCDR2);
  static const size_t expected_size = 1u;
  EXPECT_EQ(expected_size, DCPS::serialized_size(xcdr2_noswap, DCPS::KeyOnly<const XTypes::DynamicDataImpl>(data)));

  ACE_Message_Block buffer(expected_size);
  DCPS::Serializer ser(&buffer, xcdr2_noswap);
  EXPECT_TRUE(ser << DCPS::KeyOnly<const XTypes::DynamicDataImpl>(data));
  static const unsigned char expected_buffer[] = {expected_value};
  EXPECT_PRED_FORMAT2(assert_DataView, expected_buffer, buffer);
}

TEST(DDS_DCPS_XTypes_DynamicDataImpl, Appendable_WriteKeyOnly)
{
  const XTypes::TypeIdentifier& ti = DCPS::getCompleteTypeIdentifier<DCPS::DynamicDataImpl_AppendableStruct_xtag>();
  const XTypes::TypeMap& type_map = DCPS::getCompleteTypeMap<DCPS::DynamicDataImpl_AppendableStruct_xtag>();
  XTypes::TypeLookupService tls;
  tls.add(type_map.begin(), type_map.end());

  const XTypes::TypeMap::const_iterator it = type_map.find(ti);
  EXPECT_TRUE(it != type_map.end());

  DDS::DynamicType_var dt = tls.complete_to_dynamic(it->second.complete, DCPS::GUID_t());
  EXPECT_TRUE(dt);
  XTypes::DynamicDataImpl data(dt);
  static const ACE_CDR::Short expected_value = 42;
  EXPECT_EQ(DDS::RETCODE_OK, data.set_int16_value(2, expected_value));

  static const DCPS::Encoding xcdr2_le(DCPS::Encoding::KIND_XCDR2, DCPS::ENDIAN_LITTLE);
  static const size_t expected_size = 6u;
  EXPECT_EQ(expected_size, DCPS::serialized_size(xcdr2_le, DCPS::KeyOnly<const XTypes::DynamicDataImpl>(data)));

  ACE_Message_Block buffer(expected_size);
  DCPS::Serializer ser(&buffer, xcdr2_le);
  EXPECT_TRUE(ser << DCPS::KeyOnly<const XTypes::DynamicDataImpl>(data));
  static const unsigned char expected_buffer[] =
    {2, 0, 0, 0, // DHEADER
     expected_value & 0xff, (expected_value >> 8) & 0xff
    };
  EXPECT_PRED_FORMAT2(assert_DataView, expected_buffer, buffer);
}

#endif // OPENDDS_SAFETY_PROFILE
