/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XCDR2_VALUE_WRITER_H
#define OPENDDS_DCPS_XCDR2_VALUE_WRITER_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "ValueWriter.h"
#include "Serializer.h"

#include <dds/DdsDynamicDataC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Write a dynamic data object in XCDR2 format
class Xcdr2ValueWriter : public ValueWriter {
public:
  Xcdr2ValueWriter(Serializer& ser)
    : writer_(ser)
  {}

  void begin_struct(DDS::DynamicData_ptr struct_dd);
  void end_struct();
  void begin_struct_member(const DDS::MemberDescriptor& descriptor, DDS::ReturnCode_t rc);
  void end_struct_member();

  void begin_union();
  void end_union();
  void begin_discriminator();
  void end_discriminator();
  void begin_union_member(const char* name);
  void end_union_member();

  void begin_array();
  void end_array();
  void begin_sequence();
  void end_sequence();
  void begin_element(size_t idx);
  void end_element();

  void write_boolean(ACE_CDR::Boolean value);
  void write_byte(ACE_CDR::Octet value);
#if OPENDDS_HAS_EXPLICIT_INTS
  void write_int8(ACE_CDR::Int8 value);
  void write_uint8(ACE_CDR::UInt8 value);
#endif
  void write_int16(ACE_CDR::Short value);
  void write_uint16(ACE_CDR::UShort value);
  void write_int32(ACE_CDR::Long value);
  void write_uint32(ACE_CDR::ULong value);
  void write_int64(ACE_CDR::LongLong value);
  void write_uint64(ACE_CDR::ULongLong value);
  void write_float32(ACE_CDR::Float value);
  void write_float64(ACE_CDR::Double value);
  void write_float128(ACE_CDR::LongDouble value);
  void write_fixed(const OpenDDS::FaceTypes::Fixed& value);
  void write_char8(ACE_CDR::Char value);
  void write_char16(ACE_CDR::WChar value);
  void write_string(const ACE_CDR::Char* value, size_t length);
  void write_wstring(const ACE_CDR::WChar* value, size_t length);
  void write_enum(const char* /*name*/, ACE_CDR::Long value);

private:

  bool serialized_size(size_t& size, DDS::DynamicData_ptr data);
  bool serialized_size_struct(size_t& size, DDS::DynamicData_ptr data);
  bool serialized_size_union(size_t& size, DDS::DynamicData_ptr data);
  bool serialized_size_collection(size_t& size, DDS::DynamicData_ptr data);

  Serializer& writer_;

  // Hold the dynamic data objects of the top-level type and its nested members recursively.
  // The object on top is the deepest nested member so far and is being operated on.
  std::stack<DDS::DynamicData_ptr> values_;

  class StackGuard {
    StackGuard(Xcdr2ValueWriter& vw, DDS::DynamicData_ptr data)
      : value_writer_(vw)
    {
      value_writer_.values_.push(data);
    }

    ~StackGuard()
    {
      value_writer_.values_.pop();
    }

  private:
    Xcdr2ValueWriter& value_writer_;
  };
};

bool Xcdr2ValueWriter::check_rc_from_get(DDS::ReturnCode_t rc, DDS::MemberId id,
                                         DDS::TypeKind tk, const char* fn_name)
{
  if (rc != DDS::RETCODE_OK && rc != DDS::RETCODE_NO_DATA) {
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|t) NOTICE: %C: Failed to get %C member ID %u: %C\n",
                 fn_name, XTypes::typekind_to_string(tk), id, retcode_to_string(rc)));
    }
    return false;
  }
  return true;
}

void Xcdr2ValueWriter::serialized_size_member_header(size_t& size, size_t& mutable_running_total,
                                                     DDS::ReturnCode_t rc, DDS::ExtensibilityKind ek, CORBA::Boolean optional)
{
  const Encoding& encoding = writer_.encoding();
  if (optional && (ek == DDS::FINAL || ek == DDS::APPENDABLE)) {
    primitive_serialized_size_boolean(encoding, size);
    return;
  }
  if (ek == DDS::MUTABLE) {
    if (!optional || rc == DDS::RETCODE_OK) {
      serialized_size_parameter_id(encoding, size, mutable_running_total);
    }
  }
}

