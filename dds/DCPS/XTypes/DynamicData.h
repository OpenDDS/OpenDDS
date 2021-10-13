/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_H

#include "TypeObject.h"
#include "DynamicType.h"

#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/PoolAllocator.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class DynamicData {
public:
  // DynamicData can be constructed for any type kinds, except TK_ANNOTATION (since
  // data for annotations won't be serialized independently in a sample) and TK_BITSET
  // (the spec doesn't have enough information on how to handle bitsets).
  DynamicData(DCPS::Serializer& strm, DynamicType_rch type);
  DynamicData& operator=(const DynamicData& other);
  ~DynamicData() {}

  DDS::ReturnCode_t get_descriptor(MemberDescriptor& value, MemberId id) const;
  DDS::ReturnCode_t set_descriptor(MemberId id, const MemberDescriptor& value);
  bool equals(const DynamicData& other) const;

  MemberId get_member_id_by_name(DCPS::String name) const;
  MemberId get_member_id_at_index(ACE_CDR::ULong) const;
  ACE_CDR::ULong get_item_count() const;

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(MemberId id);

  DynamicData loan_value(MemberId id);
  DDS::ReturnCode_t return_loaned_value(const DynamicData& value);
  DynamicData clone() const;

  DDS::ReturnCode_t get_int32_value(ACE_CDR::Long& value, MemberId id);
  DDS::ReturnCode_t set_int32_value(MemberId id, ACE_CDR::Long value);

  DDS::ReturnCode_t get_uint32_value(ACE_CDR::ULong& value, MemberId id);
  DDS::ReturnCode_t set_uint32_value(MemberId id, ACE_CDR::ULong value);

  DDS::ReturnCode_t get_int8_value(ACE_CDR::Int8& value, MemberId id);
  DDS::ReturnCode_t set_int8_value(MemberId id, ACE_CDR::Int8 value);

  DDS::ReturnCode_t get_uint8_value(ACE_CDR::UInt8& value, MemberId id);
  DDS::ReturnCode_t set_uint8_value(MemberId id, ACE_CDR::UInt8 value);

  DDS::ReturnCode_t get_int16_value(ACE_CDR::Short& value, MemberId id);
  DDS::ReturnCode_t set_int16_value(MemberId id, ACE_CDR::Short value);

  DDS::ReturnCode_t get_uint16_value(ACE_CDR::UShort& value, MemberId id);
  DDS::ReturnCode_t set_uint16_value(MemberId id, ACE_CDR::UShort value);

  DDS::ReturnCode_t get_int64_value(ACE_CDR::LongLong& value, MemberId id);
  DDS::ReturnCode_t set_int64_value(MemberId id, ACE_CDR::LongLong value);

  DDS::ReturnCode_t get_uint64_value(ACE_CDR::ULongLong& value, MemberId id);
  DDS::ReturnCode_t set_uint64_value(MemberId id, ACE_CDR::ULongLong value);

  DDS::ReturnCode_t get_float32_value(ACE_CDR::Float& value, MemberId id);
  DDS::ReturnCode_t set_float32_value(MemberId id, ACE_CDR::Float value);

  DDS::ReturnCode_t get_float64_value(ACE_CDR::Double& value, MemberId id);
  DDS::ReturnCode_t set_float64_value(MemberId id, ACE_CDR::Double value);

  DDS::ReturnCode_t get_float128_value(ACE_CDR::LongDouble& value, MemberId id);
  DDS::ReturnCode_t set_float128_value(MemberId id, ACE_CDR::LongDouble value);

  DDS::ReturnCode_t get_char8_value(ACE_CDR::Char& value, MemberId id);
  DDS::ReturnCode_t set_char8_value(MemberId id, ACE_CDR::Char value);

  DDS::ReturnCode_t get_char16_value(ACE_CDR::WChar& value, MemberId id);
  DDS::ReturnCode_t set_char16_value(MemberId id, ACE_CDR::WChar value);

  DDS::ReturnCode_t get_byte_value(ACE_CDR::Octet& value, MemberId id);
  DDS::ReturnCode_t set_byte_value(MemberId id, ACE_CDR::Octet value);

  DDS::ReturnCode_t get_boolean_value(ACE_CDR::Boolean& value, MemberId id);
  DDS::ReturnCode_t set_boolean_value(MemberId id, ACE_CDR::Boolean value);

  DDS::ReturnCode_t get_string_value(DCPS::String& value, MemberId id);
  DDS::ReturnCode_t set_string_value(MemberId id, DCPS::String value);

  DDS::ReturnCode_t get_wstring_value(DCPS::WString& value, MemberId id);
  DDS::ReturnCode_t set_wstring_value(MemberId id, DCPS::WString value);

  DDS::ReturnCode_t get_complex_value(DynamicData& value, MemberId id);
  DDS::ReturnCode_t set_complex_value(MemberId id, DynamicData value);

  typedef Sequence<ACE_CDR::Long> Int32Seq;
  DDS::ReturnCode_t get_int32_values(Int32Seq& value, MemberId id);
  DDS::ReturnCode_t set_int32_values(MemberId id, const Int32Seq& value);

  typedef Sequence<ACE_CDR::ULong> UInt32Seq;
  DDS::ReturnCode_t get_uint32_values(UInt32Seq& value, MemberId id);
  DDS::ReturnCode_t set_uint32_values(MemberId id, const UInt32Seq& value);

  typedef Sequence<ACE_CDR::Int8> Int8Seq;
  DDS::ReturnCode_t get_int8_values(Int8Seq& value, MemberId id);
  DDS::ReturnCode_t set_int8_values(MemberId id, const Int8Seq& value);

  typedef Sequence<ACE_CDR::UInt8> UInt8Seq;
  DDS::ReturnCode_t get_uint8_values(UInt8Seq& value, MemberId id);
  DDS::ReturnCode_t set_uint8_values(MemberId id, const UInt8Seq& value);

  typedef Sequence<ACE_CDR::Short> Int16Seq;
  DDS::ReturnCode_t get_int16_values(Int16Seq& value, MemberId id);
  DDS::ReturnCode_t set_int16_values(MemberId id, const Int16Seq& value);

  typedef Sequence<ACE_CDR::UShort> UInt16Seq;
  DDS::ReturnCode_t get_uint16_values(UInt16Seq& value, MemberId id);
  DDS::ReturnCode_t set_uint16_values(MemberId id, const UInt16Seq& value);

  typedef Sequence<ACE_CDR::LongLong> Int64Seq;
  DDS::ReturnCode_t get_int64_values(Int64Seq& value, MemberId id);
  DDS::ReturnCode_t set_int64_values(MemberId id, const Int64Seq& value);

  typedef Sequence<ACE_CDR::ULongLong> UInt64Seq;
  DDS::ReturnCode_t get_uint64_values(UInt64Seq& value, MemberId id);
  DDS::ReturnCode_t set_uint64_values(MemberId id, const UInt64Seq& value);

  typedef Sequence<ACE_CDR::Float> Float32Seq;
  DDS::ReturnCode_t get_float32_values(Float32Seq& value, MemberId id);
  DDS::ReturnCode_t set_float32_values(MemberId id, const Float32Seq& value);

  typedef Sequence<ACE_CDR::Double> Float64Seq;
  DDS::ReturnCode_t get_float64_values(Float64Seq& value, MemberId id);
  DDS::ReturnCode_t set_float64_values(MemberId id, const Float64Seq& value);

  typedef Sequence<ACE_CDR::LongDouble> Float128Seq;
  DDS::ReturnCode_t get_float128_values(Float128Seq& value, MemberId id);
  DDS::ReturnCode_t set_float128_values(MemberId id, const Float128Seq& value);

  typedef Sequence<ACE_CDR::Char> CharSeq;
  DDS::ReturnCode_t get_char8_values(CharSeq& value, MemberId id);
  DDS::ReturnCode_t set_char8_values(MemberId id, const CharSeq& value);

  typedef Sequence<ACE_CDR::WChar> WCharSeq;
  DDS::ReturnCode_t get_char16_values(WCharSeq& value, MemberId id);
  DDS::ReturnCode_t set_char16_values(MemberId id, const WCharSeq& value);

  typedef Sequence<ACE_CDR::Octet> OctetSeq;
  DDS::ReturnCode_t get_byte_values(OctetSeq& value, MemberId id);
  DDS::ReturnCode_t set_byte_values(MemberId id, const OctetSeq& value);

  typedef Sequence<ACE_CDR::Boolean> BooleanSeq;
  DDS::ReturnCode_t get_boolean_values(BooleanSeq& value, MemberId id);
  DDS::ReturnCode_t set_boolean_values(MemberId id, const BooleanSeq& value);

  typedef Sequence<DCPS::String> StringSeq;
  DDS::ReturnCode_t get_string_values(StringSeq& value, MemberId id);
  DDS::ReturnCode_t set_string_values(MemberId id, const StringSeq& value);

  typedef Sequence<DCPS::WString> WStringSeq;
  DDS::ReturnCode_t get_wstring_values(WStringSeq& value, MemberId id);
  DDS::ReturnCode_t set_wstring_values(MemberId id, const WStringSeq& value);

  /// Skip the whole data corresponding to this type if it is a struct or union.
  /// This is called by a containing type when it wants to skip a member which
  /// is an object of this type.
  bool skip_all();

