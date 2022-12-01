/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataBase.h"

#  include <dds/DCPS/FilterEvaluator.h>
#  include <dds/DdsDcpsCoreTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace XTypes {
class DynamicDataImpl;
}

namespace DCPS {
bool serialized_size(const Encoding& encoding, size_t& size, const XTypes::DynamicDataImpl& data);
bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl& data);
}

namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataImpl : public DynamicDataBase {
public:
  DynamicDataImpl(DDS::DynamicType_ptr type);

  DDS::DynamicType_ptr type();

  DDS::ReturnCode_t set_descriptor(MemberId id, DDS::MemberDescriptor* value);

  CORBA::Boolean equals(DDS::DynamicData_ptr other);

  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId /*id*/);
  DDS::DynamicData_ptr loan_value(DDS::MemberId /*id*/);
  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr /*value*/);

  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int32_value(CORBA::Long&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value);

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value);

  DDS::ReturnCode_t get_int8_value(CORBA::Int8&,
                                   DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value);

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value);

  DDS::ReturnCode_t get_int16_value(CORBA::Short&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value);

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value);

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value);

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value);

  DDS::ReturnCode_t get_float32_value(CORBA::Float&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value);

  DDS::ReturnCode_t get_float64_value(CORBA::Double&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value);

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble&,
                                       DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value);

  DDS::ReturnCode_t get_char8_value(CORBA::Char&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value);

  DDS::ReturnCode_t get_char16_value(CORBA::WChar&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value);

  DDS::ReturnCode_t get_byte_value(CORBA::Octet&,
                                   DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value);

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value);

  DDS::ReturnCode_t get_string_value(char*&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value);

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value);

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value);

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_values(DDS::MemberId id,
                                     const DDS::Int32Seq& value);

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_values(DDS::MemberId id,
                                      const DDS::UInt32Seq& value);

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_values(DDS::MemberId id,
                                    const DDS::Int8Seq& value);

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_values(DDS::MemberId id,
                                     const DDS::UInt8Seq& value);

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_values(DDS::MemberId id,
                                     const DDS::Int16Seq& value);

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_values(DDS::MemberId id,
                                      const DDS::UInt16Seq& value);

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_values(DDS::MemberId id,
                                     const DDS::Int64Seq& value);

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_values(DDS::MemberId id,
                                      const DDS::UInt64Seq& value);

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_values(DDS::MemberId id,
                                       const DDS::Float32Seq& value);

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_values(DDS::MemberId id,
                                       const DDS::Float64Seq& value);

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_values(DDS::MemberId id,
                                        const DDS::Float128Seq& value);

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_values(DDS::MemberId id,
                                     const DDS::CharSeq& value);

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_values(DDS::MemberId id,
                                      const DDS::WcharSeq& value);

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_values(DDS::MemberId id,
                                    const DDS::ByteSeq& value);

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_values(DDS::MemberId id,
                                       const DDS::BooleanSeq& value);

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_values(DDS::MemberId id,
                                      const DDS::StringSeq& value);

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq&,
                                       DDS::MemberId)
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_values(DDS::MemberId id,
                                       const DDS::WstringSeq& value);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  DDS::ReturnCode_t get_simple_value(DCPS::Value& value, DDS::MemberId id);
#endif

private:
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  DDS::ReturnCode_t get_simple_value_boolean(DCPS::Value& value, DDS::MemberId id) const;
  DDS::ReturnCode_t get_simple_value_char(DCPS::Value& value, DDS::MemberId id) const;
  template<typename ValueType>
  DDS::ReturnCode_t get_simple_value_primitive(DCPS::Value& value, DDS::MemberId id) const;
  DDS::ReturnCode_t get_simple_value_string(DCPS::Value& value, DDS::MemberId id) const;
#endif

  bool is_basic_type(TypeKind tk) const;

  template<TypeKind MemberTypeKind, typename MemberType>
  bool set_value_to_struct(DDS::MemberId id, const MemberType& value,
    TypeKind enum_or_bitmask = TK_NONE, LBound lower = 0, LBound upper = 0);

  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_boolean& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_octet& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_char& value) const;
