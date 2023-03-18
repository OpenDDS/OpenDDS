/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H

#include <dds/DCPS/Definitions.h>
#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
#  include "DynamicDataBase.h"
#  include "Utils.h"

#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

// If changing these, also change the get_dynamic_data_adapter forward
// declarations in Sample.h.
template <typename T>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, const T& value);
template <typename T, typename Tag>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, const T& value);

template <typename T>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, T& value);
template <typename T, typename Tag>
DDS::DynamicData_ptr get_dynamic_data_adapter(DDS::DynamicType_ptr type, T& value);

template <typename T>
const T* get_dynamic_data_adapter_value(DDS::DynamicData_ptr dda);
template <typename T, typename Tag>
const T* get_dynamic_data_adapter_value(DDS::DynamicData_ptr dda);

template <typename T, typename Tag = void>
class DynamicDataAdapterImpl;

/**
 * Base class for all classes that allow interfacing with the C++ mapping types
 * as DynamicData.
 *
 * TODO not later:
 * - clone
 * TODO:
 * - Support direct array methods, like get_int32_values
 * - Support casting, like using get_int32_value on a int16 member. Right now
 *   the method must match the members type. This could be possible by replacing
 *   get_simple_value with, among other methods, a get_i16_raw_value that
 *   changes what it interprets the void* dest as depending on the type kind.
 *   This could also be possible by implementing the access methods in
 *   DynamicDataBase and having those handle the cast. This would enable casting
 *   in both DynamicDataAdapter and DynamicDataImpl, as well as any other future
 *   DynamicData.
 *   - Part of this is accessing all types as complex value.
 * - Implement equals, clear_value, clear_all_values, and clear_nonkey_values
 */
class OpenDDS_Dcps_Export DynamicDataAdapter : public DynamicDataBase {
public:
  DynamicDataAdapter(DDS::DynamicType_ptr type, bool read_only)
    : DynamicDataBase(type)
    , read_only_(read_only)
  {
  }

  DDS::MemberId get_member_id_by_name(const char* name);
  virtual DDS::MemberId get_member_id_at_index_impl(DDS::UInt32);
  DDS::MemberId get_member_id_at_index(DDS::UInt32 index);

  DDS::ReturnCode_t clear_all_values();
  DDS::ReturnCode_t clear_nonkey_values();
  DDS::ReturnCode_t clear_value(DDS::MemberId);
  DDS::DynamicData_ptr clone();

