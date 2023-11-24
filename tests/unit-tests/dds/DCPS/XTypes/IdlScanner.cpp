#ifndef OPENDDS_SAFETY_PROFILE
#  include <dds/DCPS/XTypes/IdlScanner.h>

#  include <dds/DCPS/XTypes/TypeDescriptorImpl.h>
#  include <dds/DCPS/XTypes/DynamicTypeImpl.h>
#  include <dds/DCPS/XTypes/DynamicTypeMemberImpl.h>

#  include "gtest/gtest.h"

using namespace OpenDDS::XTypes;
using namespace DDS;

TEST(dds_DCPS_XTypes_IdlScanner, CharacterScanner)
{
  {
    CharacterScanner s("");
    EXPECT_TRUE(s.eoi());
  }

  {
    CharacterScanner s("abc");
    EXPECT_FALSE(s.eoi());
    EXPECT_EQ(s.peek(), 'a');
    EXPECT_TRUE(s.match('a'));
    EXPECT_FALSE(s.match('c'));
    s.consume();
    EXPECT_TRUE(s.match('c'));
    EXPECT_TRUE(s.eoi());
    EXPECT_FALSE(s.match('c'));
  }
}

namespace {
  DDS::DynamicType_ptr
  make_primitive(DDS::TypeKind tk,
                 const char* name)
  {
    TypeDescriptorImpl* tdi = new TypeDescriptorImpl();
    tdi->kind(tk);
    tdi->name(name);
    DDS::TypeDescriptor_var td = tdi;
    DynamicTypeImpl* dti = new DynamicTypeImpl();
    DDS::DynamicType_var dt = dti;
    dti->set_descriptor(td);
    return dt._retn();
  }
}