#ifdef DDS_HAS_WCHAR
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_wchar& value) const;
#endif
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_int8& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const ACE_OutputCDR::from_uint8& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::Short& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::UShort& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::Long& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::ULong& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::LongLong& value) const;
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const CORBA::ULongLong& value) const;

  template<typename MemberType>
  bool cast_to_discriminator_value(CORBA::Long& disc_value, const MemberType& value) const;

  template<TypeKind MemberTypeKind, typename MemberType>
  bool set_value_to_union(DDS::MemberId id, const MemberType& value,
    TypeKind enum_or_bitmask = TK_NONE, LBound lower = 0, LBound upper = 0);

  template<TypeKind ElementTypeKind, typename ElementType>
  bool set_value_to_collection(DDS::MemberId id, const ElementType& value,
    TypeKind coll_tk, TypeKind enum_or_bitmask = TK_NONE, LBound lower = 0, LBound upper = 0);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t set_single_value(DDS::MemberId id, const ValueType& value,
    TypeKind enum_or_bitmask = TK_NONE, LBound lower = 0, LBound upper = 0);

  template<TypeKind CharKind, TypeKind StringKind, typename FromCharT>
  DDS::ReturnCode_t set_char_common(DDS::MemberId id, const FromCharT& value);

  bool check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const;
  static bool is_valid_discriminator_type(TypeKind tk);
  bool is_default_member_selected(CORBA::Long disc_val, DDS::MemberId default_id) const;
  bool read_discriminator(CORBA::Long& disc_val) const;
  DDS::MemberId find_selected_member() const;
  bool validate_discriminator(CORBA::Long disc_val, const DDS::MemberDescriptor_var& md) const;
  bool find_selected_member_and_discriminator(DDS::MemberId& selected_id,
    bool& has_disc, CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type) const;
  bool set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_ptr value);
  bool set_complex_to_union(DDS::MemberId id, DDS::DynamicData_ptr value,
                            const DDS::TypeDescriptor_var& descriptor);
  bool set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_ptr value, TypeKind tk);
  bool validate_member_id_collection(const DDS::TypeDescriptor_var& descriptor,
                                     DDS::MemberId id, TypeKind collection_tk) const;

  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_int8& value);
  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_uint8& value);
  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_char& value);
  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_octet& value);
  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_boolean& value);
#ifdef DDS_HAS_WCHAR
  bool insert_single(DDS::MemberId id, const ACE_OutputCDR::from_wchar& value);
