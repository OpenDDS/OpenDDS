/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicDataBase.h"

#  include "Utils.h"
#  include "DynamicDataFactory.h"

#  include <dds/DCPS/debug.h>
#  include <dds/DCPS/ValueHelper.h>
#  include <dds/DCPS/DCPS_Utils.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

using DCPS::LogLevel;
using DCPS::log_level;
using DCPS::retcode_to_string;

namespace {
  DDS::TypeDescriptor_var get_type_desc(DDS::DynamicType_ptr type)
  {
    if (!type) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataBase: "
                   "Passed null DynamicType pointer\n"));
      }
      return DDS::TypeDescriptor_var();
    }
    DDS::TypeDescriptor_var td;
    const DDS::ReturnCode_t rc = type->get_descriptor(td);
    if (rc != DDS::RETCODE_OK && log_level >= LogLevel::Warning) {
      const CORBA::String_var name = type->get_name();
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataBase: "
        "Failed to get type descriptor for %C\n", name.in()));
    }
    return td;
  }
}

DynamicDataBase::DynamicDataBase()
{
}

DynamicDataBase::DynamicDataBase(DDS::DynamicType_ptr type)
  : type_(get_base_type(type))
  , type_desc_(get_type_desc(type_))
{}

DDS::DynamicData* DynamicDataBase::interface_from_this() const
{
  // Operations defined in IDL interfaces don't use pointer-to-const
  // parameter types.
  return const_cast<DynamicDataBase*>(this);
}

DDS::ReturnCode_t DynamicDataBase::get_descriptor(DDS::MemberDescriptor*& value, MemberId id)
{
  DDS::DynamicTypeMember_var dtm;
  const DDS::ReturnCode_t rc = type_->get_member(dtm, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  return dtm->get_descriptor(value);
}

DDS::ReturnCode_t DynamicDataBase::set_descriptor(
  DDS::MemberId /*id*/, DDS::MemberDescriptor* /*value*/)
{
  return unsupported_method("DynamicData::set_descriptor");
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
  if (log_level >= LogLevel::Notice) {
    ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::get_member_id_by_name:"
               " Calling on an unexpected type %C\n", typekind_to_string(tk)));
  }
  return MEMBER_ID_INVALID;
}

bool DynamicDataBase::is_type_supported(TypeKind tk, const char* func_name)
{
  if (!is_basic(tk)) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::is_type_supported:"
                 " Called function %C on an unsupported type (%C)\n",
                 func_name, typekind_to_string(tk)));
    }
    return false;
  }
  return true;
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

bool DynamicDataBase::enum_string_helper(char*& strInOut, MemberId id)
{
  DDS::DynamicType_var mtype;
  DDS::ReturnCode_t rc = get_member_type(mtype, type_, id);
  if (rc != DDS::RETCODE_OK || mtype->get_kind() != TK_ENUM) {
    return false;
  }
  DDS::Int32 valAsInt;
  rc = get_enum_value(valAsInt, mtype, this, id);
  if (rc != DDS::RETCODE_OK) {
    return false;
  }
  DDS::String8_var valAsStr;
  rc = get_enumerator_name(valAsStr, valAsInt, mtype);
  if (rc != DDS::RETCODE_OK) {
    return false;
  }
  CORBA::string_free(strInOut);
  strInOut = valAsStr._retn();
  return true;
}

