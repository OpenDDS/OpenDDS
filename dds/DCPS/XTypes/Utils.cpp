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
#  include <dds/DCPS/SafetyProfileStreams.h>

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

bool MemberPathParser::consume(size_t by)
{
  if (by > left) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) Warning: MemberPathParser::consume: "
        "at pos %B with %B left trying to increment by %B\n", pos, left, by));
    }
    error = true;
    return false;
  }
  pos += by;
  left -= by;
  path += by;
  return true;
}

bool MemberPathParser::get_next_subpath()
{
  if (!path) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
        "empty or null path\n"));
    }
    error = true;
    return false;
  }

  // See if we're in a nested member or subscript and move past the '.' or '[' if we are.
  in_subscript = left > 0 ? path[0] == '[' : false;
  const bool nested_member = left > 0 ? path[0] == '.' : false;
  if (nested_member && pos == 0) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
        "at pos 0 found unexpected '.'\n"));
    }
    error = true;
    return false;
  }
  if (nested_member || in_subscript) {
    consume(1);
  }

  size_t i = 0; // Char count to consume
  size_t got = 0; // Char count to use for the result
  char c = '\0';
  bool scan = true;
  for (; i < left && scan; ++i) {
    c = path[i];
    switch (c) {
    case '.':
    case '[':
      if (in_subscript) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
            "at pos %B unexpected '%c' in a subscript\n", pos + i, c));
        }
        error = true;
        return false;
      }
      --i; // Don't consume, leave for next iteration
      // fallthrough
    case ']':
      scan = false;
      break;

    default:
      ++got;
    }
  }

  if (in_subscript && c != ']') {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
        "at pos %B expected to find a ']' to end subscript\n", pos + i));
    }
    error = true;
    return false;
  }

  if (got == 0) {
    if (in_subscript || nested_member) {
      if (log_level >= LogLevel::Notice) {
        const char* const expected = in_subscript ? "index or key" : "member name";
        if (c) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
            "at pos %B expected to find %C before '%c'\n", pos + i, expected, c));
        } else {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_next_subpath: "
            "at pos %B expected to find %C before the end of the path\n", pos + i, expected));
        }
      }
      error = true;
    }
    return false;
  }

  subpath.assign(path, got);
  return consume(i);
}

bool MemberPathParser::get_index(DDS::UInt32& index)
{
  if (!in_subscript || subpath.empty() ||
      subpath.find_first_not_of("0123456789") != DCPS::String::npos) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPathParser::get_index: "
        "\"%C\" is not a valid subscript index\n", subpath.c_str()));

    }
    return false;
  }
  return DCPS::convertToInteger(subpath, index);
}

DDS::ReturnCode_t MemberPath::resolve_string_path(DDS::DynamicType_ptr type, const DCPS::String& path)
{
  DDS::DynamicType_var current_type = get_base_type(type);
  if (!current_type) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  MemberPathParser parser(path);
  while (parser.get_next_subpath()) {
    if (parser.in_subscript) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPath::resolve_string_path: "
          "given \"%C\", which contains subscripts and these are currently not supported\n",
          path.c_str()));
      }
      return DDS::RETCODE_UNSUPPORTED;
    }
    DDS::DynamicTypeMember_var dtm;
    DDS::ReturnCode_t rc = current_type->get_member_by_name(dtm, parser.subpath.c_str());
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    id(dtm->get_id());

    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicType_var next = get_base_type(md->type());
    if (!next) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    current_type = next;
  }

  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: MemberPath::resolve_string_path: "
      "parser failed to parse \"%C\"\n", path.c_str()));
    return DDS::RETCODE_OK;
  }

  return DDS::RETCODE_BAD_PARAMETER;
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
    DDS::DynamicType_var next = get_base_type(md->type());
    if (!next) {
      return DDS::RETCODE_BAD_PARAMETER;
    }
    current_type = next;
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
  DDS::DynamicData_var current_container = DDS::DynamicData::_duplicate(data);
  while (true) {
    const DDS::MemberId current_id = *it;
    if (++it == ids.end()) {
      member_id = current_id;
      break;
    }

    DDS::DynamicData_var next;
    DDS::ReturnCode_t rc = current_container->get_complex_value(next, current_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    current_container = next;
  }
  container = current_container;

  return DDS::RETCODE_OK;
}