bool Xcdr2ValueWriter::serialized_size_primitive_value(size_t& size, DDS::TypeKind member_tk)
{
  const Encoding& encoding = writer_.encoding();
  using namespace OpenDDS::XTypes;
  switch (member_tk) {
  case TK_INT32:
    return primitive_serialized_size(encoding, size, CORBA::Long());
  case TK_UINT32:
    return primitive_serialized_size(encoding, size, CORBA::ULong());
  case TK_INT8:
    primitive_serialized_size_int8(encoding, size);
    return true;
  case TK_UINT8:
    primitive_serialized_size_uint8(encoding, size);
    return true;
  case TK_INT16:
    return primitive_serialized_size(encoding, size, CORBA::Short());
  case TK_UINT16:
    return primitive_serialized_size(encoding, size, CORBA::UShort());
  case TK_INT64:
    return primitive_serialized_size(encoding, size, CORBA::LongLong());
  case TK_UINT64:
    return primitive_serialized_size(encoding, size, CORBA::ULongLong());
  case TK_FLOAT32:
    return primitive_serialized_size(encoding, size, CORBA::Float());
  case TK_FLOAT64:
    return primitive_serialized_size(encoding, size, CORBA::Double());
  case TK_FLOAT128:
    return primitive_serialized_size(encoding, size, CORBA::LongDouble());
  case TK_CHAR8:
    primitive_serialized_size_char(encoding, size);
    return true;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    primitive_serialized_size_wchar(encoding, size);
    return true;
#endif
  case TK_BYTE:
    primitive_serialized_size_octet(encoding, size);
    return true;
  case TK_BOOLEAN:
    primitive_serialized_size_boolean(encoding, size);
    return true;
  }
  return false;
}

void Xcdr2ValueWriter::serialized_size_string_value(size_t& size, const char* str)
{
  primitive_serialized_size_ulong(writer_.encoding(), size);
  if (str) {
    size += ACE_OS::strlen(str) + 1; // Include null termination
  }
}

#ifdef DDS_HAS_WCHAR
void Xcdr2ValueWriter::serialized_size_wstring_value(size_t& size, const CORBA::WChar* wstr)
{
  primitive_serialized_size_ulong(writer_.encoding(), size);
  if (wstr) {
    size += ACE_OS::strlen(wstr) * char16_cdr_size; // Not include null termination
  }
}
#endif

