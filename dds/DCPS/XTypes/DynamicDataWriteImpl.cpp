/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicDataWriteImpl::DynamicDataWriteImpl(DDS::DynamicType_ptr type)
  : type_(get_base_type(type))
{}

template <typename ValueType>
DDS::ReturnCode_t DynamicDataWriteImpl::set_single_value(DDS::MemberId id, const ValueType& value, TypeKind tk)
{
  DDS::DynamicTypeMember_var member;
  if (type_->get_member(member, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  DDS::MemberDescriptor_var md;
  if (member->get_descriptor(md) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }

  const TypeKind member_tk = get_base_type(md->type())->get_kind();
  if (member_tk != tk) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) DynamicDataWriterImpl::set_single_value: setting value for unmatched type (%C)\n",
               typekind_to_string(member_tk)));
    return DDS::RETCODE_ERROR;
  }

  container_.single_map_.insert(make_pair(id, value));

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int32_value(DDS::MemberId id, CORBA::Long value)
{
  // TODO: Allow writing integers and unsigned integers values to an enum or bitmask member.
  return set_single_value(id, value, TK_INT32);
}

DDS::ReturnCode_t set_uint32_value(DDS::MemberId id, CORBA::ULong value)
{
  return set_single_value(id, value, TK_UINT32);
}

DDS::ReturnCode_t set_int8_value(DDS::MemberId id, CORBA::Int8 value)
{
  return set_single_value(id, value, TK_INT8);
}

DDS::ReturnCode_t set_uint8_value(DDS::MemberId id, CORBA::UInt8 value)
{
  return set_single_value(id, value, TK_UINT8);
}

DDS::ReturnCode_t set_int16_value(DDS::MemberId id, CORBA::Short value)
{
  return set_single_value(id, value, TK_INT16);
}

DDS::ReturnCode_t set_uint16_value(DDS::MemberId id, CORBA::UShort value)
{
  return set_single_value(id, value, TK_UINT16);
}

DDS::ReturnCode_t set_int64_value(DDS::MemberId id, CORBA::LongLong value)
{
  return set_single_value(id, value, TK_INT64);
}

DDS::ReturnCode_t set_uint64_value(DDS::MemberId id, CORBA::ULongLong value)
{
  return set_single_value(id, value, TK_UINT64);
}

DDS::ReturnCode_t set_float32_value(DDS::MemberId id, CORBA::Float value)
{
  return set_single_value(id, value, TK_FLOAT32);
}

DDS::ReturnCode_t set_float64_value(DDS::MemberId id, CORBA::Double value)
{
  return set_single_value(id, value, TK_FLOAT64);
}

DDS::ReturnCode_t set_float128_value(DDS::MemberId id, CORBA::LongDouble value)
{
  return set_single_value(id, value, TK_FLOAT128);
}

DDS::ReturnCode_t set_char8_value(DDS::MemberId id, CORBA::Char value)
{
  return set_single_value(id, value, TK_CHAR8);
}

DDS::ReturnCode_t set_char16_value(DDS::MemberId id, CORBA::WChar value)
{
  return set_single_value(id, value, TK_CHAR16);
}

DDS::ReturnCode_t set_byte_value(DDS::MemberId id, CORBA::Octet value)
{
  return set_single_value(id, value, TK_BYTE);
}

DDS::ReturnCode_t set_boolean_value(DDS::MemberId id, CORBA::Boolean value)
{
  return set_single_value(id, value, TK_BOOLEAN);
}

DDS::ReturnCode_t set_string_value(DDS::MemberId id, const char* value)
{
  return set_single_value(id, value, TK_STRING8);
}

DDS::ReturnCode_t set_wstring_value(DDS::MemberId id, const CORBA::WChar* value)
{
  return set_single_value(id, value, TK_STRING16);
}

DDS::ReturnCode_t set_complex_value(DDS::MemberId id, DDS::DynamicData_ptr value)
{
}

DDS::ReturnCode_t set_int32_values(DDS::MemberId id, const DDS::Int32Seq& value)
{
}

DDS::ReturnCode_t set_uint32_values(DDS::MemberId id, const DDS::UInt32Seq& value)
{
}

DDS::ReturnCode_t set_int8_values(DDS::MemberId id, const DDS::Int8Seq& value)
{
}

DDS::ReturnCode_t set_uint8_values(DDS::MemberId id, const DDS::UInt8Seq& value)
{
}

DDS::ReturnCode_t set_int16_values(DDS::MemberId id, const DDS::Int16Seq& value)
{
}

DDS::ReturnCode_t set_uint16_values(DDS::MemberId id, const DDS::UInt16Seq& value)
{
}

DDS::ReturnCode_t set_int64_values(DDS::MemberId id, const DDS::Int64Seq& value)
{
}

DDS::ReturnCode_t set_uint64_values(DDS::MemberId id, const DDS::UInt64Seq& value)
{
}

DDS::ReturnCode_t set_float32_values(DDS::MemberId id, const DDS::Float32Seq& value)
{
}

DDS::ReturnCode_t set_float64_values(DDS::MemberId id, const DDS::Float64Seq& value)
{
}

DDS::ReturnCode_t set_float128_values(DDS::MemberId id, const DDS::Float128Seq& value)
{
}

DDS::ReturnCode_t set_char8_values(DDS::MemberId id, const DDS::CharSeq& value)
{
}

DDS::ReturnCode_t set_char16_values(DDS::MemberId id, const DDS::WcharSeq& value)
{
}

DDS::ReturnCode_t set_byte_values(DDS::MemberId id, const DDS::ByteSeq& value)
{
}

DDS::ReturnCode_t set_boolean_values(DDS::MemberId id, const DDS::BooleanSeq& value)
{
}

DDS::ReturnCode_t set_string_values(DDS::MemberId id, const DDS::StringSeq& value)
{
}

DDS::ReturnCode_t set_wstring_values(DDS::MemberId id, const DDS::WstringSeq& value)
{
}

} // namespace XTypes
} // namespace OpenDDS


OPENDDS_END_VERSIONED_NAMESPACE_DECL