namespace {
  DDS::ReturnCode_t get_values_i(DDS::DynamicType_ptr type, MemberPathVec& paths, Filter filter,
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

        DynamicTypeMembersByIdImpl* const members_i =
          dynamic_cast<DynamicTypeMembersByIdImpl*>(members.in());
        if (!members_i) {
          return DDS::RETCODE_BAD_PARAMETER;
        }

        bool include_all = filter == Filter_All;
        if (filter == Filter_NestedKeys) {
          // If there are no explicit keys, then they are implied to all be keys.
          // TODO: Except when @key(FALSE)
          include_all = true;
          for (DynamicTypeMembersByIdImpl::const_iterator it = members_i->begin();
              it != members_i->end(); ++it) {
            DDS::MemberDescriptor_var md;
            DDS::ReturnCode_t rc = it->second->get_descriptor(md);
            if (rc != DDS::RETCODE_OK) {
              return rc;
            }
            if (md->is_key()) {
              include_all = false;
              break;
            }
          }
        }

        for (DynamicTypeMembersByIdImpl::const_iterator it = members_i->begin();
            it != members_i->end(); ++it) {
          DDS::MemberDescriptor_var md;
          DDS::ReturnCode_t rc = it->second->get_descriptor(md);
          if (rc != DDS::RETCODE_OK) {
            return rc;
          }
          if ((filter == Filter_NonKeys) != (include_all || md->is_key())) {
            const MemberPath path(base_path, md->id());
            rc = get_values_i(md->type(), paths,
              filter == Filter_Keys ? Filter_NestedKeys : filter, path);
            if (rc != DDS::RETCODE_OK) {
              return rc;
            }
          }
        }
      }
      break;

    case TK_UNION:
      {
        DDS::DynamicTypeMember_var disc;
        const MemberId id = DISCRIMINATOR_ID;
        const MemberPath this_path(base_path, id);
        bool include = false;
        switch (filter) {
        case Filter_Keys:
        case Filter_NonKeys:
          {
            DDS::ReturnCode_t rc = type->get_member(disc, id);
            if (rc != DDS::RETCODE_OK) {
              return rc;
            }
            DDS::MemberDescriptor_var md;
            rc = disc->get_descriptor(md);
            if (rc != DDS::RETCODE_OK) {
              return rc;
            }
            include = (filter == Filter_NonKeys) != md->is_key();
          }
          break;
        case Filter_All:
        case Filter_NestedKeys:
          // If we're here then the union field has been marked so the
          // disciminator is an implied key even if it doesn't have @key.
          // TODO: Except when @key(FALSE)
          include = true;
          break;
        }
        if (include) {
          paths.push_back(this_path);
        }
      }
      break;

