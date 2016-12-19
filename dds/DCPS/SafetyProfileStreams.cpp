/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OPENDDS_STRING
to_dds_string(::CORBA::UShort to_convert)
{
  const char* fmt = "%hu";
  const int buff_size = 5 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(int to_convert)
{
  const char* fmt = "%d";
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(unsigned int to_convert, bool as_hex)
{
  const char* fmt;
  if (as_hex) {
    fmt = "%02x";
    const int buff_size = 3; // note +1 for null terminator
    char buf[buff_size];
    ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
    return OPENDDS_STRING(buf);
  } else {
    fmt = "%u";
    const int buff_size = 20 + 1; // note +1 for null terminator
    char buf[buff_size];
    ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
    return OPENDDS_STRING(buf);
  }
}

OPENDDS_STRING
to_dds_string(long to_convert)
{
  const char* fmt = "%ld";
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(long long to_convert)
{
  const char* fmt = "%lld";
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(unsigned long long to_convert, bool as_hex)
{
  const char* fmt;
  if (as_hex) {
    fmt = "%0llx";
  } else {
    fmt = "%llu";
  }
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(unsigned long to_convert, bool as_hex)
{
  const char* fmt;
  if (as_hex) {
    fmt = "%0lx";
  } else {
    fmt = "%lu";
  }
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
