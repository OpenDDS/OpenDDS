#include "DCPS/DdsDcps_pch.h"

#include "LogAddr.h"
#include "Definitions.h"
#include "SafetyProfileStreams.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const size_t LogAddr::BufSize = AddrToStringSize;

const String LogAddr::ip(const ACE_INET_Addr& addr)
{
  char s[BufSize] = {'\0'};
  return String(addr.get_host_addr(s, BufSize));
}

const String LogAddr::port(const ACE_INET_Addr& addr)
{
  return to_dds_string(addr.get_port_number());
}

const String LogAddr::host(const ACE_INET_Addr& addr)
{
  char s[BufSize] = {'\0'};
  addr.get_host_name(s, BufSize);
  return String(s);
}

LogAddr::LogAddr(const ACE_INET_Addr& addr, Option opt)
{
  init_addr(addr, opt);
}

LogAddr::LogAddr(const NetworkAddress& addr, Option opt)
{
  init_addr(addr.to_addr(), opt);
}

void LogAddr::init_addr(const ACE_INET_Addr& addr, Option opt)
{
  switch (opt) {
    case IpPort:
      addr_ = ip(addr) + ':' + port(addr);
      break;
    case HostPort:
      addr_ = host(addr) + ':' + port(addr);
      break;
    case Ip:
      addr_ = ip(addr);
      break;
    case Port:
      addr_ = port(addr);
      break;
    case Host:
      addr_ = host(addr);
      break;
    case IpPortHost:
      addr_ = ip(addr) + ':' + port(addr) + " (" + host(addr) + ')';
    default:
      break;
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
