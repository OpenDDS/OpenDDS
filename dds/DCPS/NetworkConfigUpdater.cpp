/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ace/config.h"
#include "ace/os_include/os_ifaddrs.h"

#include "NetworkConfigUpdater.h"

#ifdef OPENDDS_NETWORK_CONFIG_UPDATER

#include <ace/Netlink_Addr.h>
#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

NetworkConfigUpdater::NetworkConfigUpdater()
{

}

bool NetworkConfigUpdater::open()
{
  ACE_DEBUG((LM_DEBUG, "NetworkConfigUpdater::open enumerating interfaces"));

  struct ifaddrs *ifap = 0;
  struct ifaddrs *p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs (&ifap) != 0)
    return false;

  // Using logic from ACE::get_ip_interfaces_getifaddrs
  // but need ifa_name which is not returned by it
  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = ifap;
       p_if != 0;
       p_if = p_if->ifa_next)
  {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET)
    {
      struct sockaddr_in *addr =
        reinterpret_cast<sockaddr_in *> (p_if->ifa_addr);

      // Sometimes the kernel returns 0.0.0.0 as the interface
      // address, skip those...
      if (addr->sin_addr.s_addr != INADDR_ANY)
      {
        address.set ((u_short) 0,
                     addr->sin_addr.s_addr,
                     0);

        NetworkInterface iface(count, p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST);
        iface.addresses.insert(address);

        NetworkConfigMonitor::add_address(count, address);
        NetworkConfigMonitor::add_interface(iface);

        ++count;
      }
    }
  }

  ::freeifaddrs (ifap);

  return true;
}

bool NetworkConfigUpdater::close()
{
  // anyting to do?
 
  return true;
}

void NetworkConfigUpdater::add_interface(const OPENDDS_STRING &name)
{
  NetworkInterface* p_nic = 0;

  struct ifaddrs *ifap = 0;
  struct ifaddrs *p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs (&ifap) != 0)
  {
      ACE_ERROR((LM_ERROR, "NetworkConfigUpdater::add_interface getifaddrs error."));
      return;
  } 

  int count = 0;

  // Pull the address out of each INET interface. 
  for (p_if = ifap;
       p_if != 0;
       p_if = p_if->ifa_next)
  {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET)
    {
      if (name == p_if->ifa_name) 
      {
        p_nic = new NetworkInterface (count, p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST);
        break;
      }

      ++count;
    }
  }

  ::freeifaddrs (ifap);

  if (p_nic) 
  {
    NetworkConfigMonitor::add_interface(*p_nic);

    delete p_nic;
  }
}

void NetworkConfigUpdater::remove_interface(const OPENDDS_STRING &name)
{
  NetworkConfigMonitor::remove_interface(get_index(name));
}

void NetworkConfigUpdater::add_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  NetworkConfigMonitor::add_address(get_index(name), address);
}

void NetworkConfigUpdater::remove_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  NetworkConfigMonitor::remove_address(get_index(name), address);
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NETWORK_CONFIG_UPDATER
