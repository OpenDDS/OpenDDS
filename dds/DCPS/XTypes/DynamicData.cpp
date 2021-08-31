/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DynamicData.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicData::DynamicData()
{}

DDS::ReturnCode_t DynamicData::get_descriptor(MemberDescriptor& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_descriptor(MemberId id, const MemberDescriptor& value)
{
}

bool DynamicData::equals(const DynamicData& other) const
{
}

MemberId DynamicData::get_member_id_by_name(String name) const
{
}

MemberId DynamicData::get_member_id_at_index(ACE_CDR::ULong) const
{
}

ACE_CDR::ULong DynamicData::get_item_count() const
{
}

DDS::ReturnCode_t DynamicData::clear_all_values()
{
}

DDS::ReturnCode_t DynamicData::clear_nonkey_values()
{
}

DDS::ReturnCode_t DynamicData::clear_value(MemberId id)
{
}

DynamicData DynamicData::loan_value(MemberId id)
{
}

DDS::ReturnCode_t DynamicData::return_loaned_value(const DynamicData& value)
{
}

DynamicData DynamicData::clone() const
{
}

DDS::ReturnCode_t DynamicData::get_int32_value(ACE_CDR::Long& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int32_value(MemberId id, ACE_CDR::Long value)
{
}

DDS::ReturnCode_t DynamicData::get_uint32_value(ACE_CDR::ULong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint32_value(MemberId id, ACE_CDR::ULong value)
{
}

DDS::ReturnCode_t DynamicData::get_int8_value(ACE_CDR::Int8& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int8_value(MemberId id, ACE_CDR::Int8 value)
{
}

DDS::ReturnCode_t DynamicData::get_uint8_value(ACE_CDR::UInt8& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint8_value(MemberId id, ACE_CDR::UInt8 value)
{
}

DDS::ReturnCode_t DynamicData::get_int16_value(ACE_CDR::Short& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int16_value(MemberId id, ACE_CDR::Short value)
{
}

DDS::ReturnCode_t DynamicData::get_uint16_value(ACE_CDR::UShort& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint16_value(MemberId id, ACE_CDR::UShort value)
{
}

DDS::ReturnCode_t DynamicData::get_int64_value(ACE_CDR::LongLong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int64_value(MemberId id, ACE_CDR::LongLong value)
{
}

DDS::ReturnCode_t DynamicData::get_uint64_value(ACE_CDR::ULongLong& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint64_value(MemberId id, ACE_CDR::ULongLong value)
{
}

DDS::ReturnCode_t DynamicData::get_float32_value(ACE_CDR::Float& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float32_value(MemberId id, ACE_CDR::Float value)
{
}

DDS::ReturnCode_t DynamicData::get_float64_value(ACE_CDR::Double& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float64_value(MemberId id, ACE_CDR::Double value)
{
}

DDS::ReturnCode_t DynamicData::get_float128_value(ACE_CDR::LongDouble& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float128_value(MemberId id, ACE_CDR::LongDouble value)
{
}

DDS::ReturnCode_t DynamicData::get_char8_value(ACE_CDR::Char& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char8_value(MemberId id, ACE_CDR::Char value)
{
}

DDS::ReturnCode_t DynamicData::get_char16_value(ACE_CDR::WChar& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char16_value(MemberId id, ACE_CDR::WChar value)
{
}

DDS::ReturnCode_t DynamicData::get_byte_value(ACE_CDR::Octet& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_byte_value(MemberId id, ACE_CDR::Octet value)
{
}

DDS::ReturnCode_t DynamicData::get_boolean_value(ACE_CDR::Boolean& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_boolean_value(MemberId id, ACE_CDR::Boolean value)
{
}

DDS::ReturnCode_t DynamicData::get_string_value(DCPS::String& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_string_value(MemberId id, DCPS::String value)
{
}

DDS::ReturnCode_t DynamicData::get_wstring_value(DCPS::WString& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_wstring_value(MemberId id, DCPS::WString value)
{
}

DDS::ReturnCode_t DynamicData::get_complex_value(DynamicData& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_complex_value(MemberId id, DynamicData value)
{
}

DDS::ReturnCode_t DynamicData::get_int32_values(Int32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int32_values(MemberId id, const Int32Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_uint32_values(UInt32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint32_values(MemberId id, const UInt32Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_int8_values(Int8Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int8_values(MemberId id, const Int8Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_uint8_values(UInt8Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint8_values(MemberId id, const UInt8Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_int16_values(Int16Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int16_values(MemberId id, const Int16Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_uint16_values(UInt16Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint16_values(MemberId id, const UInt16Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_int64_values(Int64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_int64_values(MemberId id, const Int64Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_uint64_values(UInt64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_uint64_values(MemberId id, const UInt64Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_float32_values(Float32Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float32_values(MemberId id, const Float32Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_float64_values(Float64Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float64_values(MemberId id, const Float64Seq& value)
{
}

DDS::ReturnCode_t DynamicData::get_float128_values(Float128Seq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_float128_values(MemberId id, const Float128Seq& value);
{
}

DDS::ReturnCode_t DynamicData::get_char8_values(CharSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char8_values(MemberId id, const CharSeq& value)
{
}

DDS::ReturnCode_t DynamicData::get_char16_values(WCharSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_char16_values(MemberId id, const WCharSeq& value)
{
}

DDS::ReturnCode_t DynamicData::get_byte_values(ByteSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_byte_values(MemberId id, const ByteSeq& value)
{
}

DDS::ReturnCode_t DynamicData::get_boolean_values(BooleanSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_boolean_values(MemberId id, const BooleanSeq& value)
{
}

DDS::ReturnCode_t DynamicData::get_string_values(StringSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_string_values(MemberId id, const StringSeq& value)
{
}

DDS::ReturnCode_t DynamicData::get_wstring_values(WStringSeq& value, MemberId id) const
{
}

DDS::ReturnCode_t DynamicData::set_wstring_values(MemberId id, const WStringSeq& value)
{
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
