#include "DCPS/DdsDcps_pch.h"

#include "LogAddr.h"
#include "Definitions.h"
#include <ace/OS_NS_stdio.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

LogAddr::LogAddr(const ACE_INET_Addr& addr, Fmt fmt)
{
  if (fmt == IP_PORT) {
    char buf[AddrToStringSize] = {'\0'};
    ACE_OS::snprintf(buf, AddrToStringSize, "%s:%d", addr.get_host_addr(), addr.get_port_number());
    addr_ = buf;
  } else if (fmt == HOST_PORT) {
    char buf[AddrToStringSize] = {'\0'};
    ACE_OS::snprintf(buf, AddrToStringSize, "%s:%d", addr.get_host_name(), addr.get_port_number());
    addr_ = buf;
  } else if (fmt == IP) {
    addr_ = addr.get_host_addr();
  } else if (fmt == HOST) {
    addr_ = addr.get_host_name();
  } else if (fmt == IP_PORT_HOST) {
    char buf[AddrToStringSize * 2] = {'\0'};
    ACE_OS::snprintf(buf, AddrToStringSize * 2, "%s:%d (%s)",
      addr.get_host_addr(), addr.get_port_number(), addr.get_host_name());
    addr_ = buf;
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
