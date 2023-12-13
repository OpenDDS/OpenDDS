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
  Serializer& writer_;

  // Hold the dynamic data objects of the top-level type and its nested members recursively.
  // The object on top is the deepest nested member so far and is being operated on.
  std::stack<DDS::DynamicData_ptr> values_;
};

bool Xcdr2ValueWriter::serialized_size_struct(size_t& size)
{
  // TODO(sonndinh): Move serialized_size code from DynamicDataImpl
  return true;
}

// Write header for the struct
bool Xcdr2ValueWriter::begin_struct(DDS::DynamicData_ptr struct_dd)
{
  const DDS::DynamicType_var type = struct_dd->type();
  const DDS::DynamicType_var base_type = get_base_type(type);
  DDS::TypeDescriptor_var td;
  if (!get_type_descriptor(base_type, td)) {
    return false;
  }

  size_t total_size = 0;
  const DDS::ExtensibilityKind extensibility = td->extensibility_kind();
  if (extensibility == DDS::APPENDABLE || extensibility == DDS::MUTABLE) {
    if (!serialized_size_struct(total_size) || !writer_.write_delimiter(total_size)) {
      return false;
    }
  }

  values_.push(struct_dd);
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
    } else if (tk == TK_STRING8) {
      const char* str = (const char*)member_data;
      serialized_size_string_value(encoding, member_size, str);
    }
#ifdef DDS_HAS_WCHAR
    else if (tk == TK_STRING16) {
      const CORBA::WChar* wstr = (const CORBA::WChar*)member_data;
      serialized_size_wstring_value(encoding, member_size, wstr);
    }
#endif
    else if (tk == TK_STRUCTURE || tk == TK_UNION || tk == TK_ARRAY || tk == TK_SEQUENCE) {
      const DDS::DynamicData_ptr member_dd = (const DDS::DynamicData_ptr)member_data;
      if (!serialized_size_i(encoding, member_size, member_dd)) {
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
