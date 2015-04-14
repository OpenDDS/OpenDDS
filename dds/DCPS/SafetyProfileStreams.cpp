/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/SafetyProfileStreams.h"

namespace OpenDDS {
namespace DCPS {

OPENDDS_STRING
to_dds_string(u_short to_convert)
{
  const char* fmt = "%hu";
  int buff_size = size_buffer(fmt, to_convert);
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(int to_convert)
{
  const char* fmt = "%d";
  int buff_size = size_buffer(fmt, to_convert);
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(unsigned int to_convert, bool as_hex)
{
  const char* fmt;
  int buff_size;
  if (as_hex) {
    fmt = "%02x";
    buff_size = 3; // note +1 for null terminator
  } else {
    fmt = "%u";
    buff_size = size_buffer(fmt, to_convert);
  }
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING
to_dds_string(long to_convert)
{
  const char* fmt = "%ld";
  int buff_size = size_buffer(fmt, to_convert);
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
  int buff_size = size_buffer(fmt, to_convert);
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

} // namespace DCPS
} // namespace OpenDDS