// For a member of a struct or a union
bool Xcdr2ValueWriter::serialized_size_member(size_t& size, size_t& mutable_running_total,
                                              DDS::ExtensibilityKind ek, const DDS::MemberDescriptor_var& md)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicData_ptr& data = values_.top();
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);

  const DDS::MemberId member_id = md->id();
  const CORBA::Boolean optional = md->is_optional();
  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::TypeKind member_tk = member_type->get_kind();
  DDS::TypeKind treat_member_as = member_tk;

  if (member_tk == TK_ENUM && enum_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }

  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  if (is_primitive(treat_member_as)) {
    switch (treat_member_as) {
    case TK_INT8: {
      CORBA::Int8 val;
      rc = data->get_int8_value(val, member_id);
      break;
    }
    case TK_UINT8: {
      CORBA::UInt8 val;
      rc = data->get_uint8_value(val, member_id);
      break;
    }
    case TK_INT16: {
      CORBA::Short val;
      rc = data->get_int16_value(val, member_id);
      break;
    }
    case TK_UINT16: {
      CORBA::UShort val;
      rc = data->get_uint16_value(val, member_id);
      break;
    }
    case TK_INT32: {
      CORBA::Long val;
      rc = data->get_int32_value(val, member_id);
      break;
    }
    case TK_UINT32: {
      CORBA::ULong val;
      rc = data->get_uint32_value(val, member_id);
      break;
    }
    case TK_INT64: {
      CORBA::LongLong val;
      rc = data->get_int64_value(val, member_id);
      break;
    }
    case TK_UINT64: {
      CORBA::ULongLong val;
      rc = data->get_uint64_value(val, member_id);
      break;
    }
    case TK_FLOAT32: {
      CORBA::Float val;
      rc = data->get_float32_value(val, member_id);
      break;
    }
    case TK_FLOAT64: {
      CORBA::Double val;
      rc = data->get_float64_value(val, member_id);
      break;
    }
    case TK_FLOAT128: {
      CORBA::LongDouble val;
      rc = data->get_float128_value(val, member_id);
      break;
    }
    case TK_CHAR8: {
      CORBA::Char val;
      rc = data->get_char8_value(val, member_id);
      break;
    }
#ifdef DDS_HAS_WCHAR
    case TK_CHAR16: {
      CORBA::WChar val;
      rc = data->get_char16_value(val, member_id);
      break;
    }
#endif
    case TK_BYTE: {
      CORBA::Octet val;
      rc = data->get_byte_value(val, member_id);
      break;
    }
    case TK_BOOLEAN: {
      CORBA::Boolean val = false;
      rc = data->get_boolean_value(val, member_id);
      break;
    }
    }

    if (!check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_member")) {
      return false;
    }
    serialized_size_member_header(size, mutable_running_total, rc, ek, optional);
    if (rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return serialized_size_primitive_value(size, treat_member_as);
  }

  switch (treat_member_as) {
  case TK_STRING8: {
    CORBA::String_var val;
    rc = data->get_string_value(val, member_id);
    if (!check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_member")) {
      return false;
    }
    serialized_size_member_header(size, mutable_running_total, rc, ek, optional);
    if (rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    serialized_size_string_value(size, val.in());
    return true;
  }
#ifdef DDS_HAS_WCHAR
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = data->get_wstring_value(val, member_id);
    if (!check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_member")) {
      return false;
    }
    serialized_size_member_header(size, mutable_running_total, rc, ek, optional);
    if (rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    serialized_size_wstring_value(size, val.in());
    return true;
  }
#endif
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var member_data;
    rc = data->get_complex_value(member_data, member_id);
    if (!check_rc_from_get(rc, member_id, treat_member_as, "serialized_size_member")) {
      return false;
    }
    serialized_size_member_header(size, mutable_running_total, rc, ek, optional);
    if (rc == DDS::RETCODE_NO_DATA) {
      return true;
    }
    return serialized_size(size, member_data);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialized_size_member:"
                 " Unsupported member type %C at ID %u\n", typekind_to_string(member_tk), member_id));
    }
  }
  return false;
}

