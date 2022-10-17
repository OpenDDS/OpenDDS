/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "DynamicTypeImpl.h"

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataImpl
  : public virtual DCPS::LocalObject<DDS::DynamicData> {
public:
  DynamicDataImpl(DDS::DynamicType_ptr type);

  DDS::DynamicType_ptr type();

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value, MemberId id);
  DDS::ReturnCode_t set_descriptor(MemberId id, DDS::MemberDescriptor* value);

  CORBA::Boolean equals(DDS::DynamicData_ptr other);

  MemberId get_member_id_by_name(const char* name);
  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId /*id*/);
  DDS::DynamicData_ptr loan_value(DDS::MemberId /*id*/);
  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr /*value*/);

  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int32_value(CORBA::Long&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value);

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value);

  DDS::ReturnCode_t get_int8_value(CORBA::Int8&,
                                   DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value);

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value);

  DDS::ReturnCode_t get_int16_value(CORBA::Short&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value);

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value);

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value);

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value);

  DDS::ReturnCode_t get_float32_value(CORBA::Float&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value);

  DDS::ReturnCode_t get_float64_value(CORBA::Double&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value);

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value);

  DDS::ReturnCode_t get_char8_value(CORBA::Char&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value);

  DDS::ReturnCode_t get_char16_value(CORBA::WChar&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value);

  DDS::ReturnCode_t get_byte_value(CORBA::Octet&,
                                   DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value);

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value);

  DDS::ReturnCode_t get_string_value(char*&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value);

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value);

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value);

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int32_values(DDS::MemberId id,
                                     const DDS::Int32Seq& value);

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint32_values(DDS::MemberId id,
                                      const DDS::UInt32Seq& value);

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int8_values(DDS::MemberId id,
                                    const DDS::Int8Seq& value);

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint8_values(DDS::MemberId id,
                                     const DDS::UInt8Seq& value);

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int16_values(DDS::MemberId id,
                                     const DDS::Int16Seq& value);

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint16_values(DDS::MemberId id,
                                      const DDS::UInt16Seq& value);

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_int64_values(DDS::MemberId id,
                                     const DDS::Int64Seq& value);

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_uint64_values(DDS::MemberId id,
                                      const DDS::UInt64Seq& value);

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float32_values(DDS::MemberId id,
                                       const DDS::Float32Seq& value);

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float64_values(DDS::MemberId id,
                                       const DDS::Float64Seq& value);

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_float128_values(DDS::MemberId id,
                                        const DDS::Float128Seq& value);

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char8_values(DDS::MemberId id,
                                     const DDS::CharSeq& value);

  DDS::ReturnCode_t get_char16_values(DDS::WCharSeq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_char16_values(DDS::MemberId id,
                                      const DDS::WCharSeq& value);

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_byte_values(DDS::MemberId id,
                                    const DDS::ByteSeq& value);

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_boolean_values(DDS::MemberId id,
                                       const DDS::BooleanSeq& value);

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_string_values(DDS::MemberId id,
                                      const DDS::StringSeq& value);

  DDS::ReturnCode_t get_wstring_values(DDS::WStringSeq&,
                                       DDS::MemberId) const
  {
    return DDS::RETCODE_UNSUPPORTED;
  }
  DDS::ReturnCode_t set_wstring_values(DDS::MemberId id,
                                       const DDS::WStringSeq& value);

private:

  template<TypeKind MemberTypeKind, typename MemberType>
  bool set_value_to_struct(DDS::MemberId id, const MemberType& value,
                           TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind MemberTypeKind, typename MemberType>
  bool set_value_to_union(DDS::MemberId, const MemberType& value,
                          TypeKind enum_or_bitmask, LBound lower, LBound upper);

  template<TypeKind ElementTypeKind, typename ElementType>
  bool set_value_to_collection(DDS::MemberId id, const ElementType& value,
                               TypeKind coll_tk, TypeKind enum_or_bitmaks,
                               LBound lower, LBound upper);

  template<TypeKind ValueTypeKind, typename ValueType>
  bool set_single_value(DDS::MemberId id, const ValueType& value,
                        TypeKind enum_or_bitmask = TK_NONE,
                        LBound lower = 0,
                        LBound upper = 0);

  bool check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const;
  bool is_discriminator_type(TypeKind tk) const;
  bool is_default_member_selected(CORBA::ULong disc_val, DDS::MemberId default_id) const;
  bool read_discriminator(CORBA::ULong& disc_val) const;
  DDS::MemberId find_selected_member() const;
  bool validate_discriminator(CORBA::Long disc_val, const DDS::MemberDescriptor_var& md) const;
  bool find_selected_member_and_discriminator(DDS::MemberId& selected_id,
                                              bool& has_disc,
                                              CORBA::Long& disc_val) const;
  bool set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_ptr value);
  bool set_complex_to_union(DDS::MemberId id, DDS::DynamicData_ptr value);
  bool set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_ptr value, TypeKind tk);
  bool validate_member_id_collection(const DDS::TypeDescriptor_var& descriptor,
                                     DDS::MemberId id, TypeKind collection_tk) const;


  template<typename SingleType>
  bool insert_single(DDS::MemberId id, const SingleType& value);

  bool insert_complex(DDS::MemberId id, const DDS::DynamicData_var& value);

  template<typename SequenceType>
  bool insert_sequence(DDS::MemberId id, const SequenceType& value);

  bool check_seqmem_in_struct_union(DDS::MemberId id, TypeKind enum_or_bitmask,
                                    LBound lower, LBound upper) const;
  bool check_seqmem_in_sequence_array(DDS::MemberId id, TypeKind enum_or_bitmask,
                                      LBound lower, LBound upper) const;

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

  // The actual (i.e., non-alias) DynamicType of the associated type.
  DDS::DynamicType_var type_;

  // Contain data for an instance of a basic type.
  struct SingleValue {
    SingleValue(CORBA::Long int32);
    SingleValue(CORBA::ULong uint32);
    SingleValue(CORBA::Int8 int8);
    SingleValue(CORBA::UInt8 uint8);
    SingleValue(CORBA::Short int16);
    SingleValue(CORBA::UShort uint16);
    SingleValue(CORBA::LongLong int64);
    SingleValue(CORBA::ULongLong uint64);
    SingleValue(CORBA::Float float32);
    SingleValue(CORBA::Double float64);
    SingleValue(CORBA::LongDouble float128);
    SingleValue(CORBA::Char char8);
    SingleValue(CORBA::Octet byte);
    SingleValue(CORBA::Boolean boolean);
    SingleValue(const char* str);
#ifdef DDS_HAS_WCHAR
    SingleValue(CORBA::WChar char16);
    SingleValue(CORBA::WChar* wstr);
#endif

    ~SingleValue();

    template<typename T> const T& get() const;

    TypeKind kind_;
    union {
      CORBA::Long int32_;
      CORBA::ULong uint32_;
      CORBA::Int8 int8_;
      CORBA::UInt8 uint8_;
      CORBA::Short int16_;
      CORBA::UShort uint16_;
      CORBA::LongLong int64_;
      CORBA::ULongLong uint64_;
      CORBA::Float float32_;
      CORBA::Double float64_;
      CORBA::LongDouble float128_;
      CORBA::Char char8_;
      CORBA::Octet byte_;
      CORBA::Boolean boolean_;
      const char* str_;
#ifdef DDS_HAS_WCHAR
      CORBA::WChar char16_;
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
    SequenceValue(const DDS::DoubleSeq& float64_seq);
    SequenceValue(const DDS::LongDoubleSeq& float128_seq);
    SequenceValue(const DDS::CharSeq& char8_seq);
    SequenceValue(const DDS::OctetSeq& byte_seq);
    SequenceValue(const DDS::BooleanSeq& boolean_seq);
    SequenceValue(const DDS::StringSeq& str_seq);
#ifdef DDS_HAS_WCHAR
    SequenceValue(const DDS::WCharSeq& char16_seq);
    SequenceValue(const DDS::WStringSeq& wstr_seq);
#endif

    ~SequenceValue();

    template<typename T> const T& get() const;

    TypeKind elem_kind_;
    union {
      DDS::Int32Seq int32_seq_;
      DDS::UInt32Seq uint32_seq_;
      DDS::Int8Seq int8_seq_;
      DDS::UInt8Seq uint8_seq_;
      DDS::Int16Seq int16_seq_;
      DDS::UInt16Seq uint16_seq_;
      DDS::Int64Seq int64_seq_;
      DDS::UInt64Seq uint64_seq_;
      DDS::Float32Seq float32_seq_;
      DDS::Float64Seq float64_seq_;
      DDS::Float128Seq float128_seq_;
      DDS::CharSeq char8_seq_;
      DDS::OctetSeq byte_seq_;
      DDS::BooleanSeq boolean_seq_;
      DDS::StringSeq string_seq_;
#ifdef DDS_HAS_WCHAR
      DDS::WCharSeq char16_seq_;
      DDS::WStringSeq wstring_seq_;
#endif
    };
  };

  // Container for all data written to this DynamicData object.
  // At anytime, there can be at most 1 entry for any given MemberId in all maps.
  // That is, each member is stored in at most 1 map.
  struct DataContainer {
    typedef DCPS::OPENDDS_MAP(DDS::MemberId, SingleValue)::const_iterator const_single_iterator;
    typedef DCPS::OPENDDS_MAP(DDS::MemberId, SequenceValue)::const_iterator const_sequence_iterator;
    typedef DCPS::OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var)::const_iterator const_complex_iterator;

    DataContainer(const DynamicType_var& type) : type_(type) {}

    // Get the largest index from each map.
    // Call only for collection-like types (sequence, string, etc).
    // Caller must check for an empty map.
    bool get_largest_single_index(CORBA::ULong& index) const;
    bool get_largest_sequence_index(CORBA::ULong& index) const;

    // Get the largest index in a collection of a basic type (sequence, string, array).
    // This assumes there is at least 1 value of a basic type stored.
    bool get_largest_index_basic(CORBA::ULong& index) const;

    // Get the largest index in a collection of sequences of basic type
    // (each element of the collection is a sequence of a basic type).
    // Assuming at least 1 sequence is stored.
    bool get_largest_index_basic_sequence(CORBA::ULong& index) const;

    // Get the largest index in a collection of a complex type (type for which
    // the data must be represented using its own DynamicData object). Those types
    // are neither basic nor sequence of basic type.
    // Assuming at least 1 complex is stored.
    bool get_largest_index_complex(CORBA::ULong& index) const;

    template<typename ValueType>
    bool set_default_basic_value(ValueType& value, TypeKind kind) const;

    template<typename SequenceType>
    bool set_default_basic_values(SequenceType& seq, TypeKind elem_tk) const;

    template<typename SequenceType>
    bool reconstruct_basic_sequence(SequenceType& seq, TypeKind elem_tk,
                                    CORBA::ULong size, CORBA::ULong bound) const;

    // Serialize a sequence of basic type.
    bool serialize_basic_sequence(DCPS::Serializer& ser, TypeKind elem_tk,
                                  CORBA::ULong size, CORBA::ULong bound) const;

    template<typename SequenceType>
    bool reconstruct_enum_sequence(SequenceType& seq, CORBA::ULong size, CORBA::ULong bound) const;

    bool serialize_enum_sequence_as_int8s(DCPS::Serializer& ser,
                                          CORBA::ULong size, CORBA::ULong bound) const;
    bool serialize_enum_sequence_as_int16s(DCPS::Serializer& ser,
                                           CORBA::ULong size, CORBA::ULong bound) const;
    bool serialize_enum_sequence_as_int32s(DCPS::Serializer& ser,
                                           CORBA::ULong size, CORBA::ULong bound) const;
    bool serialize_enum_sequence(DCPS::Serializer& ser, CORBA::ULong size,
                                 CORBA::ULong bitbound, CORBA::ULong seqbound) const;

    bool serialize_bitmask_sequence(DCPS::Serializer& ser,
                                    CORBA::ULong size, CORBA::ULong bitbound) const;

    DCPS::OPENDDS_MAP(DDS::MemberId, SingleValue) single_map_;
    DCPS::OPENDDS_MAP(DDS::MemberId, SequenceValue) sequence_map_;
    DCPS::OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var) complex_map;

    const DDS::DynamicType_var& type_;
  };

  DataContainer container_;

  friend void serialized_size(const DCPS::Encoding& encoding, size_t& size, const DynamicDataImpl& data);
  friend bool operator<<(DCPS::Serializer& ser, const DynamicDataImpl& data);
};

} // namespace XTypes

namespace DCPS {

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size, const XTypes::DynamicDataImpl& data);

OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, const XTypes::DynamicDataImpl& data);

} // namespace DCPS

} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
