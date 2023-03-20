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

  bool sequence_like(DDS::TypeKind tk)
  {
    return tk == TK_ARRAY || tk == TK_SEQUENCE;
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
        DDS::Boolean a_value;
        a_rc = a_data->get_boolean_value(a_value, a_id);
        if (a_rc == DDS::RETCODE_OK) {
          DDS::Boolean b_value;
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
                const ACE_CDR::UInt32 a_count = a_value->get_item_count();
                const ACE_CDR::UInt32 b_count = b_value->get_item_count();
                const ACE_CDR::UInt32 count = std::min(a_count, b_count);
                for (ACE_CDR::UInt32 i = 0;
                    a_rc == DDS::RETCODE_OK && i < count && result == 0; ++i) {
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
  if (sequence_like(container_type->get_kind())) {
    DDS::TypeDescriptor_var td;
    DDS::ReturnCode_t rc = container_type->get_descriptor(td);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    member_type = get_base_type(td->element_type());
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

bool is_int(DDS::TypeKind tk)
{
  switch (tk) {
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
    return true;
  default:
    return false;
  }
}

bool is_uint(DDS::TypeKind tk)
{
  switch (tk) {
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
    return true;
  default:
    return false;
  }
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
  DDS::Int64 v;
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

DDS::ReturnCode_t copy(DDS::DynamicData_ptr dest, DDS::DynamicData_ptr src)
{
  if (dest == src) {
    return DDS::RETCODE_OK;
  }

  DDS::DynamicType_var src_type = src->type();
  DDS::DynamicTypeMembersById_var src_members_var;
  DDS::ReturnCode_t rc = src_type->get_all_members(src_members_var);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  DynamicTypeMembersByIdImpl* src_members =
    dynamic_cast<DynamicTypeMembersByIdImpl*>(src_members_var.in());

  DDS::DynamicType_var dest_type = dest->type();
  DDS::DynamicTypeMembersById_var dest_members_var;
  rc = dest_type->get_all_members(dest_members_var);
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

    DDS::MemberDescriptor_var src_md;
    rc = src_it->second->get_descriptor(src_md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicType_var src_mem_type = get_base_type(src_md->type());
    const DDS::TypeKind src_tk = src_mem_type->get_kind();

    DDS::MemberDescriptor_var dest_md;
    rc = dest_it->second->get_descriptor(dest_md);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    DDS::DynamicType_var dest_mem_type = get_base_type(dest_md->type());
    const DDS::TypeKind dest_tk = dest_mem_type->get_kind();

    if (src_tk != dest_tk) {
      continue;
    }

    DDS::ReturnCode_t get_rc = DDS::RETCODE_OK;
    DDS::ReturnCode_t set_rc = DDS::RETCODE_OK;

    switch (src_tk) {
    case TK_BOOLEAN:
      {
        DDS::Boolean value;
        get_rc = src->get_boolean_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_boolean_value(id, value);
        }
      }
      break;

    case TK_BYTE:
      {
        DDS::Byte value;
        get_rc = src->get_byte_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_byte_value(id, value);
        }
      }
      break;

    case TK_INT8:
    case TK_INT16:
    case TK_INT32:
    case TK_INT64:
      {
        DDS::Int64 value;
        get_rc = get_int_value(value, src, id, src_tk);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = set_int_value(dest, id, dest_tk, value);
        }
      }
      break;

    case TK_UINT8:
    case TK_UINT16:
    case TK_UINT32:
    case TK_UINT64:
      {
        DDS::UInt64 value;
        get_rc = get_uint_value(value, src, id, src_tk);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = set_uint_value(dest, id, dest_tk, value);
        }
      }
      break;

    case TK_FLOAT32:
      {
        DDS::Float32 value;
        get_rc = src->get_float32_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_float32_value(id, value);
        }
      }
      break;

    case TK_FLOAT64:
      {
        DDS::Float64 value;
        get_rc = src->get_float64_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_float64_value(id, value);
        }
      }
      break;

    case TK_FLOAT128:
      {
        DDS::Float128 value;
        get_rc = src->get_float128_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_float128_value(id, value);
        }
      }
      break;

    case TK_CHAR8:
      {
        DDS::Char8 value;
        get_rc = src->get_char8_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_char8_value(id, value);
        }
      }
      break;

    case TK_CHAR16:
      {
        DDS::Char16 value;
        get_rc = src->get_char16_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_char16_value(id, value);
        }
      }
      break;

    case TK_STRING8:
      {
        CORBA::String_var value;
        get_rc = src->get_string_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_string_value(id, value);
        }
      }
      break;

    case TK_STRING16:
      {
        CORBA::WString_var value;
        get_rc = src->get_wstring_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_wstring_value(id, value);
        }
      }
      break;

    case TK_ENUM:
      {
        DDS::Int32 value;
        get_rc = get_enum_value(value, src_mem_type, src, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = set_enum_value(dest_mem_type, dest, id, value);
        }
      }
      break;

    case TK_STRUCTURE:
    case TK_UNION:
    case TK_SEQUENCE:
    case TK_ARRAY:
    case TK_MAP:
    case TK_BITSET:
      {
        DDS::DynamicData_var value;
        get_rc = src->get_complex_value(value, id);
        if (get_rc == DDS::RETCODE_OK) {
          set_rc = dest->set_complex_value(id, value);
        }
      }
      break;

    case TK_ALIAS:
    case TK_ANNOTATION:
    default:
      if (DCPS::log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "member has unexpected TypeKind %C\n", typekind_to_string(src_tk)));
      }
      get_rc = DDS::RETCODE_BAD_PARAMETER;
    }

    if (get_rc != DDS::RETCODE_OK || set_rc != DDS::RETCODE_OK) {
      const CORBA::String_var src_type_name = src_type->get_name();
      const CORBA::String_var src_member_name = src_md->name();
      const CORBA::String_var dest_type_name = dest_type->get_name();
      const CORBA::String_var dest_member_name = dest_md->name();
      if (DCPS::log_level >= DCPS::LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: copy(DynamicData): "
          "Could not copy member type %C id %u from %C.%C to %C.%C: "
          "get: %C set: %C\n",
          typekind_to_string(src_tk), id,
          src_type_name.in(), src_member_name.in(),
          dest_type_name.in(), dest_member_name.in(),
          DCPS::retcode_to_string(get_rc), DCPS::retcode_to_string(set_rc)));
      }
    }
  }

  return DDS::RETCODE_OK;
}

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
