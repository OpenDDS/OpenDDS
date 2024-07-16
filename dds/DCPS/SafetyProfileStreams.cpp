/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "SafetyProfileStreams.h"

#include "Definitions.h"

#include <ace/OS_NS_stdio.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

String to_dds_string(ACE_CDR::Octet to_convert, bool as_hex)
{
  static const int buff_size = 3 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, as_hex ? "%02x" : "%u", to_convert);
  return String(buf);
}

String to_dds_string(unsigned short to_convert, bool as_hex)
{
  static const int buff_size = 5 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, as_hex ? "%04hx" : "%hu", to_convert);
  return String(buf);
}

String to_dds_string(int to_convert)
{
  static const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%d", to_convert);
  return String(buf);
}

String to_dds_string(unsigned int to_convert, bool as_hex)
{
  static const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, as_hex ? "%08x" : "%u", to_convert);
  return String(buf);
}

String to_dds_string(long to_convert)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%ld", to_convert);
  return String(buf);
}

String to_dds_string(long long to_convert)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%lld", to_convert);
  return String(buf);
}

String to_dds_string(unsigned long long to_convert, bool as_hex)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, as_hex ? "%016llx" : "%llu", to_convert);
  return String(buf);
}

String to_dds_string(unsigned long to_convert)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%lu", to_convert);
  return String(buf);
}

String to_dds_string(const unsigned char* array, size_t length)
{
  static const size_t bytes_per_elt = 5; // largest byte as decimal plus comma and space
  String ret(length * bytes_per_elt, '\0');
  char* buf = &ret[0];
  size_t total_used = 0;
  for (size_t i = 0; i < length; ++i) {
    const int used = ACE_OS::snprintf(buf, bytes_per_elt + 1,
                                      i < length - 1 ? "%d, " : "%d", array[i]);
    if (used < 1) {
      return "";
    }
    buf += used;
    total_used += static_cast<size_t>(used);
  }

  ret.resize(total_used);
  return ret;
}

OpenDDS_Dcps_Export String to_dds_string(double to_convert)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%g", to_convert);
  return String(buf);
}

String to_dds_string(const void* to_convert)
{
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, "%p", to_convert);
  return String(buf);
}

String to_hex_dds_string(
  const unsigned char* data, const size_t size, const char delim, const size_t delim_every)
{
  return to_hex_dds_string(reinterpret_cast<const char*>(data), size, delim, delim_every);
}

static inline char nibble_to_hex_char(char nibble)
{
  nibble &= 0x0F;
  return ((nibble < 0xA) ? '0' : ('a' - 0xA)) + nibble;
}

String to_hex_dds_string(
  const char* data, size_t size, const char delim, const size_t delim_every)
{
  const bool valid_delim = delim && delim_every;
  size_t l = size * 2;
  if (valid_delim && size > 1) {
    l += size / delim_every;
    if (!(size % delim_every)) {
      l--;
    }
  }

  String rv;
  rv.reserve(l);
  for (size_t i = 0; i < size; i++) {
    if (valid_delim && i && !(i % delim_every)) {
      rv.push_back(delim);
    }
    rv.push_back(nibble_to_hex_char(data[i] >> 4));
    rv.push_back(nibble_to_hex_char(data[i]));
  }
  return rv;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