    default:
      if (base_path.level() == 0) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: get_values_i: "
            "get_values was passed an invalid topic type: %C\n",
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

DDS::ReturnCode_t get_values(DDS::DynamicType_ptr type, MemberPathVec& paths, Filter filter)
{
  return get_values_i(type, paths, filter, MemberPath());
}

DDS::ReturnCode_t get_keys(DDS::DynamicType_ptr type, MemberPathVec& paths)
{
  return get_values(type, paths, Filter_Keys);
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

bool is_key(DDS::DynamicType_ptr type, const char* field)
{
  MemberPathVec paths;
  if (get_keys(type, paths) != DDS::RETCODE_OK) {
    return false;
  }
  for (size_t i = 0; i < paths.size(); ++i) {
    DDS::DynamicTypeMember_var m;
    if (paths[i].get_member_from_type(type, m) != DDS::RETCODE_OK) {
      return false;
    }
    const CORBA::String_var name = m->get_name();
    if (0 == std::strcmp(name, field)) {
      return true;
    }
  }
  return false;
}

namespace {
  template <typename T>
  void cmp(int& result, T a, T b)
  {
    if (a < b) {
      result = -1;
    } else if (a > b) {
      result = 1;
    } else {
      result = 0;
    }
  }

  DDS::ReturnCode_t member_compare(int& result,
    DDS::DynamicData_ptr a_data, DDS::MemberId a_id,
    DDS::DynamicData_ptr b_data, DDS::MemberId b_id)
  {
    DDS::DynamicType_var a_type;
    DDS::ReturnCode_t rc = get_member_type(a_type, a_data, a_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    const DDS::TypeKind tk = a_type->get_kind();

    DDS::DynamicType_var b_type;
    rc = get_member_type(b_type, b_data, b_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    const DDS::TypeKind b_tk = b_type->get_kind();

    if (tk != b_tk) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: member_compare: "
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
        DDS::Boolean a_value = false;
        a_rc = a_data->get_boolean_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Boolean b_value = false;
          b_rc = b_data->get_boolean_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_BYTE:
      {
        DDS::Byte a_value;
        a_rc = a_data->get_byte_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Byte b_value;
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
        DDS::UInt64 a_value;
        a_rc = get_uint_value(a_value, a_data, a_id, tk);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::UInt64 b_value;
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
        DDS::Int64 a_value;
        a_rc = get_int_value(a_value, a_data, a_id, tk);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Int64 b_value;
          b_rc = get_int_value(b_value, b_data, b_id, tk);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT32:
      {
        DDS::Float32 a_value;
        a_rc = a_data->get_float32_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Float32 b_value;
          b_rc = b_data->get_float32_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT64:
      {
        DDS::Float64 a_value;
        a_rc = a_data->get_float64_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Float64 b_value;
          b_rc = b_data->get_float64_value(b_value, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_FLOAT128:
      {
        DDS::Float128 a_value;
        a_rc = a_data->get_float128_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Float128 b_value;
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
      {
        DDS::Int32 a_value;
        a_rc = get_enum_value(a_value, a_type, a_data, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Int32 b_value;
          b_rc = get_enum_value(b_value, b_type, b_data, b_id);
          if (b_rc == DDS::RETCODE_OK) {
            cmp(result, a_value, b_value);
          }
        }
      }
      break;

    case TK_BITMASK:
      {
        DDS::UInt64 a_value;
        a_rc = get_bitmask_value(a_value, a_type, a_data, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::UInt64 b_value;
          b_rc = get_bitmask_value(b_value, b_type, b_data, b_id);
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
                const DDS::UInt32 a_count = a_value->get_item_count();
                const DDS::UInt32 b_count = b_value->get_item_count();
                const DDS::UInt32 count = std::min(a_count, b_count);
                for (DDS::UInt32 i = 0; a_rc == DDS::RETCODE_OK && i < count && result == 0; ++i) {
                  a_rc = b_rc = member_compare(result,
                    a_value, a_value->get_member_id_at_index(i),
                    b_value, b_value->get_member_id_at_index(i));
                }
                if (result == 0 && a_count != b_count) {
                  result = count == a_count ? -1 : 1;
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
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: member_compare(DynamicData): "
          "member has unexpected TypeKind %C\n", typekind_to_string(b_tk)));
      }
      a_rc = DDS::RETCODE_BAD_PARAMETER;
      b_rc = DDS::RETCODE_BAD_PARAMETER;
    }

    if (a_rc != DDS::RETCODE_OK || b_rc != DDS::RETCODE_OK) {
      const CORBA::String_var b_type_name = b_data->type()->get_name();
      const CORBA::String_var a_type_name = a_data->type()->get_name();
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: member_compare(DynamicData): "
          "Could not compare member type %C id %u from %C (%C) to %C (%C)\n",
          typekind_to_string(tk), a_id,
          a_type_name.in(), retcode_to_string(a_rc),
          b_type_name.in(), retcode_to_string(b_rc)));
      }
    }

    return DDS::RETCODE_OK;
  }
}

DDS::ReturnCode_t less_than(
  bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b, Filter filter)
{
  DDS::DynamicType_var a_type = a->type();
  MemberPathVec paths;
  DDS::ReturnCode_t rc = get_values(a_type, paths, filter);
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

    int compare = 0;
    rc = member_compare(compare, a_container, a_member_id, b_container, b_member_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    if (compare != 0) {
      result = compare < 0;
      return DDS::RETCODE_OK;
    }
  }

  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t key_less_than(bool& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b)
{
  return less_than(result, a, b, Filter_Keys);
}

DDS::ReturnCode_t compare_members(int& result, DDS::DynamicData_ptr a, DDS::DynamicData_ptr b, DDS::MemberId id)
{
  return member_compare(result, a, id, b, id);
}

DDS::ReturnCode_t get_member_type(DDS::DynamicType_var& member_type,
  DDS::DynamicType_ptr container_type, DDS::MemberId id)
{
  const DDS::TypeKind container_kind = container_type->get_kind();
  if (is_sequence_like(container_kind)) {
    DDS::TypeDescriptor_var td;
    DDS::ReturnCode_t rc = container_type->get_descriptor(td);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    member_type = get_base_type(td->element_type());
  } else if (is_scalar(container_kind)) {
    if (id != MEMBER_ID_INVALID && log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: get_member_type: "
        "Accessing a %C DynamicData via id %u, not MEMBER_ID_INVALID\n",
        typekind_to_string(container_kind), id));
    }
    member_type = DDS::DynamicType::_duplicate(container_type);
    return DDS::RETCODE_OK;
  } else {
    DDS::DynamicTypeMember_var dtm;
    DDS::ReturnCode_t rc = container_type->get_member(dtm, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    member_type = get_base_type(md->type());
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t get_uint_value(
  DDS::UInt64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind)
{
  DDS::ReturnCode_t rc = DDS::RETCODE_BAD_PARAMETER;
  switch (kind) {
  case TK_UINT8:
    {
      DDS::UInt8 v;
      rc = src->get_uint8_value(v, id);
      if (rc == DDS::RETCODE_OK) {
        value = v;
      }
    }
    break;
  case TK_UINT16:
    {
      DDS::UInt16 v;
      rc = src->get_uint16_value(v, id);
      if (rc == DDS::RETCODE_OK) {
        value = v;
      }
    }
    break;
  case TK_UINT32:
    {
      DDS::UInt32 v;
      rc = src->get_uint32_value(v, id);
      if (rc == DDS::RETCODE_OK) {
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

DDS::ReturnCode_t set_uint_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, DDS::UInt64 value)
{
  switch (kind) {
  case TK_UINT8:
    return dest->set_uint8_value(id, static_cast<DDS::UInt8>(value));
  case TK_UINT16:
    return dest->set_uint16_value(id, static_cast<DDS::UInt16>(value));
  case TK_UINT32:
    return dest->set_uint32_value(id, static_cast<DDS::UInt32>(value));
  case TK_UINT64:
    return dest->set_uint64_value(id, value);
  }
  return DDS::RETCODE_BAD_PARAMETER;
}

DDS::ReturnCode_t get_int_value(
  DDS::Int64& value, DDS::DynamicData_ptr src, DDS::MemberId id, DDS::TypeKind kind)
{
  DDS::ReturnCode_t rc = DDS::RETCODE_BAD_PARAMETER;
  switch (kind) {
  case TK_INT8:
    {
      DDS::Int8 v;
      rc = src->get_int8_value(v, id);
      if (rc == DDS::RETCODE_OK) {
        value = v;
      }
    }
    break;
  case TK_INT16:
    {
      DDS::Int16 v;
      rc = src->get_int16_value(v, id);
      if (rc == DDS::RETCODE_OK) {
        value = v;
      }
    }
    break;
  case TK_INT32:
    {
      DDS::Int32 v;
      rc = src->get_int32_value(v, id);
      if (rc == DDS::RETCODE_OK) {
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

DDS::ReturnCode_t set_int_value(
  DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::TypeKind kind, DDS::Int64 value)
{
  switch (kind) {
  case TK_INT8:
    return dest->set_int8_value(id, static_cast<DDS::Int8>(value));
  case TK_INT16:
    return dest->set_int16_value(id, static_cast<DDS::Int16>(value));
  case TK_INT32:
    return dest->set_int32_value(id, static_cast<DDS::Int32>(value));
  case TK_INT64:
    return dest->set_int64_value(id, value);
  }
  return DDS::RETCODE_BAD_PARAMETER;
}

DDS::UInt32 bound_total(DDS::TypeDescriptor_var descriptor)
{
  DDS::UInt32 total = 1;
  const DDS::BoundSeq& bounds = descriptor->bound();
  for (DDS::UInt32 i = 0; i < bounds.length(); ++i) {
    total *= bounds[i];
  }
  return total;
}

DDS::ReturnCode_t bitmask_bound(DDS::DynamicType_ptr type, DDS::TypeKind& bound_kind)
{
  const DDS::TypeKind kind = type->get_kind();
  if (kind != TK_BITMASK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: bitmask_bound: "
        "expected bitmask, got %C\n",
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
  if (bound_size >= 1 && bound_size <= 8) {
    bound_kind = TK_UINT8;
  } else if (bound_size >= 9 && bound_size <= 16) {
    bound_kind = TK_UINT16;
  } else if (bound_size >= 17 && bound_size <= 32) {
    bound_kind = TK_UINT32;
  } else if (bound_size >= 33 && bound_size <= 64) {
    bound_kind = TK_UINT64;
  } else {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: bitmask_bound: "
        "Got unexpected bound size %B\n",
        bound_size));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t get_bitmask_value(
  DDS::UInt64& value, DDS::DynamicType_ptr type, DDS::DynamicData_ptr src, DDS::MemberId id)
{
  DDS::TypeKind bound_kind;
  const DDS::ReturnCode_t rc = bitmask_bound(type, bound_kind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return get_uint_value(value, src, id, bound_kind);
}

DDS::ReturnCode_t enum_bound(DDS::DynamicType_ptr enum_type, DDS::TypeKind& bound_kind)
{
  const DDS::TypeKind kind = enum_type->get_kind();
  if (kind != TK_ENUM) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: enum_bound: "
        "expected enum, got %C\n",
        typekind_to_string(kind)));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::TypeDescriptor_var td;
  const DDS::ReturnCode_t rc = enum_type->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  const size_t bound_size = td->bound()[0];
  if (bound_size >= 1 && bound_size <= 8) {
    bound_kind = TK_INT8;
  } else if (bound_size >= 9 && bound_size <= 16) {
    bound_kind = TK_INT16;
  } else if (bound_size >= 17 && bound_size <= 32) {
    bound_kind = TK_INT32;
  } else {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: enum_bound: "
        "Got unexpected bound size %B\n",
        bound_size));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t get_enum_value(
  DDS::Int32& value, DDS::DynamicType_ptr enum_type, DDS::DynamicData_ptr src, DDS::MemberId id)
{
  DDS::TypeKind bound_kind;
  DDS::ReturnCode_t rc = enum_bound(enum_type, bound_kind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  DDS::Int64 v = 0;
  rc = get_int_value(v, src, id, bound_kind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  value = static_cast<DDS::Int32>(v);
  return rc;
}

DDS::ReturnCode_t get_enumerator_name(
  DDS::String8_var& name, DDS::Int32 value, DDS::DynamicType_ptr type)
{
  DDS::DynamicTypeMember_var dtm;
  DDS::ReturnCode_t rc = type->get_member(dtm, static_cast<DDS::MemberId>(value));
  if (rc != DDS::RETCODE_OK) {
     return rc;
   }

  DDS::MemberDescriptor_var md;
  rc = dtm->get_descriptor(md);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  name = md->name();
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t get_enumerator_value(
  DDS::Int32& value, const char* name, DDS::DynamicType_ptr type)
{
  DDS::DynamicTypeMember_var dtm;
  DDS::ReturnCode_t rc = type->get_member_by_name(dtm, name);
  if (rc != DDS::RETCODE_OK) {
     return rc;
   }

  DDS::MemberDescriptor_var md;
  rc = dtm->get_descriptor(md);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }

  value = static_cast<DDS::Int32>(md->id());
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr enum_type, DDS::DynamicData_ptr dest, DDS::MemberId id, DDS::Int32 value)
{
  DDS::TypeKind bound_kind;
  const DDS::ReturnCode_t rc = enum_bound(enum_type, bound_kind);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_int_value(dest, id, bound_kind, value);
}

DDS::ReturnCode_t set_enum_value(
  DDS::DynamicType_ptr type, DDS::DynamicData_ptr dest, DDS::MemberId id, const char* enumeral_name)
{
  DDS::DynamicTypeMember_var dtm;
  DDS::ReturnCode_t rc = type->get_member_by_name(dtm, enumeral_name);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: set_enum_value: "
        "No such enumeral named \"%C\"\n",
        enumeral_name));
    }
    return rc;
  }
  DDS::MemberDescriptor_var md;
  rc = dtm->get_descriptor(md);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return set_enum_value(type, dest, id, md->id());
}

DDS::ReturnCode_t copy_member(
  DDS::DynamicData_ptr dest, DDS::MemberId dest_id,
  DDS::DynamicData_ptr src, DDS::MemberId src_id)
{
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  if (dest == src) {
    return rc;
  }

  const DDS::DynamicType_var dest_type = dest->type();
  DDS::DynamicType_var dest_member_type;
  if (dest_id != MEMBER_ID_INVALID) {
    rc = get_member_type(dest_member_type, dest_type, dest_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  } else {
    dest_member_type = get_base_type(dest_type);
  }
  const DDS::TypeKind dest_member_tk = dest_member_type->get_kind();

  const DDS::DynamicType_var src_type = src->type();
  DDS::DynamicType_var src_member_type;
  if (src_id != MEMBER_ID_INVALID) {
    rc = get_member_type(src_member_type, src_type, src_id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
  } else {
    src_member_type = get_base_type(src_type);
  }
  const DDS::TypeKind src_member_tk = src_member_type->get_kind();

  const CORBA::String_var src_type_name = src_type->get_name();
  const CORBA::String_var dest_type_name = dest_type->get_name();
  if (DCPS::DCPS_debug_level >= 8) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) copy_member(DynamicData): "
      "type %C from %C id %u to %C id %u\n",
      typekind_to_string(src_member_tk), src_type_name.in(), src_id, dest_type_name.in(), dest_id));
  }

  if (src_member_tk != dest_member_tk) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy_member(DynamicData): "
        "Can not copy member type %C id %u to type %C id %u\n",
        typekind_to_string(src_member_tk), src_id, typekind_to_string(dest_member_tk), dest_id));
    }
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t get_rc = DDS::RETCODE_OK;
  DDS::ReturnCode_t set_rc = DDS::RETCODE_OK;
  switch (src_member_tk) {
  case TK_BOOLEAN:
    {
      DDS::Boolean value = false;
      get_rc = src->get_boolean_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_boolean_value(dest_id, value);
      }
    }
    break;

  case TK_BYTE:
    {
      DDS::Byte value;
      get_rc = src->get_byte_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_byte_value(dest_id, value);
      }
    }
    break;

  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
    {
      DDS::Int64 value;
      get_rc = get_int_value(value, src, src_id, src_member_tk);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = set_int_value(dest, dest_id, dest_member_tk, value);
      }
    }
    break;

  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
    {
      DDS::UInt64 value;
      get_rc = get_uint_value(value, src, src_id, src_member_tk);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = set_uint_value(dest, dest_id, dest_member_tk, value);
      }
    }
    break;

  case TK_FLOAT32:
    {
      DDS::Float32 value;
      get_rc = src->get_float32_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_float32_value(dest_id, value);
      }
    }
    break;

  case TK_FLOAT64:
    {
      DDS::Float64 value;
      get_rc = src->get_float64_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_float64_value(dest_id, value);
      }
    }
    break;

  case TK_FLOAT128:
    {
      DDS::Float128 value;
      get_rc = src->get_float128_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_float128_value(dest_id, value);
      }
    }
    break;

  case TK_CHAR8:
    {
      DDS::Char8 value;
      get_rc = src->get_char8_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_char8_value(dest_id, value);
      }
    }
    break;