bool Xcdr2ValueWriter::serialized_size_struct(size_t& size)
{
  const DDS::DynamicType_var type = values_.top()->type();
  const DDS::DynamicType_var base_type = XTypes::get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (base_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }

  const Encoding& encoding = writer_.encoding();

  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  size_t mutable_running_total = 0;
  for (CORBA::ULong i = 0; i < base_type->get_member_count(); ++i) {
    DDS::DynamicTypeMember_var dtm;
    if (base_type->get_member_by_index(dtm, i) != DDS::RETCODE_OK) {
      return false;
    }
    DDS::MemberDescriptor_var md;
    if (dtm->get_descriptor(md) != DDS::RETCODE_OK) {
      return false;
    }
    if (!serialized_size_member(size, mutable_running_total, extensibility, md)) {
      return false;
    }
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

bool Xcdr2ValueWriter::serialized_size_enum(size_t& size, const DDS::DynamicType_var& enum_type)
{
  using namespace OpenDDS::XTypes;
  DDS::TypeKind equivalent_int_tk;
  if (enum_bound(enum_type, equivalent_int_tk) != DDS::RETCODE_OK) {
    return false;
  }
  const Encoding& encoding = writer_.encoding();
  switch (equivalent_int_tk) {
  case TK_INT8:
    primitive_serialized_size_int8(encoding, size);
    return true;
  case TK_INT16:
    return primitive_serialized_size(encoding, size, CORBA::Short());
  case TK_INT32:
    return primitive_serialized_size(encoding, size, CORBA::Long());
  }
  return false;
}

bool Xcdr2ValueWriter::serialized_size_discriminator(size_t& size, size_t& mutable_running_total,
                                                     const DDS::DynamicType_var& disc_type, DDS::ExtensibilityKind ek)
{
  if (ek == DDS::MUTABLE) {
    serialized_size_parameter_id(writer_.encoding(), size, mutable_running_total);
  }
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  if (XTypes::is_primitive(disc_tk)) {
    return serialized_size_primitive_value(writer_.encoding(), size, disc_tk);
  }
  return serialized_size_enum(size, disc_type);
}

bool Xcdr2ValueWriter::get_discriminator_value(CORBA::Long& disc_val, const DDS::DynamicType_var& disc_type)
{
  using namespace OpenDDS::XTypes;
  const DDS::DynamicData_ptr& union_data = values_.top();
  const DDS::TypeKind disc_tk = disc_type->get_kind();
  DDS::TypeKind treat_as = disc_tk;
  if (disc_tk == TK_ENUM && enum_bound(disc_type, treat_as) != DDS::RETCODE_OK) {
    return false;
  }

  const DDS::MemberId id = DISCRIMINATOR_ID;
  switch (treat_as) {
  case TK_BOOLEAN: {
    CORBA::Boolean value = false;
    const DDS::ReturnCode_t rc = union_data->get_boolean_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_BYTE: {
    CORBA::Octet value;
    const DDS::ReturnCode_t rc = union_data->get_byte_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_CHAR8: {
    CORBA::Char value;
    const DDS::ReturnCode_t rc = union_data->get_char8_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_CHAR16: {
    CORBA::WChar value;
    const DDS::ReturnCode_t rc = union_data->get_char16_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT8: {
    CORBA::Int8 value;
    const DDS::ReturnCode_t rc = union_data->get_int8_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_UINT8: {
    CORBA::UInt8 value;
    const DDS::ReturnCode_t rc = union_data->get_uint8_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT16: {
    CORBA::Short value;
    const DDS::ReturnCode_t rc = union_data->get_int16_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = value;
    return true;
  }
  case TK_UINT16: {
    CORBA::UShort value;
    const DDS::ReturnCode_t rc = union_data->get_uint16_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT32: {
    const DDS::ReturnCode_t rc = union_data->get_int32_value(disc_val, id);
    return check_rc_from_get(rc, id, disc_tk, "get_discriminator_value");
  }
  case TK_UINT32: {
    CORBA::ULong value;
    const DDS::ReturnCode_t rc = union_data->get_uint32_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_INT64: {
    CORBA::LongLong value;
    const DDS::ReturnCode_t rc = union_data->get_int64_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  case TK_UINT64: {
    CORBA::ULongLong value;
    const DDS::ReturnCode_t rc = union_data->get_uint64_value(value, id);
    if (!check_rc_from_get(rc, id, disc_tk, "get_discriminator_value")) {
      return false;
    }
    disc_val = static_cast<CORBA::Long>(value);
    return true;
  }
  }
  return false;
}

bool Xcdr2ValueWriter::serialized_size_union(size_t& size)
{
  const DDS::DynamicData_ptr& data = values_.top();
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = XTypes::get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (base_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }

  const Encoding& encoding = writer_.encoding();

  // Dheader
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    serialized_size_delimiter(encoding, size);
  }

  // Discriminator
  size_t mutable_running_total = 0;
  DDS::DynamicType_var disc_type = XTypes::get_base_type(td->discriminator_type());
  if (!serialized_size_discriminator(size, mutable_running_total, disc_type, extensibility)) {
    return false;
  }

  CORBA::Long disc_val;
  if (!get_discriminator_value(disc_val, data, disc_type)) {
    return false;
  }

  // Selected branch
  bool has_branch = false;
  DDS::MemberDescriptor_var selected_md;
  if (get_selected_union_branch(base_type, disc_val, has_branch, selected_md) != DDS::RETCODE_OK) {
    return false;
  }

  if (has_branch && !serialized_size_member(size, mutable_running_total, extensibility, selected_md)) {
    return false;
  }

  if (extensibility == DDS::MUTABLE) {
    serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);
  }
  return true;
}

void Xcdr2ValueWriter::serialized_size_primitive_elements(const Encoding& encoding, size_t& size,
                                                          DDS::TypeKind elem_tk, CORBA::ULong length)
{
  using namespace OpenDDS::XTypes;
  switch (elem_tk) {
  case TK_INT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Long(), length);
    }
    return;
  case TK_UINT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::ULong(), length);
    }
    return;
  case TK_INT8:
    if (length != 0) {
      primitive_serialized_size_int8(encoding, size, length);
    }
    return;
  case TK_UINT8:
    if (length != 0) {
      primitive_serialized_size_uint8(encoding, size, length);
    }
    return;
  case TK_INT16:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Short(), length);
    }
    return;
  case TK_UINT16:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::UShort(), length);
    }
    return;
  case TK_INT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::LongLong(), length);
    }
    return;
  case TK_UINT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::ULongLong(), length);
    }
    return;
  case TK_FLOAT32:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Float(), length);
    }
    return;
  case TK_FLOAT64:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::Double(), length);
    }
    return;
  case TK_FLOAT128:
    if (length != 0) {
      primitive_serialized_size(encoding, size, CORBA::LongDouble(), length);
    }
    return;
  case TK_CHAR8:
    if (length != 0) {
      primitive_serialized_size_char(encoding, size, length);
    }
    return;
