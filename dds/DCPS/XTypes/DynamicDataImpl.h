/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataBase.h"

#  include <dds/DCPS/FilterEvaluator.h>
#  include <dds/DCPS/Sample.h>
#  include <dds/DCPS/ValueWriter.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace XTypes {
class DynamicDataImpl;
}

namespace XTypes {

class OpenDDS_Dcps_Export DynamicDataImpl : public DynamicDataBase {
public:
  // An optional, read-only backing store can be passed as a last resort for reading data.
  DynamicDataImpl(DDS::DynamicType_ptr type, DDS::DynamicData_ptr backing_store = 0);
  DynamicDataImpl(const DynamicDataImpl& other);

  DDS::ReturnCode_t set_descriptor(MemberId id, DDS::MemberDescriptor* value);

  MemberId get_member_id_at_index(ACE_CDR::ULong index);
  ACE_CDR::ULong get_item_count();

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId id);

  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int8_value(CORBA::Int8& value,
                                   DDS::MemberId id);
  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value);

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value);

  DDS::ReturnCode_t get_int16_value(CORBA::Short& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value);

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value);

  DDS::ReturnCode_t get_int32_value(CORBA::Long& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value);

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong& value,
                                     DDS::MemberId);
  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value);

  DDS::ReturnCode_t get_int64_value_impl(CORBA::LongLong& value, DDS::MemberId id);
  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value);

  DDS::ReturnCode_t get_uint64_value_impl(CORBA::ULongLong& value, DDS::MemberId id);
  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value);

  DDS::ReturnCode_t get_float32_value(CORBA::Float& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value);

  DDS::ReturnCode_t get_float64_value(CORBA::Double& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value);

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value);

  DDS::ReturnCode_t get_char8_value(CORBA::Char& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value);

  DDS::ReturnCode_t get_char16_value(CORBA::WChar& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value);

  DDS::ReturnCode_t get_byte_value(CORBA::Octet& value,
                                   DDS::MemberId id);
  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value);

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value);

  DDS::ReturnCode_t get_string_value(char*& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value);

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value);

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value);

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int32_values(DDS::MemberId id,
                                     const DDS::Int32Seq& value);

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint32_values(DDS::MemberId id,
                                      const DDS::UInt32Seq& value);

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_int8_values(DDS::MemberId id,
                                    const DDS::Int8Seq& value);

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_uint8_values(DDS::MemberId id,
                                     const DDS::UInt8Seq& value);

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int16_values(DDS::MemberId id,
                                     const DDS::Int16Seq& value);

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint16_values(DDS::MemberId id,
                                      const DDS::UInt16Seq& value);

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_int64_values(DDS::MemberId id,
                                     const DDS::Int64Seq& value);

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_uint64_values(DDS::MemberId id,
                                      const DDS::UInt64Seq& value);

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float32_values(DDS::MemberId id,
                                       const DDS::Float32Seq& value);

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_float64_values(DDS::MemberId id,
                                       const DDS::Float64Seq& value);

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq& value,
                                        DDS::MemberId id);
  DDS::ReturnCode_t set_float128_values(DDS::MemberId id,
                                        const DDS::Float128Seq& value);

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq& value,
                                     DDS::MemberId id);
  DDS::ReturnCode_t set_char8_values(DDS::MemberId id,
                                     const DDS::CharSeq& value);

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_char16_values(DDS::MemberId id,
                                      const DDS::WcharSeq& value);

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq& value,
                                    DDS::MemberId id);
  DDS::ReturnCode_t set_byte_values(DDS::MemberId id,
                                    const DDS::ByteSeq& value);

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_boolean_values(DDS::MemberId id,
                                       const DDS::BooleanSeq& value);

  DDS::ReturnCode_t get_string_values(DDS::StringSeq& value,
                                      DDS::MemberId id);
  DDS::ReturnCode_t set_string_values(DDS::MemberId id,
                                      const DDS::StringSeq& value);

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq& value,
                                       DDS::MemberId id);
  DDS::ReturnCode_t set_wstring_values(DDS::MemberId id,
                                       const DDS::WstringSeq& value);

  bool serialized_size(const DCPS::Encoding& enc, size_t& size, DCPS::Sample::Extent ext) const;
  bool serialize(DCPS::Serializer& ser, DCPS::Sample::Extent ext) const;