  case TK_CHAR16:
    {
      DDS::Char16 value;
      get_rc = src->get_char16_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_char16_value(dest_id, value);
      }
    }
    break;

  case TK_STRING8:
    {
      CORBA::String_var value;
      get_rc = src->get_string_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_string_value(dest_id, value);
      }
    }
    break;

  case TK_STRING16:
    {
      CORBA::WString_var value;
      get_rc = src->get_wstring_value(value, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = dest->set_wstring_value(dest_id, value);
      }
    }
    break;

  case TK_ENUM:
    {
      DDS::Int32 value;
      get_rc = get_enum_value(value, src_member_type, src, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        set_rc = set_enum_value(dest_member_type, dest, dest_id, value);
      }
    }
    break;

  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_STRUCTURE:
  case TK_UNION:
    {
      DDS::DynamicData_var subsrc;
      get_rc = src->get_complex_value(subsrc, src_id);
      if (get_rc == DDS::RETCODE_OK) {
        DDS::DynamicType_var base_dest_type = get_base_type(dest_type);
        const DDS::TypeKind base_dest_tk = base_dest_type->get_kind();
        const DDS::UInt32 dest_count = dest->get_item_count();
        bool dest_member_optional = false;
        if (base_dest_tk == TK_STRUCTURE || base_dest_tk == TK_UNION) {
          DDS::DynamicTypeMember_var dest_dtm;
          rc = dest_type->get_member(dest_dtm, dest_id);
          if (rc != DDS::RETCODE_OK) {
            return rc;
          }
          DDS::MemberDescriptor_var dest_md;
          rc = dest_dtm->get_descriptor(dest_md);
          if (rc != DDS::RETCODE_OK) {
            return rc;
          }
          dest_member_optional = dest_md->is_optional();
        }

        // TODO: The sequence index check assumes that ID is mapped exactly to index.
        // This is an internal detail and should be removed.
        if ((base_dest_tk == TK_SEQUENCE && dest_id == dest_count) ||
            base_dest_tk == TK_UNION || dest_member_optional) {
          DDS::DynamicData_var tmp = new DynamicDataImpl(dest_member_type);
          set_rc = dest->set_complex_value(dest_id, tmp);
        }
        if (set_rc == DDS::RETCODE_OK) {
          DDS::DynamicData_var subdest;
          get_rc = dest->get_complex_value(subdest, dest_id);
          if (get_rc == DDS::RETCODE_OK) {
            set_rc = copy(subdest, subsrc);
          }
        }
      }
    }
    break;

  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
        "member has unexpected TypeKind %C\n", typekind_to_string(src_member_tk)));
    }
    get_rc = DDS::RETCODE_UNSUPPORTED;
  }

  if (get_rc == DDS::RETCODE_NO_DATA) {
    if (DCPS::DCPS_debug_level >= 8) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) copy(DynamicData): "
        "Did not copy member type %C from %C id %u to %C id %u: get returned %C\n",
        typekind_to_string(src_member_tk), src_type_name.in(), src_id, dest_type_name.in(), dest_id,
        retcode_to_string(get_rc)));
    }
    return DDS::RETCODE_OK;
  }

  if (get_rc != DDS::RETCODE_OK || set_rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      const DDS::TypeKind tk = src_type->get_kind();
      if (tk == TK_STRUCTURE || tk == TK_UNION) {
        CORBA::String_var src_member_name;
        DDS::MemberDescriptor_var src_md;
        DDS::ReturnCode_t rc = src->get_descriptor(src_md, src_id);
        if (rc == DDS::RETCODE_OK) {
          src_member_name = src_md->name();
        }
        CORBA::String_var dest_member_name;
        DDS::MemberDescriptor_var dest_md;
        rc = dest->get_descriptor(dest_md, dest_id);
        if (rc == DDS::RETCODE_OK) {
          dest_member_name = dest_md->name();
        }
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "Could not copy member type %C from %C.%C id %u to %C.%C id %u: get: %C set: %C\n",
          typekind_to_string(src_member_tk),
          src_type_name.in(), src_member_name.in() ? src_member_name.in() : "?", src_id,
          dest_type_name.in(), dest_member_name.in() ? dest_member_name.in() : "?", dest_id,
          retcode_to_string(get_rc), retcode_to_string(set_rc)));
      } else {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "Could not copy member type %C from %C id %u to %C id %u: get: %C set: %C\n",
          typekind_to_string(src_member_tk), src_type_name.in(), src_id, dest_type_name.in(), dest_id,
          retcode_to_string(get_rc), retcode_to_string(set_rc)));
      }
    }
    rc = get_rc != DDS::RETCODE_OK ? get_rc : set_rc;
  }

  return rc;
}

