/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "DynamicDataAdapter.h"

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DDS::MemberId DynamicDataAdapter::get_member_id_by_name(const char* name)
{
  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member_by_name(dtm, name) != DDS::RETCODE_OK) {
    return MEMBER_ID_INVALID;
  }
  return dtm->get_id();
}

DDS::MemberId DynamicDataAdapter::get_member_id_at_index_impl(DDS::UInt32)
{
  OPENDDS_ASSERT(false);
  return MEMBER_ID_INVALID;
}

DDS::MemberId DynamicDataAdapter::get_member_id_at_index(DDS::UInt32 index)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_STRUCTURE:
    {
      DDS::DynamicTypeMember_var dtm;
      if (type_->get_member_by_index(dtm, index) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      return dtm->get_id();
    }
  case TK_SEQUENCE:
    return get_member_id_at_index_impl(index);
  case TK_ARRAY:
    {
      DDS::TypeDescriptor_var td;
      if (type_->get_descriptor(td) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      if (check_index("get_member_id_at_index", index, bound_total(td)) != DDS::RETCODE_OK) {
        return MEMBER_ID_INVALID;
      }
      return index;
    }
  default:
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
        "not supported for %C\n",
        type_name.in(), typekind_to_string(tk)));
    }
    return MEMBER_ID_INVALID;
  }
}

DDS::ReturnCode_t DynamicDataAdapter::clear_all_values()
{
  return unsupported_method("DynamicDataAdapater::clear_all_values");
}

DDS::ReturnCode_t DynamicDataAdapter::clear_nonkey_values()
{
  return unsupported_method("DynamicDataAdapater::clear_nonkey_values");
}

DDS::ReturnCode_t DynamicDataAdapter::clear_value(DDS::MemberId)
{
  return unsupported_method("DynamicDataAdapater::clear_value");
}

DDS::DynamicData_ptr DynamicDataAdapter::clone()
{
  unsupported_method("DynamicDataAdapater::clone");
  return 0;
}

DDS::ReturnCode_t DynamicDataAdapter::invalid_id(const char* method, DDS::MemberId id) const
{
  if (DCPS::log_level >= DCPS::LogLevel::Notice) {
    const CORBA::String_var type_name = type_->get_name();
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataAdapterImpl<%C>::%C: invalid member id %u\n",
      type_name.in(), method, id));
  }
  return DDS::RETCODE_BAD_PARAMETER;
}

DDS::ReturnCode_t DynamicDataAdapter::missing_dda(const char* method, DDS::MemberId id) const
{
  if (DCPS::log_level >= DCPS::LogLevel::Notice) {
    const CORBA::String_var type_name = type_->get_name();
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataAdapterImpl<%C>::%C: "
      "member id %u doesn't have a DynamicDataAdapterImpl\n",
      type_name.in(), method, id));
  }
  return DDS::RETCODE_BAD_PARAMETER;
}

DDS::ReturnCode_t DynamicDataAdapter::assert_mutable(const char* method) const
{
  if (read_only_) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataAdapterImpl<%C>::%C: "
        "can't set values as this DynamicDataAdapter is read only\n",
        type_name.in(), method));
    }
    return DDS::RETCODE_ILLEGAL_OPERATION;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataAdapter::check_index(
  const char* method, DDS::UInt32 index, DDS::UInt32 size) const
{
  if (index >= size) {
    if (OpenDDS::DCPS::log_level >= OpenDDS::DCPS::LogLevel::Notice) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataAdapterImpl<%C>::%C: "
        "index %u is larger or equal to than the size (%u)\n",
        type_name.in(), method, index, size));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t DynamicDataAdapter::check_member(
  DDS::DynamicType_var& member_type, const char* method, DDS::TypeKind tk, DDS::MemberId id)
{
  return DynamicDataBase::check_member(member_type, method, "access", id, tk);
}

DDS::ReturnCode_t DynamicDataAdapter::check_member(
  const char* method, DDS::TypeKind tk, DDS::MemberId id)
{
  DDS::DynamicType_var member_type;
  return check_member(member_type, method, tk, id);
}

DDS::ReturnCode_t DynamicDataAdapter::get_byte_raw_value(
  const char* method, void* dest, DDS::TypeKind tk,
  DDS::Byte source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::Byte*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_byte_raw_value(
  const char* method, DDS::Byte& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::Byte*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_bool_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, DDS::Boolean source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::Boolean*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_bool_raw_value(
  const char* method, DDS::Boolean& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::Boolean*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_i8_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, DDS::Int8 source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::Int8*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_i8_raw_value(
  const char* method, DDS::Int8& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::Int8*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_u8_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, DDS::UInt8 source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::UInt8*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_u8_raw_value(
  const char* method, DDS::UInt8& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::UInt8*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_c8_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, DDS::Char8 source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::Char8*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_c8_raw_value(
  const char* method, DDS::Char8& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::Char8*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_c16_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, DDS::Char16 source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  *static_cast<DDS::Char16*>(dest) = source;
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_c16_raw_value(
  const char* method, DDS::Char16& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = *static_cast<const DDS::Char16*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_s8_raw_value(
  const char* method, void* dest, DDS::TypeKind tk, const char* source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  char*& dest_value = *static_cast<char**>(dest);
  CORBA::string_free(dest_value);
  dest_value = CORBA::string_dup(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_s8_raw_value(
  const char* method, char*& dest, DDS::MemberId id, const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  CORBA::string_free(dest);
  dest = CORBA::string_dup(static_cast<const char*>(source));
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_cpp11_s8_raw_value(
  const char* method, void* dest, DDS::TypeKind tk,
  const std::string& source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  char*& dest_value = *static_cast<char**>(dest);
  CORBA::string_free(dest_value);
  dest_value = CORBA::string_dup(source.c_str());
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_cpp11_s8_raw_value(
  const char* method, std::string& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = static_cast<const char*>(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_s16_raw_value(
  const char* method, void* dest, DDS::TypeKind tk,
  const DDS::Char16* source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  DDS::Char16*& dest_value = *static_cast<DDS::Char16**>(dest);
  CORBA::wstring_free(dest_value);
  dest_value = CORBA::wstring_dup(source);
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_s16_raw_value(
  const char* method, DDS::Char16*& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  CORBA::wstring_free(dest);
  dest = CORBA::wstring_dup(static_cast<const DDS::Char16*>(source));
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::get_cpp11_s16_raw_value(
  const char* method, void* dest, DDS::TypeKind tk,
  const std::wstring& source, DDS::MemberId id)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  DDS::Char16*& dest_value = *static_cast<DDS::Char16**>(dest);
  CORBA::wstring_free(dest_value);
  dest_value = CORBA::wstring_dup(source.c_str());
  return rc;
}

DDS::ReturnCode_t DynamicDataAdapter::set_cpp11_s16_raw_value(
  const char* method, std::wstring& dest, DDS::MemberId id,
  const void* source, DDS::TypeKind tk)
{
  const DDS::ReturnCode_t rc = check_member(method, tk, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  dest = static_cast<const DDS::Char16*>(source);
  return rc;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
