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
#include <dds/CorbaSeq/LongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ULongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/Int8SeqTypeSupportImpl.h>
#include <dds/CorbaSeq/UInt8SeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ShortSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/UShortSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/LongLongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/ULongLongSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/FloatSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/DoubleSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/LongDoubleSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/CharSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/WCharSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/OctetSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/BooleanSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/StringSeqTypeSupportImpl.h>
#include <dds/CorbaSeq/WStringSeqTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicData {
public:
  DynamicData();

  /// This creates a duplicated ACE_Message_Block chain from the provided chain.
  /// The duplicated chain is released when the object is destroyed. Caller is
  /// responsible for the release of the input message block chain.
  DynamicData(ACE_Message_Block* chain,
              const DCPS::Encoding& encoding,
              const DynamicType_rch& type);

  /// Use this when you want to pass the alignment state of a given Serializer object over.
  /// A typical use case would be when a part of the data has already been consumed from
  /// @a ser and you want to give the remaining to DynamicData.
  DynamicData(DCPS::Serializer& ser, const DynamicType_rch& type);

  DynamicData(const DynamicData& other);
  DynamicData& operator=(const DynamicData& other);

  ~DynamicData();

  DDS::ReturnCode_t get_descriptor(MemberDescriptor& value, MemberId id) const;
  DDS::ReturnCode_t set_descriptor(MemberId id, const MemberDescriptor& value);

  MemberId get_member_id_by_name(DCPS::String name) const;
  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DynamicData clone() const;

  DDS::ReturnCode_t get_int32_value(ACE_CDR::Long& value, MemberId id);
  DDS::ReturnCode_t get_uint32_value(ACE_CDR::ULong& value, MemberId id);
  DDS::ReturnCode_t get_int8_value(ACE_CDR::Int8& value, MemberId id);
  DDS::ReturnCode_t get_uint8_value(ACE_CDR::UInt8& value, MemberId id);
  DDS::ReturnCode_t get_int16_value(ACE_CDR::Short& value, MemberId id);
  DDS::ReturnCode_t get_uint16_value(ACE_CDR::UShort& value, MemberId id);
  DDS::ReturnCode_t get_int64_value(ACE_CDR::LongLong& value, MemberId id);
  DDS::ReturnCode_t get_uint64_value(ACE_CDR::ULongLong& value, MemberId id);
  DDS::ReturnCode_t get_float32_value(ACE_CDR::Float& value, MemberId id);
  DDS::ReturnCode_t get_float64_value(ACE_CDR::Double& value, MemberId id);
  DDS::ReturnCode_t get_float128_value(ACE_CDR::LongDouble& value, MemberId id);
  DDS::ReturnCode_t get_char8_value(ACE_CDR::Char& value, MemberId id);
  DDS::ReturnCode_t get_char16_value(ACE_CDR::WChar& value, MemberId id);
  DDS::ReturnCode_t get_byte_value(ACE_CDR::Octet& value, MemberId id);
  DDS::ReturnCode_t get_boolean_value(ACE_CDR::Boolean& value, MemberId id);
  DDS::ReturnCode_t get_string_value(ACE_CDR::Char*& value, MemberId id);
  DDS::ReturnCode_t get_wstring_value(ACE_CDR::WChar*& value, MemberId id);
  DDS::ReturnCode_t get_complex_value(DynamicData& value, MemberId id);

  DDS::ReturnCode_t get_int32_values(CORBA::LongSeq& value, MemberId id);
  DDS::ReturnCode_t get_uint32_values(CORBA::ULongSeq& value, MemberId id);
  DDS::ReturnCode_t get_int8_values(CORBA::Int8Seq& value, MemberId id);
  DDS::ReturnCode_t get_uint8_values(CORBA::UInt8Seq& value, MemberId id);
  DDS::ReturnCode_t get_int16_values(CORBA::ShortSeq& value, MemberId id);
  DDS::ReturnCode_t get_uint16_values(CORBA::UShortSeq& value, MemberId id);
  DDS::ReturnCode_t get_int64_values(CORBA::LongLongSeq& value, MemberId id);
  DDS::ReturnCode_t get_uint64_values(CORBA::ULongLongSeq& value, MemberId id);
  DDS::ReturnCode_t get_float32_values(CORBA::FloatSeq& value, MemberId id);
  DDS::ReturnCode_t get_float64_values(CORBA::DoubleSeq& value, MemberId id);
  DDS::ReturnCode_t get_float128_values(CORBA::LongDoubleSeq& value, MemberId id);
  DDS::ReturnCode_t get_char8_values(CORBA::CharSeq& value, MemberId id);
  DDS::ReturnCode_t get_char16_values(CORBA::WCharSeq& value, MemberId id);
  DDS::ReturnCode_t get_byte_values(CORBA::OctetSeq& value, MemberId id);
  DDS::ReturnCode_t get_boolean_values(CORBA::BooleanSeq& value, MemberId id);
  DDS::ReturnCode_t get_string_values(CORBA::StringSeq& value, MemberId id);
  DDS::ReturnCode_t get_wstring_values(CORBA::WStringSeq& value, MemberId id);