private:
  CORBA::ULong get_string_item_count() const;
  CORBA::ULong get_sequence_item_count() const;
  bool has_member(DDS::MemberId id) const;
  void erase_member(DDS::MemberId id);

  /// Group of functions to read a basic value represented by this DynamicData instance
  bool read_basic_value(ACE_OutputCDR::from_int8& value);
  bool read_basic_value(ACE_OutputCDR::from_uint8& value);
  bool read_basic_value(CORBA::Short& value);
  bool read_basic_value(CORBA::UShort& value);
  bool read_basic_value(CORBA::Long& value);
  bool read_basic_value(CORBA::ULong& value);
  bool read_basic_value(CORBA::LongLong& value);
  bool read_basic_value(CORBA::ULongLong& value);
  bool read_basic_value(CORBA::Float& value);
  bool read_basic_value(CORBA::Double& value);
  bool read_basic_value(CORBA::LongDouble& value);
  bool read_basic_value(ACE_OutputCDR::from_char& value);
  bool read_basic_value(ACE_OutputCDR::from_wchar& value);
  bool read_basic_value(ACE_OutputCDR::from_octet& value);
  bool read_basic_value(ACE_OutputCDR::from_boolean& value);
  bool read_basic_value(char*& value) const;
#ifdef DDS_HAS_WCHAR
  bool read_basic_value(CORBA::WChar*& value) const;
#endif

  void cast_to_enum_value(ACE_OutputCDR::from_int8& dst, CORBA::Long src) const;
  void cast_to_enum_value(CORBA::Short& dst, CORBA::Long src) const;
  void cast_to_enum_value(CORBA::Long& dst, CORBA::Long src) const;

  template<typename ValueType>
  void cast_to_enum_value(ValueType& dst, CORBA::Long src) const;

  void set_backing_store(DDS::DynamicData_ptr backing_store);

  // Wrappers for reading different types from the backing store
  bool get_value_from_backing_store(ACE_OutputCDR::from_int8& value, DDS::MemberId id);
  bool get_value_from_backing_store(ACE_OutputCDR::from_uint8& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::Short& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::UShort& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::Long& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::ULong& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::LongLong& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::ULongLong& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::Float& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::Double& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::LongDouble& value, DDS::MemberId id);
  bool get_value_from_backing_store(ACE_OutputCDR::from_octet& value, DDS::MemberId id);
  bool get_value_from_backing_store(char*& value, DDS::MemberId id);
  bool get_value_from_backing_store(CORBA::WChar*& value, DDS::MemberId id);
  bool get_value_from_backing_store(ACE_OutputCDR::from_char& value, DDS::MemberId id);
  bool get_value_from_backing_store(ACE_OutputCDR::from_wchar& value, DDS::MemberId id);
  bool get_value_from_backing_store(ACE_OutputCDR::from_boolean& value, DDS::MemberId id);

  /// Read a basic member from a containing type
  template<typename ValueType>
  bool read_basic_in_single_map(ValueType& value, DDS::MemberId id);

  template<typename ValueType>
  bool read_basic_in_complex_map(ValueType& value, DDS::MemberId id);

  template<typename ValueType>
  bool read_basic_member(ValueType& value, DDS::MemberId id);

  template<typename ValueType>
  DDS::ReturnCode_t get_value_from_self(ValueType& value, DDS::MemberId id);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_value_from_enum(ValueType& value, DDS::MemberId id);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_value_from_bitmask(ValueType& value, DDS::MemberId id);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_value_from_struct(ValueType& value, DDS::MemberId id);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_value_from_union(ValueType& value, DDS::MemberId id);

  bool check_out_of_bound_read(DDS::MemberId id);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_value_from_collection(ValueType& value, DDS::MemberId id);

  template<TypeKind CharKind, TypeKind StringKind, typename FromCharT, typename CharT>
  DDS::ReturnCode_t get_char_common(CharT& value, DDS::MemberId id);

  template<typename UIntType>
  DDS::ReturnCode_t get_boolean_from_bitmask(CORBA::ULong index, CORBA::Boolean& value);

  template<TypeKind MemberTypeKind, typename MemberType>
  DDS::ReturnCode_t set_value_to_struct(DDS::MemberId id, const MemberType& value);

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

  DDS::ReturnCode_t set_union_discriminator_helper(CORBA::Long disc_val, const char* func_name);

  bool get_union_member_type(DDS::MemberId id, DDS::DynamicType_var& member_type,
                             DDS::MemberDescriptor_var& md) const;
  bool find_selected_union_branch(bool& has_existing_branch, DDS::MemberDescriptor_var& existing_md);

  template<TypeKind MemberTypeKind, typename MemberType>
  DDS::ReturnCode_t set_value_to_union(DDS::MemberId id, const MemberType& value);

  bool check_out_of_bound_write(DDS::MemberId id);

  template<TypeKind ElementTypeKind, typename ElementType>
  DDS::ReturnCode_t set_value_to_collection(DDS::MemberId id, const ElementType& value);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t set_single_value(DDS::MemberId id, const ValueType& value);

  template<TypeKind CharKind, TypeKind StringKind, typename FromCharT>
  DDS::ReturnCode_t set_char_common(DDS::MemberId id, const FromCharT& value);

  // Conversion between index and ID for collection
  CORBA::ULong index_to_id(CORBA::ULong index) const
  {
    return index;
  }

  CORBA::ULong id_to_index(CORBA::ULong id) const
  {
    return id;
  }

  bool check_index_from_id(TypeKind tk, DDS::MemberId id, CORBA::ULong bound) const;
  static bool is_valid_discriminator_type(TypeKind tk);
  bool is_default_member_selected(CORBA::Long disc_val, DDS::MemberId default_id) const;
  bool read_discriminator(CORBA::Long& disc_val);
  bool validate_discriminator(CORBA::Long disc_val, const DDS::MemberDescriptor_var& md) const;

  DDS::ReturnCode_t set_complex_to_struct(DDS::MemberId id, DDS::DynamicData_var value);
  DDS::ReturnCode_t set_complex_to_union(DDS::MemberId id, DDS::DynamicData_var value);
  DDS::ReturnCode_t set_complex_to_collection(DDS::MemberId id, DDS::DynamicData_var value);

  DDS::ReturnCode_t clear_value_i(DDS::MemberId id, const DDS::DynamicType_var& member_type);

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
  bool check_seqmem_in_struct_and_union(DDS::MemberId id, DDS::MemberDescriptor_var& md) const;

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_struct(DDS::MemberId id, const SequenceType& value);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_union(DDS::MemberId id, const SequenceType& value);

  template<TypeKind ElementTypeKind, typename SequenceType>
  bool set_values_to_collection(DDS::MemberId id, const SequenceType& value);

  template<TypeKind ElementTypeKind, typename SequenceType>
  DDS::ReturnCode_t set_sequence_values(DDS::MemberId id, const SequenceType& value);

  template<TypeKind ValueTypeKind, typename ValueType>
  DDS::ReturnCode_t get_single_value(ValueType& value, DDS::MemberId id);

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
  void set_default_basic_value(char*& value) const;
  void set_default_basic_value(ACE_OutputCDR::from_boolean& value) const;
