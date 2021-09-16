/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_H

#include "TypeObject.h"
#include "DynamicType.h"

#include <dds/DCPS/PoolAllocator.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class DynamicData {
public:
  DynamicData(Serializer& strm, DynamicType_rch type)
    : strm_(strm)
    , start_rpos_(strm_.rpos())
    , type_(type)
  {
    type_->get_descriptor(descriptor_);
  }

  DDS::ReturnCode_t get_descriptor(MemberDescriptor& value, MemberId id) const;
  DDS::ReturnCode_t set_descriptor(MemberId id, const MemberDescriptor& value);
  bool equals(const DynamicData& other) const;

  MemberId get_member_id_by_name(String name) const;
  MemberId get_member_id_at_index(ACE_CDR::ULong) const;
  ACE_CDR::ULong get_item_count() const;

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(MemberId id);

  DynamicData loan_value(MemberId id);
  DDS::ReturnCode_t return_loaned_value(const DynamicData& value);
  DynamicData clone() const;

  DDS::ReturnCode_t get_int32_value(ACE_CDR::Long& value, MemberId id) const;
  DDS::ReturnCode_t set_int32_value(MemberId id, ACE_CDR::Long value);

  DDS::ReturnCode_t get_uint32_value(ACE_CDR::ULong& value, MemberId id) const;
  DDS::ReturnCode_t set_uint32_value(MemberId id, ACE_CDR::ULong value);

  DDS::ReturnCode_t get_int8_value(ACE_CDR::Int8& value, MemberId id) const;
  DDS::ReturnCode_t set_int8_value(MemberId id, ACE_CDR::Int8 value);

  DDS::ReturnCode_t get_uint8_value(ACE_CDR::UInt8& value, MemberId id) const;
  DDS::ReturnCode_t set_uint8_value(MemberId id, ACE_CDR::UInt8 value);

  DDS::ReturnCode_t get_int16_value(ACE_CDR::Short& value, MemberId id) const;
  DDS::ReturnCode_t set_int16_value(MemberId id, ACE_CDR::Short value);

  DDS::ReturnCode_t get_uint16_value(ACE_CDR::UShort& value, MemberId id) const;
  DDS::ReturnCode_t set_uint16_value(MemberId id, ACE_CDR::UShort value);

  DDS::ReturnCode_t get_int64_value(ACE_CDR::LongLong& value, MemberId id) const;
  DDS::ReturnCode_t set_int64_value(MemberId id, ACE_CDR::LongLong value);

  DDS::ReturnCode_t get_uint64_value(ACE_CDR::ULongLong& value, MemberId id) const;
  DDS::ReturnCode_t set_uint64_value(MemberId id, ACE_CDR::ULongLong value);

  DDS::ReturnCode_t get_float32_value(ACE_CDR::Float& value, MemberId id) const;
  DDS::ReturnCode_t set_float32_value(MemberId id, ACE_CDR::Float value);

  DDS::ReturnCode_t get_float64_value(ACE_CDR::Double& value, MemberId id) const;
  DDS::ReturnCode_t set_float64_value(MemberId id, ACE_CDR::Double value);

  DDS::ReturnCode_t get_float128_value(ACE_CDR::LongDouble& value, MemberId id) const;
  DDS::ReturnCode_t set_float128_value(MemberId id, ACE_CDR::LongDouble value);

  DDS::ReturnCode_t get_char8_value(ACE_CDR::Char& value, MemberId id) const;
  DDS::ReturnCode_t set_char8_value(MemberId id, ACE_CDR::Char value);

  DDS::ReturnCode_t get_char16_value(ACE_CDR::WChar& value, MemberId id) const;
  DDS::ReturnCode_t set_char16_value(MemberId id, ACE_CDR::WChar value);

  DDS::ReturnCode_t get_byte_value(ACE_CDR::Octet& value, MemberId id) const;
  DDS::ReturnCode_t set_byte_value(MemberId id, ACE_CDR::Octet value);

  DDS::ReturnCode_t get_boolean_value(ACE_CDR::Boolean& value, MemberId id) const;
  DDS::ReturnCode_t set_boolean_value(MemberId id, ACE_CDR::Boolean value);

  DDS::ReturnCode_t get_string_value(DCPS::String& value, MemberId id) const;
  DDS::ReturnCode_t set_string_value(MemberId id, DCPS::String value);

  DDS::ReturnCode_t get_wstring_value(DCPS::WString& value, MemberId id) const;
  DDS::ReturnCode_t set_wstring_value(MemberId id, DCPS::WString value);

  DDS::ReturnCode_t get_complex_value(DynamicData& value, MemberId id) const;
  DDS::ReturnCode_t set_complex_value(MemberId id, DynamicData value);

  typedef Sequence<ACE_CDR::Long> Int32Seq;
  DDS::ReturnCode_t get_int32_values(Int32Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_int32_values(MemberId id, const Int32Seq& value);

  typedef Sequence<ACE_CDR::ULong> UInt32Seq;
  DDS::ReturnCode_t get_uint32_values(UInt32Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_uint32_values(MemberId id, const UInt32Seq& value);

  typedef Sequence<ACE_CDR::Int8> Int8Seq;
  DDS::ReturnCode_t get_int8_values(Int8Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_int8_values(MemberId id, const Int8Seq& value);

  typedef Sequence<ACE_CDR::UInt8> UInt8Seq;
  DDS::ReturnCode_t get_uint8_values(UInt8Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_uint8_values(MemberId id, const UInt8Seq& value);

  typedef Sequence<ACE_CDR::Short> Int16Seq;
  DDS::ReturnCode_t get_int16_values(Int16Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_int16_values(MemberId id, const Int16Seq& value);

  typedef Sequence<ACE_CDR::UShort> UInt16Seq;
  DDS::ReturnCode_t get_uint16_values(UInt16Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_uint16_values(MemberId id, const UInt16Seq& value);

  typedef Sequence<ACE_CDR::LongLong> Int64Seq;
  DDS::ReturnCode_t get_int64_values(Int64Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_int64_values(MemberId id, const Int64Seq& value);

  typedef Sequence<ACE_CDR::ULongLong> UInt64Seq;
  DDS::ReturnCode_t get_uint64_values(UInt64Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_uint64_values(MemberId id, const UInt64Seq& value);

  typedef Sequence<ACE_CDR::Float> Float32Seq;
  DDS::ReturnCode_t get_float32_values(Float32Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_float32_values(MemberId id, const Float32Seq& value);

  typedef Sequence<ACE_CDR::Double> Float64Seq;
  DDS::ReturnCode_t get_float64_values(Float64Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_float64_values(MemberId id, const Float64Seq& value);

  typedef Sequence<ACE_CDR::LongDouble> Float128Seq;
  DDS::ReturnCode_t get_float128_values(Float128Seq& value, MemberId id) const;
  DDS::ReturnCode_t set_float128_values(MemberId id, const Float128Seq& value);

  typedef Sequence<ACE_CDR::Char> CharSeq;
  DDS::ReturnCode_t get_char8_values(CharSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_char8_values(MemberId id, const CharSeq& value);

  typedef Sequence<ACE_CDR::WChar> WCharSeq;
  DDS::ReturnCode_t get_char16_values(WCharSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_char16_values(MemberId id, const WCharSeq& value);

  typedef Sequence<ACE_CDR::Octet> ByteSeq;
  DDS::ReturnCode_t get_byte_values(ByteSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_byte_values(MemberId id, const ByteSeq& value);

  typedef Sequence<ACE_CDR::Boolean> BooleanSeq;
  DDS::ReturnCode_t get_boolean_values(BooleanSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_boolean_values(MemberId id, const BooleanSeq& value);

  typedef Sequence<DCPS::String> StringSeq;
  DDS::ReturnCode_t get_string_values(StringSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_string_values(MemberId id, const StringSeq& value);

  typedef Sequence<DCPS::WString> WStringSeq;
  DDS::ReturnCode_t get_wstring_values(WStringSeq& value, MemberId id) const;
  DDS::ReturnCode_t set_wstring_values(MemberId id, const WStringSeq& value);

  /// Skip the whole part of the data stream that corresponds to this type.
  /// This is called by a containing type when it wants to skip a member which
  /// is an object of this type.
  bool skip_all();

private:

  template <typename ValueType, typename TypeKindCode>
  DDS::ReturnCode_t get_individual_value(ValueType& value, MemberId id);

  /// Move the read pointer to the member with a given ID and type kind.
  bool find_member(MemberId id, TypeKind kind);

  /// Reset the read pointer to point to the beginning of the stream.
  void reset_rpos();

  bool skip(const char* func_name, const char* description, size_t n, int size = 1);

  /// Skip a member at the given index of a final or appendable type.
  /// When the method is called, the read position of the stream
  /// must be at the beginning of the member.
  bool skip_member_by_index(ACE_CDR::ULong index);
  bool skip_member(DynamicType_rch type);

  bool skip_sequence_member(DynamicType_rch type);
  bool skip_array_member(DynamicType_rch type);
  bool skip_map_member(DynamicType_rch type);

  /// Skip a non-primitive collection member. That is, a sequence or an array of non-primitive
  /// elements, or a map with at least either key type or value type is non-primitive.
  bool skip_collection_member(TypeKind tk);

  bool skip_struct_member(DynamicType_rch type);
  bool skip_union_member(DynamicType_rch type);

  // These helper methods can be moved to DynamicType class.
  DynamicType_rch get_base_type(DynamicType_rch alias_type) const;
  bool is_primitive(DynamicType_rch type, ACE_CDR::ULong& size) const;
  const char* typekind_to_string(TypeKind tk) const;

  Serializer& strm_;
  const size_t start_rpos_;

  DynamicType_rch type_;
  TypeDescriptor descriptor_;

  /// Cache the offset from the point in the stream where the data of this DynamicData object
  /// starts to the point where a member with a given ID starts.
  OPENDDS_MAP<MemberId, size_t> offset_lookup_table_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
