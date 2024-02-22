/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PRINTER_VALUE_WRITER_H
#define OPENDDS_DCPS_PRINTER_VALUE_WRITER_H

#ifndef OPENDDS_SAFETY_PROFILE

#include "ValueWriter.h"
#include "ValueHelper.h"
#include "dcps_export.h"
#include "Definitions.h"

#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsTopicC.h>

#include <iosfwd>
#include <sstream>
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Convert values to a readable format.
class PrinterValueWriter : public ValueWriter {
public:
  explicit PrinterValueWriter(unsigned int a_indent = TheServiceParticipant->printer_value_writer_indent())
    : indent_(a_indent)
    , current_indent_(0)
    , at_newline_(true)
  {}

  bool begin_struct(Extensibility extensibility = FINAL);
  bool end_struct();
  bool begin_struct_member(MemberParam params);
  bool end_struct_member();

  bool begin_union(Extensibility extensibility = FINAL);
  bool end_union();
  bool begin_discriminator(MemberParam params);
  bool end_discriminator();
  bool begin_union_member(MemberParam params);
  bool end_union_member();

  bool begin_array(XTypes::TypeKind elem_tk = XTypes::TK_NONE);
  bool end_array();
  bool begin_sequence(XTypes::TypeKind elem_tk = XTypes::TK_NONE, ACE_CDR::ULong length = 0);
  bool end_sequence();
  bool begin_element(ACE_CDR::ULong idx);
  bool end_element();

  bool write_boolean(ACE_CDR::Boolean value);
  bool write_byte(ACE_CDR::Octet value);
#if OPENDDS_HAS_EXPLICIT_INTS
  bool write_int8(ACE_CDR::Int8 value);
  bool write_uint8(ACE_CDR::UInt8 value);
#endif
  bool write_int16(ACE_CDR::Short value);
  bool write_uint16(ACE_CDR::UShort value);
  bool write_int32(ACE_CDR::Long value);
  bool write_uint32(ACE_CDR::ULong value);
  bool write_int64(ACE_CDR::LongLong value);
  bool write_uint64(ACE_CDR::ULongLong value);
  bool write_float32(ACE_CDR::Float value);
  bool write_float64(ACE_CDR::Double value);
  bool write_float128(ACE_CDR::LongDouble value);
  bool write_fixed(const ACE_CDR::Fixed& value);
  bool write_char8(ACE_CDR::Char value);
  bool write_char16(ACE_CDR::WChar value);
  bool write_string(const ACE_CDR::Char* value, size_t length);
  bool write_wstring(const ACE_CDR::WChar* value, size_t length);
  bool write_enum(ACE_CDR::Long value, const EnumHelper& helper);
  bool write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper);
  bool write_absent_value();

  std::string str() const
  {
    return stream_.str();
  }

private:
  std::string newline()
  {
    if (at_newline_) {
      return "";
    } else {
      at_newline_ = false;
      return "\n";
    }
  }

  const unsigned int indent_;
  std::stringstream stream_;
  unsigned int current_indent_;
  bool at_newline_;
};

bool PrinterValueWriter::begin_struct(Extensibility /*extensibility*/)
{
  current_indent_ += indent_;
  return true;
}

bool PrinterValueWriter::end_struct()
{
  current_indent_ -= indent_;
  return true;
}

bool PrinterValueWriter::begin_struct_member(MemberParam params)
{
  stream_ << newline() << std::string(current_indent_, ' ') << params.name << ": ";
  at_newline_ = false;
  return true;
}

bool PrinterValueWriter::end_struct_member()
{
  return true;
}

bool PrinterValueWriter::begin_union(Extensibility /*extensibility*/)
{
  current_indent_ += indent_;
  return true;
}

bool PrinterValueWriter::end_union()
{
  current_indent_ -= indent_;
  return true;
}

