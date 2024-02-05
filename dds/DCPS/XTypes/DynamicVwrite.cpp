/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicVwrite.h"
#  include "Utils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool check_rc(DDS::ReturnCode_t rc, DDS::MemberId id, DDS::TypeKind tk, const char* fn_name)
{
  return XTypes::check_rc_from_get(rc, id, tk, fn_name, LogLevel::Warning);
}

void begin_member_helper(ValueWriter& vw, VWriterMemberParam* params,
                         DDS::ReturnCode_t rc, DDS::TypeKind containing_tk)
{
  if (containing_tk != XTypes::TK_STRUCTURE && containing_tk != XTypes::TK_UNION) {
    return;
  }
  if (rc == DDS::RETCODE_NO_DATA) {
    params->present = false;
  }
  if (containing_tk == XTypes::TK_STRUCTURE) {
    vw.begin_struct_member(*params);
  } else {
    vw.begin_union_member(*params);
  }
}

DDS::ReturnCode_t get_equivalent_kind(const DDS::DynamicType_var& type, XTypes::TypeKind& treat_as)
{
  const DDS::TypeKind tk = type->get_kind();
  treat_as = tk;

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  if (tk == XTypes::TK_ENUM) {
    rc = XTypes::enum_bound(type, treat_as);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: get_equivalent_kind: enum_bound failed (%C)\n",
                   retcode_to_string(rc)));
      }
      return rc;
    }
  } else if (tk == XTypes::TK_BITMASK) {
    rc = XTypes::bitmask_bound(type, treat_as);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: get_equivalent_kind: bitmask_bound failed (%C)\n",
                   retcode_to_string(rc)));
      }
      return rc;
    }
  }
  return DDS::RETCODE_OK;
}