  DDS::ReturnCode_t get_int8_value(CORBA::Int8& value,
                                   DDS::MemberId id)
  {
    return get_raw_value("get_int8_value", &value, TK_INT8, id);
  }

  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value)
  {
    return set_raw_value("set_int8_value", id, &value, TK_INT8);
  }

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8& value,
                                    DDS::MemberId id)
  {
    return get_raw_value("get_uint8_value", &value, TK_UINT8, id);
  }

  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value)
  {
    return set_raw_value("set_uint8_value", id, &value, TK_UINT8);
  }

  DDS::ReturnCode_t get_int16_value(CORBA::Short& value,
                                    DDS::MemberId id)
  {
    return get_raw_value("get_int16_value", &value, TK_INT16, id);
  }

  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value)
  {
    return set_raw_value("set_int16_value", id, &value, TK_INT16);
  }

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort& value,
                                     DDS::MemberId id)
  {
    return get_raw_value("get_uint16_value", &value, TK_UINT16, id);
  }

  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value)
  {
    return set_raw_value("set_uint16_value", id, &value, TK_UINT16);
  }

  DDS::ReturnCode_t get_int32_value(CORBA::Long& value,
                                    DDS::MemberId id)
  {
    return get_raw_value("get_int32_value", &value, TK_INT32, id);
  }

  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value)
  {
    return set_raw_value("set_int32_value", id, &value, TK_INT32);
  }

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong& value,
                                     DDS::MemberId id)
  {
    return get_raw_value("get_uint32_value", &value, TK_UINT32, id);
  }

  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value)
  {
    return set_raw_value("set_uint32_value", id, &value, TK_UINT32);
  }

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong& value,
                                    DDS::MemberId id)
  {
    return get_raw_value("get_int64_value", &value, TK_INT64, id);
  }

  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value)
  {
    return set_raw_value("set_int64_value", id, &value, TK_INT64);
  }

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong& value,
                                     DDS::MemberId id)
  {
    return get_raw_value("get_uint64_value", &value, TK_UINT64, id);
  }

  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value)
  {
    return set_raw_value("set_uint64_value", id, &value, TK_UINT64);
  }

  DDS::ReturnCode_t get_float32_value(CORBA::Float& value,
                                      DDS::MemberId id)
  {
    return get_raw_value("get_float32_value", &value, TK_FLOAT32, id);
  }

  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value)
  {
    return set_raw_value("set_float32_value", id, &value, TK_FLOAT32);
  }

  DDS::ReturnCode_t get_float64_value(CORBA::Double& value,
                                      DDS::MemberId id)
  {
    return get_raw_value("get_float64_value", &value, TK_FLOAT64, id);
  }

  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value)
  {
    return set_raw_value("set_float64_value", id, &value, TK_FLOAT64);
  }

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble& value,
                                       DDS::MemberId id)
  {
    return get_raw_value("get_float128_value", &value, TK_FLOAT128, id);
  }

  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value)
  {
    return set_raw_value("set_float128_value", id, &value, TK_FLOAT128);
  }

  DDS::ReturnCode_t get_char8_value(CORBA::Char& value,
                                    DDS::MemberId id)
  {
    return get_raw_value("get_char8_value", &value, TK_CHAR8, id);
  }

  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value)
  {
    return set_raw_value("set_char8_value", id, &value, TK_CHAR8);
  }

  DDS::ReturnCode_t get_char16_value(CORBA::WChar& value,
                                     DDS::MemberId id)
  {
    return get_raw_value("get_char16_value", &value, TK_CHAR16, id);
  }

  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value)
  {
    return set_raw_value("set_char16_value", id, &value, TK_CHAR16);
  }

  DDS::ReturnCode_t get_byte_value(DDS::Byte& value,
                                   DDS::MemberId id)
  {
    return get_raw_value("get_byte_value", &value, TK_BYTE, id);
  }

  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value)
  {
    return set_raw_value("set_byte_value", id, &value, TK_BYTE);
  }

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean& value,
                                      DDS::MemberId id)
  {
    return get_raw_value("get_boolean_value", &value, TK_BYTE, id);
  }

  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value)
  {
    return set_raw_value("set_boolean_value", id, &value, TK_BOOLEAN);
  }

  DDS::ReturnCode_t get_string_value(char*& value,
                                     DDS::MemberId id)
  {
    return get_raw_value("get_string_value", &value, TK_STRING8, id);
  }

  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value)
  {
    return set_raw_value("set_string_value", id, value, TK_STRING8);
  }

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*& value,
                                      DDS::MemberId id)
  {
    return get_raw_value("get_wstring_value", &value, TK_STRING16, id);
  }

  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value)
  {
    return set_raw_value("set_wstring_value", id, value, TK_STRING16);
  }

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr& value,
                                      DDS::MemberId id)
  {
    return get_raw_value("get_complex_value", &value, TK_NONE, id);
  }

  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value)
  {
    return set_raw_value("set_complex_value", id, value, TK_NONE);
  }

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_int32_values");
  }

  DDS::ReturnCode_t set_int32_values(DDS::MemberId,
                                     const DDS::Int32Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_int32_values");
  }

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_uint32_values");
  }

  DDS::ReturnCode_t set_uint32_values(DDS::MemberId,
                                      const DDS::UInt32Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_uint32_values");
  }

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_int8_values");
  }

  DDS::ReturnCode_t set_int8_values(DDS::MemberId,
                                    const DDS::Int8Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_int8_values");
  }

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_uint8_values");
  }

  DDS::ReturnCode_t set_uint8_values(DDS::MemberId,
                                     const DDS::UInt8Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_uint8_values");
  }

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_int16_values");
  }

  DDS::ReturnCode_t set_int16_values(DDS::MemberId,
                                     const DDS::Int16Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_int16_values");
  }

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_uint16_values");
  }

  DDS::ReturnCode_t set_uint16_values(DDS::MemberId,
                                      const DDS::UInt16Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_uint16_values");
  }

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_int64_values");
  }

  DDS::ReturnCode_t set_int64_values(DDS::MemberId,
                                     const DDS::Int64Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_int64_values");
  }

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_uint64_values");
  }

  DDS::ReturnCode_t set_uint64_values(DDS::MemberId,
                                      const DDS::UInt64Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_uint64_values");
  }

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_float32_values");
  }

  DDS::ReturnCode_t set_float32_values(DDS::MemberId,
                                       const DDS::Float32Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_float32_values");
  }

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_float64_values");
  }

  DDS::ReturnCode_t set_float64_values(DDS::MemberId,
                                       const DDS::Float64Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_float64_values");
  }

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_float128_values");
  }

  DDS::ReturnCode_t set_float128_values(DDS::MemberId,
                                        const DDS::Float128Seq&)
  {
    return unsupported_method("DynamicDataAdapater::set_float128_values");
  }

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_char8_values");
  }

  DDS::ReturnCode_t set_char8_values(DDS::MemberId,
                                     const DDS::CharSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_char8_values");
  }

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq&,
                                      DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_char16_values");
  }

  DDS::ReturnCode_t set_char16_values(DDS::MemberId,
                                      const DDS::WcharSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_char16_values");
  }

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_byte_values");
  }

  DDS::ReturnCode_t set_byte_values(DDS::MemberId,
                                    const DDS::ByteSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_byte_values");
  }

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_boolean_values");
  }

  DDS::ReturnCode_t set_boolean_values(DDS::MemberId,
                                       const DDS::BooleanSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_boolean_values");
  }

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_string_values");
  }

  DDS::ReturnCode_t set_string_values(DDS::MemberId,
                                      const DDS::StringSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_string_values");
  }

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq&,
                                       DDS::MemberId)
  {
    return unsupported_method("DynamicDataAdapater::get_wstring_values");
  }

  DDS::ReturnCode_t set_wstring_values(DDS::MemberId,
                                       const DDS::WstringSeq&)
  {
    return unsupported_method("DynamicDataAdapater::set_wstring_values");
  }

