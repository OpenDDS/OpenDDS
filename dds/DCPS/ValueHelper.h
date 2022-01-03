/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_VALUE_HELPER_H
#define OPENDDS_DCPS_VALUE_HELPER_H

#include "RestoreOutputStreamState.h"
#include "Definitions.h"

#include <ostream>
#include <iostream>
#include <iomanip>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

template <typename IntType>
std::ostream& signed_int_helper(std::ostream& o, IntType value, IntType min)
{
  /*
   * It seems that in C/C++ the minus sign and the bare number are parsed
   * separately for negative integer literals. This can cause compilers
   * to complain when using the minimum value of a signed integer because
   * the number without the minus sign is 1 past the max signed value.
   *
   * https://stackoverflow.com/questions/65007935
   *
   * Apparently the workaround is to write it as `VALUE_PLUS_ONE - 1`.
   */
  const bool min_value = value == min;
  if (min_value) ++value;
  o << value;
  if (min_value) o << " - 1";
  return o;
}

#if ACE_SIZEOF_LONG_DOUBLE != 16
inline ostream& operator<<(ostream& os, const ACE_CDR::LongDouble& val)
{
  os << ACE_CDR::LongDouble::NativeImpl(val);
  return os;
}
#endif

inline
std::ostream& hex_value(std::ostream& o, unsigned value, size_t bytes)
{
  OpenDDS::DCPS::RestoreOutputStreamState ross(o);
  o << std::hex << std::setw(bytes * 2) << std::setfill('0') << value;
  return o;
}

template <typename CharType>
unsigned char_value(CharType value)
{
  return value;
}

#if CHAR_MIN < 0
/*
 * If char is signed, then it needs to be reinterpreted as unsigned char or
 * else static casting '\xff' to a 32-bit unsigned int would result in
 * 0xffffffff because those are both the signed 2's complement forms of -1.
 */
template <>
inline unsigned char_value<char>(char value)
{
  return reinterpret_cast<unsigned char&>(value);
}
#endif

template <typename CharType>
std::ostream& char_helper(std::ostream& o, CharType value)
{
  switch (value) {
  case '\'':
  case '\"':
  case '\\':
  case '\?':
    return o << '\\' << static_cast<char>(value);
  case '\n':
    return o << "\\n";
  case '\t':
    return o << "\\t";
  case '\v':
    return o << "\\v";
  case '\b':
    return o << "\\b";
  case '\r':
    return o << "\\r";
  case '\f':
    return o << "\\f";
  case '\a':
    return o << "\\a";
  }
  const unsigned cvalue = char_value(value);
  if (cvalue <= UCHAR_MAX && isprint(cvalue)) {
    return o << static_cast<char>(value);
  }
  return hex_value(o << "\\x", cvalue, sizeof(CharType) == 1 ? 1 : 2);
}

template <typename CharType>
std::ostream& string_helper(std::ostream& o, const CharType* value)
{
  for (size_t i = 0; value[i] != 0; ++i) {
    char_helper<CharType>(o, value[i]);
  }
  return o;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_VALUE_HELPER_H */
