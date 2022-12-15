/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "Utils.h"

#  include "DynamicDataImpl.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/DCPS_Utils.h>

#  include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

using DCPS::retcode_to_string;
using DCPS::LogLevel;
using DCPS::log_level;

DDS::ReturnCode_t extensibility(DDS::DynamicType_ptr type, DCPS::Extensibility& ext)
{
  DDS::DynamicType_var base_type = get_base_type(type);
  if (!base_type) {
    return DDS::RETCODE_BAD_PARAMETER;
  }
  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
  case TK_UNION:
    {
      DDS::TypeDescriptor_var td;
      const DDS::ReturnCode_t rc = type->get_descriptor(td);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      ext = dds_to_opendds_ext(td->extensibility_kind());
    }
    break;
  default:
    ext = DCPS::FINAL;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t max_extensibility(DDS::DynamicType_ptr type, DCPS::Extensibility& ext)
{
  DDS::ReturnCode_t rc = extensibility(type, ext);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  DDS::DynamicType_var base_type = get_base_type(type);
  const TypeKind tk = base_type->get_kind();
  if (tk != TK_STRUCTURE && tk != TK_UNION) {
    return DDS::RETCODE_OK;
  }

  DDS::DynamicTypeMembersById_var members;
  rc = base_type->get_all_members(members);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  DynamicTypeMembersByIdImpl* const members_impl =
    dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
  if (!members_impl) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  for (DynamicTypeMembersByIdImpl::const_iterator it = members_impl->begin();
      it != members_impl->end(); ++it) {
    DDS::MemberDescriptor_var md;
    rc = it->second->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    DDS::DynamicType_ptr member_type = md->type();
    if (!member_type) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    DCPS::Extensibility member_ext;
    DDS::ReturnCode_t rc = max_extensibility(member_type, member_ext);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    ext = std::max(member_ext, ext);
  }

  return DDS::RETCODE_OK;
}

DCPS::Extensibility dds_to_opendds_ext(DDS::ExtensibilityKind ext)
{
  switch (ext) {
  case DDS::FINAL:
    return DCPS::FINAL;
  case DDS::APPENDABLE:
    return DCPS::APPENDABLE;
  case DDS::MUTABLE:
    return DCPS::MUTABLE;
  }
  OPENDDS_ASSERT(false);
  return DCPS::FINAL;
}

DDS::ReturnCode_t MemberPath::get_member_from_type(
  DDS::DynamicType_ptr type, DDS::DynamicTypeMember_var& member)
{
  member = 0;
  if (ids.empty()) {
    return DDS::RETCODE_ILLEGAL_OPERATION;
  }

  DDS::DynamicType_var base_type = get_base_type(type);
  if (!base_type) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  MemberIdVec::iterator it = ids.begin();
  DDS::DynamicType_var current_type = DDS::DynamicType::_duplicate(type);
  DDS::DynamicTypeMember_var current_member;
  while (true) {
    DDS::ReturnCode_t rc = current_type->get_member(current_member, *it);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    if (++it == ids.end()) {
      break;
    }

    DDS::MemberDescriptor_var md;
    rc = current_member->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    current_type = get_base_type(md->type());
    if (!base_type) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
  }
  member = current_member;

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t MemberPath::get_member_from_data(
  DDS::DynamicData_ptr data, DDS::DynamicData_var& container, DDS::MemberId& member_id)
{
  container = 0;
  if (ids.empty()) {
    return DDS::RETCODE_ILLEGAL_OPERATION;
  }

  MemberIdVec::iterator it = ids.begin();
  DDS::DynamicData_var current_data = DDS::DynamicData::_duplicate(data);
  while (true) {
    const DDS::MemberId current_id = *it;
    if (++it == ids.end()) {
      member_id = current_id;
      break;
    }

    DDS::ReturnCode_t rc = current_data->get_complex_value(current_data, current_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  }
  container = current_data._retn();

  return DDS::RETCODE_OK;
}

namespace {
  DDS::ReturnCode_t get_keys_i(DDS::DynamicType_ptr type, MemberPathVec& paths,
    const MemberPath& base_path);

  DDS::ReturnCode_t get_keys_i_struct(DynamicTypeMembersByIdImpl* members, MemberPathVec& paths,
    const MemberPath& base_path, bool implied_all_check = false)
  {
    bool implied_all = false;
    if (!implied_all_check && base_path.level() > 0) {
      // If there are no explicit keys, then they are implied to all be keys.
      // TODO: Except when @key(FALSE)
      MemberPathVec explicit_key;
      const DDS::ReturnCode_t rc = get_keys_i_struct(members, explicit_key, base_path, true);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      implied_all = explicit_key.empty();
    }

    for (DynamicTypeMembersByIdImpl::const_iterator it = members->begin();
        it != members->end(); ++it) {
      DDS::MemberDescriptor_var md;
      DDS::ReturnCode_t rc = it->second->get_descriptor(md);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      if (implied_all || md->is_key()) {
        const DDS::MemberId id = md->id();
        if (implied_all_check) {
          paths.push_back(MemberPath(base_path, id));
          break; // Just need one explict key to know we can't imply all are keys
        }

        rc = get_keys_i(md->type(), paths, MemberPath(base_path, id));
        if (rc != DDS::RETCODE_OK) {
          return rc;
        }
      }
    }

    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t get_keys_i(DDS::DynamicType_ptr type, MemberPathVec& paths,
    const MemberPath& base_path)
  {
    DDS::DynamicType_var base_type = get_base_type(type);
    if (!base_type) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    const TypeKind kind = base_type->get_kind();
    switch (kind) {
    case TK_STRUCTURE:
      {
        DDS::DynamicTypeMembersById_var members;
        DDS::ReturnCode_t rc = type->get_all_members(members);
        if (rc != DDS::RETCODE_OK) {
          return rc;
        }

        DynamicTypeMembersByIdImpl* const members_impl =
          dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
        if (!members_impl) {
          return DDS::RETCODE_BAD_PARAMETER;
        }

        return get_keys_i_struct(members_impl, paths, base_path);
      }
    case TK_UNION:
      {
        DDS::DynamicTypeMember_var disc;
        const MemberId id = DISCRIMINATOR_ID;
        const MemberPath this_path(base_path, id);
        if (base_path.level() == 0) {
          DDS::ReturnCode_t rc = type->get_member(disc, id);
          if (rc != DDS::RETCODE_OK) {
            return rc;
          }
          DDS::MemberDescriptor_var md;
          rc = disc->get_descriptor(md);
          if (rc != DDS::RETCODE_OK) {
            return rc;
          }
          if (md->is_key()) {
            paths.push_back(this_path);
          }
        } else {
          // If we're here then the union field has been marked so the
          // disciminator is an implied key even if it doesn't have @key.
          // TODO: Except when @key(FALSE)
          paths.push_back(this_path);
        }
      }
      break;
    default:
      if (base_path.level() == 0) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: get_keys_i: "
            "get_keys was passed an invalid topic type: %C\n",
            typekind_to_string(kind)));
        }
        return DDS::RETCODE_BAD_PARAMETER;
      }
      paths.push_back(base_path);
      break;
    }

    return DDS::RETCODE_OK;
  }
}

