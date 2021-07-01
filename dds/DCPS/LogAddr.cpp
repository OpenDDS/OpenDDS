#include "DCPS/DdsDcps_pch.h"

#include "LogAddr.h"

#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

LogAddr::LogAddr(const ACE_INET_Addr& addr, Fmt fmt)
{
  std::stringstream s;
  if (fmt == IP) {
    s << addr.get_host_addr();
  } else if (fmt == IP_PORT) {
    s << addr.get_host_addr() << ':' << addr.get_port_number();
  } else if (fmt == HOST) {
    s << addr.get_host_name();
  } else if (fmt == HOST_PORT) {
    s << addr.get_host_name() << ':' << addr.get_port_number();
  } else if (fmt == IP_PORT_HOST) {
    s << addr.get_host_addr() << ':' << addr.get_port_number() << " (" << addr.get_host_name() << ')';
  }
  addr_ = s.str();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