TEST(dds_DCPS_XTypes_IdlScanner, IdlScanner)
{
  {
    CharacterScanner cs("");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_eoi());
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("TRUE");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_boolean(true));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("FALSE");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_boolean(false));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'X'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character('X'));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\n'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character('\n'));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\1'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character(01));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\12'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character(012));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\123'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character(0123));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\xF'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character(0xF));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("'\\xFF'");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_character(-1));
    EXPECT_TRUE(is.eoi());
  }

  {
    CharacterScanner cs("\"Hello\"");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_string("Hello"));
    EXPECT_TRUE(is.eoi());
  }
  {
    // Null is not allowed.
    CharacterScanner cs("\"Hello\\0\"");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_error());
  }

  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 12));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(true, 12));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("01");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-01");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(true, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0x1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0x12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(false, 0x12));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-0x12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_integer(true, 0x12));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-10.05e-12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_float(true, 1005, true, 14));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("10.05e-12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_float(false, 1005, true, 14));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.05e-12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_float(true, 5, true, 14));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-10.e-12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_float(true, 10, true, 12));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-10.05e12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_float(true, 1005, false, 10));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("A");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("A"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("Z");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("Z"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("a");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("a"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("z");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("z"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("z");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("z"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("_");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_error());
    EXPECT_FALSE(is.eoi());
  }
  {
    CharacterScanner cs("my_member_name_99");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(), IdlToken::make_identifier("my_member_name_99"));
    EXPECT_TRUE(is.eoi());
  }

  DDS::DynamicType_var boolean_type = make_primitive(TK_BOOLEAN, "Boolean");
  DDS::DynamicType_var byte_type = make_primitive(TK_BYTE, "Byte");
  DDS::DynamicType_var int16_type = make_primitive(TK_INT16, "Int16");
  DDS::DynamicType_var int32_type = make_primitive(TK_INT32, "Int32");
  DDS::DynamicType_var int64_type = make_primitive(TK_INT64, "Int64");
  DDS::DynamicType_var uint16_type = make_primitive(TK_UINT16, "UInt16");
  DDS::DynamicType_var uint32_type = make_primitive(TK_UINT32, "UInt32");
  DDS::DynamicType_var uint64_type = make_primitive(TK_UINT64, "UInt64");
  DDS::DynamicType_var float32_type = make_primitive(TK_FLOAT32, "Float32");
  DDS::DynamicType_var float64_type = make_primitive(TK_FLOAT64, "Float64");
  DDS::DynamicType_var float128_type = make_primitive(TK_FLOAT128, "Float128");
  DDS::DynamicType_var int8_type = make_primitive(TK_INT8, "Int8");
  DDS::DynamicType_var uint8_type = make_primitive(TK_UINT8, "Uint8");
  DDS::DynamicType_var char8_type = make_primitive(TK_CHAR8, "Char8");
  DDS::DynamicType_var string8_type = make_primitive(TK_STRING8, "String8");

  TypeDescriptorImpl* tdi = new TypeDescriptorImpl();
  tdi->kind(TK_ENUM);
  tdi->name("MyEnumType");
  DDS::TypeDescriptor_var td = tdi;
  DynamicTypeImpl* dti = new DynamicTypeImpl();
  DDS::DynamicType_var enum_type = dti;
  dti->set_descriptor(td);

  DynamicTypeMemberImpl* dtmi = new DynamicTypeMemberImpl();
  DDS::DynamicTypeMember_var dtm = dtmi;

  MemberDescriptorImpl* mdi = new MemberDescriptorImpl();
  DDS::MemberDescriptor_var md = mdi;
  mdi->name("A_MEMBER");
  mdi->id(0);
  mdi->type(enum_type);
  mdi->index(0);

  dtmi->set_descriptor(md);

  dti->insert_dynamic_member(dtm);

  {
    CharacterScanner cs("TRUE");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(boolean_type), IdlToken::make_boolean(true));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("FALSE");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(boolean_type), IdlToken::make_boolean(false));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("maybe");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(boolean_type), IdlToken::make_error());
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("true");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(boolean_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("false");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(boolean_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0x0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0xfF");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 255));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0x100");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("01");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0377");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 0377));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0400");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("255");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_integer(false, 255));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("256");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(byte_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-32769");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int16_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-32768");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int16_type), IdlToken::make_integer(true, 32768));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int16_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("32767");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int16_type), IdlToken::make_integer(false, 32767));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("32768");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int16_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-2147483649");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int32_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-2147483648");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int32_type), IdlToken::make_integer(true, 2147483648));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int32_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("2147483647");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int32_type), IdlToken::make_integer(false, 2147483647));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("2147483648");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int32_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-9223372036854775809");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int64_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-9223372036854775808");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int64_type), IdlToken::make_integer(true, 9223372036854775808UL));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int64_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("9223372036854775807");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int64_type), IdlToken::make_integer(false, 9223372036854775807));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("9223372036854775808");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int64_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint16_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint16_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("65535");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint16_type), IdlToken::make_integer(false, 65535));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("65536");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint16_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint32_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint32_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("4294967295");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint32_type), IdlToken::make_integer(false, 4294967295));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("4294967296");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint32_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint64_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint64_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("18446744073709551615");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint64_type), IdlToken::make_integer(false, 18446744073709551615UL));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("18446744073709551616");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint64_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_float(true, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_float(false, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_float(true, 5, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_float(true, 1, true, 10));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1.5");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_float(true, 15, true, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float32_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_float(true, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_float(false, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_float(true, 5, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_float(true, 1, true, 10));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1.5");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_float(true, 15, true, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float64_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_float(true, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("1.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_float(false, 15, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.5e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_float(true, 5, true, 11));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_float(true, 1, true, 10));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-1.5");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_float(true, 15, true, 1));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("-.e-10");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(float128_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-129");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int8_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("-128");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int8_type), IdlToken::make_integer(true, 128));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int8_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("127");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int8_type), IdlToken::make_integer(false, 127));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("128");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(int8_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("-1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint8_type), IdlToken::make_error());
  }
  {
    CharacterScanner cs("0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint8_type), IdlToken::make_integer(false, 0));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("255");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint8_type), IdlToken::make_integer(false, 255));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("65536");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(uint8_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("X");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character('X'));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\n");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character('\n'));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\1");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character(01));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\12");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character(012));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\123");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character(0123));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\xF");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character(0xF));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("\\xFF");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(char8_type), IdlToken::make_character(-1));
    EXPECT_TRUE(is.eoi());
  }

  {
    CharacterScanner cs("Hello");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(string8_type), IdlToken::make_string("Hello"));
    EXPECT_TRUE(is.eoi());
  }
  {
    // Null is not allowed.
    CharacterScanner cs("Hello\\0");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(string8_type), IdlToken::make_error());
  }

  {
    CharacterScanner cs("A_MEMBER");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(enum_type), IdlToken::make_identifier("A_MEMBER"));
    EXPECT_TRUE(is.eoi());
  }
  {
    CharacterScanner cs("NOT_A_MEMBER");
    IdlScanner is(cs);
    EXPECT_EQ(is.scan_token(enum_type), IdlToken::make_error());
  }

  // Break the circular reference.
  mdi->type(0);
}

#endif // OPENDDS_SAFETY_PROFILE
