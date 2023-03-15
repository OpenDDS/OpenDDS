/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
#define OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H

#if !defined OPENDDS_SAFETY_PROFILE && !defined OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
#  define OPENDDS_HAS_DYNAMIC_DATA_ADAPTER 1
#else
#  define OPENDDS_HAS_DYNAMIC_DATA_ADAPTER 0
#endif

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

#  include "DynamicTypeImpl.h"
#  include "Utils.h"

#  include <dds/DCPS/FilterEvaluator.h>
#  include <dds/DCPS/debug.h>

#  include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

template <typename T>
class DynamicDataAdapter;

class RawAdapter {
public:
  virtual ~RawAdapter()
  {
  }

  virtual DDS::UInt32 get_item_count(const void* value) const = 0;

  virtual DDS::MemberId get_member_id_at_index(void* /*wrapped*/, DDS::UInt32 /*index*/) const
  {
    // TODO
    return MEMBER_ID_INVALID;
  }

  virtual DDS::ReturnCode_t get_value(
    void* dest, DDS::TypeKind tk,
    void* wrapped, DDS::DynamicType* type, DDS::MemberId id) const = 0;

  virtual DDS::ReturnCode_t set_value(
    void* wrapped, DDS::DynamicType* type, DDS::MemberId id,
    const void* source, DDS::TypeKind tk) const = 0;

protected:
  DDS::ReturnCode_t invalid_id(const char* type_name, bool set, DDS::MemberId id) const
  {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: RawAdapterImpl<%C>::%C_value: invalid member id %u\n",
        type_name, set ? "set" : "get", id));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::ReturnCode_t check_type(
    DDS::DynamicType* container_type, DDS::MemberId id, DDS::TypeKind their_tk,
    DDS::DynamicType_var& member_type) const
  {
    DDS::ReturnCode_t rc = get_member_type(member_type, container_type, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::DynamicTypeMember_var dtm;
    rc = container_type->get_member(dtm, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    const TypeKind type_kind = member_type->get_kind();
    TypeKind cmp_type_kind = type_kind;
    switch (type_kind) {
    case TK_ENUM:
      rc = enum_bound(member_type, cmp_type_kind);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      break;

    case TK_BITMASK:
      rc = bitmask_bound(member_type, cmp_type_kind);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      break;
    }

    bool invalid_tk = true;
    if (is_basic(cmp_type_kind)) {
      invalid_tk = cmp_type_kind != their_tk;
    } else if (their_tk == TK_NONE) {
      invalid_tk = !is_complex(type_kind);
    }
    if (invalid_tk) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        const CORBA::String_var member_name = md->name();
        const CORBA::String_var type_name = container_type->get_name();
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: RawAdapterImpl::check_type: "
          "trying to access %C.%C id %u kind %C (%C) as an invalid kind %C\n",
          type_name.in(), member_name.in(), id,
          typekind_to_string(cmp_type_kind), typekind_to_string(type_kind),
          their_tk == TK_NONE ? "complex" : typekind_to_string(their_tk)));
      }
      return DDS::RETCODE_BAD_PARAMETER;
    }

    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t check_type(DDS::DynamicType* container_type,
    DDS::MemberId id, DDS::TypeKind their_tk) const
  {
    DDS::DynamicType_var member_type;
    return check_type(container_type, id, their_tk, member_type);
  }

  template <typename T>
  DDS::ReturnCode_t get_simple_value(
    void* dest, DDS::TypeKind tk, const T& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<T*>(dest) = value;
    return rc;
  }

  template <typename T>
  DDS::ReturnCode_t set_simple_value(
    T& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const T*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_byte_value(
    void* dest, DDS::TypeKind tk, const DDS::Byte& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Byte*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_byte_value(
    DDS::Byte& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::Byte*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_bool_value(
    void* dest, DDS::TypeKind tk, const DDS::Boolean& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Boolean*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_bool_value(
    DDS::Boolean& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::Boolean*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_i8_value(
    void* dest, DDS::TypeKind tk, const DDS::Int8& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Int8*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_i8_value(
    DDS::Int8& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::Int8*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_u8_value(
    void* dest, DDS::TypeKind tk, const DDS::UInt8& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::UInt8*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_u8_value(
    DDS::UInt8& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::UInt8*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_c8_value(
    void* dest, DDS::TypeKind tk, const DDS::Char8& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Char8*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_c8_value(
    DDS::Char8& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::Char8*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_c16_value(
    void* dest, DDS::TypeKind tk, DDS::Char16 value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Char16*>(dest) = value;
    return rc;
  }

  DDS::ReturnCode_t set_c16_value(
    DDS::Char16& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = *static_cast<const DDS::Char16*>(source);
    return rc;
  }

  /// For now dest must be a Int32 and tk must be TK_INT32
  template <typename Enum>
  DDS::ReturnCode_t get_enum_value(
    void* dest, DDS::TypeKind tk, const Enum& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    *static_cast<DDS::Int32*>(dest) = static_cast<DDS::Int32>(value);
    return rc;
  }

  /// For now source must be a Int32 and tk must be TK_INT32
  template <typename Enum>
  DDS::ReturnCode_t set_enum_value(
    Enum& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = static_cast<Enum>(*static_cast<const DDS::Int32*>(source));
    return rc;
  }

  DDS::ReturnCode_t get_s8_value(
    void* dest, DDS::TypeKind tk, const char* value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    char*& dest_value = *static_cast<char**>(dest);
    CORBA::string_free(dest_value);
    dest_value = CORBA::string_dup(value);
    return rc;
  }

  DDS::ReturnCode_t set_s8_value(
    char*& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    CORBA::string_free(value);
    value = CORBA::string_dup(static_cast<const char*>(source));
    return rc;
  }

  DDS::ReturnCode_t get_cpp11_s8_value(
    void* dest, DDS::TypeKind tk, const std::string& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    char*& dest_value = *static_cast<char**>(dest);
    CORBA::string_free(dest_value);
    dest_value = CORBA::string_dup(value.c_str());
    return rc;
  }

  DDS::ReturnCode_t set_cpp11_s8_value(
    std::string& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = static_cast<const char*>(source);
    return rc;
  }

  DDS::ReturnCode_t get_s16_value(
    void* dest, DDS::TypeKind tk, const DDS::Char16* value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::Char16*& dest_value = *static_cast<DDS::Char16**>(dest);
    CORBA::wstring_free(dest_value);
    dest_value = CORBA::wstring_dup(value);
    return rc;
  }

  DDS::ReturnCode_t set_s16_value(
    DDS::Char16*& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    CORBA::wstring_free(value);
    value = CORBA::wstring_dup(static_cast<const DDS::Char16*>(source));
    return rc;
  }

  DDS::ReturnCode_t get_cpp11_s16_value(
    void* dest, DDS::TypeKind tk, const std::wstring& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::Char16*& dest_value = *static_cast<DDS::Char16**>(dest);
    CORBA::wstring_free(dest_value);
    dest_value = CORBA::wstring_dup(value.c_str());
    return rc;
  }

  DDS::ReturnCode_t set_cpp11_s16_value(
    std::wstring& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    const DDS::ReturnCode_t rc = check_type(type, id, tk);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    value = static_cast<const DDS::Char16*>(source);
    return rc;
  }

  template <typename T>
  DDS::ReturnCode_t get_complex_value(
    void* dest, DDS::TypeKind tk, T& value, DDS::DynamicType* type, DDS::MemberId id) const
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_type(type, id, tk, member_type);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicData*& dest_value = *static_cast<DDS::DynamicData**>(dest);
    CORBA::release(dest_value);
    dest_value = new DynamicDataAdapter<T>(member_type, value);
    return rc;
  }

  template <typename T>
  DDS::ReturnCode_t set_direct_complex_value(
    T& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_type(type, id, tk, member_type);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicData* const source_dd = static_cast<DDS::DynamicData*>(const_cast<void*>(source));
    DynamicDataAdapter<T>* const source_dda = dynamic_cast<DynamicDataAdapter<T>*>(source_dd);
    // If the source is another DynamicDataAdapter of the member type then do a
    // direct copy, else do an indirect copy. See set_indirect_complex_value for
    // the classic mapping array case.
    if (source_dda) {
      if (&source_dda->wrapped() != &value) {
        value = source_dda->wrapped();
      }
      return DDS::RETCODE_OK;
    }
    DynamicDataAdapter<T> dest(member_type, value);
    return copy(&dest, source_dd);
  }

  // In the classic mapping arrays are C arrays, which can't be copied using =,
  // so only do a indirect copy.
  template <typename T>
  DDS::ReturnCode_t set_indirect_complex_value(
    T& value, DDS::DynamicType* type, DDS::MemberId id, const void* source, DDS::TypeKind tk) const
  {
    DDS::DynamicType_var member_type;
    const DDS::ReturnCode_t rc = check_type(type, id, tk, member_type);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DynamicDataAdapter<T> dest(member_type, value);
    return copy(&dest, static_cast<DDS::DynamicData*>(const_cast<void*>(source)));
  }
};

template <typename T>
const RawAdapter& get_raw_adapter();

template <typename T>
class RawAdapterImpl;

template <typename T>
class DynamicDataAdapter : public DDS::DynamicData {
public:
  DynamicDataAdapter(DDS::DynamicType_ptr type, T& value)
    : type_(DDS::DynamicType::_duplicate(type))
    , value_(value)
    , raw_adapter_(get_raw_adapter<T>())
  {}

  DynamicDataAdapter(DDS::DynamicType_ptr type, const T& value)
    : type_(DDS::DynamicType::_duplicate(type))
    , value_(const_cast<T&>(value))
    , raw_adapter_(get_raw_adapter<T>())
  {}

  const T& wrapped() const
  {
    return value_;
  }

  DDS::DynamicType_ptr type()
  {
    return DDS::DynamicType::_duplicate(type_);
  }

  DDS::ReturnCode_t get_descriptor(DDS::MemberDescriptor*& value,
                                   DDS::MemberId id)
  {
    DDS::DynamicTypeMember_var dtm;
    const DDS::ReturnCode_t rc = type_->get_member(dtm, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    return dtm->get_descriptor(value);
  }

  DDS::ReturnCode_t set_descriptor(DDS::MemberId,
                                   DDS::MemberDescriptor*)
  {
    return unsupported("set_descriptor");
  }

  CORBA::Boolean equals(DDS::DynamicData_ptr)
  {
    unsupported("equals");
    return false;
  }

  DDS::MemberId get_member_id_by_name(const char* name)
  {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_name(dtm, name) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }

    return dtm->get_id();
  }

  DDS::MemberId get_member_id_at_index(CORBA::ULong index)
  {
    DDS::DynamicTypeMember_var dtm;
    if (type_->get_member_by_index(dtm, index) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }

    return dtm->get_id();
  }

  CORBA::ULong get_item_count()
  {
    unsupported("get_item_count");
    return 0;
  }

  DDS::ReturnCode_t clear_all_values()
  {
    return unsupported("clear_all_values");
  }

  DDS::ReturnCode_t clear_nonkey_values()
  {
    return unsupported("clear_nonkey_values");
  }

  DDS::ReturnCode_t clear_value(DDS::MemberId)
  {
    return unsupported("clear_value");
  }

  DDS::DynamicData_ptr loan_value(DDS::MemberId)
  {
    unsupported("loan_value");
    return 0;
  }

  DDS::ReturnCode_t return_loaned_value(DDS::DynamicData_ptr)
  {
    return unsupported("return_loaned_value");
  }

  DDS::DynamicData_ptr clone()
  {
    unsupported("clone");
    return 0;
  }

  DDS::ReturnCode_t get_int8_value(CORBA::Int8& value,
                                   DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_INT8, &value_, type_, id);
  }

  DDS::ReturnCode_t set_int8_value(DDS::MemberId id,
                                   CORBA::Int8 value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_INT8);
  }

  DDS::ReturnCode_t get_uint8_value(CORBA::UInt8& value,
                                    DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_UINT8, &value_, type_, id);
  }

  DDS::ReturnCode_t set_uint8_value(DDS::MemberId id,
                                    CORBA::UInt8 value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_UINT8);
  }

  DDS::ReturnCode_t get_int16_value(CORBA::Short& value,
                                    DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_INT16, &value_, type_, id);
  }

  DDS::ReturnCode_t set_int16_value(DDS::MemberId id,
                                    CORBA::Short value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_INT16);
  }

  DDS::ReturnCode_t get_uint16_value(CORBA::UShort& value,
                                     DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_UINT16, &value_, type_, id);
  }

  DDS::ReturnCode_t set_uint16_value(DDS::MemberId id,
                                     CORBA::UShort value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_UINT16);
  }

  DDS::ReturnCode_t get_int32_value(CORBA::Long& value,
                                    DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_INT32, &value_, type_, id);
  }

  DDS::ReturnCode_t set_int32_value(DDS::MemberId id,
                                    CORBA::Long value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_INT32);
  }

  DDS::ReturnCode_t get_uint32_value(CORBA::ULong& value,
                                     DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_UINT32, &value_, type_, id);
  }

  DDS::ReturnCode_t set_uint32_value(DDS::MemberId id,
                                     CORBA::ULong value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_UINT32);
  }

  DDS::ReturnCode_t get_int64_value(CORBA::LongLong& value,
                                    DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_INT64, &value_, type_, id);
  }

  DDS::ReturnCode_t set_int64_value(DDS::MemberId id,
                                    CORBA::LongLong value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_INT64);
  }

  DDS::ReturnCode_t get_uint64_value(CORBA::ULongLong& value,
                                     DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_UINT64, &value_, type_, id);
  }

  DDS::ReturnCode_t set_uint64_value(DDS::MemberId id,
                                     CORBA::ULongLong value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_UINT64);
  }

  DDS::ReturnCode_t get_float32_value(CORBA::Float& value,
                                      DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_FLOAT32, &value_, type_, id);
  }

  DDS::ReturnCode_t set_float32_value(DDS::MemberId id,
                                      CORBA::Float value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_FLOAT32);
  }

  DDS::ReturnCode_t get_float64_value(CORBA::Double& value,
                                      DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_FLOAT64, &value_, type_, id);
  }

  DDS::ReturnCode_t set_float64_value(DDS::MemberId id,
                                      CORBA::Double value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_FLOAT64);
  }

  DDS::ReturnCode_t get_float128_value(CORBA::LongDouble& value,
                                       DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_FLOAT128, &value_, type_, id);
  }

  DDS::ReturnCode_t set_float128_value(DDS::MemberId id,
                                       CORBA::LongDouble value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_FLOAT128);
  }

  DDS::ReturnCode_t get_char8_value(CORBA::Char& value,
                                    DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_CHAR8, &value_, type_, id);
  }

  DDS::ReturnCode_t set_char8_value(DDS::MemberId id,
                                    CORBA::Char value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_CHAR8);
  }

  DDS::ReturnCode_t get_char16_value(CORBA::WChar& value,
                                     DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_CHAR16, &value_, type_, id);
  }

  DDS::ReturnCode_t set_char16_value(DDS::MemberId id,
                                     CORBA::WChar value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_CHAR16);
  }

  DDS::ReturnCode_t get_byte_value(DDS::Byte& value,
                                   DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_BYTE, &value_, type_, id);
  }

  DDS::ReturnCode_t set_byte_value(DDS::MemberId id,
                                   CORBA::Octet value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_BYTE);
  }

  DDS::ReturnCode_t get_boolean_value(CORBA::Boolean& value,
                                      DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_BYTE, &value_, type_, id);
  }

  DDS::ReturnCode_t set_boolean_value(DDS::MemberId id,
                                      CORBA::Boolean value)
  {
    return raw_adapter_.set_value(&value_, type_, id, &value, TK_BOOLEAN);
  }

  DDS::ReturnCode_t get_string_value(char*& value,
                                     DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_STRING8, &value_, type_, id);
  }

  DDS::ReturnCode_t set_string_value(DDS::MemberId id,
                                     const char* value)
  {
    return raw_adapter_.set_value(&value_, type_, id, value, TK_STRING8);
  }

  DDS::ReturnCode_t get_wstring_value(CORBA::WChar*& value,
                                      DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_STRING16, &value_, type_, id);
  }

  DDS::ReturnCode_t set_wstring_value(DDS::MemberId id,
                                      const CORBA::WChar* value)
  {
    return raw_adapter_.set_value(&value_, type_, id, value, TK_STRING16);
  }

  DDS::ReturnCode_t get_complex_value(DDS::DynamicData_ptr& value,
                                      DDS::MemberId id)
  {
    return raw_adapter_.get_value(&value, TK_NONE, &value_, type_, id);
  }

  DDS::ReturnCode_t set_complex_value(DDS::MemberId id,
                                      DDS::DynamicData_ptr value)
  {
    return raw_adapter_.set_value(&value_, type_, id, value, TK_NONE);
  }

  DDS::ReturnCode_t get_int32_values(DDS::Int32Seq&,
                                     DDS::MemberId)
  {
    return unsupported("get_int32_values");
  }

  DDS::ReturnCode_t set_int32_values(DDS::MemberId,
                                     const DDS::Int32Seq&)
  {
    return unsupported("set_int32_values");
  }

  DDS::ReturnCode_t get_uint32_values(DDS::UInt32Seq&,
                                      DDS::MemberId)
  {
    return unsupported("get_uint32_values");
  }

  DDS::ReturnCode_t set_uint32_values(DDS::MemberId,
                                      const DDS::UInt32Seq&)
  {
    return unsupported("set_uint32_values");
  }

  DDS::ReturnCode_t get_int8_values(DDS::Int8Seq&,
                                    DDS::MemberId)
  {
    return unsupported("get_int8_values");
  }

  DDS::ReturnCode_t set_int8_values(DDS::MemberId,
                                    const DDS::Int8Seq&)
  {
    return unsupported("set_int8_values");
  }

  DDS::ReturnCode_t get_uint8_values(DDS::UInt8Seq&,
                                     DDS::MemberId)
  {
    return unsupported("get_uint8_values");
  }

  DDS::ReturnCode_t set_uint8_values(DDS::MemberId,
                                     const DDS::UInt8Seq&)
  {
    return unsupported("set_uint8_values");
  }

  DDS::ReturnCode_t get_int16_values(DDS::Int16Seq&,
                                     DDS::MemberId)
  {
    return unsupported("get_int16_values");
  }

  DDS::ReturnCode_t set_int16_values(DDS::MemberId,
                                     const DDS::Int16Seq&)
  {
    return unsupported("set_int16_values");
  }

  DDS::ReturnCode_t get_uint16_values(DDS::UInt16Seq&,
                                      DDS::MemberId)
  {
    return unsupported("get_uint16_values");
  }

  DDS::ReturnCode_t set_uint16_values(DDS::MemberId,
                                      const DDS::UInt16Seq&)
  {
    return unsupported("set_uint16_values");
  }

  DDS::ReturnCode_t get_int64_values(DDS::Int64Seq&,
                                     DDS::MemberId)
  {
    return unsupported("get_int64_values");
  }

  DDS::ReturnCode_t set_int64_values(DDS::MemberId,
                                     const DDS::Int64Seq&)
  {
    return unsupported("set_int64_values");
  }

  DDS::ReturnCode_t get_uint64_values(DDS::UInt64Seq&,
                                      DDS::MemberId)
  {
    return unsupported("get_uint64_values");
  }

  DDS::ReturnCode_t set_uint64_values(DDS::MemberId,
                                      const DDS::UInt64Seq&)
  {
    return unsupported("set_uint64_values");
  }

  DDS::ReturnCode_t get_float32_values(DDS::Float32Seq&,
                                       DDS::MemberId)
  {
    return unsupported("get_float32_values");
  }

  DDS::ReturnCode_t set_float32_values(DDS::MemberId,
                                       const DDS::Float32Seq&)
  {
    return unsupported("set_float32_values");
  }

  DDS::ReturnCode_t get_float64_values(DDS::Float64Seq&,
                                       DDS::MemberId)
  {
    return unsupported("get_float64_values");
  }

  DDS::ReturnCode_t set_float64_values(DDS::MemberId,
                                       const DDS::Float64Seq&)
  {
    return unsupported("set_float64_values");
  }

  DDS::ReturnCode_t get_float128_values(DDS::Float128Seq&,
                                        DDS::MemberId)
  {
    return unsupported("get_float128_values");
  }

  DDS::ReturnCode_t set_float128_values(DDS::MemberId,
                                        const DDS::Float128Seq&)
  {
    return unsupported("set_float128_values");
  }

  DDS::ReturnCode_t get_char8_values(DDS::CharSeq&,
                                     DDS::MemberId)
  {
    return unsupported("get_char8_values");
  }

  DDS::ReturnCode_t set_char8_values(DDS::MemberId,
                                     const DDS::CharSeq&)
  {
    return unsupported("set_char8_values");
  }

  DDS::ReturnCode_t get_char16_values(DDS::WcharSeq&,
                                      DDS::MemberId)
  {
    return unsupported("get_char16_values");
  }

  DDS::ReturnCode_t set_char16_values(DDS::MemberId,
                                      const DDS::WcharSeq&)
  {
    return unsupported("set_char16_values");
  }

  DDS::ReturnCode_t get_byte_values(DDS::ByteSeq&,
                                    DDS::MemberId)
  {
    return unsupported("get_byte_values");
  }

  DDS::ReturnCode_t set_byte_values(DDS::MemberId,
                                    const DDS::ByteSeq&)
  {
    return unsupported("set_byte_values");
  }

  DDS::ReturnCode_t get_boolean_values(DDS::BooleanSeq&,
                                       DDS::MemberId)
  {
    return unsupported("get_boolean_values");
  }

  DDS::ReturnCode_t set_boolean_values(DDS::MemberId,
                                       const DDS::BooleanSeq&)
  {
    return unsupported("set_boolean_values");
  }

  DDS::ReturnCode_t get_string_values(DDS::StringSeq&,
                                      DDS::MemberId)
  {
    return unsupported("get_string_values");
  }

  DDS::ReturnCode_t set_string_values(DDS::MemberId,
                                      const DDS::StringSeq&)
  {
    return unsupported("set_string_values");
  }

  DDS::ReturnCode_t get_wstring_values(DDS::WstringSeq&,
                                       DDS::MemberId)
  {
    return unsupported("get_wstring_values");
  }

  DDS::ReturnCode_t set_wstring_values(DDS::MemberId,
                                       const DDS::WstringSeq&)
  {
    return unsupported("set_wstring_values");
  }

protected:
  DDS::ReturnCode_t unsupported(const char* method_name) const
  {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapter::%C: not implemented\n", method_name));
    }
    return DDS::RETCODE_UNSUPPORTED;
  }

private:
  DDS::DynamicType_var type_;
  T& value_;
  const RawAdapter& raw_adapter_;
};

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

#endif // OPENDDS_DCPS_XTYPES_DYNAMIC_DATA_ADAPTER_H