DDS::ReturnCode_t get_keys(DDS::DynamicType_ptr type, MemberPathVec& paths)
{
  return get_keys_i(type, paths, MemberPath());
}

DDS::ReturnCode_t key_count(DDS::DynamicType_ptr type, size_t& count)
{
  MemberPathVec paths;
  const DDS::ReturnCode_t rc = get_keys(type, paths);
  if (rc == DDS::RETCODE_OK) {
    count = paths.size();
  }
  return rc;
}

namespace {
  DDS::ReturnCode_t uint_like_bound(
    DDS::DynamicType_ptr type, size_t& bound_max, DDS::TypeKind& bound_kind)
  {
    const DDS::TypeKind kind = type->get_kind();
    if (kind != TK_ENUM && kind != TK_BITMASK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: uint_like_bound: "
          "expected bound uint-like, got %C\n",
          typekind_to_string(kind)));
      }
      return DDS::RETCODE_BAD_PARAMETER;
    }

    DDS::TypeDescriptor_var td;
    const DDS::ReturnCode_t rc = type->get_descriptor(td);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }

    const size_t bound_size = td->bound()[0];
    // enum max is 32, bitmask is 64
    if (bound_size >= 1 && bound_size <= 8) {
      bound_kind = TK_UINT8;
    } else if (bound_size >= 9 && bound_size <= 16) {
      bound_kind = TK_UINT16;
    } else if (bound_size >= 17 && bound_size <= 32) {
      bound_kind = TK_UINT32;
    } else if (bound_size >= 33 && bound_size <= 64 && kind == TK_BITMASK) {
      bound_kind = TK_UINT64;
    } else {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: uint_like_bound: "
          "Got unexpected bound size %B for %C\n",
          bound_size, typekind_to_string(kind)));
      }
      return DDS::RETCODE_BAD_PARAMETER;
    }
    bound_max = (1 << bound_size) - 1;
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t get_uint_value(
    CORBA::UInt64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind)
  {
    DDS::ReturnCode_t rc = DDS::RETCODE_BAD_PARAMETER;
    switch (kind) {
    case TK_UINT8:
      {
        CORBA::UInt8 v;
        rc = src->get_uint8_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_UINT16:
      {
        CORBA::UInt16 v;
        rc = src->get_uint16_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_UINT32:
      {
        CORBA::UInt32 v;
        rc = src->get_uint32_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_UINT64:
      rc = src->get_uint64_value(value, id);
      break;
    }
    return rc;
  }

  DDS::ReturnCode_t get_enum_or_bitmask_value(
    CORBA::UInt64& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id)
  {
    size_t bound_max;
    DDS::TypeKind bound_kind;
    const DDS::ReturnCode_t rc = uint_like_bound(type, bound_max, bound_kind);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    return get_uint_value(value, src, id, bound_kind);
  }

  DDS::ReturnCode_t get_int_value(
    CORBA::Int64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind)
  {
    DDS::ReturnCode_t rc = DDS::RETCODE_BAD_PARAMETER;
    switch (kind) {
    case TK_INT8:
      {
        CORBA::Int8 v;
        rc = src->get_int8_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_INT16:
      {
        CORBA::Int16 v;
        rc = src->get_int16_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_INT32:
      {
        CORBA::Int32 v;
        rc = src->get_int32_value(v, id);
        if (rc != DDS::RETCODE_OK) {
          value = v;
        }
      }
      break;
    case TK_INT64:
      rc = src->get_int64_value(value, id);
      break;
    }
    return rc;
  }

  template <typename T>
  void cmp(int& result, T a, T b)
  {
    if (a < b) {
      result = 1;
    } else if (a > b) {
      result = -1;
    }
    result = 0;
  }

  DDS::ReturnCode_t member_key_compare(int& result,
    DDS::DynamicData_ptr a_data, DDS::MemberId a_id,
    DDS::DynamicData_ptr b_data, DDS::MemberId b_id)
  {
    DDS::MemberDescriptor_var a_md;
    DDS::ReturnCode_t rc = a_data->get_descriptor(a_md, a_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicType_var a_type = get_base_type(a_md->type());
    const DDS::TypeKind tk = a_type->get_kind();

    DDS::MemberDescriptor_var b_md;
    b_data->get_descriptor(b_md, b_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicType_var b_type = get_base_type(b_md->type());
    const DDS::TypeKind b_tk = b_type->get_kind();

    if (tk != b_tk) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: member_key_compare: "
          "trying to compare a %C to a %C\n",
          typekind_to_string(tk),
          typekind_to_string(b_tk)));
      }
      return DDS::RETCODE_BAD_PARAMETER;
    }

    DDS::ReturnCode_t a_rc = DDS::RETCODE_OK;
    DDS::ReturnCode_t b_rc = DDS::RETCODE_OK;

    switch (tk) {
    case TK_BOOLEAN:
      {
        CORBA::Boolean a_value;
        a_rc = a_data->get_boolean_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Boolean b_value;
          b_rc = b_data->get_boolean_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_BYTE:
      {
        CORBA::Octet a_value;
        a_rc = a_data->get_byte_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Octet b_value;
          b_rc = b_data->get_byte_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_UINT8:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
      {
        CORBA::UInt64 a_value;
        a_rc = get_uint_value(a_value, a_data, a_id, tk);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::UInt64 b_value;
          b_rc = get_uint_value(b_value, b_data, b_id, tk);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_INT8:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
      {
        CORBA::Int64 a_value;
        a_rc = get_int_value(a_value, a_data, a_id, tk);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Int64 b_value;
          b_rc = get_int_value(b_value, b_data, b_id, tk);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT32:
      {
        CORBA::Float a_value;
        a_rc = a_data->get_float32_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Float b_value;
          b_rc = b_data->get_float32_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT64:
      {
        CORBA::Double a_value;
        a_rc = a_data->get_float64_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Double b_value;
          b_rc = b_data->get_float64_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT128:
      {
        CORBA::LongDouble a_value;
        a_rc = a_data->get_float128_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::LongDouble b_value;
          b_rc = b_data->get_float128_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_CHAR8:
      {
        CORBA::Char a_value;
        a_rc = a_data->get_char8_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::Char b_value;
          b_rc = b_data->get_char8_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_CHAR16:
      {
        CORBA::WChar a_value;
        a_rc = a_data->get_char16_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::WChar b_value;
          b_rc = b_data->get_char16_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_STRING8:
      {
        CORBA::String_var a_value;
        a_rc = a_data->get_string_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::String_var b_value;
          b_rc = b_data->get_string_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            result = std::strcmp(a_value.in(), b_value.in());
          }
        }
      }
      break;

    case TK_STRING16:
      {
        CORBA::WString_var a_value;
        a_rc = a_data->get_wstring_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::WString_var b_value;
          b_rc = b_data->get_wstring_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            result = std::wcscmp(a_value.in(), b_value.in());
          }
        }
      }
      break;

    case TK_ENUM:
    case TK_BITMASK:
      {
        CORBA::UInt64 a_value;
        a_rc = get_enum_or_bitmask_value(a_value, a_type, a_data, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          CORBA::UInt64 b_value;
          b_rc = get_enum_or_bitmask_value(b_value, b_type, b_data, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    // TODO(iguessthislldo): I hate to leave this for later because it makes maps
    // and bitsets just that tiny bit harder, but I'm not certain how keys
    // would work with them.
    /*
    case TK_MAP:
    case TK_BITSET:
    */
    case TK_SEQUENCE:
    case TK_ARRAY:
      {
        DDS::DynamicData_var a_value;
        a_rc = a_data->get_complex_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::DynamicData_var b_value;
          b_rc = b_data->get_complex_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            switch (tk) {
            case TK_ARRAY:
            case TK_SEQUENCE:
              {
                const ACE_CDR::UInt32 a_count = a_value->get_item_count();
                const ACE_CDR::UInt32 b_count = b_value->get_item_count();
                const ACE_CDR::UInt32 count = std::min(a_count, b_count);
                for (ACE_CDR::UInt32 i = 0;
                    a_rc == DDS::RETCODE_OK && i < count && result == 0; ++i) {
                  a_rc = b_rc = member_key_compare(result,
                    a_value, a_value->get_member_id_at_index(i),
                    b_value, b_value->get_member_id_at_index(i));
                }
                if (result == 0 && a_count != b_count) {
                  // This mimics strcmp
                  result = count == a_count ? 1 : -1;
                }
              }
              break;

            default:
              OPENDDS_ASSERT(false);
              break;
            }
          }
        }
      }
      break;

    case TK_STRUCTURE:
    case TK_UNION:
      // get_keys shouldn't be returning these directly, they should be
      // returning either the key members of the structure or the
      // discriminator of the union.
    case TK_ALIAS:
    case TK_ANNOTATION:
    default:
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "member has unexpected TypeKind %C\n", typekind_to_string(b_tk)));
      }
      a_rc = DDS::RETCODE_BAD_PARAMETER;
      b_rc = DDS::RETCODE_BAD_PARAMETER;
    }

    if (a_rc != DDS::RETCODE_OK || b_rc != DDS::RETCODE_OK) {
      const CORBA::String_var b_type_name = b_type->get_name();
      const CORBA::String_var b_member_name = b_md->name();
      const CORBA::String_var a_type_name = a_type->get_name();
      const CORBA::String_var a_member_name = a_md->name();
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "Could not copy member type %C id %u from %C.%C to %C.%C: "
          "get: %C set: %C\n",
          typekind_to_string(b_tk), a_id,
          b_type_name.in(), b_member_name.in(),
          a_type_name.in(), a_member_name.in(),
          retcode_to_string(a_rc), retcode_to_string(b_rc)));
      }
    }

    return DDS::RETCODE_OK;
  }
}

DDS::ReturnCode_t key_less_than(bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b)
{
  DDS::DynamicType_var a_type = a->type();
  MemberPathVec paths;
  DDS::ReturnCode_t rc = get_keys(a_type, paths);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: key_compare: get_keys failed: %C\n",
        retcode_to_string(rc)));
    }
    return rc;
  }

  result = false;
  for (MemberPathVec::iterator it = paths.begin(); it != paths.end(); it++) {
    DDS::DynamicData_var a_container;
    DDS::MemberId a_member_id;
    rc = it->get_member_from_data(a, a_container, a_member_id);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: key_compare: "
          "get_member_from_data for a failed: %C\n",
          retcode_to_string(rc)));
      }
      return rc;
    }

    DDS::DynamicData_var b_container;
    DDS::MemberId b_member_id;
    rc = it->get_member_from_data(b, b_container, b_member_id);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: key_compare: "
          "get_member_from_data for b failed: %C\n",
          retcode_to_string(rc)));
      }
      return rc;
    }

    int c = 0;
    rc = member_key_compare(c, a_container, a_member_id, b_container, b_member_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    if (c != 0) {
      result = c < 0;
      return DDS::RETCODE_OK;
    }
  }

  return DDS::RETCODE_OK;
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
