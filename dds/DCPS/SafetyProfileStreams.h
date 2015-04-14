#ifndef SAFETY_PROFILE_STREAMS_H
#define SAFETY_PROFILE_STREAMS_H

#include "dds/DCPS/PoolAllocator.h"
#include "dcps_export.h"


#ifndef OPENDDS_SAFETY_PROFILE
#include <fstream>
#include <iostream>
#include <iomanip>

#ifdef ACE_LYNXOS_MAJOR
#include <strstream>
#define STRINGSTREAM std::strstream
#define STRINGSTREAM_CSTR
#else
#include <sstream>
#define STRINGSTREAM std::stringstream
#define STRINGSTREAM_CSTR .c_str()
#endif

#endif //OPENDDS_SAFETY_PROFILE

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(u_short to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(int to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(unsigned int to_convert, bool as_hex = false);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(long to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(unsigned long to_convert, bool as_hex = false);

template <typename T>
OpenDDS_Dcps_Export int size_buffer(const char* fmt, const T size_of)
{
  int sz = ACE_OS::snprintf(NULL, 0, fmt, size_of);
  return sz + 1; // note +1 for null terminator
}

template <typename T>
OpenDDS_Dcps_Export OPENDDS_STRING
to_dds_string (const T* to_convert)
{
  const char* fmt = "%p";
  int buff_size = size_buffer(fmt, to_convert);
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

} // namespace DCPS
} // namespace OpenDDS

#endif //SAFETY_PROFILE_STREAMS_H