bool PrinterValueWriter::begin_discriminator(MemberParam /*params*/)
{
  stream_ << newline() << std::string(current_indent_, ' ') << "$discriminator: ";
  at_newline_ = false;
  return true;
}

bool PrinterValueWriter::end_discriminator()
{
  return true;
}

bool PrinterValueWriter::begin_union_member(MemberParam params)
{
  stream_ << newline() << std::string(current_indent_, ' ') << params.name << ": ";
  at_newline_ = false;
  return true;
}

bool PrinterValueWriter::end_union_member()
{
  return true;
}

bool PrinterValueWriter::begin_array(XTypes::TypeKind /*elem_tk*/)
{
  current_indent_ += indent_;
  return true;
}

bool PrinterValueWriter::end_array()
{
  current_indent_ -= indent_;
  return true;
}

bool PrinterValueWriter::begin_sequence(XTypes::TypeKind /*elem_tk*/, ACE_CDR::ULong /*length*/)
{
  current_indent_ += indent_;
  return true;
}

bool PrinterValueWriter::end_sequence()
{
  current_indent_ -= indent_;
  return true;
}

bool PrinterValueWriter::begin_element(ACE_CDR::ULong idx)
{
  stream_ << newline() << std::string(current_indent_, ' ') << '[' << idx << "]: ";
  at_newline_ = false;
  return true;
}

bool PrinterValueWriter::end_element()
{
  return true;
}

bool PrinterValueWriter::write_boolean(ACE_CDR::Boolean value)
{
  stream_ << (value ? "true" : "false");
  return true;
}

bool PrinterValueWriter::write_byte(ACE_CDR::Octet value)
{
  stream_ << std::hex << "0x" << static_cast<unsigned int>(value) << std::dec;
  return true;
}

#if OPENDDS_HAS_EXPLICIT_INTS

bool PrinterValueWriter::write_int8(ACE_CDR::Int8 value)
{
  stream_ << static_cast<int>(value);
  return true;
}

bool PrinterValueWriter::write_uint8(ACE_CDR::UInt8 value)
{
  stream_ << static_cast<unsigned int>(value);
  return true;
}
#endif

bool PrinterValueWriter::write_int16(ACE_CDR::Short value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_uint16(ACE_CDR::UShort value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_int32(ACE_CDR::Long value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_uint32(ACE_CDR::ULong value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_int64(ACE_CDR::LongLong value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_float32(ACE_CDR::Float value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_float64(ACE_CDR::Double value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  stream_ << value;
  return true;
}

bool PrinterValueWriter::write_fixed(const ACE_CDR::Fixed& value)
{
  char buffer[ACE_CDR::Fixed::MAX_STRING_SIZE];
  if (value.to_string(buffer, sizeof buffer)) {
    stream_ << buffer;
  }
  return true;
}

bool PrinterValueWriter::write_char8(ACE_CDR::Char value)
{
  char_helper(stream_, value);
  return true;
}

bool PrinterValueWriter::write_char16(ACE_CDR::WChar value)
{
  char_helper(stream_, value);
  return true;
}

bool PrinterValueWriter::write_string(const ACE_CDR::Char* value, size_t length)
{
  string_helper(stream_, value, length);
  return true;
}

bool PrinterValueWriter::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  string_helper(stream_, value, length);
  return true;
}

bool PrinterValueWriter::write_enum(ACE_CDR::Long value, const EnumHelper& helper)
{
  const char* name = 0;
  if (helper.get_name(name, value)) {
    stream_ << name << " (" << value << ")";
    return true;
  }
  return false;
}

bool PrinterValueWriter::write_bitmask(ACE_CDR::ULongLong value, const BitmaskHelper& helper)
{
  stream_ << bitmask_to_string(value, helper);
  return true;
}

bool PrinterValueWriter::write_absent_value()
{
  stream_ << "null";
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

#endif  /* OPENDDS_DCPS_PRINTER_VALUE_WRITER_H */