#ifdef DDS_HAS_WCHAR
  void set_default_basic_value(ACE_OutputCDR::from_wchar& value) const;
  void set_default_basic_value(const CORBA::WChar*& value) const;
  void set_default_basic_value(CORBA::WChar*& value) const;
#endif

  bool set_default_enum_value(const DDS::DynamicType_var& dt, CORBA::Long& value) const;

  template<typename ElementType, typename CollectionType>
  bool set_default_enum_values(CollectionType& collection,
                               const DDS::DynamicType_var& enum_type) const;

  void set_default_bitmask_value(ACE_OutputCDR::from_uint8& value) const;
  void set_default_bitmask_value(CORBA::UShort& value) const;
  void set_default_bitmask_value(CORBA::ULong& value) const;
  void set_default_bitmask_value(CORBA::ULongLong& value) const;

  template<typename Type>
  void set_default_bitmask_value(Type& value) const;

  template<typename CollectionType>
  void set_default_bitmask_values(CollectionType& col) const;

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

  bool set_default_discriminator_value(CORBA::Long& value,
                                       const DDS::DynamicType_var& disc_type) const;

  typedef OPENDDS_VECTOR(CORBA::ULong) IndexToIdMap;

  // Contain data for an instance of a basic type.
  struct SingleValue {
    SingleValue();
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
    SingleValue(const SingleValue& other);
    SingleValue& operator=(const SingleValue& other);
    void copy(const SingleValue& other);

    ~SingleValue();

    // Return a reference to the stored value. Mostly for serialization.
    template<typename T> const T& get() const;

    // Return a duplication of the stored string/wstring.
    // Used for the get_* interfaces of DynamicData.
    // Caller is responsible for release the returned string.
    char* get_string() const;
    CORBA::WChar* get_wstring() const;

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

    SequenceValue(const SequenceValue& rhs);
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

  private:
    SequenceValue& operator=(const SequenceValue& rhs);
  };

  typedef OPENDDS_MAP(DDS::MemberId, SingleValue)::const_iterator const_single_iterator;
  typedef OPENDDS_MAP(DDS::MemberId, SequenceValue)::const_iterator const_sequence_iterator;
  typedef OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var)::const_iterator const_complex_iterator;

  // Container for all data written to this DynamicData object.
  // At anytime, there can be at most 1 entry for any given MemberId in all maps.
  // That is, each member is stored in at most 1 map.
  struct DataContainer {
    DataContainer(const DDS::DynamicType_var& type, const DynamicDataImpl* data)
      : type_(type)
      , type_desc_(data->type_desc_)
      , data_(data)
    {}

    DataContainer(const DataContainer& other, const DynamicDataImpl* data)
      : single_map_(other.single_map_)
      , sequence_map_(other.sequence_map_)
      , complex_map_(other.complex_map_)
      , type_(data->type_)
      , type_desc_(data->type_desc_)
      , data_(data)
    {}

    void clear();

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

    // Internal data
    OPENDDS_MAP(DDS::MemberId, SingleValue) single_map_;
    OPENDDS_MAP(DDS::MemberId, SequenceValue) sequence_map_;
    OPENDDS_MAP(DDS::MemberId, DDS::DynamicData_var) complex_map_;

    const DDS::DynamicType_var& type_;
    const DDS::TypeDescriptor_var& type_desc_;
    const DynamicDataImpl* data_;
  };

  // Copy a value of a basic member from single map to a DynamicData object.
  bool move_single_to_complex(const const_single_iterator& it, DynamicDataImpl* data);
  bool move_single_to_complex_i(const const_single_iterator& it, DynamicDataImpl* data, TypeKind treat_as);

  template<typename SequenceType>
  void move_sequence_helper(const const_sequence_iterator& it, DynamicDataImpl* data);

  // Copy values of a basic sequence member from sequence map to a DynamicData object.
  bool move_sequence_to_complex(const const_sequence_iterator& it, DynamicDataImpl* data);

  DDS::ReturnCode_t set_member_backing_store(DynamicDataImpl* member_ddi, DDS::MemberId id);

  // Indicate whether the value of a member is found in the complex map or
  // one of the other two maps or not found from any map in the container.
  enum FoundStatus { FOUND_IN_COMPLEX_MAP, FOUND_IN_NON_COMPLEX_MAP, NOT_FOUND };

  bool get_complex_from_container(DDS::DynamicData_var& value, DDS::MemberId id,
                                  FoundStatus& found_status);
  DDS::ReturnCode_t get_complex_from_aggregated(DDS::DynamicData_var& dd_var, DDS::MemberId id);
  DDS::ReturnCode_t get_complex_from_struct(DDS::DynamicData_ptr& value, DDS::MemberId id);
  DDS::ReturnCode_t get_complex_from_union(DDS::DynamicData_ptr& value, DDS::MemberId id);
  DDS::ReturnCode_t get_complex_from_collection(DDS::DynamicData_ptr& value, DDS::MemberId id);

  bool read_disc_from_single_map(CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type,
                                 const const_single_iterator& it) const;
  bool read_disc_from_backing_store(CORBA::Long& disc_val, DDS::MemberId id,
                                    const DDS::DynamicType_var& disc_type);

  // Add a single value for any valid discriminator value that selects the given member
  bool insert_valid_discriminator(DDS::MemberDescriptor* memberSelected);
  bool insert_discriminator(ACE_CDR::Long value);
  void clear_container();

  // Container for data set by the user.
  DataContainer container_;

  // Immutable backing store to retrieve data in case the container doesn't have it.
  // The type associated with the backing store can be different (but assignable to)
  // the type associated with this object. Reading from the backing store is considered
  // best effort, that is failure to read from it doesn't cascade to the caller.
  DDS::DynamicData_var backing_store_;

  bool reconstruct_string_value(CORBA::Char* str) const;
  bool reconstruct_wstring_value(CORBA::WChar* wstr) const;
  bool has_discriminator_value(const_single_iterator& single_it, const_complex_iterator& complex_it) const;
  bool get_discriminator_value(CORBA::Long& value, const const_single_iterator& single_it,
    const const_complex_iterator& complex_it, const DDS::DynamicType_var& disc_type);
};

} // namespace XTypes

namespace DCPS {

OpenDDS_Dcps_Export
bool serialized_size(const Encoding& encoding, size_t& size, DDS::DynamicData_ptr data);

OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, DDS::DynamicData_ptr data);

OpenDDS_Dcps_Export
bool serialized_size(const Encoding& encoding, size_t& size, const KeyOnly<DDS::DynamicData_ptr>& key);

OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, const KeyOnly<DDS::DynamicData_ptr>& key);

}

} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_IMPL_H
