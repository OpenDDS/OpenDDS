#include "DCPS/DdsDcps_pch.h"

#include "LogAddr.h"

#include <dds/DCPS/Definitions.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

LogAddr::LogAddr(const ACE_INET_Addr& addr, Fmt fmt)
{
  if (fmt == IP_PORT || fmt == HOST_PORT) {
    ACE_TCHAR buf[AddrToStringSize] = {'\0'};
    if (addr.addr_to_string(buf, AddrToStringSize, fmt == IP_PORT) == 0) {
      addr_ = buf;
    }
  } else if (fmt == IP) {
    addr_ = addr.get_host_addr();
  } else if (fmt == HOST) {
    addr_ = addr.get_host_name();
  } else if (fmt == IP_PORT_HOST) {
    ACE_TCHAR buf[AddrToStringSize] = {'\0'};
    if (addr.addr_to_string(buf, AddrToStringSize) == 0) {
      addr_ = buf;
      addr_ += " (";
      addr_ += addr.get_host_name();
      addr_ += ')';
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
