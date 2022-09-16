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

DDS::ReturnCode_t DynamicDataWriteImpl::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_descriptor(MemberId id, DDS::MemberDescriptor* value)
{
  // TODO
}

CORBA::Boolean DynamicDataWriteImpl::equals(DDS::DynamicData_ptr other)
{
  // TODO
}

MemberId DynamicDataWriteImpl::get_member_id_by_name(const char* name)
{
  // TODO
}

MemberId DynamicDataWriteImpl::get_member_id_at_index(ACE_CDR::ULong index)
{
  // TODO
}

ACE_CDR::ULong DynamicDataWriteImpl::get_item_count()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_all_values()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_nonkey_values()
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::clear_value(DDS::MemberId /*id*/)
{
  // TODO
}

DDS::DynamicData_ptr DynamicDataWriteImpl::loan_value(DDS::MemberId /*id*/)
{
  // TODO
}

DDS::ReturnCode_t DynamicDataWriteImpl::return_loaned_value(DDS::DynamicData_ptr /*value*/)
{
  // TODO
}

DDS::DynamicData_ptr DynamicDataWriteImpl::clone()
{
  // TODO
}

template <typename ValueType>
DDS::ReturnCode_t DynamicDataWriteImpl::set_value_to_struct(DDS::MemberId id, const ValueType& value, TypeKind tk)
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

// If a DynamicData represents data of a primitive type or enum/bitmask, the data is stored
// in the single_map_ in the container_ where id is MEMBER_ID_INVALID as per the XTypes spec.
// If a DynamicData represents data of a string/wstring, the characters of the string/wstring
// are also stored in the single_map_ with id of each character translated from its index.
// Similarly, if a DynamicData represents data of a sequence of a basic type, its elements are
// also stored in the single_map_ with id tranlated from the element index.
template <TypeKind ValueTypeKind, typename ValueType>
DDS::ReturnCode_t DynamicDataWriteImpl::set_single_value(DDS::MemberId id, const ValueType& value,
                                                         TypeKind enum_or_bitmask, LBound lower, LBound upper)
{
  if (!is_type_supported(ValueTypeKind, "set_single_value")) {
    return DDS::RETCODE_ERROR;
  }

  DDS::TypeDescriptor_var descriptor;
  if (type_->get_descriptor(descriptor) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  
  const TypeKind tk = type_->get_kind();
  bool good = true;

  if (tk == enum_or_bitmask) {
    // Member ID must be MEMBER_ID_INVALID for the write to succeed as per spec.
    const LBound bit_bound = descriptor->bound()[0];
    good = id == MEMBER_ID_INVALID && bit_bound >= lower && bit_bound <= upper &&
      container_.single_map_.insert(make_pair(id, value)).second;
  } else {
    switch (tk) {
    case ValueTypeKind:
      // TODO: Only allow primitive types in this case?
      good = is_primitive(tk) && id == MEMBER_ID_INVALID &&
        container_.single_map_.insert(make_pair(id, value)).second;
      break;
    case TK_STRUCTURE:
      set_value_to_struct(id, value);
      break;
    case TK_UNION:
      set_value_to_union(id, value);
      break;
    case TK_SEQUENCE:
    case TK_ARRAY:
    case TK_MAP:
      set_value_to_collection(id, value);
      break;
    default:
      break;
    }
  }

  if (!good && DCPS::DCPS_debug_level >= 1) {
    // TODO: Use correct logging level
    ACE_ERROR((LM_ERROR, "(%P|%t) DynamicDataWriteImpl::set_single_value: "
               "Failed to write to DynamicData object of type %C\n", typekind_to_string(tk)));
  }
  return good ? DDS::RETCODE_OK : DDS::RETCODE_ERROR;
}

DDS::ReturnCode_t DynamicDataWriteImpl::set_int32_value(DDS::MemberId id, CORBA::Long value)
{
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
#ifdef DDS_HAS_WCHAR
  return set_single_value(id, value, TK_CHAR16);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
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
#ifdef DDS_HAS_WCHAR
  return set_single_value(id, value, TK_STRING16);
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
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
#ifdef DDS_HAS_WCHAR
  //TODO
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
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
#ifdef DDS_HAS_WCHAR
  // TODO
#else
  return DDS::RETCODE_UNSUPPORTED;
#endif
}

DynamicData::SingleValue::SingleValue(CORBA::Long i32)
  : kind_(TK_INT32), i32_(i32)
{}

DynamicData::SingleValue::SingleValue(CORBA::ULong ui32)
  : kind_(TK_UINT32), ui32_(ui32)
{}

DynamicData::SingleValue::SingleValue(CORBA::Int8 i8)
  : kind_(TK_INT8), i8_(i8)
{}

DynamicData::SingleValue::SingleValue(CORBA::UInt8 ui8)
  : kind_(TK_UINT8), ui8_(ui8)
{}

DynamicData::SingleValue::SingleValue(CORBA::Short i16)
  : kind_(TK_INT16), i16_(i16)
{}

DynamicData::SingleValue::SingleValue(CORBA::UShort ui16)
  : kind_(TK_UINT16), ui16_(ui16)
{}

DynamicData::SingleValue::SingleValue(CORBA::LongLong i64)
  : kind_(TK_INT64), i64_(i64)
{}

DynamicData::SingleValue::SingleValue(CORBA::ULongLong ui64)
  : kind_(TK_UINT64), ui64_(ui64)
{}

DynamicData::SingleValue::SingleValue(CORBA::Float f32)
  : kind_(TK_FLOAT32), f32_(f32)
{}

DynamicData::SingleValue::SingleValue(CORBA::Double f64)
  : kind_(TK_FLOAT64), f64_(f64)
{}

DynamicData::SingleValue::SingleValue(CORBA::LongDouble f128)
  : kind_(TK_FLOAT128), f128_(f128)
{}

DynamicData::SingleValue::SingleValue(CORBA::Char c8)
  : kind_(TK_CHAR8), c8_(c8)
{}

DynamicData::SingleValue::SingleValue(CORBA::Octet byte)
  : kind_(TK_BYTE), byte_(byte)
{}

DynamicData::SingleValue::SingleValue(CORBA::Boolean boolean)
  : kind_(TK_BOOLEAN), boolean_(boolean)
{}

DynamicData::SingleValue::SingleValue(const char* str)
  : kind_(TK_STRING8), str_(ACE_OS::strdup(str))
{}

#ifdef DDS_HAS_WCHAR
DynamicData::SingleValue::SingleValue(CORBA::WChar c16)
  : kind_(TK_CHAR16), c16_(c16)
{}

// TODO: Does ACE_OS::strdup works with wide string?
DynamicData::SingleValue::SingleValue(const CORBA::WChar* wstr)
  : kind_(TK_STRING16), wstr_(ACE_OS::strdup(wstr))
{}
#endif

DynamicData::SingleValue::~SingleValue()
{
  if (kind_ == TK_STRING8 || kind_ == TK_STRING16) {
    // TODO: Does this work for wstring?
    ACE_OS::free((void*)str_);
  }
}

namespace DCPS {

bool operator<<(Serializer& ser, const DynamicDataWriteImpl& dd)
{
  // TODO
  return true;
}

}

} // namespace XTypes
} // namespace OpenDDS


OPENDDS_END_VERSIONED_NAMESPACE_DECL