DDS::ReturnCode_t DynamicDataBase::check_member(
  DDS::MemberDescriptor_var& md, DDS::DynamicType_var& type,
  const char* method, const char* action, DDS::MemberId id, DDS::TypeKind tk)
{
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (type_->get_kind()) {
  case TK_STRING8:
  case TK_STRING16:
  case TK_SEQUENCE:
  case TK_ARRAY:
  case TK_MAP:
    type = get_base_type(type_desc_->element_type());
    break;
  case TK_BITMASK:
  case TK_STRUCTURE:
  case TK_UNION:
    rc = get_descriptor(md, id);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    type = get_base_type(md->type());
    if (!type) {
      return DDS::RETCODE_ERROR;
    }
    break;
  default:
    return DDS::RETCODE_BAD_PARAMETER;
  }

  const TypeKind type_kind = type->get_kind();
  TypeKind cmp_type_kind = type_kind;
  switch (type_kind) {
  case TK_ENUM:
    rc = enum_bound(type, cmp_type_kind);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    break;
  case TK_BITMASK:
    rc = bitmask_bound(type, cmp_type_kind);
    if (rc != DDS::RETCODE_OK) {
      return rc;
    }
    break;
  }

  bool invalid_tk = true;
  if (is_basic(cmp_type_kind)) {
    invalid_tk = cmp_type_kind != tk;
  } else if (tk == TK_NONE) {
    invalid_tk = !is_complex(type_kind);
  }
  if (invalid_tk) {
    if (log_level >= LogLevel::Notice) {
      const CORBA::String_var member_name = md->name();
      const CORBA::String_var type_name = type_->get_name();
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: %C: "
        "trying to %C %C.%C id %u kind %C (%C) as an invalid kind %C\n",
        method, action, type_name.in(), member_name.in(), id,
        typekind_to_string(cmp_type_kind), typekind_to_string(type_kind),
        typekind_to_string(tk)));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return DDS::RETCODE_OK;
}

DDS::MemberId DynamicDataBase::get_union_default_member(DDS::DynamicType* type)
{
  //FUTURE: non-zero defaults for union discriminators are not currently represented
  // in the MemberDescriptors created by converting CompleteTypeObject to DynamicType.
  // When they are supported, change disc_default below to a value derived from the
  // 'type' parameter.  Note that 64-bit discriminators are not represented in TypeObject.
  static const ACE_CDR::Long disc_default = 0;
  DDS::MemberId default_branch = MEMBER_ID_INVALID;
  const ACE_CDR::ULong members = type->get_member_count();
  for (ACE_CDR::ULong i = 0; i < members; ++i) {
    DDS::DynamicTypeMember_var member;
    if (type->get_member_by_index(member, i) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }
    if (member->get_id() == DISCRIMINATOR_ID) {
      continue;
    }
    DDS::MemberDescriptor_var mdesc;
    if (member->get_descriptor(mdesc) != DDS::RETCODE_OK) {
      return MEMBER_ID_INVALID;
    }
    if (mdesc->is_default_label()) {
      default_branch = mdesc->id();
    } else {
      const DDS::UnionCaseLabelSeq& lseq = mdesc->label();
      for (ACE_CDR::ULong lbl = 0; lbl < lseq.length(); ++lbl) {
        if (lseq[lbl] == disc_default) {
          return mdesc->id();
        }
      }
    }
  }
  // Reaching this point means that there is no explicit label for the default
  // value of the discriminator.  If there is a default branch, its member is
  // selected.  Otherwise the 'MEMBER_ID_INVALID' constant is returned.
  return default_branch;
}

DDS::ReturnCode_t DynamicDataBase::get_selected_union_branch(
  bool& found_selected_member, DDS::MemberDescriptor_var& selected_md)
{
  // TODO: Support UInt64 and Int64 (https://issues.omg.org/issues/DDSXTY14-36)
  DDS::Int64 i64_disc;
  DDS::ReturnCode_t rc = get_int64_value(i64_disc, DISCRIMINATOR_ID);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  if (i64_disc < ACE_INT32_MIN || i64_disc > ACE_INT32_MAX) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::get_selected_union_branch: "
        "union discriminator can't fit in int32: %q\n", i64_disc));
    }
    return DDS::RETCODE_ERROR;
  }
  return XTypes::get_selected_union_branch(type_, static_cast<DDS::Int32>(i64_disc),
                                           found_selected_member, selected_md);
}

