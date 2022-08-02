/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "TypeObject.h"
#include "DynamicTypeImpl.h"

#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/Serializer.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// To align with XTypes v1.3 Annex C, names for sequences of built-in types are
// provided in the same namespace as the DynamicData class.  If Safety Profile
// is enabled, these sequences are defined in OpenDDS.  Otherwise they come from
// TAO since they are part of the IDL-to-C++ mapping spec.

class OpenDDS_Dcps_Export DynamicDataImpl : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataImpl();

  /// This creates a duplicated ACE_Message_Block chain from the provided chain.
  /// The duplicated chain is released when the object is destroyed. Caller is
  /// responsible for the release of the input message block chain.
  DynamicDataImpl(ACE_Message_Block* chain,
                  const DCPS::Encoding& encoding,
                  DDS::DynamicType_ptr type);

  /// Use this when you want to pass the alignment state of a given Serializer object over.
  /// A typical use case would be when a part of the data has already been consumed from
  /// @a ser and you want to give the remaining to DynamicData.
  DynamicDataImpl(DCPS::Serializer& ser, DDS::DynamicType_ptr type);

  DynamicDataImpl(const DynamicDataImpl& other);
  DynamicDataImpl& operator=(const DynamicDataImpl& other);

  ~DynamicDataImpl();

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::ReturnCode_t set_descriptor(MemberId id, DDS::MemberDescriptor* value);

  MemberId get_member_id_by_name(const char* name);
  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId /*id*/);
  DDS::DynamicData_ptr loan_value(DDS::MemberId /*id*/);
  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr /*value*/);

  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int32_value(CORBA::Long& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int32_value(DDS::MemberId,
                                    CORBA::Long)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint32_value(DDS::MemberId,
                                     CORBA::ULong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int8_value(CORBA::Int8& value,
                                   DDS::MemberId id);
  DDS::ReturnCode_t set_int8_value(DDS::MemberId,
                                   CORBA::Int8)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_uint8_value(DDS::MemberId,
                                    CORBA::UInt8)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int16_value(CORBA::Short& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int16_value(DDS::MemberId,
                                    CORBA::Short)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint16_value(DDS::MemberId,
                                     CORBA::UShort)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int64_value(DDS::MemberId,
                                    CORBA::LongLong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint64_value(DDS::MemberId,
                                     CORBA::ULongLong)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float32_value(CORBA::Float& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_float32_value(DDS::MemberId,
                                      CORBA::Float)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float64_value(CORBA::Double& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_float64_value(DDS::MemberId,
                                      CORBA::Double)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float128_value(DDS::MemberId,
                                       CORBA::LongDouble)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char8_value(CORBA::Char& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_char8_value(DDS::MemberId,
                                    CORBA::Char)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char16_value(CORBA::WChar& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_char16_value(DDS::MemberId,
                                     CORBA::WChar)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_byte_value(CORBA::Octet& value,
                                   DDS::MemberId id);
  DDS::ReturnCode_t set_byte_value(DDS::MemberId,
                                   CORBA::Octet)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_boolean_value(DDS::MemberId,
                                      CORBA::Boolean)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_string_value(char*& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_string_value(DDS::MemberId,
                                     const char*)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_wstring_value(DDS::MemberId,
                                      const CORBA::WChar *)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_complex_value(DDS::MemberId,
                                      DDS::DynamicData_ptr)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int32_values(DDS::MemberId,
                                     const DDS::Int32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint32_values(DDS::MemberId,
                                      const DDS::UInt32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int8_values(DDS::MemberId,
                                    const DDS::Int8Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint8_values(DDS::MemberId,
                                     const DDS::UInt8Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int16_values(DDS::MemberId,
                                     const DDS::Int16Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint16_values(DDS::MemberId,
                                      const DDS::UInt16Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int64_values(DDS::MemberId,
                                     const DDS::Int64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint64_values(DDS::MemberId,
                                      const DDS::UInt64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float32_values(DDS::MemberId,
                                       const DDS::Float32Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float64_values(DDS::MemberId,
                                       const DDS::Float64Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq& value,
                                        DDS::MemberId id);
  DDS::ReturnCode_t set_float128_values(DDS::MemberId,
                                        const DDS::Float128Seq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_char8_values(DDS::MemberId,
                                     const DDS::CharSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_char16_values(DDS::MemberId,
                                      const DDS::WcharSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_byte_values(DDS::MemberId,
                                    const DDS::ByteSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_boolean_values(DDS::MemberId,
                                       const DDS::BooleanSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_string_values(DDS::StringSeq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_string_values(DDS::MemberId,
                                      const DDS::StringSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_wstring_values(DDS::MemberId,
                                       const DDS::WstringSeq&)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }

  DDS::DynamicType_ptr type();

  bool check_xcdr1_mutable(DDS::DynamicType_ptr dt);

  CORBA::Boolean equals(DDS::DynamicData_ptr other);

private:

  class ScopedChainManager {
  public:
    explicit ScopedChainManager(DynamicDataImpl& dd)
      : dd_(dd)
      , dup_(dd_.chain_->duplicate())
    {
      dd_.setup_stream(dup_.get());
    }

    ~ScopedChainManager()
    {
      dd_.release_chains();
    }

  private:
    DynamicDataImpl& dd_;
    DCPS::Message_Block_Ptr dup_;
  };

  void copy(const DynamicDataImpl& other);

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
  bool skip_to_struct_member(DDS::MemberDescriptor* member_desc, MemberId id);

  bool get_from_struct_common_checks(DDS::MemberDescriptor_var& md, MemberId id,
                                     TypeKind kind, bool is_sequence = false);

  /// Return the member descriptor for the selected member from a union data or null.
  DDS::MemberDescriptor* get_union_selected_member();

  DDS::MemberDescriptor* get_from_union_common_checks(MemberId id, const char* func_name);

  ///@{
  /** Skip to an element with a given ID in a sequence or array, or skip the entire collection. */
  bool skip_to_sequence_element(MemberId id, DDS::DynamicType_ptr coll_type = 0);
  bool skip_to_array_element(MemberId id, DDS::DynamicType_ptr coll_type = 0);
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

  bool read_discriminator(const DDS::DynamicType_ptr disc_type, DDS::ExtensibilityKind union_ek, ACE_CDR::Long& label);

  /// Skip a member of a final or appendable struct at the given index.
  ///
  bool skip_struct_member_at_index(ACE_CDR::ULong index, ACE_CDR::ULong& num_skipped);

  /// Skip a member with the given type. The member can be a part of any containing type,
  /// such as a member in a struct or union, an element in a sequence or array, etc.
  /// Note that this assumes any header preceding this type, e.g. EMHEADER if this is
  /// a member of a mutable struct, is already consumed, and the read pointer is pointing
  /// to the actual data of the member.
  bool skip_member(DDS::DynamicType_ptr member_type);

  ///@{
  /** Skip a member which is a sequence, array, or map. */
  bool skip_sequence_member(DDS::DynamicType_ptr type);
  bool skip_array_member(DDS::DynamicType_ptr type);
  bool skip_map_member(DDS::DynamicType_ptr type);
  ///@}

  /// Skip a non-primitive collection member. That is, a sequence or an array of non-primitive
  /// elements, or a map with at least either key type or value type is non-primitive.
  bool skip_collection_member(DDS::DynamicType_ptr coll_type);

  /// Skip a member which is a structure or a union.
  bool skip_aggregated_member(DDS::DynamicType_ptr type);

  void release_chains();

  bool is_primitive(TypeKind tk) const;
  bool get_primitive_size(DDS::DynamicType_ptr dt, ACE_CDR::ULong& size) const;

  bool has_optional_member(bool& has_optional) const;

  bool get_index_from_id(MemberId id, ACE_CDR::ULong& index, ACE_CDR::ULong bound) const;
  const char* typekind_to_string(TypeKind tk) const;

  /// A set of strings used to prevent infinite recursion when checking for XCDR1 Mutable
  typedef OPENDDS_SET(DCPS::String) DynamicTypeNameSet;
  bool check_xcdr1_mutable_i(DDS::DynamicType_ptr dt, DynamicTypeNameSet& dtns);

  typedef OPENDDS_VECTOR(ACE_Message_Block*) IntermediateChains;
  const IntermediateChains& get_intermediate_chains() const { return chains_to_release; }

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
  IntermediateChains chains_to_release;

  /// This DynamicData object holds data for this type.
  DDS::DynamicType_var type_;

  static const ACE_CDR::ULong ITEM_COUNT_INVALID = ACE_UINT32_MAX;

  /// Cache the number of items (i.e., members or elements) in the data it holds.
  ACE_CDR::ULong item_count_;
};

OpenDDS_Dcps_Export bool print_dynamic_data(DDS::DynamicData_ptr dd,
                                            DCPS::String& type_string,
                                            DCPS::String& indent);

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
