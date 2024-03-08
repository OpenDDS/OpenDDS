/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#ifndef OPENDDS_SAFETY_PROFILE
#  include "DynamicVwrite.h"
#  include "Utils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace {
using namespace OpenDDS;
using namespace OpenDDS::DCPS;

bool check_rc(DDS::ReturnCode_t rc, DDS::MemberId id, DDS::TypeKind tk, const char* fn_name)
{
  return XTypes::check_rc_from_get(rc, id, tk, fn_name, LogLevel::Notice);
}

bool begin_member_helper(ValueWriter& vw, MemberParam* params,
                         DDS::ReturnCode_t rc, DDS::TypeKind containing_tk)
{
  if (containing_tk == XTypes::TK_STRUCTURE || containing_tk == XTypes::TK_UNION) {
    if (rc == DDS::RETCODE_NO_DATA) {
      params->present = false;
    }
    if (containing_tk == XTypes::TK_STRUCTURE) {
      if (!vw.begin_struct_member(*params)) {
        return false;
      }
    } else {
      if (!vw.begin_union_member(*params)) {
        return false;
      }
    }
  }
  return true;
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

template <typename T>
bool vwrite_primitive_array_helper(
  ValueWriter& vw, const T* buffer, bool (ValueWriter::*pmf)(const T*, ACE_CDR::ULong),
  XTypes::TypeKind orig_elem_kind, const DDS::BoundSeq& dims, CORBA::ULong dim_idx,
  CORBA::ULong start_blk_idx, CORBA::ULong blk_len)
{
  using namespace XTypes;
  if (!vw.begin_array(orig_elem_kind)) {
    return false;
  }

  if (dim_idx == dims.length() - 1) {
    if (!(vw.*pmf)(buffer + start_blk_idx, blk_len)) {
      return false;
    }
  } else {
    const CORBA::ULong dim = dims[dim_idx];
    const CORBA::ULong subblk_len = blk_len / dim;
    for (CORBA::ULong i = 0; i < dim; ++i) {
      start_blk_idx += i * subblk_len;
      if (!vw.begin_element(i) ||
          !vwrite_primitive_array_helper(vw, buffer, pmf, orig_elem_kind, dims,
                                         dim_idx + 1, start_blk_idx, subblk_len) ||
          !vw.end_element()) {
        return false;
      }
    }
  }

  return vw.end_array();
}

DDS::ReturnCode_t vwrite_primitive_collection(
  ValueWriter& vw, DDS::DynamicData_ptr value, DDS::MemberId id,
  XTypes::TypeKind elem_kind, XTypes::TypeKind orig_elem_kind, XTypes::TypeKind col_tk,
  const DDS::BoundSeq& dims, XTypes::TypeKind containing_tk, MemberParam* params)
{
  using namespace XTypes;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (elem_kind) {
  case TK_INT8: {
    DDS::Int8Seq val;
    rc = value->get_int8_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_int8_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_int8_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_UINT8: {
    DDS::UInt8Seq val;
    rc = value->get_uint8_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_uint8_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_uint8_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_INT16: {
    DDS::Int16Seq val;
    rc = value->get_int16_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_int16_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_int16_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_UINT16: {
    DDS::UInt16Seq val;
    rc = value->get_uint16_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_uint16_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_uint16_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_INT32: {
    DDS::Int32Seq val;
    rc = value->get_int32_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_int32_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_int32_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_UINT32: {
    DDS::UInt32Seq val;
    rc = value->get_uint32_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_uint32_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_uint32_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_INT64: {
    DDS::Int64Seq val;
    rc = value->get_int64_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_int64_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_int64_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_UINT64: {
    DDS::UInt64Seq val;
    rc = value->get_uint64_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_uint64_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_uint64_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_FLOAT32: {
    DDS::Float32Seq val;
    rc = value->get_float32_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_float32_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_float32_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_FLOAT64: {
    DDS::Float64Seq val;
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    rc = value->get_float64_values(val, id);
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_float64_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_float64_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_FLOAT128: {
    DDS::Float128Seq val;
    rc = value->get_float128_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_float128_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_float128_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_CHAR8: {
    DDS::CharSeq val;
    rc = value->get_char8_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_char8_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_char8_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    DDS::WcharSeq val;
    rc = value->get_char16_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_char16_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_char16_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
#endif
  case TK_BYTE: {
    DDS::ByteSeq val;
    rc = value->get_byte_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_byte_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_byte_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  case TK_BOOLEAN: {
    DDS::BooleanSeq val;
    rc = value->get_boolean_values(val, id);
    if (!check_rc(rc, id, col_tk, "vwrite_primitive_collection")) {
      break;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      rc = DDS::RETCODE_ERROR;
      break;
    }
    if (rc == DDS::RETCODE_OK) {
      if (col_tk == TK_SEQUENCE) {
        if (!vw.begin_sequence(orig_elem_kind, val.length()) ||
            !vw.write_boolean_array(val.get_buffer(), val.length()) ||
            !vw.end_sequence()) {
          rc = DDS::RETCODE_ERROR;
        }
      } else if (col_tk == TK_ARRAY) {
        if (!vwrite_primitive_array_helper(vw, val.get_buffer(), &ValueWriter::write_boolean_array,
                                           orig_elem_kind, dims, 0, 0, val.length())) {
          rc = DDS::RETCODE_ERROR;
        }
      }
    } else if (!vw.write_absent_value()) {
      rc = DDS::RETCODE_ERROR;
    }
    break;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_primitive_collection:"
                 " Expect a primitive type, receive %C\n", XTypes::typekind_to_string(elem_kind)));
    }
    return DDS::RETCODE_BAD_PARAMETER;
  }
  return rc;
}

template <typename ValueT, typename HelperT>
bool set_enumerated_helper(const DDS::DynamicType_var& type, HelperT& helper)
{
  DDS::DynamicTypeMembersById_var members_var;
  if (type->get_all_members(members_var) != DDS::RETCODE_OK) {
    return false;
  }
  OpenDDS::XTypes::DynamicTypeMembersByIdImpl* members =
    dynamic_cast<OpenDDS::XTypes::DynamicTypeMembersByIdImpl*>(members_var.in());
  if (!members) {
    return false;
  }

  OPENDDS_VECTOR(typename HelperT::Pair) pairs(members->size());
  size_t i = 0;
  for (OpenDDS::XTypes::DynamicTypeMembersByIdImpl::const_iterator it = members->begin();
       it != members->end(); ++it, ++i) {
    DDS::MemberDescriptor_var md;
    if (it->second->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    typename HelperT::Pair tmp = { md->name(), static_cast<ValueT>(md->id()) };
    pairs[i] = tmp;
  }
  helper.pairs(pairs);
  return true;
}

// Argument containing_tk and params only apply when this is a member of a struct or union,
// and is ignored when this is an element of a sequence or array.
bool vwrite_item(ValueWriter& vw, DDS::DynamicData_ptr value, DDS::MemberId id,
                 const DDS::DynamicType_var& item_type, DDS::TypeKind containing_tk = XTypes::TK_NONE,
                 MemberParam* params = 0)
{
  using namespace OpenDDS::XTypes;
  const DDS::TypeKind item_tk = item_type->get_kind();
  DDS::TypeKind treat_as;
  if (get_equivalent_kind(item_type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }

  ListEnumHelper enum_helper(treat_as);
  DDS::TypeDescriptor_var td;
  if (item_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  MapBitmaskHelper bitmask_helper(treat_as);
  if (item_tk == TK_BITMASK) {
    bitmask_helper.bit_bound(td->bound()[0]);
  }
  if ((item_tk == TK_ENUM && !set_enumerated_helper<ACE_CDR::Long>(item_type, enum_helper)) ||
      (item_tk == TK_BITMASK && !set_enumerated_helper<ACE_CDR::UShort>(item_type, bitmask_helper))) {
    return false;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (treat_as) {
  case TK_INT8: {
    CORBA::Int8 val = 0;
    rc = value->get_int8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        if (!vw.write_enum(val, enum_helper)) {
          return false;
        }
      } else if (!vw.write_int8(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_UINT8: {
    CORBA::UInt8 val = 0;
    rc = value->get_uint8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_BITMASK) {
        if (!vw.write_bitmask(val, bitmask_helper)) {
          return false;
        }
      } else if (!vw.write_uint8(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_INT16: {
    CORBA::Short val = 0;
    rc = value->get_int16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        if (!vw.write_enum(val, enum_helper)) {
          return false;
        }
      } else if (!vw.write_int16(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_UINT16: {
    CORBA::UShort val = 0;
    rc = value->get_uint16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_BITMASK) {
        if (!vw.write_bitmask(val, bitmask_helper)) {
          return false;
        }
      } else if (!vw.write_uint16(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_INT32: {
    CORBA::Long val = 0;
    rc = value->get_int32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_ENUM) {
        if (!vw.write_enum(val, enum_helper)) {
          return false;
        }
      } else if (!vw.write_int32(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_UINT32: {
    CORBA::ULong val = 0;
    rc = value->get_uint32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_BITMASK) {
        if (!vw.write_bitmask(val, bitmask_helper)) {
          return false;
        }
      } else if (!vw.write_uint32(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_INT64: {
    CORBA::LongLong val = 0;
    rc =  value->get_int64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_int64(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_UINT64: {
    CORBA::ULongLong val = 0;
    rc = value->get_uint64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (item_tk == TK_BITMASK) {
        if (!vw.write_bitmask(val, bitmask_helper)) {
          return false;
        }
      } else if (!vw.write_uint64(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_FLOAT32: {
    CORBA::Float val = 0.0f;
    rc = value->get_float32_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_float32(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_FLOAT64: {
    CORBA::Double val = 0.0;
    rc = value->get_float64_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_float64(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_FLOAT128: {
    CORBA::LongDouble val;
    ACE_CDR_LONG_DOUBLE_ASSIGNMENT(val, 0.0l);
    rc = value->get_float128_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_float128(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_CHAR8: {
    CORBA::Char val = '\0';
    rc = value->get_char8_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_char8(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val = L'\0';
    rc = value->get_char16_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_char16(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
#endif
  case TK_BYTE: {
    CORBA::Octet val = 0x00;
    rc = value->get_byte_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_byte(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = value->get_boolean_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_boolean(val)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  case TK_STRING8: {
    CORBA::String_var val;
    rc = value->get_string_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_string(val.in())) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = value->get_wstring_value(val, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vw.write_wstring(val.in())) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
#endif
  case TK_SEQUENCE:
  case TK_ARRAY: {
    DDS::TypeDescriptor_var col_td;
    rc = item_type->get_descriptor(col_td);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_item: get_descriptor for %C failed (%C)\n",
                   typekind_to_string(treat_as), retcode_to_string(rc)));
      }
      return false;
    }
    DDS::DynamicType_var elem_type = get_base_type(col_td->element_type());
    const TypeKind elem_kind = elem_type->get_kind();
    TypeKind treat_elem_as;
    if (get_equivalent_kind(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
      return false;
    }
    // Try writing the whole primitive collection. If the get sequence operations
    // are not supported by the dynamic data object, fall back to write element one by one.
    if (is_primitive(treat_elem_as)) {
      rc = vwrite_primitive_collection(vw, value, id, treat_elem_as, elem_kind,
                                       treat_as, col_td->bound(), containing_tk, params);
      if (rc == DDS::RETCODE_OK || rc == DDS::RETCODE_NO_DATA) {
        break;
      }
      if (rc != DDS::RETCODE_UNSUPPORTED) {
        return false;
      }
    }
  }
    // fallthrough
  case TK_STRUCTURE:
  case TK_UNION: {
    DDS::DynamicData_var member_data;
    rc = value->get_complex_value(member_data, id);
    if (!check_rc(rc, id, treat_as, "vwrite_item")) {
      return false;
    }
    if (!begin_member_helper(vw, params, rc, containing_tk)) {
      return false;
    }
    if (rc == DDS::RETCODE_OK) {
      if (!vwrite(vw, member_data)) {
        return false;
      }
    } else if (!vw.write_absent_value()) {
      return false;
    }
    break;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_item: Unsupported type %C\n",
                 typekind_to_string(treat_as)));
    }
    return false;
  }

  if (containing_tk == TK_STRUCTURE) {
    if (!vw.end_struct_member()) {
      return false;
    }
  } else if (containing_tk == TK_UNION) {
    if (!vw.end_union_member()) {
      return false;
    }
  }
  return true;
}

bool vwrite_member(ValueWriter&vw, DDS::DynamicData_ptr value,
                   const DDS::MemberDescriptor_var& md, DDS::TypeKind containing_tk)
{
  const DDS::MemberId id = md->id();
  const DDS::DynamicType_var member_type = XTypes::get_base_type(md->type());
  MemberParam params(id, md->is_must_understand() || md->is_key(), md->name(), md->is_optional(), true);
  return vwrite_item(vw, value, id, member_type, containing_tk, &params);
}

bool vwrite_struct(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var type_desc;
  DDS::ReturnCode_t rc = dt->get_descriptor(type_desc);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_struct: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }

  const Extensibility extensibility = XTypes::dds_to_opendds_ext(type_desc->extensibility_kind());
  if (!vw.begin_struct(extensibility)) {
    return false;
  }
  for (CORBA::ULong i = 0; i < dt->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    rc = dt->get_member_by_index(dtm, i);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_struct: get_member_by_index %u failed (%C)\n",
                   i, retcode_to_string(rc)));
      }
      return false;
    }
    DDS::MemberDescriptor_var md;
    rc = dtm->get_descriptor(md);
    if (rc != DDS::RETCODE_OK) {
      if (log_level >= LogLevel::Notice) {
        ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_struct:"
                   " get_descriptor for member at index %u failed (%C)\n", i, retcode_to_string(rc)));
      }
      return false;
    }
    if (!vwrite_member(vw, value, md, XTypes::TK_STRUCTURE)) {
      return false;
    }
  }
  return vw.end_struct();
}

bool vwrite_discriminator(ValueWriter& vw, DDS::DynamicData_ptr value,
                          const DDS::MemberDescriptor_var& md, CORBA::Long& disc_val)
{
  using namespace OpenDDS::XTypes;
  const DDS::MemberId id = DISCRIMINATOR_ID;
  const DDS::DynamicType_var disc_type = get_base_type(md->type());
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  DDS::TypeKind treat_disc_as;
  if (get_equivalent_kind(disc_type, treat_disc_as) != DDS::RETCODE_OK) {
    return false;
  }

  ListEnumHelper enum_helper(treat_disc_as);
  if (disc_tk == TK_ENUM && !set_enumerated_helper<ACE_CDR::Long>(disc_type, enum_helper)) {
    return false;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (treat_disc_as) {
  case TK_BOOLEAN: {
    CORBA::Boolean val = false;
    rc = value->get_boolean_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_boolean(val)) {
      return false;
    }
    break;
  }
  case TK_BYTE: {
    CORBA::Octet val = 0x00;
    rc = value->get_byte_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_byte(val)) {
      return false;
    }
    break;
  }
  case TK_CHAR8: {
    CORBA::Char val = '\0';
    rc = value->get_char8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_char8(val)) {
      return false;
    }
    break;
  }
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16: {
    CORBA::WChar val = L'\0';
    rc = value->get_char16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_char16(val)) {
      return false;
    }
    break;
  }
#endif
  case TK_INT8: {
    CORBA::Int8 val = 0;
    rc = value->get_int8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (disc_tk == TK_ENUM) {
      if (!vw.write_enum(val, enum_helper)) {
        return false;
      }
    } else {
      if (!vw.write_int8(val)) {
        return false;
      }
    }
    break;
  }
  case TK_UINT8: {
    CORBA::UInt8 val = 0;
    rc = value->get_uint8_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_uint8(val)) {
      return false;
    }
    break;
  }
  case TK_INT16: {
    CORBA::Short val = 0;
    rc = value->get_int16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (disc_tk == TK_ENUM) {
      if (!vw.write_enum(val, enum_helper)) {
        return false;
      }
    } else {
      if (!vw.write_int16(val)) {
        return false;
      }
    }
    break;
  }
  case TK_UINT16: {
    CORBA::UShort val = 0;
    rc = value->get_uint16_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_uint16(val)) {
      return false;
    }
    break;
  }
  case TK_INT32: {
    rc = value->get_int32_value(disc_val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    if (disc_tk == TK_ENUM) {
      if (!vw.write_enum(disc_val, enum_helper)) {
        return false;
      }
    } else {
      if (!vw.write_int32(disc_val)) {
        return false;
      }
    }
    break;
  }
  case TK_UINT32: {
    CORBA::ULong val = 0;
    rc = value->get_uint32_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_uint32(val)) {
      return false;
    }
    break;
  }
  case TK_INT64: {
    CORBA::LongLong val = 0;
    rc = value->get_int64_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_int64(val)) {
      return false;
    }
    break;
  }
  case TK_UINT64: {
    CORBA::ULongLong val = 0;
    rc = value->get_uint64_value(val, id);
    if (!check_rc(rc, id, disc_tk, "vwrite_discriminator")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(val);
    if (!vw.write_uint64(val)) {
      return false;
    }
    break;
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_discriminator: Invalid discriminator type %C\n",
                 typekind_to_string(disc_tk)));
    }
    return false;
  }
  return true;
}

bool vwrite_union(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var type_desc;
  DDS::ReturnCode_t rc = dt->get_descriptor(type_desc);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_union: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }

  const Extensibility extensibility = XTypes::dds_to_opendds_ext(type_desc->extensibility_kind());
  if (!vw.begin_union(extensibility)) {
    return false;
  }

  DDS::DynamicTypeMember_var dtm;
  rc = dt->get_member(dtm, XTypes::DISCRIMINATOR_ID);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_union: get_member failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }
  DDS::MemberDescriptor_var disc_md;
  rc = dtm->get_descriptor(disc_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_union: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }
  MemberParam params(XTypes::DISCRIMINATOR_SERIALIZED_ID,
                     disc_md->is_must_understand() || disc_md->is_key(),
                     disc_md->name(), disc_md->is_optional(), true);
  if (!vw.begin_discriminator(params)) {
    return false;
  }
  CORBA::Long disc_val = 0;
  if (!vwrite_discriminator(vw, value, disc_md, disc_val)) {
    return false;
  }
  if (!vw.end_discriminator()) {
    return false;
  }

  bool has_branch = false;
  DDS::MemberDescriptor_var selected_md;
  rc = XTypes::get_selected_union_branch(dt, disc_val, has_branch, selected_md);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_union: get_selected_union_branch failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }
  if (has_branch && !vwrite_member(vw, value, selected_md, XTypes::TK_UNION)) {
    return false;
  }

  return vw.end_union();
}

bool vwrite_element(ValueWriter& vw, DDS::DynamicData_ptr value,
                    const DDS::DynamicType_var& elem_dt, CORBA::ULong idx)
{
  const DDS::MemberId id = value->get_member_id_at_index(idx);
  if (id == XTypes::MEMBER_ID_INVALID) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_element: get_member_id_at_index %u failed\n", idx));
    }
    return false;
  }
  return vwrite_item(vw, value, id, elem_dt);
}

bool vwrite_array_helper(ValueWriter& vw, CORBA::ULong dim_idx, const DDS::BoundSeq& dims,
                         DDS::BoundSeq& idx_vec, const DDS::DynamicType_var& elem_type,
                         DDS::DynamicData_ptr value)
{
  const XTypes::TypeKind elem_kind = elem_type->get_kind();
  const CORBA::ULong dims_len = dims.length();

  if (!vw.begin_array(elem_kind)) {
    return false;
  }

  for (CORBA::ULong i = 0; i < dims[dim_idx]; ++i) {
    if (!vw.begin_element(i)) {
      return false;
    }
    idx_vec[dim_idx] = i;
    if (dim_idx == dims_len - 1) {
      CORBA::ULong flat_idx = 0;
      const DDS::ReturnCode_t rc = XTypes::flat_index(flat_idx, idx_vec, dims);
      if (rc != DDS::RETCODE_OK) {
        if (log_level >= LogLevel::Notice) {
          ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_array_helper: flat_index failed (%C)\n",
                     retcode_to_string(rc)));
        }
        return false;
      }
      if (!vwrite_element(vw, value, elem_type, flat_idx)) {
        return false;
      }
    } else if (!vwrite_array_helper(vw, dim_idx + 1, dims, idx_vec, elem_type, value)) {
      return false;
    }
    if (!vw.end_element()) {
      return false;
    }
  }

  return vw.end_array();
}

bool vwrite_array(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var td;
  DDS::ReturnCode_t rc = dt->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_array: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }

  DDS::DynamicType_var elem_type = XTypes::get_base_type(td->element_type());
  const DDS::BoundSeq& dims = td->bound();
  DDS::BoundSeq idx_vec;
  idx_vec.length(dims.length());

  return vwrite_array_helper(vw, 0, dims, idx_vec, elem_type, value);
}

bool vwrite_sequence(ValueWriter& vw, DDS::DynamicData_ptr value, const DDS::DynamicType_var& dt)
{
  DDS::TypeDescriptor_var td;
  DDS::ReturnCode_t rc = dt->get_descriptor(td);
  if (rc != DDS::RETCODE_OK) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: vwrite_sequence: get_descriptor failed (%C)\n",
                 retcode_to_string(rc)));
    }
    return false;
  }
  DDS::DynamicType_var elem_type = XTypes::get_base_type(td->element_type());
  const CORBA::ULong length = value->get_item_count();

  if (!vw.begin_sequence(elem_type->get_kind(), length)) {
    return false;
  }
  for (CORBA::ULong i = 0; i < length; ++i) {
    if (!vw.begin_element(i) ||
        !vwrite_element(vw, value, elem_type, i) ||
        !vw.end_element()) {
      return false;
    }
  }
  return vw.end_sequence();
}

}

namespace OpenDDS {
namespace DCPS {

bool vwrite(ValueWriter& vw, DDS::DynamicData_ptr value)
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
  return false;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
