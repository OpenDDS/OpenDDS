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
bool
ValueWriter::write_float128(long double value)
{
  ACE_CDR::LongDouble ld;
  ACE_CDR_LONG_DOUBLE_ASSIGNMENT(ld, value);
  return write_float128(ld);
}
#endif

bool
ValueWriter::write_boolean_array(const ACE_CDR::Boolean* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_boolean(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_byte_array(const ACE_CDR::Octet* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_byte(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

#if OPENDDS_HAS_EXPLICIT_INTS
bool
ValueWriter::write_int8_array(const ACE_CDR::Int8* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_int8(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_uint8_array(const ACE_CDR::UInt8* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_uint8(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}
#endif

bool
ValueWriter::write_int16_array(const ACE_CDR::Short* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_int16(x[i])||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_uint16_array(const ACE_CDR::UShort* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_uint16(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_int32_array(const ACE_CDR::Long* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_int32(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_uint32_array(const ACE_CDR::ULong* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_uint32(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_int64_array(const ACE_CDR::LongLong* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_int64(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_uint64_array(const ACE_CDR::ULongLong* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_uint64(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_float32_array(const ACE_CDR::Float* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_float32(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_float64_array(const ACE_CDR::Double* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_float64(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_float128_array(const ACE_CDR::LongDouble* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_float128(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_char8_array(const ACE_CDR::Char* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_char8(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

bool
ValueWriter::write_char16_array(const ACE_CDR::WChar* x, ACE_CDR::ULong length)
{
  for (ACE_CDR::ULong i = 0; i != length; ++i) {
    if (!begin_element(i) ||
        !write_char16(x[i]) ||
        !end_element()) {
      return false;
    }
  }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