DDS::ReturnCode_t vwrite_primitive_array_i(ValueWriter& vw, DDS::DynamicData_ptr value, DDS::MemberId id,
                                           XTypes::TypeKind elem_kind, XTypes::TypeKind orig_elem_kind, bool for_sequence)
{
  using namespace XTypes;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (elem_kind) {
  case TK_INT8: {
    DDS::Int8Seq val;
    rc = value->get_int8_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_int8_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_UINT8: {
    DDS::UInt8Seq val;
    rc = value->get_uint8_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_uint8_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_INT16: {
    DDS::Int16Seq val;
    rc = value->get_int16_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_int16_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_UINT16: {
    DDS::UInt16Seq val;
    rc = value->get_uint16_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_uint16_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_INT32: {
    DDS::Int32Seq val;
    rc = value->get_int32_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_int32_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_UINT32: {
    DDS::UInt32Seq val;
    rc = value->get_uint32_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_uint32_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_INT64: {
    DDS::Int64Seq val;
    rc = value->get_int64_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_int64_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_UINT64: {
    DDS::UInt64Seq val;
    rc = value->get_uint64_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_uint64_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_FLOAT32: {
    DDS::Float32Seq val;
    rc = value->get_float32_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_float32_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_FLOAT64: {
    DDS::Float64Seq val;
    rc = value->get_float64_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_float64_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_FLOAT128: {
    DDS::Float128Seq val;
    rc = value->get_float128_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_float128_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_CHAR8: {
    DDS::CharSeq val;
    rc = value->get_char8_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_char8_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    DDS::WcharSeq val;
    rc = value->get_char16_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_char16_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
#endif
  case TK_BYTE: {
    DDS::ByteSeq val;
    rc = value->get_byte_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_byte_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  case TK_BOOLEAN: {
    DDS::BooleanSeq val;
    rc = value->get_boolean_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (for_sequence) {
        vw.begin_sequence(orig_elem_kind, val.length());
      }
      vw.write_boolean_array(val.get_buffer(), val.length());
      if (for_sequence) {
        vw.end_sequence();
      }
    }
    break;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_primitive_array_i:"
                 " Expect a primitive type, receive %C\n", XTypes::typekind_to_string(elem_kind)));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return rc;
}

// Argument containing_tk and params only apply when this is a member of a struct or union,
// and is ignored when this is an element of a sequence or array.
void vwrite_item(ValueWriter& vw, DDS::DynamicData_ptr value, DDS::MemberId id,
                 const DDS::DynamicType_var& item_type, DDS::TypeKind containing_tk = XTypes::TK_NONE,
                 VWriterMemberParam* params = 0)
{
  using namespace OpenDDS::XTypes;
  const DDS::TypeKind item_tk = item_type->get_kind();
  DDS::TypeKind treat_as;
  if (get_equivalent_kind(item_type, treat_as) != DDS::RETCODE_OK) {
    return;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (treat_as) {
  case TK_INT8: {
    CORBA::Int8 val = 0;
    rc = value->get_int8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        write_enum(vw, item_type, val, treat_as);
      } else {
        vw.write_int8(val);
      }
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_UINT8: {
    CORBA::UInt8 val = 0;
    rc = value->get_uint8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_uint8(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_INT16: {
    CORBA::Short val = 0;
    rc = value->get_int16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        write_enum(vw, item_type, val, treat_as);
      } else {
        vw.write_int16(val);
      }
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_UINT16: {
    CORBA::UShort val = 0;
    rc = value->get_uint16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_uint16(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_INT32: {
    CORBA::Long val = 0;
    rc = value->get_int32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        write_enum(vw, item_type, val, treat_as);
      } else {
        vw.write_int32(val);
      }
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_UINT32: {
    CORBA::ULong val = 0;
    rc = value->get_uint32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_uint32(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_INT64: {
    CORBA::LongLong val = 0;
    rc =  value->get_int64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_int64(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_UINT64: {
    CORBA::ULongLong val = 0;
    rc = value->get_uint64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_uint64(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_FLOAT32: {
    CORBA::Float val = 0.0f;
    rc = value->get_float32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_float32(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_FLOAT64: {
    CORBA::Double val = 0.0;
    rc = value->get_float64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_float64(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_FLOAT128: {
    CORBA::LongDouble val;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(val, 0.0l);
    rc = value->get_float128_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_float128(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_CHAR8: {
    CORBA::Char val = '\0';
    rc = value->get_char8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_char8(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val = L'\0';
    rc = value->get_char16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_char16(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
#endif
  case TK_BYTE: {
    CORBA::Octet val = 0x00;
    rc = value->get_byte_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_byte(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = value->get_boolean_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_boolean(val);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  case TK_STRING8: {
    CORBA::String_var val;
    rc = value->get_string_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_string(val.in());
    } else {
      vw.write_absent_value();
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = value->get_wstring_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vw.write_wstring(val.in());
    } else {
      vw.write_absent_value();
    }
    break;
  }
#endif
  case TK_SEQUENCE: {
    DDS::TypeDescriptor_var seq_td;
    rc = item_type->get_descriptor(seq_td);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_item: get_descriptor for sequence failed (%C)\n",
                   retcode_to_string(rc)));
      }
      return;
    }
    DDS::DynamicType_var elem_type = get_base_type(seq_td->element_type());
    const TypeKind elem_kind = elem_type->get_kind();
    TypeKind treat_elem_as;
    if (get_equivalent_kind(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
      return;
    }
    // Try writing the whole primitive sequence. If fails, fall back to write element one by one.
    if (is_primitive(treat_elem_as) &&
        vwrite_primitive_array_i(vw, value, id, treat_elem_as, elem_kind, true) == DDS::RETCODE_OK) {
      break;
    }
  }
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY: {
    DDS::DynamicData_var member_data;
    rc = value->get_complex_value(member_data, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return;
    }
    begin_member_helper(vw, params, rc, containing_tk);
    if (rc == DDS::RETCODE_OK) {
      vwrite(vw, member_data);
    } else {
      vw.write_absent_value();
    }
    break;
  }
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_item: Unsupported type %C\n",
                 typekind_to_string(treat_as)));
    }
    return;
  }

  if (containing_tk == TK_STRUCTURE) {
    vw.end_struct_member();
  } else if (containing_tk == TK_UNION) {
    vw.end_union_member();
  }
}

void vwrite_member(ValueWriter&vw, DDS::DynamicData_ptr value,
                   const DDS::MemberDescriptor_var& md, DDS::TypeKind containing_tk)
{
  const DDS::MemberId id = md->id();
  const DDS::DynamicType_var member_type = XTypes::get_base_type(md->type());
  VWriterMemberParam params(id, md->is_must_understand() || md->is_key(), md->name(),
                            md->is_optional(), true);
  vwrite_item(vw, value, id, member_type, containing_tk, &params);
}

void vwrite_struct(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var type_desc;
  DDS::ReturnCode_t rc = dt->get_descriptor(type_desc);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_struct: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }

  const Extensibility extensibility = XTypes::dds_to_opendds_ext(type_desc->extensibility_kind());
  vw.begin_struct(extensibility);
  for (CORBA::ULong i = 0; i < dt->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    rc = dt->get_member_by_index(dtm, i);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_struct: get_member_by_index %u failed (%C)\n",
                   i, retcode_to_string(rc)));
      }
      return;
    }
    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_struct:"
                   " get_descriptor for member at index %u failed (%C)\n", i, retcode_to_string(rc)));
      }
      return;
    }
    vwrite_member(vw, value, md, XTypes::TK_STRUCTURE);
  }
  vw.end_struct();
}

void vwrite_discriminator(ValueWriter& vw, DDS::DynamicData_ptr value,
                          const DDS::MemberDescriptor_var& md, CORBA::Long& disc_val)
{
  using namespace OpenDDS::XTypes;
  const DDS::MemberId id = DISCRIMINATOR_ID;
  const DDS::DynamicType_var disc_type = get_base_type(md->type());
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  DDS::TypeKind treat_disc_as = disc_tk;

  DDS::ReturnCode_t rc;
  if (disc_tk == TK_ENUM) {
    rc = enum_bound(disc_type, treat_disc_as);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_discriminator: enum_bound failed (%C)\n",
                   retcode_to_string(rc)));
      }
      return;
    }
  }

  switch (treat_disc_as) {
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = value->get_boolean_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_boolean(val);
    break;
  }
  case TK_BYTE: {
    CORBA::Octet val = 0x00;
    rc = value->get_byte_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_byte(val);
    break;
  }
  case TK_CHAR8: {
    CORBA::Char val = '\0';
    rc = value->get_char8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_char8(val);
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val = L'\0';
    rc = value->get_char16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_char16(val);
    break;
  }
#endif
  case TK_INT8: {
    CORBA::Int8 val = 0;
    rc = value->get_int8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (disc_tk == TK_ENUM) {
      write_enum(vw, disc_type, val, treat_disc_as);
    } else {
      vw.write_int8(val);
    }
    break;
  }
  case TK_UINT8: {
    CORBA::UInt8 val = 0;
    rc = value->get_uint8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_uint8(val);
    break;
  }
  case TK_INT16: {
    CORBA::Short val = 0;
    rc = value->get_int16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (disc_tk == TK_ENUM) {
      write_enum(vw, disc_type, val, treat_disc_as);
    } else {
      vw.write_int16(val);
    }
    break;
  }
  case TK_UINT16: {
    CORBA::UShort val = 0;
    rc = value->get_uint16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_uint16(val);
    break;
  }
  case TK_INT32: {
    rc = value->get_int32_value(disc_val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    if (disc_tk == TK_ENUM) {
      write_enum(vw, disc_type, disc_val, treat_disc_as);
    } else {
      vw.write_int32(disc_val);
    }
    break;
  }
  case TK_UINT32: {
    CORBA::ULong val = 0;
    rc = value->get_uint32_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_uint32(val);
    break;
  }
  case TK_INT64: {
    CORBA::LongLong val = 0;
    rc = value->get_int64_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_int64(val);
    break;
  }
  case TK_UINT64: {
    CORBA::ULongLong val = 0;
    rc = value->get_uint64_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return;
    }
    disc_val = static_cast<CORBA::Long>(val);
    vw.write_uint64(val);
    break;
  }
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_discriminator: Invalid discriminator type %C\n",
                 typekind_to_string(disc_tk)));
    }
  }
}

void vwrite_union(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var type_desc;
  DDS::ReturnCode_t rc = dt->get_descriptor(type_desc);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_union: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }

  const Extensibility extensibility = XTypes::dds_to_opendds_ext(type_desc->extensibility_kind());
  vw.begin_union(extensibility);

  // Discriminator
  DDS::DynamicTypeMember_var dtm;
  rc = dt->get_member(dtm, XTypes::DISCRIMINATOR_ID);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_union: get_member failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }
  DDS::MemberDescriptor_var disc_md;
  rc = dtm->get_descriptor(disc_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_union: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }
  // Discriminator Id is always 0?
  VWriterMemberParam params(0, disc_md->is_must_understand() || disc_md->is_key(),
                            disc_md->name(), disc_md->is_optional(), true);
  vw.begin_discriminator(params);
  CORBA::Long disc_val = 0;
  vwrite_discriminator(vw, value, disc_md, disc_val);
  vw.end_discriminator();

  // Selected branch
  bool has_branch = false;
  DDS::MemberDescriptor_var selected_md;
  rc = XTypes::get_selected_union_branch(dt, disc_val, has_branch, selected_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_union: get_selected_union_branch failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }
  if (has_branch) {
    vwrite_member(vw, value, selected_md, XTypes::TK_UNION);
  }

  vw.end_union();
}

void vwrite_element(ValueWriter& vw, DDS::DynamicData_ptr value,
                    const DDS::DynamicType_var& elem_dt, CORBA::ULong idx)
{
  const DDS::MemberId id = value->get_member_id_at_index(idx);
  if (id == XTypes::MEMBER_ID_INVALID) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_element: get_member_id_at_index %u failed\n", idx));
    }
    return;
  }
  vwrite_item(vw, value, id, elem_dt);
}

DDS::ReturnCode_t vwrite_primitive_array(ValueWriter& vw, DDS::DynamicData_ptr value,
                                         XTypes::TypeKind prim_kind, XTypes::TypeKind orig_kind, CORBA::ULong arr_flat_idx)
{
  // TODO: To support this optimization for dynamic data, we need to have the semantics
  // of the MemberId argument dependent on whether get_*_value or get_*_values is called
  // on the dynamic data object. In particular, in get_*_value, id is the Id of the final
  // element type; in get_*_values, id is the Id of the innermost single-dimension array
  // in relative to the whole original array.
  const DDS::MemberId id = value->get_member_id_at_index(arr_flat_idx);
  if (id == XTypes::MEMBER_ID_INVALID) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_primitive_array: get_member_id_at_index %u failed\n", arr_flat_idx));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }

  return vwrite_primitive_array_i(vw, value, id, prim_kind, orig_kind, false /*for_sequence*/);
}

void vwrite_array_helper(ValueWriter& vw, CORBA::ULong dim_idx, const DDS::BoundSeq& dims,
                         std::vector<CORBA::ULong> idx_vec, const DDS::DynamicType_var& elem_type,
                         DDS::DynamicData_ptr value)
{
  const XTypes::TypeKind elem_kind = elem_type->get_kind();
  const CORBA::ULong dims_len = dims.length();
  XTypes::TypeKind treat_elem_as;
  if (get_equivalent_kind(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return;
  }

  const bool try_optimize = XTypes::is_primitive(treat_elem_as) && dim_idx == dims_len - 1;
  bool optimize_failed = false;
  vw.begin_array(elem_kind);
  if (try_optimize) {
    // Try writing the innermost arrays using write_*_array.
    // Fall back to write elements one by one if fails.
    CORBA::ULong arr_flat_idx = 0;
    const DDS::ReturnCode_t rc = XTypes::flat_index(arr_flat_idx, idx_vec, dims, dim_idx);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Warning) {
        ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_array_helper: flat_index failed (%C)\n",
                   retcode_to_string(rc)));
      }
      return;
    }
    optimize_failed = vwrite_primitive_array(vw, value, treat_elem_as, elem_kind, arr_flat_idx) != DDS::RETCODE_OK;
  }

  if (!try_optimize || optimize_failed) {
    for (CORBA::ULong i = 0; i < dims[dim_idx]; ++i) {
      vw.begin_element(i);
      idx_vec[dim_idx] = i;
      if (dim_idx == dims_len - 1) {
        CORBA::ULong flat_idx = 0;
        const DDS::ReturnCode_t rc = XTypes::flat_index(flat_idx, idx_vec, dims, dims_len);
        if (rc != DDS::RETCODE_OK) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_array_helper: flat_index failed (%C)\n",
                       retcode_to_string(rc)));
          }
          return;
        }
        vwrite_element(vw, value, elem_type, flat_idx);
      } else {
        vwrite_array_helper(vw, dim_idx+1, dims, idx_vec, elem_type, value);
      }
      vw.end_element();
    }
  }
  vw.end_array();
}

