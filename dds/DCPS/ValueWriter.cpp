/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ValueWriter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef NONNATIVE_LONGDOUBLE
void
ValueWriter::write_float128(long double value)
{
  ACE_CDR::LongDouble ld;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld, value);
  write_float128(ld);
}
#endif

void
ValueWriter::write_boolean_array(const ACE_CDR::Boolean* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_boolean(x[i]);
    end_element();
  }
}

void
ValueWriter::write_byte_array(const ACE_CDR::Octet* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_byte(x[i]);
    end_element();
  }
}

#if OPENDDS_HAS_EXPLICIT_INTS
void
ValueWriter::write_int8_array(const ACE_CDR::Int8* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_int8(x[i]);
    end_element();
  }
}

void
ValueWriter::write_uint8_array(const ACE_CDR::UInt8* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_uint8(x[i]);
    end_element();
  }
}
#endif

void
ValueWriter::write_int16_array(const ACE_CDR::Short* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_int16(x[i]);
    end_element();
  }
}

void
ValueWriter::write_uint16_array(const ACE_CDR::UShort* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_uint16(x[i]);
    end_element();
  }
}

void
ValueWriter::write_int32_array(const ACE_CDR::Long* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_int32(x[i]);
    end_element();
  }
}

void
ValueWriter::write_uint32_array(const ACE_CDR::ULong* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_uint32(x[i]);
    end_element();
  }
}

void
ValueWriter::write_int64_array(const ACE_CDR::LongLong* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_int64(x[i]);
    end_element();
  }
}

void
ValueWriter::write_uint64_array(const ACE_CDR::ULongLong* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_uint64(x[i]);
    end_element();
  }
}

void
ValueWriter::write_float32_array(const ACE_CDR::Float* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_float32(x[i]);
    end_element();
  }
}

void
ValueWriter::write_float64_array(const ACE_CDR::Double* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_float64(x[i]);
    end_element();
  }
}

void
ValueWriter::write_float128_array(const ACE_CDR::LongDouble* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_float128(x[i]);
    end_element();
  }
}

void
ValueWriter::write_char8_array(const ACE_CDR::Char* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_char8(x[i]);
    end_element();
  }
}

void
ValueWriter::write_char16_array(const ACE_CDR::WChar* x, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    begin_element(i);
    write_char16(x[i]);
    end_element();
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
