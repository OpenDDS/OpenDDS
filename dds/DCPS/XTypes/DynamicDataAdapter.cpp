/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "DynamicDataAdapter.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/DCPS_Utils.h>

#if OPENDDS_HAS_DYNAMIC_DATA_ADAPTER

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

using DCPS::LogLevel;
using DCPS::log_level;
using DCPS::retcode_to_string;

DDS::UInt32 DynamicDataAdapter::get_item_count()
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_ENUM:
    return 1;
  case TK_UNION:
    {
      bool branch_active;
      DDS::MemberDescriptor_var active_branch;
      DDS::ReturnCode_t rc = get_selected_union_branch(branch_active, active_branch);
      if (rc != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Warning) {
          const CORBA::String_var type_name = type_->get_name();
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::item: "
            "get_selected_union_branch returned %C\n",
            type_name.in(), retcode_to_string(rc)));
        }
        return MEMBER_ID_INVALID;
      }
      return branch_active ? 2 : 1;
    }
    break;
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_BITMASK:
  case TK_ARRAY:
  case TK_STRUCTURE:
  case TK_MAP:
  case TK_BITSET:
    if (log_level >= LogLevel::Error) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DynamicDataAdapterImpl<%C>::get_item_count: "
        "this %C should have implemented get_item_count\n",
        type_name.in(), typekind_to_string(tk)));
    }
    return 0;
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_item_count: "
        "unexpected type %C\n", type_name.in(), typekind_to_string(tk)));
    }
    return 0;
  }
}

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
  DDS::ReturnCode_t rc;
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_STRUCTURE:
    {
      DDS::DynamicTypeMember_var dtm;
      rc = type_->get_member_by_index(dtm, index);
      if (rc != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Warning) {
          const CORBA::String_var type_name = type_->get_name();
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
            "get_member_by_index returned %C\n",
            type_name.in(), retcode_to_string(rc)));
        }
        return MEMBER_ID_INVALID;
      }
      return dtm->get_id();
    }
  case TK_UNION:
    {
      if (index == 0) {
        return DISCRIMINATOR_ID;
      } else if (index == 1) {
        bool branch_active;
        DDS::MemberDescriptor_var active_branch;
        DDS::ReturnCode_t rc = get_selected_union_branch(branch_active, active_branch);
        if (rc != DDS::RETCODE_OK) {
          if (DCPS::log_level >= DCPS::LogLevel::Warning) {
            const CORBA::String_var type_name = type_->get_name();
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
              "get_selected_union_branch returned %C\n",
              type_name.in(), retcode_to_string(rc)));
          }
          return MEMBER_ID_INVALID;
        }
        if (branch_active) {
          return active_branch->id();
        } else if (DCPS::log_level >= DCPS::LogLevel::Warning) {
          const CORBA::String_var type_name = type_->get_name();
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
            "union doesn't have an active branch, so index 1 is invalid\n",
            type_name.in()));
        }
      } else if (DCPS::log_level >= DCPS::LogLevel::Warning) {
        const CORBA::String_var type_name = type_->get_name();
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
          "index %u is invalid for unions\n",
          type_name.in(), index));
      }
      return MEMBER_ID_INVALID;
    }
  case TK_SEQUENCE:
    return get_member_id_at_index_impl(index);
  case TK_ARRAY:
    {
      DDS::ReturnCode_t rc = check_index("get_member_id_at_index", index, bound_total(type_desc_));
      if (rc != DDS::RETCODE_OK) {
        if (DCPS::log_level >= DCPS::LogLevel::Warning) {
          const CORBA::String_var type_name = type_->get_name();
          ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
            "check_index returned %C\n",
            type_name.in(), retcode_to_string(rc)));
        }
        return MEMBER_ID_INVALID;
      }
      return index;
    }
  default:
    if (DCPS::log_level >= DCPS::LogLevel::Warning) {
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataAdapterImpl<%C>::get_member_id_at_index: "
        "not supported for %C\n",
        type_name.in(), typekind_to_string(tk)));
    }
    return MEMBER_ID_INVALID;
  }
}

DDS::ReturnCode_t DynamicDataAdapter::clear_nonkey_values()
{
  return unsupported_method("DynamicDataAdapater::clear_nonkey_values");
}

DDS::ReturnCode_t DynamicDataAdapter::clear_value(DDS::MemberId)
{
  return unsupported_method("DynamicDataAdapater::clear_value");
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
  if (rc == DDS::RETCODE_OK) {
    dest = static_cast<const DDS::Char16*>(source);
  }
  return rc;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_HAS_DYNAMIC_DATA_ADAPTER
