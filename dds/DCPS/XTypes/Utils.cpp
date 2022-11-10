/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "Utils.h"

#  include "DynamicDataImpl.h"

#  include <dds/DCPS/debug.h>

#  include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

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
        if (DCPS::log_level >= DCPS::LogLevel::Notice) {
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

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
