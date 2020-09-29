/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "JsonValueWriter.h"

#include <cmath>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Test {

void JsonValueWriter::begin_struct()
{
  out_ << '{';
  value_count_.push_back(0);
}

void JsonValueWriter::end_struct()
{
  out_ << '}';
  value_count_.pop_back();
}

void JsonValueWriter::begin_union()
{
  out_ << '{';
  value_count_.push_back(0);
}

void JsonValueWriter::end_union()
{
  out_ << '}';
  value_count_.pop_back();
}

void JsonValueWriter::begin_discriminator()
{
  if (value_count_.back() != 0) {
    out_ << ',';
  }
  out_ << "\"$discriminator\":";
}

void JsonValueWriter::end_discriminator()
{
  ++value_count_.back();
}

void JsonValueWriter::begin_field(const char* name)
{
  if (value_count_.back() != 0) {
    out_ << ',';
  }
  // TODO: Escape.
  out_ << '"' << name << "\":";
}

void JsonValueWriter::end_field()
{
  ++value_count_.back();
}

void JsonValueWriter::begin_array()
{
  out_ << '[';
  value_count_.push_back(0);
}

void JsonValueWriter::end_array()
{
  out_ << ']';
  value_count_.pop_back();
}

void JsonValueWriter::begin_sequence()
{
  out_ << '[';
  value_count_.push_back(0);
}

void JsonValueWriter::end_sequence()
{
  out_ << ']';
  value_count_.pop_back();
}

void JsonValueWriter::begin_element(size_t /*idx*/)
{
  if (value_count_.back() != 0) {
    out_ << ',';
  }
}

void JsonValueWriter::end_element()
{
  ++value_count_.back();
}

void JsonValueWriter::write_boolean(ACE_CDR::Boolean value)
{
  out_ << (value ? "true" : "false");
}

void JsonValueWriter::write_byte(ACE_CDR::Octet value)
{
  out_ << static_cast<unsigned int>(value);
}

void JsonValueWriter::write_int8(ACE_CDR::Char value)
{
  out_ << static_cast<int>(value);
}

void JsonValueWriter::write_uint8(ACE_CDR::Octet value)
{
  out_ << static_cast<unsigned int>(value);
}

void JsonValueWriter::write_int16(ACE_CDR::Short value)
{
  out_ << static_cast<int>(value);
}

void JsonValueWriter::write_uint16(ACE_CDR::UShort value)
{
  out_ << static_cast<unsigned int>(value);
}

void JsonValueWriter::write_int32(ACE_CDR::Long value)
{
  out_ << value;
}

void JsonValueWriter::write_uint32(ACE_CDR::ULong value)
{
  out_ << value;
}

void JsonValueWriter::write_int64(ACE_CDR::LongLong value)
{
  if (value >= -9007199254740991 && value <= 9007199254740991) {
    out_ << value;
  } else {
    out_ << '"' << value << '"';
  }
}

void JsonValueWriter::write_uint64(ACE_CDR::ULongLong value)
{
  if (value <= 9007199254740991) {
    out_ << value;
  } else {
    out_ << '"' << value << '"';
  }
}

void JsonValueWriter::write_float32(ACE_CDR::Float value)
{
  // TODO: Formatting?
  if (isinf(value)) {
    if (value < 0) {
      out_ << "\"-inf\"";
    } else {
      out_ << "\"inf\"";
    }
  } else if(isnan(value)) {
    out_ << "\"nan\"";
  } else {
    out_ << value;
  }
}

void JsonValueWriter::write_float64(ACE_CDR::Double value)
{
  // TODO: Formatting?
  if (isinf(value)) {
    if (value < 0) {
      out_ << "\"-inf\"";
    } else {
      out_ << "\"inf\"";
    }
  } else if(isnan(value)) {
    out_ << "\"nan\"";
  } else {
    out_ << value;
  }
}

void JsonValueWriter::write_float128(ACE_CDR::LongDouble value)
{
  // TODO: Formatting?
  // TODO: Base64 encode.
  if (isinf(value)) {
    if (value < 0) {
      out_ << "\"-inf\"";
    } else {
      out_ << "\"inf\"";
    }
  } else if(isnan(value)) {
    out_ << "\"nan\"";
  } else {
    out_ << value;
  }
}

void JsonValueWriter::write_fixed(const OpenDDS::FaceTypes::Fixed& /*value*/)
{
  // TODO
  out_ << "\"fixed\"";
}

void JsonValueWriter::write_char8(ACE_CDR::Char value)
{
  // TODO; Escape.
  out_ << '"' << value << '"';
}

void JsonValueWriter::write_char16(ACE_CDR::WChar value)
{
  // TODO; Escape.
  // TODO: Encode.
  out_ << '"' << value << '"';
}

void JsonValueWriter::write_string(const ACE_CDR::Char* value)
{
  // TODO; Escape.
  out_ << '"' << value << '"';
}

void JsonValueWriter::write_wstring(const ACE_CDR::WChar* value)
{
  // TODO; Escape.
  // TODO: Encode.
  out_ << '"' << value << '"';
}

void JsonValueWriter::write_enum(const char* name,
                                 ACE_CDR::Long /*value*/)
{
  // TODO; Escape.
  out_ << '"' << name << '"';
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
