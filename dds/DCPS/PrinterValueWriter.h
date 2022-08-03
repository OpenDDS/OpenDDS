/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PRINTER_VALUE_WRITER_H
#define OPENDDS_DCPS_PRINTER_VALUE_WRITER_H

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

  void begin_struct();
  void end_struct();
  void begin_struct_member(const DDS::MemberDescriptor& /*descriptor*/);
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

void PrinterValueWriter::begin_struct()
{
  current_indent_ += indent_;
}

void PrinterValueWriter::end_struct()
{
  current_indent_ -= indent_;
}

void PrinterValueWriter::begin_struct_member(const DDS::MemberDescriptor& descriptor)
{
  stream_ << newline() << std::string(current_indent_, ' ') << descriptor.name() << ": ";
  at_newline_ = false;
}

void PrinterValueWriter::end_struct_member()
{}

void PrinterValueWriter::begin_union()
{
  current_indent_ += indent_;
}

void PrinterValueWriter::end_union()
{
  current_indent_ -= indent_;
}

void PrinterValueWriter::begin_discriminator()
{
  stream_ << newline() << std::string(current_indent_, ' ') << "$discriminator: ";
  at_newline_ = false;
}

void PrinterValueWriter::end_discriminator()
{}

void PrinterValueWriter::begin_union_member(const char* name)
{
  stream_ << newline() << std::string(current_indent_, ' ') << name << ": ";
  at_newline_ = false;
}

void PrinterValueWriter::end_union_member()
{}

void PrinterValueWriter::begin_array()
{
  current_indent_ += indent_;
}

void PrinterValueWriter::end_array()
{
  current_indent_ -= indent_;
}

void PrinterValueWriter::begin_sequence()
{
  current_indent_ += indent_;
}

void PrinterValueWriter::end_sequence()
{
  current_indent_ -= indent_;
}

void PrinterValueWriter::begin_element(size_t idx)
{
  stream_ << newline() << std::string(current_indent_, ' ') << '[' << idx << "]: ";
  at_newline_ = false;
}

void PrinterValueWriter::end_element()
{}

void PrinterValueWriter::write_boolean(ACE_CDR::Boolean value)
{
  stream_ << (value ? "true" : "false");
}

void PrinterValueWriter::write_byte(ACE_CDR::Octet value)
{
  stream_ << std::hex << "0x" << static_cast<unsigned int>(value) << std::dec;
}

#if OPENDDS_HAS_EXPLICIT_INTS

void PrinterValueWriter::write_int8(ACE_CDR::Int8 value)
{
  stream_ << static_cast<int>(value);
}

void PrinterValueWriter::write_uint8(ACE_CDR::UInt8 value)
{
  stream_ << static_cast<unsigned int>(value);
}
#endif

void PrinterValueWriter::write_int16(ACE_CDR::Short value)
{
  stream_ << value;
}

void PrinterValueWriter::write_uint16(ACE_CDR::UShort value)
{
  stream_ << value;
}

void PrinterValueWriter::write_int32(ACE_CDR::Long value)
{
  stream_ << value;
}

void PrinterValueWriter::write_uint32(ACE_CDR::ULong value)
{
  stream_ << value;
}

void PrinterValueWriter::write_int64(ACE_CDR::LongLong value)
{
  stream_ << value;
}

void PrinterValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  stream_ << value;
}

void PrinterValueWriter::write_float32(ACE_CDR::Float value)
{
  stream_ << value;
}

void PrinterValueWriter::write_float64(ACE_CDR::Double value)
{
  stream_ << value;
}

void PrinterValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  stream_ << value;
}

void PrinterValueWriter::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  // FUTURE
  stream_ << "fixed";
}

void PrinterValueWriter::write_char8(ACE_CDR::Char value)
{
  char_helper(stream_, value);
}

void PrinterValueWriter::write_char16(ACE_CDR::WChar value)
{
  char_helper(stream_, value);
}

void PrinterValueWriter::write_string(const ACE_CDR::Char* value, size_t length)
{
  string_helper(stream_, value, length);
}

void PrinterValueWriter::write_wstring(const ACE_CDR::WChar* value, size_t length)
{
  string_helper(stream_, value, length);
}

void PrinterValueWriter::write_enum(const char* name,
                                    ACE_CDR::Long value)
{
  stream_ << name << " (" << value << ")";
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_PRINTER_VALUE_WRITER_H */