#ifdef DDS_HAS_WCHAR
  case TK_CHAR16:
    if (length != 0) {
      primitive_serialized_size_wchar(encoding, size, length);
    }
    return;
#endif
  case TK_BYTE:
    if (length != 0) {
      primitive_serialized_size_octet(encoding, size, length);
    }
    return;
  case TK_BOOLEAN:
    if (length != 0) {
      primitive_serialized_size_boolean(encoding, size, length);
    }
    return;
  }
}

bool serialized_size_element(DDS::DynamicData_ptr col_data, const Encoding& encoding,
                             size_t& size, DDS::MemberId elem_id, DDS::TypeKind elem_tk)
{
  using namespace OpenDDS::XTypes;
  DDS::ReturnCode_t rc = DDS::RETCODE_OK;
  switch (elem_tk) {
  case TK_STRING8: {
    CORBA::String_var val;
    rc = col_data->get_string_value(val, elem_id);
    if (!check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_element")) {
      return false;
    }
    serialized_size_string_value(encoding, size, val.in());
    return true;
  }
  case TK_STRING16: {
    CORBA::WString_var val;
    rc = col_data->get_wstring_value(val, elem_id);
    if (!check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_element")) {
      return false;
    }
    serialized_size_wstring_value(encoding, size, val.in());
    return true;
  }
  case TK_STRUCTURE:
  case TK_UNION:
  case TK_ARRAY:
  case TK_SEQUENCE: {
    DDS::DynamicData_var elem_data;
    rc = col_data->get_complex_value(elem_data, elem_id);
    if (!check_rc_from_get(rc, elem_id, elem_tk, "serialized_size_element")) {
      return false;
    }
    return serialized_size(size, elem_data);
  }
  default:
    if (log_level >= LogLevel::Notice) {
      ACE_ERROR((LM_NOTICE, "(%P|%t) NOTICE: serialized_size_element:"
                 " Unsupported element type %C at ID %u\n", typekind_to_string(elem_tk), elem_id));
    }
  }
  return false;
}

