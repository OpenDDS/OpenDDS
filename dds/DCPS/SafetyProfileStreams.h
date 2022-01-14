#ifndef OPENDDS_DCPS_SAFETYPROFILESTREAMS_H
#define OPENDDS_DCPS_SAFETYPROFILESTREAMS_H

#include "dcps_export.h"
#include "PoolAllocator.h"

#include <ace/INET_Addr.h>
#include <ace/OS_NS_stdio.h>

#ifdef OPENDDS_SAFETY_PROFILE
#  include <cstdlib> // For strto*
#else
#  include <fstream>
#  include <iostream>
#  include <iomanip>
#  include <sstream>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export String to_dds_string(unsigned short to_convert);
OpenDDS_Dcps_Export String to_dds_string(int to_convert);
OpenDDS_Dcps_Export String to_dds_string(unsigned int to_convert, bool as_hex = false);
OpenDDS_Dcps_Export String to_dds_string(long to_convert);
OpenDDS_Dcps_Export String to_dds_string(long long to_convert);
OpenDDS_Dcps_Export String to_dds_string(unsigned long long to_convert, bool as_hex = false);
OpenDDS_Dcps_Export String to_dds_string(unsigned long to_convert, bool as_hex = false);
OpenDDS_Dcps_Export String to_dds_string(const unsigned char* array, size_t length);
// Pass-through for conditional compilation situtations, i.e., type may be an integer or string.
inline String to_dds_string(const String& to_convert)
{
  return to_convert;
}

//@{
/**
 * Converts a series of bytes at data to a optionally delimited String
 * of hexadecimal numbers.
 *
 * If delim is '\0' (the default) or delim_every is 0, then the output will not
 * be delimited.
 */
OpenDDS_Dcps_Export String to_hex_dds_string(
  const unsigned char* data, size_t size, char delim = '\0', size_t delim_every = 1);
OpenDDS_Dcps_Export String to_hex_dds_string(
  const char* data, size_t size, char delim = '\0', size_t delim_every = 1);
//@}

/// Convert Pointer to String
template <typename T>
inline
String to_dds_string(const T* to_convert)
{
  const char* fmt = "%p";
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return String(buf);
}

/**
 * Convert string s to value of integral type T.
 *
 * Returns true for success, false for error
 */
template <typename T>
bool convertToInteger(const String& s, T& value)
{
#ifdef OPENDDS_SAFETY_PROFILE
  char* end;
  const long conv = std::strtol(s.c_str(), &end, 10);
  if (end == s.c_str()) return false;
  value = static_cast<T>(conv);
#else
  std::stringstream istr(s.c_str());
  if (!(istr >> value) || (istr.peek() != EOF)) return false;
#endif
  return true;
}

/**
 * Convert string s to value of double type T.
 *
 * Returns true for success, false for error
 */
template <typename T>
bool convertToDouble(const String& s, T& value)
{
#ifdef OPENDDS_SAFETY_PROFILE
  char* end;
  const double conv = std::strtod(s.c_str(), &end);
  if (end == s.c_str()) return false;
  value = static_cast<T>(conv);
#else
  std::stringstream istr(s.c_str());
  if (!(istr >> value) || (istr.peek() != EOF)) return false;
#endif
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_SAFETYPROFILESTREAMS_H