#endif

  template<typename SingleType>
  bool insert_single(DDS::MemberId id, const SingleType& value);

  bool insert_complex(DDS::MemberId id, const DDS::DynamicData_var& value);

  template<typename SequenceType>
  bool insert_sequence(DDS::MemberId id, const SequenceType& value);

  template<TypeKind ElementTypeKind>
  bool check_seqmem_in_struct_and_union(DDS::MemberId id, TypeKind enum_or_bitmask,
                                        LBound lower, LBound upper) const;
  template<TypeKind ElementTypeKind>
  bool check_seqmem_in_sequence_and_array(DDS::MemberId id, CORBA::ULong bound,
                                          TypeKind enum_or_bitmask, LBound lower, LBound upper) const;

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_struct(DDS::MemberId id, const SequenceType& value,
                            TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_union(DDS::MemberId id, const SequenceType& value,
                           TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_sequence(DDS::MemberId id, const SequenceType& value,
                              TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_array(DDS::MemberId id, const SequenceType& value,
                           TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename SequenceType>
  DDS::ReturnCode_t set_sequence_values(DDS::MemberId id, const SequenceType& value,
                                        TypeKind enum_or_bitmask = TK_NONE,
                                        LBound lower = 0, LBound upper = 0);

  // Contain data for an instance of a basic type.
  struct SingleValue {
    SingleValue(CORBA::Long int32);
    SingleValue(CORBA::ULong uint32);
    SingleValue(ACE_OutputCDR::from_int8 from_int8);
    SingleValue(ACE_OutputCDR::from_uint8 from_uint8);
    SingleValue(CORBA::Short int16);
    SingleValue(CORBA::UShort uint16);
    SingleValue(CORBA::LongLong int64);
    SingleValue(CORBA::ULongLong uint64);
    SingleValue(CORBA::Float float32);
    SingleValue(CORBA::Double float64);
    SingleValue(CORBA::LongDouble float128);
    SingleValue(ACE_OutputCDR::from_char from_char);
    SingleValue(ACE_OutputCDR::from_octet from_octet);
    SingleValue(ACE_OutputCDR::from_boolean from_bool);
    SingleValue(const char* str);
#ifdef DDS_HAS_WCHAR
    SingleValue(ACE_OutputCDR::from_wchar from_wchar);
    SingleValue(const CORBA::WChar* wstr);
#endif

    ~SingleValue();

    template<typename T> const T& get() const;

    TypeKind kind_;
    // Used for types that need ACE_OutputCDR disambiguators.
    void* active_;
    union {
      CORBA::Long int32_;
      CORBA::ULong uint32_;
      unsigned char int8_[sizeof(ACE_OutputCDR::from_int8)];
      unsigned char uint8_[sizeof(ACE_OutputCDR::from_uint8)];
      CORBA::Short int16_;
      CORBA::UShort uint16_;
      CORBA::LongLong int64_;
      CORBA::ULongLong uint64_;
      CORBA::Float float32_;
      CORBA::Double float64_;
      CORBA::LongDouble float128_;
      unsigned char char8_[sizeof(ACE_OutputCDR::from_char)];
      unsigned char byte_[sizeof(ACE_OutputCDR::from_octet)];
      unsigned char boolean_[sizeof(ACE_OutputCDR::from_boolean)];
      const char* str_;
#ifdef DDS_HAS_WCHAR
      unsigned char char16_[sizeof(ACE_OutputCDR::from_wchar)];
      const CORBA::WChar* wstr_;
#endif
    };
  };

  struct SequenceValue {
    SequenceValue(const DDS::Int32Seq& int32_seq);
    SequenceValue(const DDS::UInt32Seq& uint32_seq);
    SequenceValue(const DDS::Int8Seq& int8_seq);
    SequenceValue(const DDS::UInt8Seq& uint8_seq);
    SequenceValue(const DDS::Int16Seq& int16_seq);
    SequenceValue(const DDS::UInt16Seq& uint16_seq);
    SequenceValue(const DDS::Int64Seq& int64_seq);
    SequenceValue(const DDS::UInt64Seq& uint64_seq);
    SequenceValue(const DDS::Float32Seq& float32_seq);
    SequenceValue(const DDS::Float64Seq& float64_seq);
    SequenceValue(const DDS::Float128Seq& float128_seq);
    SequenceValue(const DDS::CharSeq& char8_seq);
    SequenceValue(const DDS::ByteSeq& byte_seq);
    SequenceValue(const DDS::BooleanSeq& boolean_seq);
    SequenceValue(const DDS::StringSeq& str_seq);
#ifdef DDS_HAS_WCHAR
    SequenceValue(const DDS::WcharSeq& char16_seq);
    SequenceValue(const DDS::WstringSeq& wstr_seq);
#endif

    ~SequenceValue();

    template<typename T> const T& get() const;

    TypeKind elem_kind_;
    void* active_;
    union {
#define SEQUENCE_VALUE_MEMBER(T, N) unsigned char N ## _[sizeof(T)]
      SEQUENCE_VALUE_MEMBER(DDS::Int32Seq, int32_seq);
      SEQUENCE_VALUE_MEMBER(DDS::UInt32Seq, uint32_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Int8Seq, int8_seq);
      SEQUENCE_VALUE_MEMBER(DDS::UInt8Seq, uint8_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Int16Seq, int16_seq);
      SEQUENCE_VALUE_MEMBER(DDS::UInt16Seq, uint16_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Int64Seq, int64_seq);
      SEQUENCE_VALUE_MEMBER(DDS::UInt64Seq, uint64_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Float32Seq, float32_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Float64Seq, float64_seq);
      SEQUENCE_VALUE_MEMBER(DDS::Float128Seq, float128_seq);
      SEQUENCE_VALUE_MEMBER(DDS::CharSeq, char8_seq);
      SEQUENCE_VALUE_MEMBER(DDS::ByteSeq, byte_seq);
      SEQUENCE_VALUE_MEMBER(DDS::BooleanSeq, boolean_seq);
      SEQUENCE_VALUE_MEMBER(DDS::StringSeq, string_seq);
#ifdef DDS_HAS_WCHAR
      SEQUENCE_VALUE_MEMBER(DDS::WcharSeq, char16_seq);
      SEQUENCE_VALUE_MEMBER(DDS::WstringSeq, wstring_seq);
#endif
#undef SEQUENCE_VALUE_MEMBER
    };
  };

  typedef OPENDDS_VECTOR(CORBA::ULong) IndexToIdMap;

  // Container for all data written to this DynamicData object.
  // At anytime, there can be at most 1 entry for any given MemberId in all maps.
  // That is, each member is stored in at most 1 map.
  struct DataContainer {
    typedef OPENDDS_MAP(DDS::MemberId, SingleValue)::const_iterator const_single_iterator;
    typedef OPENDDS_MAP(DDS::MemberId, SequenceValue)::const_iterator const_sequence_iterator;
    typedef OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var)::const_iterator const_complex_iterator;

    DataContainer(const DDS::DynamicType_var& type, const DynamicDataImpl* data)
      : type_(type), data_(data) {}

    // Get the largest index of all elements in each map.
    // Call only for collection-like types (sequence, string, etc).
    // Must be called with a non-empty map.
    bool get_largest_single_index(CORBA::ULong& index) const;
    bool get_largest_sequence_index(CORBA::ULong& index) const;
    bool get_largest_complex_index(CORBA::ULong& index) const;

    // Get the largest index in a collection of a basic type (sequence, string, array).
    // This assumes there is at least 1 value of a basic type stored.
    bool get_largest_index_basic(CORBA::ULong& index) const;

    // Get the largest index in a collection of sequences of basic type
    // (each element of the collection is a sequence of a basic type).
    // Assuming at least 1 sequence is stored.
    bool get_largest_index_basic_sequence(CORBA::ULong& index) const;

    bool serialize_single_value(DCPS::Serializer& ser, const SingleValue& sv) const;

    template<typename PrimitiveType>
    bool serialize_primitive_value(DCPS::Serializer& ser, PrimitiveType default_value) const;
    bool serialized_size_enum(const DCPS::Encoding& encoding,
                              size_t& size, const DDS::DynamicType_var& enum_type) const;
    bool serialize_enum_default_value(DCPS::Serializer& ser,
                                      const DDS::DynamicType_var& enum_type) const;
    bool serialize_enum_value(DCPS::Serializer& ser) const;
    bool serialized_size_bitmask(const DCPS::Encoding& encoding,
                                 size_t& size, const DDS::DynamicType_var& bitmask_type) const;
    bool serialize_bitmask_default_value(DCPS::Serializer& ser,
                                         const DDS::DynamicType_var& bitmask_type) const;
    bool serialize_bitmask_value(DCPS::Serializer& ser) const;
    bool reconstruct_string_value(CORBA::Char* str) const;
    bool serialized_size_string(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_string_value(DCPS::Serializer& ser) const;
    bool reconstruct_wstring_value(CORBA::WChar* wstr) const;
    bool serialized_size_wstring(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_wstring_value(DCPS::Serializer& ser) const;
    void serialized_size_primitive_sequence(const DCPS::Encoding& encoding, size_t& size,
                                            TypeKind elem_tk, CORBA::ULong length) const;

    void set_default_basic_value(CORBA::Long& value) const;
    void set_default_basic_value(CORBA::ULong& value) const;
    void set_default_basic_value(ACE_OutputCDR::from_int8& value) const;
    void set_default_basic_value(ACE_OutputCDR::from_uint8& value) const;
    void set_default_basic_value(CORBA::Short& value) const;
    void set_default_basic_value(CORBA::UShort& value) const;
    void set_default_basic_value(CORBA::LongLong& value) const;
    void set_default_basic_value(CORBA::ULongLong& value) const;
    void set_default_basic_value(CORBA::Float& value) const;
    void set_default_basic_value(CORBA::Double& value) const;
    void set_default_basic_value(CORBA::LongDouble& value) const;
    void set_default_basic_value(ACE_OutputCDR::from_char& value) const;
    void set_default_basic_value(ACE_OutputCDR::from_octet& value) const;
    void set_default_basic_value(const char*& value) const;
    void set_default_basic_value(ACE_OutputCDR::from_boolean& value) const;
#ifdef DDS_HAS_WCHAR
    void set_default_basic_value(ACE_OutputCDR::from_wchar& value) const;
    void set_default_basic_value(const CORBA::WChar*& value) const;
#endif

    void set_default_primitive_values(DDS::Int8Seq& collection) const;
    void set_default_primitive_values(DDS::UInt8Seq& collection) const;
    void set_default_primitive_values(DDS::CharSeq& collection) const;
    void set_default_primitive_values(DDS::ByteSeq& collection) const;
    void set_default_primitive_values(DDS::BooleanSeq& collection) const;
#ifdef DDS_HAS_WCHAR
    void set_default_primitive_values(DDS::WcharSeq& collection) const;
#endif

    template<typename CollectionType>
    void set_default_primitive_values(CollectionType& collection) const;

    template<typename ElementType, typename CollectionType>
    bool set_primitive_values(CollectionType& collection, CORBA::ULong bound,
                              const ElementType& elem_tag) const;

    template<typename ElementType, typename CollectionType>
    bool reconstruct_primitive_collection(CollectionType& collection,
      CORBA::ULong size, CORBA::ULong bound, const ElementType& elem_tag) const;

    bool serialize_primitive_sequence(DCPS::Serializer& ser, TypeKind elem_tk,
                                      CORBA::ULong size, CORBA::ULong bound) const;

    void serialized_size_string_common(const DCPS::Encoding& encoding, size_t& size,
                                       const char* str) const;
#ifdef DDS_HAS_WCHAR
    void serialized_size_string_common(const DCPS::Encoding& encoding, size_t& size,
                                       const CORBA::WChar* wstr) const;
#endif
    void serialized_size_string_common(const DCPS::Encoding& encoding, size_t& size,
                                       const SingleValue& sv) const;

    template<typename StringType>
    bool serialized_size_generic_string_collection(const DCPS::Encoding& encoding, size_t& size,
                                                   const IndexToIdMap& index_to_id) const;
    template<typename StringType>
    bool serialized_size_generic_string_sequence(const DCPS::Encoding& encoding, size_t& size,
                                                 const IndexToIdMap& index_to_id) const;
    template<typename StringType>
    bool serialize_generic_string_collection(DCPS::Serializer& ser,
                                             const IndexToIdMap& index_to_id) const;
    template<typename StringType>
    bool serialize_generic_string_sequence(DCPS::Serializer& ser, CORBA::ULong length,
                                           CORBA::ULong bound) const;

    template<typename ElementType, typename CollectionType>
    bool set_default_enum_values(CollectionType& collection,
                                 const DDS::DynamicType_var& enum_type) const;

    template<typename ElementType, typename WrapElementType, typename CollectionType>
    bool reconstruct_enum_collection(CollectionType& collection, CORBA::ULong size,
      CORBA::ULong bound, const DDS::DynamicType_var& enum_type, const WrapElementType& elem_tag) const;

    void serialized_size_enum_sequence_as_int8s(const DCPS::Encoding& encoding, size_t& size,
                                                CORBA::ULong length) const;
    void serialized_size_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                       const DDS::Int8Seq& seq) const;
    bool serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser, const DDS::Int8Seq& enumseq) const;
    bool serialize_enum_sequence_as_int8s(DCPS::Serializer& ser, CORBA::ULong size,
      CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const;
    void serialized_size_enum_sequence_as_int16s(const DCPS::Encoding& encoding, size_t& size,
                                                 CORBA::ULong length) const;
    void serialized_size_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                       const DDS::Int16Seq& seq) const;
    bool serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser, const DDS::Int16Seq& enumseq) const;
    bool serialize_enum_sequence_as_int16s(DCPS::Serializer& ser, CORBA::ULong size,
      CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const;
    void serialized_size_enum_sequence_as_int32s(const DCPS::Encoding& encoding, size_t& size,
                                                 CORBA::ULong length) const;
    void serialized_size_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                       const DDS::Int32Seq& seq) const;
    bool serialize_enum_sequence_as_ints_i(DCPS::Serializer& ser, const DDS::Int32Seq& enumseq) const;
    bool serialize_enum_sequence_as_int32s(DCPS::Serializer& ser, CORBA::ULong size,
      CORBA::ULong bound, const DDS::DynamicType_var& enum_type) const;

    void serialized_size_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                       CORBA::ULong length, CORBA::ULong bitbound) const;
    bool serialize_enum_sequence(DCPS::Serializer& ser, CORBA::ULong size, CORBA::ULong bitbound,
                                 CORBA::ULong seqbound, const DDS::DynamicType_var& enum_type) const;

    template<typename CollectionType>
    void set_default_bitmask_values(CollectionType& col) const;

    template<typename WrapElementType, typename CollectionType>
    bool reconstruct_bitmask_collection(CollectionType& collection, CORBA::ULong size,
                                        CORBA::ULong bound, const WrapElementType& elem_tag) const;
    void serialized_size_bitmask_sequence_as_uint8s(const DCPS::Encoding& encoding,
                                                    size_t& size, CORBA::ULong length) const;
    void serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          const DDS::UInt8Seq& seq) const;
    bool serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
                                               const DDS::UInt8Seq& bitmask_seq) const;
    bool serialize_bitmask_sequence_as_uint8s(DCPS::Serializer& ser, CORBA::ULong size,
                                              CORBA::ULong bound) const;
    void serialized_size_bitmask_sequence_as_uint16s(const DCPS::Encoding& encoding, size_t& size,
                                                     CORBA::ULong length) const;
    void serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          const DDS::UInt16Seq& seq) const;
    bool serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
                                               const DDS::UInt16Seq& bitmask_seq) const;
    bool serialize_bitmask_sequence_as_uint16s(DCPS::Serializer& ser, CORBA::ULong size,
                                               CORBA::ULong bound) const;
    void serialized_size_bitmask_sequence_as_uint32s(const DCPS::Encoding& encoding, size_t& size,
                                                     CORBA::ULong length) const;
    void serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          const DDS::UInt32Seq& seq) const;
    bool serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
                                               const DDS::UInt32Seq& bitmask_seq) const;
    bool serialize_bitmask_sequence_as_uint32s(DCPS::Serializer& ser, CORBA::ULong size,
                                               CORBA::ULong bound) const;
    void serialized_size_bitmask_sequence_as_uint64s(const DCPS::Encoding& encoding, size_t& size,
                                                     CORBA::ULong length) const;
    void serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          const DDS::UInt64Seq& seq) const;
    bool serialize_bitmask_sequence_as_uints_i(DCPS::Serializer& ser,
                                               const DDS::UInt64Seq& bitmask_seq) const;
    bool serialize_bitmask_sequence_as_uint64s(DCPS::Serializer& ser, CORBA::ULong size,
                                               CORBA::ULong bound) const;
    void serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          CORBA::ULong length, CORBA::ULong bitbound) const;
    bool serialize_bitmask_sequence(DCPS::Serializer& ser, CORBA::ULong size,
                                    CORBA::ULong bitbound, CORBA::ULong seqbound) const;

    bool serialized_size_sequence_value(const DCPS::Encoding& encoding, size_t& size,
                                        const SequenceValue& sv) const;
    bool serialize_sequence_value(DCPS::Serializer& ser, const SequenceValue& sv) const;
    bool get_index_to_id_map(IndexToIdMap& index_to_id, CORBA::ULong bound) const;
    bool serialized_size_complex_member_i(const DCPS::Encoding& encoding, size_t& size,
                                          DDS::MemberId id) const;

    template<typename SequenceType>
    bool serialized_size_nested_basic_sequences(const DCPS::Encoding& encoding, size_t& size,
      const IndexToIdMap& index_to_id, SequenceType protoseq) const;

    template<typename SequenceType>
    bool serialized_size_nesting_basic_sequence(const DCPS::Encoding& encoding, size_t& size,
      const IndexToIdMap& index_to_id, SequenceType protoseq) const;

    bool serialize_complex_member_i(DCPS::Serializer& ser, DDS::MemberId id) const;

    template<typename SequenceType>
    bool serialize_nested_basic_sequences(DCPS::Serializer& ser, const IndexToIdMap& index_to_id,
                                          SequenceType protoseq) const;

    template<typename SequenceType>
    bool serialize_nesting_basic_sequence_i(DCPS::Serializer& ser, CORBA::ULong size,
                                            CORBA::ULong bound, SequenceType protoseq) const;

    bool serialized_size_nesting_basic_sequence(const DCPS::Encoding& encoding, size_t& size,
      TypeKind nested_elem_tk, const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_basic_sequence(DCPS::Serializer& ser, TypeKind nested_elem_tk,
                                          CORBA::ULong size, CORBA::ULong bound) const;
    bool serialized_size_nested_enum_sequences(const DCPS::Encoding& encoding, size_t& size,
                                               const IndexToIdMap& index_to_id) const;
    bool serialized_size_nesting_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                               const IndexToIdMap& index_to_id) const;
    bool serialize_nested_enum_sequences(DCPS::Serializer& ser, const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_enum_sequence(DCPS::Serializer& ser, CORBA::ULong size,
                                         CORBA::ULong bound) const;
    bool serialized_size_nested_bitmask_sequences(const DCPS::Encoding& encoding, size_t& size,
                                                  const IndexToIdMap& index_to_id) const;
    bool serialized_size_nesting_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                                  const IndexToIdMap& index_to_id) const;
    bool serialize_nested_bitmask_sequences(DCPS::Serializer& ser,
                                            const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_bitmask_sequence(DCPS::Serializer& ser, CORBA::ULong size,
                                            CORBA::ULong bound) const;
    bool serialized_size_complex_member(const DCPS::Encoding& encoding, size_t& size,
                                        DDS::MemberId id, const DDS::DynamicType_var& elem_type) const;
    bool serialized_size_complex_sequence(const DCPS::Encoding& encoding, size_t& size,
      const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type) const;
    bool serialize_complex_sequence_i(DCPS::Serializer& ser, const IndexToIdMap& index_to_id,
                                      const DDS::DynamicType_var& elem_type) const;
    bool serialize_complex_sequence(DCPS::Serializer& ser, CORBA::ULong size, CORBA::ULong bound,
                                    const DDS::DynamicType_var& elem_type) const;
    bool get_index_to_id_from_complex(IndexToIdMap& index_to_id, CORBA::ULong bound) const;
    bool serialized_size_sequence(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_sequence(DCPS::Serializer& ser) const;

    // Serialize array
    void serialized_size_primitive_array(const DCPS::Encoding& encoding, size_t& size,
                                         TypeKind elem_tk, CORBA::ULong length) const;
    bool serialize_primitive_array(DCPS::Serializer& ser, TypeKind elem_tk, CORBA::ULong length) const;

    template<typename StringType>
    bool serialized_size_generic_string_array(const DCPS::Encoding& encoding, size_t& size,
                                              const IndexToIdMap& index_to_id) const;
    template<typename StringType>
    bool serialize_generic_string_array(DCPS::Serializer& ser, CORBA::ULong length) const;
    void serialized_size_enum_array_as_int8s(const DCPS::Encoding& encoding, size_t& size,
                                             CORBA::ULong length) const;
    bool serialize_enum_array_as_ints_i(DCPS::Serializer& ser, const DDS::Int8Seq& enumarr) const;
    bool serialize_enum_array_as_int8s(DCPS::Serializer& ser, CORBA::ULong length,
                                       const DDS::DynamicType_var& enum_type) const;
    void serialized_size_enum_array_as_int16s(const DCPS::Encoding& encoding, size_t& size,
                                              CORBA::ULong length) const;
    bool serialize_enum_array_as_ints_i(DCPS::Serializer& ser, const DDS::Int16Seq& enumarr) const;
    bool serialize_enum_array_as_int16s(DCPS::Serializer& ser, CORBA::ULong length,
                                        const DDS::DynamicType_var& enum_type) const;
    void serialized_size_enum_array_as_int32s(const DCPS::Encoding& encoding, size_t& size,
                                              CORBA::ULong length) const;
    bool serialize_enum_array_as_ints_i(DCPS::Serializer& ser, const DDS::Int32Seq& enumarr) const;
    bool serialize_enum_array_as_int32s(DCPS::Serializer& ser, CORBA::ULong length,
                                        const DDS::DynamicType_var& enum_type) const;
    void serialized_size_enum_array(const DCPS::Encoding& encoding, size_t& size,
                                    CORBA::ULong length, CORBA::ULong bitbound) const;
    bool serialize_enum_array(DCPS::Serializer& ser, CORBA::ULong bitbound, CORBA::ULong length,
                              const DDS::DynamicType_var& enum_type) const;

    void serialized_size_bitmask_array_as_uint8s(const DCPS::Encoding& encoding, size_t& size,
                                                 CORBA::ULong length) const;
    bool serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
                                            const DDS::UInt8Seq& bitmask_arr) const;
    bool serialize_bitmask_array_as_uint8s(DCPS::Serializer& ser, CORBA::ULong length) const;
    void serialized_size_bitmask_array_as_uint16s(const DCPS::Encoding& encoding, size_t& size,
                                                  CORBA::ULong length) const;
    bool serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
                                            const DDS::UInt16Seq& bitmask_arr) const;
    bool serialize_bitmask_array_as_uint16s(DCPS::Serializer& ser, CORBA::ULong length) const;
    void serialized_size_bitmask_array_as_uint32s(const DCPS::Encoding& encoding, size_t& size,
                                                  CORBA::ULong length) const;
    bool serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
                                            const DDS::UInt32Seq& bitmask_arr) const;
    bool serialize_bitmask_array_as_uint32s(DCPS::Serializer& ser, CORBA::ULong length) const;
    void serialized_size_bitmask_array_as_uint64s(const DCPS::Encoding& encoding, size_t& size,
                                                  CORBA::ULong length) const;
    bool serialize_bitmask_array_as_uints_i(DCPS::Serializer& ser,
                                            const DDS::UInt64Seq& bitmask_arr) const;
    bool serialize_bitmask_array_as_uint64s(DCPS::Serializer& ser, CORBA::ULong length) const;
    void serialized_size_bitmask_array(const DCPS::Encoding& encoding, size_t& size,
                                       CORBA::ULong length, CORBA::ULong bitbound) const;
    bool serialize_bitmask_array(DCPS::Serializer& ser, CORBA::ULong bitbound,
                                 CORBA::ULong length) const;

    template<typename SequenceType>
    bool serialized_size_nesting_basic_array(const DCPS::Encoding& encoding, size_t& size,
      const IndexToIdMap& index_to_id, SequenceType protoseq) const;

    template<typename SequenceType>
    bool serialize_nesting_basic_array_i(DCPS::Serializer& ser, CORBA::ULong length,
                                         SequenceType protoseq) const;
    bool serialized_size_nesting_basic_array(const DCPS::Encoding& encoding, size_t& size,
      TypeKind nested_elem_tk, const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_basic_array(DCPS::Serializer& ser, TypeKind nested_elem_tk,
                                       CORBA::ULong length) const;
    bool serialized_size_nesting_enum_array(const DCPS::Encoding& encoding, size_t& size,
                                            const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_enum_array(DCPS::Serializer& ser, CORBA::ULong length) const;
    bool serialized_size_nesting_bitmask_array(const DCPS::Encoding& encoding, size_t& size,
                                               const IndexToIdMap& index_to_id) const;
    bool serialize_nesting_bitmask_array(DCPS::Serializer& ser, CORBA::ULong length) const;
    bool serialized_size_complex_array(const DCPS::Encoding& encoding, size_t& size,
      const IndexToIdMap& index_to_id, const DDS::DynamicType_var& elem_type) const;
    bool serialize_complex_array(DCPS::Serializer& ser, CORBA::ULong length,
                                 const DDS::DynamicType_var& elem_type) const;
    bool serialized_size_array(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_array(DCPS::Serializer& ser) const;

    bool serialized_size_primitive_member(const DCPS::Encoding& encoding, size_t& size,
                                          TypeKind member_tk) const;

    bool serialized_size_basic_member_default_value(const DCPS::Encoding& encoding, size_t& size,
                                                    TypeKind member_tk) const;
    bool serialized_size_basic_member(const DCPS::Encoding& encoding, size_t& size,
                                      TypeKind member_tk, const_single_iterator it) const;
    bool serialize_basic_member_default_value(DCPS::Serializer& ser, TypeKind member_tk) const;

    bool serialized_size_single_aggregated_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      const_single_iterator it, const DDS::DynamicType_var& member_type, bool optional,
      DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const;
    bool serialize_single_aggregated_member_xcdr2(DCPS::Serializer& ser, const_single_iterator it,
      const DDS::DynamicType_var& member_type, bool optional, bool must_understand,
      DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_complex_aggregated_member_xcdr2_default(const DCPS::Encoding& encoding,
      size_t& size, const DDS::DynamicType_var& member_type, bool optional,
      DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const;
    bool serialize_complex_aggregated_member_xcdr2_default(DCPS::Serializer& ser, DDS::MemberId id,
      const DDS::DynamicType_var& member_type, bool optional, bool must_understand,
      DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_complex_aggregated_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      const_complex_iterator it, bool optional, DDS::ExtensibilityKind extensibility,
      size_t& mutable_running_total) const;
    bool serialize_complex_aggregated_member_xcdr2(DCPS::Serializer& ser, const_complex_iterator it,
      bool optional, bool must_understand, DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_basic_struct_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      DDS::MemberId id, const DDS::DynamicType_var& member_type, bool optional,
      DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const;
    bool serialize_basic_struct_member_xcdr2(DCPS::Serializer& ser, DDS::MemberId id,
      const DDS::DynamicType_var& member_type, bool optional, bool must_understand,
      DDS::ExtensibilityKind extensibility) const;

    void serialized_size_sequence_member_default_value(const DCPS::Encoding& encoding,
                                                       size_t& size, TypeKind elem_tk) const;
    bool serialize_sequence_member_default_value(DCPS::Serializer& ser, TypeKind elem_tk) const;

    bool serialized_size_basic_sequence(const DCPS::Encoding& encoding, size_t& size,
                                        const_sequence_iterator it) const;
    bool serialize_basic_sequence(DCPS::Serializer& ser, const_sequence_iterator it) const;
    bool serialized_size_enum_sequence(const DCPS::Encoding& encoding, size_t& size,
                                       const_sequence_iterator it) const;
    bool serialize_enum_sequence(DCPS::Serializer& ser, const_sequence_iterator it) const;
    bool serialized_size_bitmask_sequence(const DCPS::Encoding& encoding, size_t& size,
                                          const_sequence_iterator it) const;
    bool serialize_bitmask_sequence(DCPS::Serializer& ser, const_sequence_iterator it) const;
    void serialized_size_sequence_aggregated_member_xcdr2(const DCPS::Encoding& encoding,
      size_t& size, const_sequence_iterator it, TypeKind elem_tk, bool optional,
      DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const;
    bool serialize_sequence_aggregated_member_xcdr2(DCPS::Serializer& ser, const_sequence_iterator it,
      TypeKind elem_tk, bool optional, bool must_understand, DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_sequence_struct_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      DDS::MemberId id, TypeKind elem_tk, bool optional,
      DDS::ExtensibilityKind extensibility, size_t& mutable_running_total) const;
    bool serialize_sequence_struct_member_xcdr2(DCPS::Serializer& ser, DDS::MemberId id,
      TypeKind elem_tk, bool optional, bool must_understand, DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_structure_xcdr2(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_structure_xcdr2(DCPS::Serializer& ser) const;
    bool serialized_size_structure_xcdr1(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_structure_xcdr1(DCPS::Serializer& ser) const;
    bool serialized_size_structure(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_structure(DCPS::Serializer& ser) const;

    bool set_default_discriminator_value(CORBA::Long& value,
                                         const DDS::DynamicType_var& disc_type) const;
    bool get_discriminator_value(CORBA::Long& value, const_single_iterator single_it,
      const_complex_iterator complex_it, const DDS::DynamicType_var& disc_type) const;
    bool serialized_size_discriminator_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      const DDS::DynamicType_var& disc_type, DDS::ExtensibilityKind extensibility,
      size_t& mutable_running_total) const;
    bool serialize_discriminator_member_xcdr2(DCPS::Serializer& ser, CORBA::Long value,
      const DDS::DynamicType_var& disc_type, DDS::ExtensibilityKind extensibility) const;
    bool serialized_size_selected_member_xcdr2(const DCPS::Encoding& encoding, size_t& size,
      DDS::MemberId selected_id, DDS::ExtensibilityKind extensibility,
      size_t& mutable_running_total) const;
    bool serialize_selected_member_xcdr2(DCPS::Serializer& ser, DDS::MemberId selected_id,
                                         DDS::ExtensibilityKind extensibility) const;
    bool select_union_member(CORBA::Long disc_value, bool& found_selected_member,
                             DDS::MemberDescriptor_var& selected_md) const;
    bool serialized_size_union_xcdr2(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_union_xcdr2(DCPS::Serializer& ser) const;
    bool serialized_size_union_xcdr1(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_union_xcdr1(DCPS::Serializer& ser) const;
    bool serialized_size_union(const DCPS::Encoding& encoding, size_t& size) const;
    bool serialize_union(DCPS::Serializer& ser) const;

    // Internal data
    OPENDDS_MAP(DDS::MemberId, SingleValue) single_map_;
    OPENDDS_MAP(DDS::MemberId, SequenceValue) sequence_map_;
    OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var) complex_map_;

    const DDS::DynamicType_var& type_;
    const DynamicDataImpl* data_;
  };

  bool read_discriminator(CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type,
                          DataContainer::const_single_iterator it) const;

  DataContainer container_;

  friend OpenDDS_Dcps_Export
  bool DCPS::serialized_size(const DCPS::Encoding& encoding, size_t& size, const DynamicDataImpl& data);

  friend OpenDDS_Dcps_Export
  bool DCPS::operator<<(DCPS::Serializer& ser, const DynamicDataImpl& data);
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