bool DynamicDataBase::discriminator_selects_no_member(DDS::Int32 disc) const
{
  bool found_selected_member;
  DDS::MemberDescriptor_var selected_md;
  const DDS::ReturnCode_t rc = XTypes::get_selected_union_branch(type_, disc, found_selected_member, selected_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: DynamicDataBase::discriminator_selects_no_member: "
        "get_selected_union_branch failed: %C\n", retcode_to_string(rc)));
    }
    return false;
  }
  return !found_selected_member;
}

DDS::ReturnCode_t DynamicDataBase::unsupported_method(const char* method_name, bool warning) const
{
  if (log_level >= (warning ? LogLevel::Warning : LogLevel::Notice)) {
    ACE_ERROR((warning ? LM_WARNING : LM_NOTICE, "(%P|%t) %C: %C: not implemented\n",
      warning ? "WARNING" : "NOTICE", method_name));
  }
  return DDS::RETCODE_UNSUPPORTED;
}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
namespace {
  template <typename T>
  DDS::ReturnCode_t get_some_value(DCPS::Value& value, DDS::MemberId id, DDS::DynamicData& dyn,
                                   DDS::ReturnCode_t (DDS::DynamicData::* pmf)(T&, DDS::MemberId))
  {
    T v;
    const DDS::ReturnCode_t ret = (dyn.*pmf)(v, id);
    if (ret == DDS::RETCODE_OK) {
      value = v;
    }
    return ret;
  }

  DDS::ReturnCode_t get_some_value(DCPS::Value& value, DDS::MemberId id, DDS::DynamicData& dyn,
                                   DDS::ReturnCode_t (DDS::DynamicData::* pmf)(char*&, DDS::MemberId))
  {
    CORBA::String_var v;
    const DDS::ReturnCode_t ret = (dyn.*pmf)(v, id);
    if (ret == DDS::RETCODE_OK) {
      value = v.in();
    }
    return ret;
  }

  DDS::ReturnCode_t get_some_value(DCPS::Value& value, DDS::MemberId id, DDS::DynamicData& dyn,
                                   DDS::ReturnCode_t (DDS::DynamicData::* pmf)(ACE_CDR::WChar*&, DDS::MemberId))
  {
    CORBA::WString_var v;
    const DDS::ReturnCode_t ret = (dyn.*pmf)(v, id);
    if (ret == DDS::RETCODE_OK) {
      value = v.in();
    }
    return ret;
  }
}

DDS::ReturnCode_t DynamicDataBase::get_simple_value(DCPS::Value& value, DDS::MemberId id)
{
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = get_member_type(member_type, type_, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  const TypeKind member_kind = member_type->get_kind();
  switch (member_kind) {
  case TK_BOOLEAN:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_boolean_value);
  case TK_BYTE:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_byte_value);
  case TK_INT8:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_int8_value);
  case TK_INT16:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_int16_value);
  case TK_INT32:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_int32_value);
  case TK_INT64:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_int64_value);
  case TK_UINT8:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_uint8_value);
  case TK_UINT16:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_uint16_value);
  case TK_UINT32:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_uint32_value);
  case TK_UINT64:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_uint64_value);
  case TK_FLOAT32:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_float32_value);
  case TK_FLOAT64:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_float64_value);
  case TK_FLOAT128:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_float128_value);
  case TK_CHAR8:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_char8_value);
  case TK_CHAR16:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_char16_value);
  case TK_ENUM:
  case TK_STRING8:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_string_value);
  case TK_STRING16:
    return get_some_value(value, id, *this, &DDS::DynamicData::get_wstring_value);
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::get_simple_value: "
                 "Member type %C is not supported by DCPS::Value\n",
                 typekind_to_string(member_kind)));
    }
  }
  return DDS::RETCODE_ERROR;
}
#endif

DDS::DynamicType_ptr DynamicDataBase::type()
{
  return DDS::DynamicType::_duplicate(type_);
}

DDS::Boolean DynamicDataBase::equals(DDS::DynamicData_ptr /*other*/)
{
  unsupported_method("DynamicDataBase::equals", true);
  return false;
}