protected:
  const bool read_only_;

  DDS::ReturnCode_t invalid_id(const char* method, DDS::MemberId id) const;
  DDS::ReturnCode_t missing_dda(const char* method, DDS::MemberId id) const;
  DDS::ReturnCode_t assert_mutable(const char* method) const;
  DDS::ReturnCode_t check_index(const char* method, DDS::UInt32 index, DDS::UInt32 size) const;
  DDS::ReturnCode_t check_member(
    DDS::DynamicType_var& member_type, const char* method, DDS::TypeKind tk, DDS::MemberId id);
  DDS::ReturnCode_t check_member(const char* method, DDS::TypeKind tk, DDS::MemberId id);

  virtual DDS::ReturnCode_t get_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::MemberId id) = 0;

  virtual DDS::ReturnCode_t set_raw_value(
    const char* method, DDS::MemberId id, const void* source, DDS::TypeKind tk) = 0;

  template <typename T>
  DDS::ReturnCode_t get_simple_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, T source, DDS::MemberId id)
  {
    const DDS::ReturnCode_t rc = check_member(method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<T*>(dest) = source;
    return rc;
  }

  template <typename T>
  DDS::ReturnCode_t set_simple_raw_value(
    const char* method, T& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk)
  {
    const DDS::ReturnCode_t rc = check_member(method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    dest = *static_cast<const T*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_byte_raw_value(
    const char* method, void* dest, DDS::TypeKind tk,
    DDS::Byte source, DDS::MemberId id);
  DDS::ReturnCode_t set_byte_raw_value(
    const char* method, DDS::Byte& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_bool_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::Boolean source, DDS::MemberId id);
  DDS::ReturnCode_t set_bool_raw_value(
    const char* method, DDS::Boolean& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_i8_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::Int8 source, DDS::MemberId id);
  DDS::ReturnCode_t set_i8_raw_value(
    const char* method, DDS::Int8& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_u8_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::UInt8 source, DDS::MemberId id);
  DDS::ReturnCode_t set_u8_raw_value(
    const char* method, DDS::UInt8& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_c8_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::Char8 source, DDS::MemberId id);
  DDS::ReturnCode_t set_c8_raw_value(
    const char* method, DDS::Char8& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_c16_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, DDS::Char16 source, DDS::MemberId id);
  DDS::ReturnCode_t set_c16_raw_value(
    const char* method, DDS::Char16& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);

  /// For now dest must be a Int32 and tk must be TK_INT32
  template <typename Enum>
  DDS::ReturnCode_t get_enum_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, Enum source, DDS::MemberId id)
  {
    const DDS::ReturnCode_t rc = check_member(method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Int32*>(dest) = static_cast<DDS::Int32>(source);
    return rc;
  }

  /// For now source must be a Int32 and tk must be TK_INT32
  template <typename Enum>
  DDS::ReturnCode_t set_enum_raw_value(
    const char* method, Enum& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk)
  {
    const DDS::ReturnCode_t rc = check_member(method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    dest = static_cast<Enum>(*static_cast<const DDS::Int32*>(source));
    return rc;
  }

  DDS::ReturnCode_t get_s8_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, const char* source, DDS::MemberId id);
  DDS::ReturnCode_t set_s8_raw_value(
    const char* method, char*& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_cpp11_s8_raw_value(
    const char* method, void* dest, DDS::TypeKind tk,
    const std::string& source, DDS::MemberId id);
  DDS::ReturnCode_t set_cpp11_s8_raw_value(
    const char* method, std::string& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_s16_raw_value(
    const char* method, void* dest, DDS::TypeKind tk,
    const DDS::Char16* source, DDS::MemberId id);
  DDS::ReturnCode_t set_s16_raw_value(
    const char* method, DDS::Char16*& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);
  DDS::ReturnCode_t get_cpp11_s16_raw_value(
    const char* method, void* dest, DDS::TypeKind tk,
    const std::wstring& source, DDS::MemberId id);
  DDS::ReturnCode_t set_cpp11_s16_raw_value(
    const char* method, std::wstring& dest, DDS::MemberId id,
    const void* source, DDS::TypeKind tk);

  template <typename T, typename Tag = void>
  DDS::ReturnCode_t get_complex_raw_value(
    const char* method, void* dest, DDS::TypeKind tk, T& source, DDS::MemberId id)
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_member(member_type, method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicData*& dest_value = *static_cast<DDS::DynamicData**>(dest);
    CORBA::release(dest_value);
    dest_value = get_dynamic_data_adapter<T, Tag>(member_type, source);
    if (!dest_value) {
      return missing_dda(method, id);
    }
    return rc;
  }

  template <typename T, typename Tag = void>
  DDS::ReturnCode_t set_indirect_complex_raw_value_impl(
    const char* method, T& dest, DDS::MemberId id, DDS::DynamicType_ptr member_type,
    DDS::DynamicData_ptr source_dd)
  {
    DDS::DynamicData_var dest_dda = get_dynamic_data_adapter<T, Tag>(member_type, dest);
    if (!dest_dda) {
      return missing_dda(method, id);
    }
    return copy(dest_dda, source_dd);
  }

  template <typename T, typename Tag = void>
  DDS::ReturnCode_t set_direct_complex_raw_value(
    const char* method, T& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk)
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_member(member_type, method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::DynamicData* const source_dd = static_cast<DDS::DynamicData*>(const_cast<void*>(source));

    // If the source is another DynamicDataAdapter of the member type then do a
    // direct copy, else do an indirect copy.
    const T* const source_value = get_dynamic_data_adapter_value<T, Tag>(source_dd);
    if (source_value) {
      if (source_value != &dest) {
        dest = *source_value;
      }
      return DDS::RETCODE_OK;
    }

    return set_indirect_complex_raw_value_impl<T, Tag>(method, dest, id, member_type, source_dd);
  }

  // In the classic mapping arrays are C arrays, which can't be copied using =,
  // so only do a indirect copy.
  template <typename T, typename Tag = void>
  DDS::ReturnCode_t set_indirect_complex_raw_value(
    const char* method, T& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk)
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_member(member_type, method, tk, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return set_indirect_complex_raw_value_impl<T, Tag>(method, dest, id, member_type,
      static_cast<DDS::DynamicData*>(const_cast<void*>(source)));
  }
};

template <typename T>
class DynamicDataAdapter_T : public DynamicDataAdapter {
public:
  DynamicDataAdapter_T(DDS::DynamicType_ptr type, T& value)
    : DynamicDataAdapter(type, /* read_only = */ false)
    , value_(value)
  {
  }

  DynamicDataAdapter_T(DDS::DynamicType_ptr type, const T& value)
    : DynamicDataAdapter(type, /* read_only = */ true)
    , value_(const_cast<T&>(value))
  {
  }

  const T& wrapped() const
  {
    return value_;
  }

protected:
  T& value_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
