/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "dds/DCPS/SafetyProfileStreams.h"

#ifdef OPENDDS_SECURITY
#  include "dds/DdsSecurityCoreC.h"
#endif

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
    fmt = "%0.8lx";
  } else {
    fmt = "%lu";
  }
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

OPENDDS_STRING to_hex_dds_string(
  const unsigned char* data, const size_t size, const char delim, const size_t delim_every)
{
  return to_hex_dds_string(reinterpret_cast<const char*>(data), size, delim, delim_every);
}

static inline char nibble_to_hex_char(char nibble)
{
  nibble &= 0x0F;
  return ((nibble < 0xA) ? '0' : ('a' - 0xA)) + nibble;
}

OPENDDS_STRING to_hex_dds_string(
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

  OPENDDS_STRING rv;
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

const char* retcode_to_string(DDS::ReturnCode_t value)
{
  switch (value) {
  case DDS::RETCODE_OK:
    return "OK";
  case DDS::RETCODE_ERROR:
    return "Error";
  case DDS::RETCODE_UNSUPPORTED:
    return "Unsupported";
  case DDS::RETCODE_BAD_PARAMETER:
    return "Bad parameter";
  case DDS::RETCODE_PRECONDITION_NOT_MET:
    return "Precondition not met";
  case DDS::RETCODE_OUT_OF_RESOURCES:
    return "Out of resources";
  case DDS::RETCODE_NOT_ENABLED:
    return "Not enabled";
  case DDS::RETCODE_IMMUTABLE_POLICY:
    return "Immutable policy";
  case DDS::RETCODE_INCONSISTENT_POLICY:
    return "Inconsistent policy";
  case DDS::RETCODE_ALREADY_DELETED:
    return "Already deleted";
  case DDS::RETCODE_TIMEOUT:
    return "Timeout";
  case DDS::RETCODE_NO_DATA:
    return "No data";
  case DDS::RETCODE_ILLEGAL_OPERATION:
    return "Illegal operation";
#ifdef OPENDDS_SECURITY
  case DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY:
    return "Not allowed by security";
#endif
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: retcode_to_string: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));
    return "Invalid return code";
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
