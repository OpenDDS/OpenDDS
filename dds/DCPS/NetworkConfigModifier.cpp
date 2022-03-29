/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigModifier.h"

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER

#include "debug.h"

#include <ace/OS_NS_sys_socket.h>

#include <ace/os_include/os_ifaddrs.h>

#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool NetworkConfigModifier::open()
{
  update_interfaces();
  return true;
}

bool NetworkConfigModifier::close()
{
  NetworkConfigMonitor::clear();
  return true;
}

void NetworkConfigModifier::update_interfaces()
{
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) NetworkConfigModifier::update_interfaces: enumerating interfaces.\n"));
  }

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  if (::getifaddrs(&p_ifa) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: NetworkConfigModifier::update_interfaces: %p\n",
                 "getifaddrs error"));
    }

    return;
  }

  // Using logic from ACE::get_ip_interfaces_getifaddrs
  // but need ifa_name which is not returned by it
  List nia_list;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next) {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in* addr = reinterpret_cast<sockaddr_in*> (p_if->ifa_addr);

      // Sometimes the kernel returns 0.0.0.0 as the interface
      // address, skip those...
      if (addr->sin_addr.s_addr != INADDR_ANY) {
        ACE_INET_Addr address;
        address.set((u_short) 0, addr->sin_addr.s_addr, 0);

        nia_list.push_back(NetworkInterfaceAddress(p_if->ifa_name, p_if->ifa_flags & (IFF_MULTICAST | IFF_LOOPBACK), NetworkAddress(address)));
      }
    }
# if defined (ACE_HAS_IPV6)
    else if (p_if->ifa_addr->sa_family == AF_INET6) {
      struct sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (p_if->ifa_addr);

      // Skip the ANY address
      if (!IN6_IS_ADDR_UNSPECIFIED(&addr->sin6_addr)) {
        ACE_INET_Addr address;
        address.set(reinterpret_cast<struct sockaddr_in *> (addr), sizeof(sockaddr_in6));

        nia_list.push_back(NetworkInterfaceAddress(ACE_OS::if_nametoindex(p_if->ifa_name), p_if->ifa_name, p_if->ifa_flags & (IFF_MULTICAST | IFF_LOOPBACK), address));
      }
    }
# endif /* ACE_HAS_IPV6 */
  }

  ::freeifaddrs (p_ifa);

  NetworkConfigMonitor::set(nia_list);
}

void NetworkConfigModifier::add_interface(const OPENDDS_STRING&)
{
  // This is a no-op.
}

void NetworkConfigModifier::remove_interface(const OPENDDS_STRING& name)
{
  NetworkConfigMonitor::remove_interface(name);
}

void NetworkConfigModifier::add_address(const OPENDDS_STRING& name,
                                        bool can_multicast,
                                        const ACE_INET_Addr& addr)
{
  NetworkConfigMonitor::set(NetworkInterfaceAddress(name, can_multicast, NetworkAddress(addr)));
}

void NetworkConfigModifier::remove_address(const OPENDDS_STRING& name, const ACE_INET_Addr& addr)
{
  NetworkConfigMonitor::remove_address(name, NetworkAddress(addr));
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NETWORK_CONFIG_MONITOR