private:
  void copy(const DynamicData& other);

  /// Skip the whole data corresponding to this type if it is a struct or union.
  /// This is called by a containing type when it wants to skip a member which
  /// is an object of this type.
  bool skip_all();

  /// Verify that a given type is primitive or string or wstring.
  ///
  bool is_type_supported(TypeKind tk, const char* func_name);

  /// Setup the strm_ object so that it has the correct alignment state.
  ///
  void setup_stream(ACE_Message_Block* chain);

  /// Reading a single value as a given type. For instance, an enum with bit bound
  /// of 32 is read as an 32-bit integer and thus TK_INT32 should be passed to @a tk.
  template<typename ValueType>
  bool read_value(ValueType& value, TypeKind tk);

  ///@{
  /** Reading a value of type primitive, string, or wstring as a member of a struct, union,
   *  or a collection (sequence, array, map). TK_ENUM should be passed to @a enum_or_bitmask
   *  if @value is a 8-bit, 16-bit, or 32-bit signed integer type. In that case, @lower and
   *  @upper should be set to form the bit_bound range of the enum type that matches
   *  the number of bits of @a value. For instance, if we are reading a 8-bit integer, then
   *  @enum_or_bitmask is TK_ENUM, @a lower is 1 and @a upper is 8. This allows reading
   *  an enum with a particular bit bound as an integer with the matching size.
   *  Similarly, if we are reading an unsigned integer, set @a enum_or_bitmask to TK_BITMASK,
   *  and @a lower and @a upper to form a corresponding range for the bitmask's bit bound.
   */
  template<TypeKind MemberTypeKind, typename MemberType>
  bool get_value_from_struct(MemberType& value, MemberId id,
                             TypeKind enum_or_bitmask = TK_NONE,
                             LBound lower = 0,
                             LBound upper = 0);

  template<TypeKind MemberTypeKind, typename MemberType>
  bool get_value_from_union(MemberType& value, MemberId id,
                            TypeKind enum_or_bitmask = TK_NONE,
                            LBound lower = 0,
                            LBound upper = 0);

  template<TypeKind ElementTypeKind, typename ElementType>
  bool get_value_from_collection(ElementType& value, MemberId id, TypeKind collection_tk,
                                 TypeKind enum_or_bitmask = TK_NONE,
                                 LBound lower = 0,
                                 LBound upper = 0);
  ///@}

  /// Read a single value of type primitive (except char8, char16, and boolean), string,
  /// or wstring.
  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_single_value(ValueType& value, MemberId id,
                                     TypeKind enum_or_bitmask = TK_NONE,
                                     LBound lower = 0,
                                     LBound upper = 0);

  /// Common method to read a single char8 or char16 value.
  template<TypeKind CharKind, TypeKind StringKind, typename ToCharT, typename CharT>
  DDS::ReturnCode_t get_char_common(CharT& value, MemberId id);

  template<typename UIntType, TypeKind UIntTypeKind>
  bool get_boolean_from_bitmask(ACE_CDR::ULong index, ACE_CDR::Boolean& value);

  /// Skip to a member with a given ID in a struct.
  ///
  bool skip_to_struct_member(const MemberDescriptor& member_desc, MemberId id);

  bool get_from_struct_common_checks(MemberDescriptor& md, MemberId id,
                                     TypeKind kind, bool is_sequence = false);

  /// Find member descriptor for the selected member from a union data.
  /// In case the union doesn't have a selected member, @a out_md has an invalid id field.
  bool get_union_selected_member(MemberDescriptor& out_md);

  bool get_from_union_common_checks(MemberId id, const char* func_name, MemberDescriptor& md);

  ///@{
  /** Skip to an element with a given ID in a sequence or array. */
  bool skip_to_sequence_element(MemberId id);
  bool skip_to_array_element(MemberId id);
  ///@}

  /// Skip to an element with a given ID in a map. The key associated with that
  /// element is also skipped.
  bool skip_to_map_element(MemberId id);

  /// Read a sequence with element type @a elem_tk and store the result in @a value,
  /// which is a sequence of primitives or strings or wstrings. Sequence of enums or
  /// bitmasks are read as a sequence of signed and unsigned integers, respectively.
  /// In that case, @a elem_tk is set to TK_ENUM or TK_BITMASK.
  template<typename SequenceType>
  bool read_values(SequenceType& value, TypeKind elem_tk);

  ///@{
  /** Templates for reading a sequence of primitives, strings or wstrings
   *  as a member (or an element) of a given containing type. See get_value_from_struct
   *  and the similar methods for the use of @a enum_or_bitmask, @a lower, @a upper.
   */
  template<TypeKind ElementTypeKind, typename SequenceType>
  bool get_values_from_struct(SequenceType& value, MemberId id,
                              TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool get_values_from_union(SequenceType& value, MemberId id,
                             TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool get_values_from_sequence(SequenceType& value, MemberId id,
                                TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool get_values_from_array(SequenceType& value, MemberId id,
                             TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool get_values_from_map(SequenceType& value, MemberId id,
                           TypeKind enum_or_bitmask, LBound lower, LBound upper);
  ///@}

  /// Common method to read a value sequence of any type (primitive, string, wstring).
  template<TypeKind ElementTypeKind, typename SequenceType>
  DDS::ReturnCode_t get_sequence_values(SequenceType& value, MemberId id,
                                        TypeKind enum_or_bitmask = TK_NONE,
                                        LBound lower = 0,
                                        LBound upper = 0);

  bool skip(const char* func_name, const char* description, size_t n, int size = 1);

  bool read_discriminator(const DynamicType_rch& disc_type, ExtensibilityKind union_ek, ACE_CDR::Long& label);

  /// Skip a member of a final or appendable struct at the given index.
  ///
  bool skip_struct_member_by_index(ACE_CDR::ULong index);

  /// Skip a member with the given type. The member can be a part of any containing type,
  /// such as a member in a struct or union, an element in a sequence or array, etc.
  /// Note that this assumes any header preceding this type, e.g. EMHEADER if this is
  /// a member of a mutable struct, is already consumed, and the read pointer is pointing
  /// to the actual data of the member.
  bool skip_member(DynamicType_rch member_type);

  ///@{
  /** Skip a member which is a sequence, array, or map. */
  bool skip_sequence_member(DynamicType_rch type);
  bool skip_array_member(DynamicType_rch type);
  bool skip_map_member(DynamicType_rch type);
  ///@}

  /// Skip a non-primitive collection member. That is, a sequence or an array of non-primitive
  /// elements, or a map with at least either key type or value type is non-primitive.
  bool skip_collection_member(TypeKind tk);

  /// Skip a member which is a structure or a union.
  bool skip_aggregated_member(const DynamicType_rch& type);

  void release_chains();

  // TODO: This method can be moved to DynamicType-related classes.
  DynamicType_rch get_base_type(const DynamicType_rch& alias_type) const;
  bool is_primitive(TypeKind tk, ACE_CDR::ULong& size) const;

  bool get_index_from_id(MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;
  const char* typekind_to_string(TypeKind tk) const;

  /// A duplicate of the original message block chain passed from the constructor.
  /// This is released in the destructor.
  ACE_Message_Block* chain_;

  DCPS::Encoding encoding_;

  /// Indicate whether the alignment state of a Serializer object associated
  /// with this DynamicData needs to be reset.
  bool reset_align_state_;

  /// The alignment state that a Serializer object associated with this DynamicData
  /// object will be set to.
  DCPS::Serializer::RdState align_state_;

  /// Each public interface creates a new Serializer object with a message block
  /// chain that is a duplicate from chain_.
  DCPS::Serializer strm_;

  /// Message block chains created during each get_*_value or get_*_values method's
  /// execution that need to be released when the method ends. Those chains are created
  /// when the method skips a nested aggregated type (i.e., struct and union) by
  /// calling skip_aggregated_member().
  OPENDDS_VECTOR(ACE_Message_Block*) chains_to_release;

  /// This DynamicData object holds data for this type.
  DynamicType_rch type_;

  /// Cache the descriptor for the same type for convenience.
  TypeDescriptor descriptor_;

  const ACE_CDR::ULong ITEM_COUNT_INVALID = UINT32_MAX;

  /// Cache the number of items (i.e., members or elements) in the data it holds.
  ACE_CDR::ULong item_count_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