DDS::ReturnCode_t copy(DDS::DynamicData_ptr dest, DDS::DynamicData_ptr src)
{
  if (dest == src) {
    return DDS::RETCODE_OK;
  }

  const DDS::DynamicType_var dest_type = dest->type();
  const DDS::DynamicType_var actual_dest_type = get_base_type(dest_type);
  const DDS::TypeKind dest_tk = actual_dest_type->get_kind();

  const DDS::DynamicType_var src_type = src->type();
  const DDS::DynamicType_var actual_src_type = get_base_type(src_type);
  const DDS::TypeKind src_tk = actual_src_type->get_kind();

  if (src_tk != dest_tk) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: copy(DynamicData): "
        "Can not copy type %C to type %C\n",
        typekind_to_string(src_tk), typekind_to_string(dest_tk)));
    }
    return DDS::RETCODE_OK;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (src_tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_STRING8:
  case TK_STRING16:
  case TK_ENUM:
    return copy_member(dest, MEMBER_ID_INVALID, src, MEMBER_ID_INVALID);

  case TK_UNION:
    {
      if (src->get_item_count() == 2) {
        const DDS::MemberId id = src->get_member_id_at_index(1);
        rc = copy_member(dest, id, src, id);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
              "Couldn't set union branch: %C\n", retcode_to_string(rc)));
          }
          return rc;
        }
      }
      rc = copy_member(dest, DISCRIMINATOR_ID, src, DISCRIMINATOR_ID);
      if (rc != DDS::RETCODE_OK && log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "Couldn't set union disciminator: %C\n", retcode_to_string(rc)));
      }
      return rc;
    }

  case TK_STRUCTURE:
    {
      DDS::DynamicTypeMembersById_var src_members_var;
      rc = actual_src_type->get_all_members(src_members_var);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      DynamicTypeMembersByIdImpl* src_members =
        dynamic_cast<DynamicTypeMembersByIdImpl*>(src_members_var.in());

      DDS::DynamicTypeMembersById_var dest_members_var;
      rc = actual_dest_type->get_all_members(dest_members_var);
      if (rc != DDS::RETCODE_OK) {
        return rc;
      }
      DynamicTypeMembersByIdImpl* dest_members =
        dynamic_cast<DynamicTypeMembersByIdImpl*>(dest_members_var.in());

      for (DynamicTypeMembersByIdImpl::const_iterator src_it = src_members->begin();
           src_it != src_members->end(); ++src_it) {
        const DDS::MemberId id = src_it->first;
        const DynamicTypeMembersByIdImpl::const_iterator dest_it = dest_members->find(id);
        if (dest_it == dest_members->end()) {
          continue;
        }

        const DDS::ReturnCode_t this_rc = copy_member(dest, id, src, id);
        if (this_rc != DDS::RETCODE_OK && this_rc != DDS::RETCODE_NO_DATA && rc == DDS::RETCODE_OK) {
          rc = this_rc;
        }
      }
    }
    break;

  case TK_SEQUENCE:
  case TK_ARRAY:
    {
      const DDS::UInt32 count = src->get_item_count();
      for (DDS::UInt32 i = 0; i < count; ++i) {
        const DDS::ReturnCode_t this_rc = copy_member(
          dest, dest->get_member_id_at_index(i),
          src, src->get_member_id_at_index(i));
        if (this_rc != DDS::RETCODE_OK && rc == DDS::RETCODE_OK) {
          rc = this_rc;
        }
      }
    }
    break;

  case TK_MAP:
  case TK_BITSET:
  case TK_ALIAS:
  case TK_ANNOTATION:
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
        "member has unexpected TypeKind %C\n", typekind_to_string(src_tk)));
    }
    rc = DDS::RETCODE_UNSUPPORTED;
  }

  return rc;
}