bool Xcdr2ValueWriter::serialized_size_collection(size_t& size)
{
  const DDS::DynamicData_ptr& col_data = values_.top();
  const DDS::DynamicType_var type = col_data->type();
  const DDS::DynamicType_var base_type = XTypes::get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (base_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  DDS::DynamicType_var elem_type = XTypes::get_base_type(td->element_type());
  const DDS::TypeKind elem_tk = elem_type->get_kind();
  DDS::TypeKind treat_elem_as = elem_tk;

  if (elem_tk == TK_ENUM && enum_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (elem_tk == TK_BITMASK && bitmask_bound(elem_type, treat_elem_as) != DDS::RETCODE_OK) {
    return false;
  }

  const Encoding& encoding = writer_.encoding();

  // Dheader
  if (!is_primitive(elem_tk)) {
    serialized_size_delimiter(encoding, size);
  }

  if (base_type->get_kind() == TK_SEQUENCE) {
    // Sequence length.
    primitive_serialized_size_ulong(encoding, size);
  }

  const CORBA::ULong item_count = col_data->get_item_count();
  if (is_primitive(treat_elem_as)) {
    serialized_size_primitive_elements(encoding, size, treat_elem_as, item_count);
    return true;
  }

  // Non-primitive element types.
  for (CORBA::ULong i = 0; i < item_count; ++i) {
    const DDS::MemberId elem_id = col_data->get_member_id_at_index(i);
    if (elem_id == MEMBER_ID_INVALID ||
        !serialized_size_element(col_data, encoding, size, elem_id, treat_elem_as)) {
      return false;
    }
  }
  return true;
}

bool Xcdr2ValueWriter::serialized_size(size_t& size, DDS::DynamicData_ptr data)
{
  StackGuard guard(*this, data);
  const DDS::DynamicType_var type = data->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  switch (base_type->get_kind()) {
  case TK_STRUCTURE:
    return serialized_size_struct(size);
  case TK_UNION:
    return serialized_size_union(size);
  case TK_ARRAY:
  case TK_SEQUENCE:
    return serialized_size_collection(size);
  }
  return false;
}

// Write header for the struct
bool Xcdr2ValueWriter::begin_struct(DDS::DynamicData_ptr struct_dd)
{
  const DDS::DynamicType_var type = struct_dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (base_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }

  // Not popping the dynamic data object until end_struct call.
  values_.push(struct_dd);

  size_t total_size = 0;
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_struct(total_size) || !writer_.write_delimiter(total_size)) {
      return false;
    }
  }
  return true;
}

bool Xcdr2ValueWriter::end_struct()
{
  values_.pop();
  return true;
}

// Write header for the member
bool Xcdr2ValueWriter::begin_struct_member(const DDS::MemberDescriptor& md, DDS::ReturnCode_t rc,
                                           void* member_data)
{
  // Extensiblity of the containing struct.
  const DDS::DynamicType_var type = values_.top()->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (base_type->get_descriptor(td) != DDS::RETCODE_OK) {
    return false;
  }
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();

  // Member properties
  const DDS::MemberId id = md->id();
  const CORBA::Boolean optional = md->is_optional();
  const CORBA::Boolean must_understand = md->is_must_understand() || md->is_key();
  const DDS::DynamicType_var member_type = get_base_type(md->type());
  const DDS::TypeKind member_tk = member_type->get_kind();
  DDS::TypeKind treat_member_as = member_tk;

  if (member_tk == TK_ENUM && enum_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }
  if (member_tk == TK_BITMASK && bitmask_bound(member_type, treat_member_as) != DDS::RETCODE_OK) {
    return false;
  }

  // Write member header
  if (optional && rc == DDS::RETCODE_NO_DATA) {
    if (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE) {
      return writer_ << ACE_OutputCDR::from_boolean(false);
    }
    return true;
  }

  if (optional && (extensibility == DDS::FINAL || extensibility == DDS::APPENDABLE)) {
    return writer_ << ACE_OutputCDR::from_boolean(true);
  } else if (extensibility == DDS::MUTABLE) {
    const Encoding& encoding = writer_.encoding();
    size_t member_size = 0;
    if (is_primitive(treat_member_as)) {
      if (!serialized_size_primitive_value(encoding, member_size, treat_member_as)) {
        return false;
      }
    } else if (member_tk == TK_STRING8) {
      const char* str = (const char*)member_data;
      serialized_size_string_value(encoding, member_size, str);
    }
#ifdef DDS_HAS_WCHAR
    else if (member_tk == TK_STRING16) {
      const CORBA::WChar* wstr = (const CORBA::WChar*)member_data;
      serialized_size_wstring_value(encoding, member_size, wstr);
    }
#endif
    else if (member_tk == TK_STRUCTURE || member_tk == TK_UNION ||
             member_tk == TK_ARRAY || member_tk == TK_SEQUENCE) {
      const DDS::DynamicData_ptr member_dd = (const DDS::DynamicData_ptr)member_data;
      if (!serialized_size(member_size, member_dd)) {
        return false;
      }
    } else {
      return false;
    }
    return ser.write_parameter_id(id, member_size, must_understand);
  }
  return true;
}

bool Xcdr2ValueWriter::end_struct_member()
{
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif /* OPENDDS_DCPS_XCDR2_VALUE_WRITER_H */
