#ifndef SAFETY_PROFILE_STREAMS_H
#define SAFETY_PROFILE_STREAMS_H

#include "dds/DCPS/PoolAllocator.h"
#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include "ace/OS_NS_stdio.h"
#include "tao/Basic_Types.h"

#ifndef OPENDDS_SAFETY_PROFILE
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#endif //OPENDDS_SAFETY_PROFILE

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(::CORBA::UShort to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(int to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(unsigned int to_convert, bool as_hex = false);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(long to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(long long to_convert);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(unsigned long long to_convert, bool as_hex = false);
OpenDDS_Dcps_Export OPENDDS_STRING to_dds_string(unsigned long to_convert, bool as_hex = false);

OpenDDS_Dcps_Export const char* retcode_to_string(DDS::ReturnCode_t value);

//@{
/**
 * Converts a series of bytes at data to a optionally delimited OPENDDS_STRING
 * of hexadecimal numbers.
 *
 * If delim is '\0' (the default) or delim_every is 0, then the output will not
 * be delimited.
 */
OpenDDS_Dcps_Export OPENDDS_STRING to_hex_dds_string(
  const unsigned char* data, size_t size, char delim = '\0', size_t delim_every = 1);
OpenDDS_Dcps_Export OPENDDS_STRING to_hex_dds_string(
  const char* data, size_t size, char delim = '\0', size_t delim_every = 1);
//@}

/// Convert Pointer to OPENDDS_STRING
template <typename T>
inline OPENDDS_STRING
to_dds_string(const T* to_convert)
{
  const char* fmt = "%p";
  const int buff_size = 20 + 1; // note +1 for null terminator
  char buf[buff_size];
  ACE_OS::snprintf(&buf[0], buff_size, fmt, to_convert);
  return OPENDDS_STRING(buf);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif //SAFETY_PROFILE_STREAMS_H
