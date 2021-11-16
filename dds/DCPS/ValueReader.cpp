/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ValueReader.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef NONNATIVE_LONGDOUBLE
bool
ValueReader::read_float128(long double& value)
{
  ACE_CDR::LongDouble ld;
  if (!read_float128(ld)) {
    return false;
  }
  value = ld;
  return true;
}
#endif

bool
ValueReader::read_boolean_array(ACE_CDR::Boolean* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_boolean(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_byte_array(ACE_CDR::Octet* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_byte(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

#if OPENDDS_HAS_EXPLICIT_INTS
bool
ValueReader::read_int8_array(ACE_CDR::Int8* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_int8(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_uint8_array(ACE_CDR::UInt8* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_uint8(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}
#endif

bool
ValueReader::read_int16_array(ACE_CDR::Short* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_int16(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_uint16_array(ACE_CDR::UShort* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_uint16(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_int32_array(ACE_CDR::Long* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_int32(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_uint32_array(ACE_CDR::ULong* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_uint32(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_int64_array(ACE_CDR::LongLong* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_int64(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_uint64_array(ACE_CDR::ULongLong* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_uint64(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_float32_array(ACE_CDR::Float* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_float32(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_float64_array(ACE_CDR::Double* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_float64(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_float128_array(ACE_CDR::LongDouble* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_float128(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_char8_array(ACE_CDR::Char* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_char8(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

bool
ValueReader::read_char16_array(ACE_CDR::WChar* value, size_t length)
{
  for (size_t i = 0; i != length; ++i) {
    if (!begin_element()) return false;
    if (!read_char16(value[i])) return false;
    if (!end_element()) return false;
  }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
