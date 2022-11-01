/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "Utils.h"

#  include "DynamicDataImpl.h"

#  include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

DDS::ReturnCode_t extensibility(DDS::DynamicType_ptr type, DCPS::Extensibility& ext)
{
  if (!type) {
    return DDS::RETCODE_BAD_PARAMETER;
  }

  DDS::DynamicType_var base_type = get_base_type(type);
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

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