private:

  /// Verify that a given type is primitive or string or wstring.
  bool is_type_supported(TypeKind tk, const char* func_name);

  /// Wrappers for reading a single value as a given type.
  template<typename ValueType>
  bool read_value(ValueType& value, TypeKind tk);

  //  bool read_value(ACE_CDR::Int8& value, TypeKind);
  //  bool read_value(ACE_CDR::UInt8& value, TypeKind);
  //  bool read_value(ACE_CDR::Char& value, TypeKind);
  //  bool read_value(ACE_CDR::WChar& value, TypeKind);
  //  bool read_value(ACE_CDR::Octet& value, TypeKind);
  //  bool read_value(ACE_CDR::Boolean& value, TypeKind);
  bool read_value(DCPS::String& value, TypeKind);
  bool read_value(DCPS::WString& value, TypeKind);

  /// Templates for reading a single value of type primitive or string or
  /// wstring from a corresponding containing type.
  template<typename MemberType, TypeKind MemberTypeKind>
  bool get_value_from_struct(MemberType& value, MemberId id,
                             TypeKind enum_or_bitmask = TK_NONE,
                             LBound lower = 0,
                             LBound upper = 0);

  template<typename MemberType, TypeKind MemberTypeKind>
  bool get_value_from_union(MemberType& value, MemberId id,
                            TypeKind enum_or_bitmask = TK_NONE,
                            LBound lower = 0,
                            LBound upper = 0);

  template<typename ElementType, TypeKind ElementTypeKind>
  bool get_value_from_collection(ElementType& value, MemberId id, TypeKind collection_tk,
                                 TypeKind enum_or_bitmask = TK_NONE,
                                 LBound lower = 0,
                                 LBound upper = 0);

  template<typename ValueType, TypeKind ValueTypeKind>
  DDS::ReturnCode_t get_single_value(ValueType& value, MemberId id,
                                     TypeKind enum_or_bitmask = TK_NONE,
                                     LBound lower = 0,
                                     LBound upper = 0);

  template<typename UIntType, TypeKind UIntTypeKind>
  bool get_boolean_from_bitmask(ACE_CDR::ULong index, ACE_CDR::Boolean& value);

  /// Skip to a member with a given ID in a struct.
  bool skip_to_struct_member(const MemberDescriptor& member_desc, MemberId id);

  bool get_from_struct_common_checks(MemberDescriptor& md, MemberId id,
                                     TypeKind kind, bool is_sequence = false);

  /// Find member descriptor for the selected member from a union data.
  bool get_union_selected_member(MemberDescriptor& out_md);

  bool get_from_union_common_checks(MemberId id, const char* func_name, MemberDescriptor& md);

  /// Skip to an element with a given ID in a sequence or array.
  bool skip_to_sequence_element(MemberId id);
  bool skip_to_array_element(MemberId id);

  /// Skip to an element with a given ID in a map. The key associated with that
  /// element is also skipped.
  bool skip_to_map_element(MemberId id);

  /// Read a sequence with element type @a element_typekind and store in @a value,
  /// which is a sequence of primitives or strings or wstrings.
  template<typename SequenceType>
  bool read_values(SequenceType& value, TypeKind element_typekind);

  /*
  bool read_values(Int32Seq& value);
  bool read_values(UInt32Seq& value);
  bool read_values(Int8Seq& value);
  bool read_values(UInt8Seq& value);
  bool read_values(Int16Seq& value);
  bool read_values(UInt16Seq& value);
  bool read_values(Int64Seq& value);
  bool read_values(UInt64Seq& value);
  bool read_values(Float32Seq& value);
  bool read_values(Float64Seq& value);
  bool read_values(Float128Seq& value);
  bool read_values(CharSeq& value);
  bool read_values(WCharSeq& value);
  bool read_values(OctetSeq& value);
  bool read_values(BooleanSeq& value);
  */
  bool read_values(StringSeq& value, TypeKind);
  bool read_values(WStringSeq& value, TypeKind);

  // Templates for reading a sequence of primitives or strings or wstrings
  // as a member (or an element) of a given containing type.
  template<typename SequenceType, TypeKind ElementTypeKind>
  bool get_values_from_struct(SequenceType& value, MemberId id,
                              TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<typename SequenceType, TypeKind ElementTypeKind>
  bool get_values_from_union(SequenceType& value, MemberId id,
                             TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<typename SequenceType, TypeKind ElementTypeKind>
  bool get_values_from_sequence(SequenceType& value, MemberId id,
                                TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<typename SequenceType, TypeKind ElementTypeKind>
  bool get_values_from_array(SequenceType& value, MemberId id,
                             TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<typename SequenceType, TypeKind ElementTypeKind>
  bool get_values_from_map(SequenceType& value, MemberId id,
                           TypeKind enum_or_bitmask, LBound lower, LBound upper);

  /// Template that reads sequence of values from all valid containing types.
  template<typename SequenceType, TypeKind ElementTypeKind>
  DDS::ReturnCode_t get_sequence_values(SequenceType& value, MemberId id,
                                        TypeKind enum_or_bitmask = TK_NONE,
                                        LBound lower = 0,
                                        LBound upper = 0);

  /// Move the read pointer to the member with a given ID.
  /// In case the member is not a sequence, @a kind is the type kind of the member.
  /// If the requested member is a sequence, @a kind is the type kind of the
  /// elements of the sequence.
  bool find_struct_member(MemberId id, TypeKind kind, bool is_sequence = false);

  bool skip(const char* func_name, const char* description, size_t n, int size = 1);

  bool read_discriminator(const DynamicType_rch& disc_type, ExtensibilityKind union_ek, ACE_CDR::Long& label);

  /// Skip a member of a final or appendable struct at the given index.
  bool skip_struct_member_by_index(ACE_CDR::ULong index);

  /// Skip a member of the given type. The member can be a part of any containing type,
  /// such as a member in a struct or union, an element in a sequence or array, etc.
  /// Note that this assumes any header preceding this type, e.g. EMHEADER if this is
  /// a member of a mutable struct, is already consumed, and the read pointer is pointing
  /// to the actual data of the member.
  bool skip_member(DynamicType_rch member_type);

  /// Skip a member which is a sequence, array, or map.
  bool skip_sequence_member(DynamicType_rch type);
  bool skip_array_member(DynamicType_rch type);
  bool skip_map_member(DynamicType_rch type);

  /// Skip a non-primitive collection member. That is, a sequence or an array of non-primitive
  /// elements, or a map with at least either key type or value type is non-primitive.
  bool skip_collection_member(TypeKind tk);

  /// Skip a member which is a structure.
  bool skip_struct_member(DynamicType_rch type);

  /// Skip a member which is an union.
  bool skip_union_member(DynamicType_rch type);

  // These methods can be moved to DynamicType-related classes.
  DynamicType_rch get_base_type(DynamicType_rch alias_type) const;
  bool is_primitive(TypeKind tk, ACE_CDR::ULong& size) const;

  bool get_index_from_id(MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;
  const char* typekind_to_string(TypeKind tk) const;

  /// Reset the read pointer to point to the beginning of the stream.
  void reset_rpos();

  DCPS::Serializer& strm_;
  const size_t start_rpos_;

  DynamicType_rch type_;
  TypeDescriptor descriptor_;

  /// Cache the offset from the point in the stream where the data of this DynamicData object
  /// starts to the point where a member with a given ID starts.
  //  OPENDDS_MAP<MemberId, size_t> offset_lookup_table_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