void vwrite_array(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var td;
  DDS::ReturnCode_t rc = dt->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_array: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }

  DDS::DynamicType_var elem_type = XTypes::get_base_type(td->element_type());
  const DDS::BoundSeq& dims = td->bound();
  std::vector<CORBA::ULong> idx_vec(dims.length());

  vwrite_array_helper(vw, 0, dims, idx_vec, elem_type, value);
}

void vwrite_sequence(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var td;
  DDS::ReturnCode_t rc = dt->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite_sequence: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return;
  }
  DDS::DynamicType_var elem_type = XTypes::get_base_type(td->element_type());

  const CORBA::ULong length = value->get_item_count();
  vw.begin_sequence(elem_type->get_kind(), length);
  for (CORBA::ULong i = 0; i < length; ++i) {
    vw.begin_element(i);
    vwrite_element(vw, value, elem_type, i);
    vw.end_element();
  }
  vw.end_sequence();
}

void vwrite(ValueWriter& vw, DDS::DynamicData_ptr value)
{
  using namespace XTypes;
  const DDS::DynamicType_var type = value->type();
  const DDS::DynamicType_var base_type = XTypes::get_base_type(type);
  const DDS::TypeKind tk = base_type->get_kind();
  switch (tk) {
  case TK_STRUCTURE:
    return vwrite_struct(vw, value, base_type);
  case TK_UNION:
    return vwrite_union(vw, value, base_type);
  case TK_ARRAY:
    return vwrite_array(vw, value, base_type);
  case TK_SEQUENCE:
    return vwrite_sequence(vw, value, base_type);
  default:
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: vwrite: Unexpected type %C\n",
                 XTypes::typekind_to_string(tk)));
    }
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE