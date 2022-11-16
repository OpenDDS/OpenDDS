/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataBase.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

DynamicDataBase::DynamicDataBase() {}

DynamicDataBase::DynamicDataBase(DDS::DynamicType_ptr type)
  : type_(get_base_type(type))
{}

DDS::ReturnCode_t DynamicDataBase::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  DDS::DynamicTypeMember_var dtm;
  if (type_->get_member(dtm, id) != DDS::RETCODE_OK) {
    return DDS::RETCODE_ERROR;
  }
  return dtm->get_descriptor(value);
}

DDS::MemberId DynamicDataBase::get_member_id_by_name(const char* name)
{
  const TypeKind tk = type_->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT16:
  case TK_INT32:
  case TK_INT64:
  case TK_UINT16:
  case TK_UINT32:
  case TK_UINT64:
  case TK_FLOAT32:
  case TK_FLOAT64:
  case TK_FLOAT128:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_CHAR16:
  case TK_ENUM:
    return MEMBER_ID_INVALID;
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_ARRAY:
    // Elements of string, sequence, array must be accessed by index.
    return MEMBER_ID_INVALID;
  case TK_MAP:
    // Values in map can be accessed by strings which is converted from map keys.
    // But need to find out how this conversion works. In the meantime, only allow
    // accessing map using index.
    return MEMBER_ID_INVALID;
  case TK_BITMASK:
  case TK_STRUCTURE:
  case TK_UNION: {
    DDS::DynamicTypeMember_var member;
    if (type_->get_member_by_name(member, name) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }
    DDS::MemberDescriptor_var descriptor;
    if (member->get_descriptor(descriptor) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }
    if (tk == TK_BITMASK) {
      // Bitmask's flags don't have ID, so use index instead.
      return descriptor->index();
    } else {
      return descriptor->id();
    }
  }
  }
  if (DCPS::log_level >= DCPS::LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::get_member_id_by_name:"
               " Calling on an unexpected type %C\n", typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

bool DynamicDataBase::is_type_supported(TypeKind tk, const char* func_name)
{
  if (!is_primitive(tk) && tk != TK_STRING8 && tk != TK_STRING16) {
    if (DCPS::log_level >= DCPS::LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::is_type_supported:"
                 " Called function %C on an unsupported type (%C)\n",
                 func_name, typekind_to_string(tk)));
    }
    return false;
  }
  return true;
}

bool DynamicDataBase::is_primitive(TypeKind tk) const
{
  switch (tk) {
  case TK_BOOLEAN:
  case TK_BYTE:
  case TK_INT8:
  case TK_UINT8:
  case TK_CHAR8:
  case TK_INT16:
  case TK_UINT16:
  case TK_CHAR16:
  case TK_INT32:
  case TK_UINT32:
  case TK_FLOAT32:
  case TK_INT64:
  case TK_UINT64:
  case TK_FLOAT64:
  case TK_FLOAT128:
    return true;
  default:
    return false;
  }
}

bool DynamicDataBase::get_index_from_id(DDS::MemberId id, ACE_CDR::ULong& index,
                                        ACE_CDR::ULong bound) const
{
  // The mapping from id to index must be consistent with get_member_id_at_index
  // for these types. In particular, index and id are equal given that it doesn't
  // go out of bound.
  switch (type_->get_kind()) {
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_MAP:
    if (bound == 0 || id < bound) {
      index = id;
      return true;
    }
    break;
  case TK_BITMASK:
  case TK_ARRAY:
    if (id < bound) {
      index = id;
      return true;
    }
  }
  return false;
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