DDS::DynamicData_ptr DynamicDataBase::loan_value(DDS::MemberId /*id*/)
{
  unsupported_method("DynamicDataBase::loan_value");
  return 0;
}

DDS::ReturnCode_t DynamicDataBase::return_loaned_value(DDS::DynamicData_ptr /*other*/)
{
  return unsupported_method("DynamicDataBase::return_loaned_value");
}

DDS::DynamicData_ptr DynamicDataBase::clone()
{
  DDS::DynamicData_var new_copy = DDS::DynamicDataFactory::get_instance()->create_data(type_);
  if (!new_copy || copy(new_copy, this) != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::clone: Failed to create a copy\n"));
    }
    return 0;
  }
  return new_copy._retn();
}

namespace {
  DDS::ReturnCode_t invalid_cast(const char* method, TypeKind to, TypeKind from)
  {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: DynamicDataBase::%C: Can't cast %C to %C\n",
        method, typekind_to_string(from), typekind_to_string(to)));
    }
    return DDS::RETCODE_ILLEGAL_OPERATION;
  }
}

DDS::ReturnCode_t DynamicDataBase::get_int64_value(DDS::Int64& value, DDS::MemberId id)
{
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = get_member_type(member_type, type_, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  const TypeKind tk = member_type->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
    {
      DDS::Boolean tmp;
      rc = get_boolean_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  case TK_BYTE:
    {
      DDS::Byte tmp;
      rc = get_byte_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
    return get_int_value(value, this, id, tk);
  case TK_INT64:
    return get_int64_value_impl(value, id);
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
    {
      DDS::UInt64 tmp;
      rc = get_uint_value(tmp, this, id, tk);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  case TK_CHAR8:
    {
      DDS::Char8 tmp;
      rc = get_char8_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  case TK_CHAR16:
    {
      DDS::Char16 tmp;
      rc = get_char16_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  case TK_ENUM:
    {
      DDS::Int32 tmp;
      rc = get_enum_value(tmp, member_type, this, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::Int64>(tmp);
      }
      return rc;
    }
  default:
    return invalid_cast("get_int64_value", TK_INT64, tk);
  }
}

DDS::ReturnCode_t DynamicDataBase::get_uint64_value(DDS::UInt64& value, DDS::MemberId id)
{
  DDS::DynamicType_var member_type;
  DDS::ReturnCode_t rc = get_member_type(member_type, type_, id);
  if (rc != DDS::RETCODE_OK) {
    return rc;
  }
  const TypeKind tk = member_type->get_kind();
  switch (tk) {
  case TK_BOOLEAN:
    {
      DDS::Boolean tmp;
      rc = get_boolean_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = tmp ? 1 : 0;
      }
      return rc;
    }
  case TK_BYTE:
    {
      DDS::Byte tmp;
      rc = get_byte_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::UInt64>(tmp);
      }
      return rc;
    }
  case TK_INT8:
  case TK_INT16:
  case TK_INT32:
    {
      DDS::Int64 tmp;
      rc = get_int_value(tmp, this, id, tk);
      if (rc == DDS::RETCODE_OK) {
        value = static_cast<DDS::UInt64>(tmp);
      }
      return rc;
    }
  case TK_UINT8:
  case TK_UINT16:
  case TK_UINT32:
    return get_uint_value(value, this, id, tk);
  case TK_UINT64:
    return get_uint64_value_impl(value, id);
  case TK_CHAR8:
    {
      DDS::Char8 tmp;
      rc = get_char8_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = DCPS::char_value(tmp);
      }
      return rc;
    }
  case TK_CHAR16:
    {
      DDS::Char16 tmp;
      rc = get_char16_value(tmp, id);
      if (rc == DDS::RETCODE_OK) {
        value = DCPS::char_value(tmp);
      }
      return rc;
    }
  default:
    return invalid_cast("get_uint64_value", TK_UINT64, tk);
  }
}

} // namespace XTypes
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