DDS::ReturnCode_t get_selected_union_branch(
  DDS::DynamicType_var union_type, DDS::Int32 disc,
  bool& found_selected_member, DDS::MemberDescriptor_var& selected_md)
{
  found_selected_member = false;
  bool has_default = false;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  DDS::MemberDescriptor_var default_md;
  for (DDS::UInt32 i = 0; i < union_type->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    rc = union_type->get_member_by_index(dtm, i);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    if (dtm->get_id() == DISCRIMINATOR_ID) {
      continue;
    }
    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    bool found_matched_label = false;
    const DDS::UnionCaseLabelSeq labels = md->label();
    for (DDS::UInt32 j = 0; !found_matched_label && j < labels.length(); ++j) {
      if (disc == labels[j]) {
        found_matched_label = true;
      }
    }
    if (found_matched_label) {
      selected_md = md;
      found_selected_member = true;
      break;
    }
    if (md->is_default_label()) {
      default_md = md;
      has_default = true;
    }
  }
  if (!found_selected_member && has_default) {
    selected_md = default_md;
    found_selected_member = true;
  }
  return rc;
}

bool has_explicit_keys(DDS::DynamicType* dt)
{
  // see dds_generator.h struct_has_explicit_keys() in opendds_idl
  DDS::TypeDescriptor_var type_descriptor;
  DDS::ReturnCode_t ret = dt->get_descriptor(type_descriptor);
  if (ret != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType* const base = type_descriptor->base_type();
  if (base && has_explicit_keys(base)) {
    return true;
  }

  for (ACE_CDR::ULong i = 0; i < dt->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var member;
    ret = dt->get_member_by_index(member, i);
    if (ret != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var descriptor;
    ret = member->get_descriptor(descriptor);
    if (ret != DDS::RETCODE_OK) {
      return false;
    }
    if (descriptor->is_key()) {
      return true;
    }
  }
  return false;
}

DDS::ReturnCode_t flat_index(CORBA::ULong& flat_idx, const DDS::BoundSeq& idx_vec,
                             const DDS::BoundSeq& dims)
{
  if (idx_vec.length() != dims.length()) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: flat_index: Number of dimensions (%u) != "
                 " size of the index vector (%u)\n", dims.length(), idx_vec.length()));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  CORBA::ULong ret_val = 0;
  CORBA::ULong factor = 1;
  for (CORBA::ULong i = dims.length() - 1; i > 0; --i) {
    const CORBA::ULong dim = dims[i];
    if (idx_vec[i] >= dim) {
      if (DCPS::log_level >= DCPS::LogLevel::Notice) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: flat_index: %u-th index (%u) is invalid for"
                   " the %u-th dimension (%u)", i, idx_vec[i], i, dim));
      }
      return DDS::RETCODE_BAD_PARAMETER;
    }
    ret_val += factor * idx_vec[i];
    factor *= dim;
  }
  flat_idx = ret_val + factor * idx_vec[0];
  return DDS::RETCODE_OK;
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
