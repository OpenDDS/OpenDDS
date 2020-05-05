/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigModifier.h"

#ifdef OPENDDS_NETWORK_CONFIG_MODIFIER

#include <ace/OS_NS_sys_socket.h>

#include <ace/os_include/os_ifaddrs.h>

#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

NetworkConfigModifier::NetworkConfigModifier()
{
}

bool NetworkConfigModifier::open()
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigModifier::open() enumerating interfaces.\n")));
  }

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0)
    return false;

  // Using logic from ACE::get_ip_interfaces_getifaddrs
  // but need ifa_name which is not returned by it
  typedef std::map<std::string, NetworkInterface> Nics;
  Nics nics;

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
        address.set((u_short) 0, addr->sin_addr.s_addr, 0);

        std::pair<Nics::iterator, bool> p = nics.insert(std::make_pair(p_if->ifa_name, NetworkInterface(ACE_OS::if_nametoindex(p_if->ifa_name), p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST)));

        p.first->second.addresses.insert(address);
      }
    }
# if defined (ACE_HAS_IPV6)
    else if (p_if->ifa_addr->sa_family == AF_INET6) {
      struct sockaddr_in6 *addr = reinterpret_cast<sockaddr_in6 *> (p_if->ifa_addr);

      // Skip the ANY address
      if (!IN6_IS_ADDR_UNSPECIFIED(&addr->sin6_addr)) {
        address.set(reinterpret_cast<struct sockaddr_in *> (addr), sizeof(sockaddr_in6));

        std::pair<Nics::iterator, bool> p = nics.insert(std::make_pair(p_if->ifa_name, NetworkInterface(ACE_OS::if_nametoindex(p_if->ifa_name), p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST)));

        p.first->second.addresses.insert(address);
      }
    }
# endif /* ACE_HAS_IPV6 */
  }

  ::freeifaddrs (p_ifa);

  for (Nics::const_iterator pos = nics.begin(), limit = nics.end(); pos != limit; ++pos) {
    NetworkConfigMonitor::add_interface(pos->second);
  }

  return true;
}

bool NetworkConfigModifier::close()
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigModifier::close()\n")));
  }
  return true;
}

void NetworkConfigModifier::add_interface(const OPENDDS_STRING &name)
{
  NetworkInterface* p_nic = 0;

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigModifier::add_interface %p "),
        ACE_TEXT("getifaddrs error.\n")));
    }
    return;
  }

  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next) {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET || p_if->ifa_addr->sa_family == AF_INET6) {
      if (name == p_if->ifa_name) {
        p_nic = new NetworkInterface(count, p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST);
        break;
      }

      ++count;
    }
  }

  ::freeifaddrs (p_ifa);

  if (p_nic) {
    NetworkConfigMonitor::add_interface(*p_nic);
    delete p_nic;
  }

  validate_interfaces_index();
}

void NetworkConfigModifier::remove_interface(const OPENDDS_STRING &name)
{
  int index = get_index(name);
  if (index != -1) {
    NetworkConfigMonitor::remove_interface(index);
  }
  else {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigModifier::remove_interface. Interface '%C' not found.\n"), name.c_str()));
    }
  }
}

void NetworkConfigModifier::add_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  int index = get_index(name);
  if (index != -1) {
    NetworkConfigMonitor::add_address(index, address);
  }
  else {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigModifier::add_address. Interface '%C' not found.\n"), name.c_str()));
    }
  }
}

void NetworkConfigModifier::remove_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  int index = get_index(name);
  if (index != -1) {
    NetworkConfigMonitor::remove_address(index, address);
  } else {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigModifier::remove_address. Interface '%C' not found.\n"), name.c_str()));
    }
  }
}

void NetworkConfigModifier::validate_interfaces_index()
{
  // if the OS has added an interface, which happens when
  // turning on/off wifi or cellular on iOS, then an
  // interface could have a different index from the index
  // stored in network_interfaces_
  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR:NetworkConfigModifier::add_interface getifaddrs error.\n")));
    }
    return;
  }

  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next) {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET || p_if->ifa_addr->sa_family == AF_INET6) {
      const OPENDDS_STRING name(p_if->ifa_name);
      NetworkInterfaces nics = get();
      NetworkInterfaces::iterator nic_pos = std::find_if(nics.begin(), nics.end(), NetworkInterfaceName(name));
      if (nic_pos != nics.end()) {
        nic_pos->index(count);
      }
      ++count;
    }
  }

  ::freeifaddrs (p_ifa);
}

int NetworkConfigModifier::get_index(const OPENDDS_STRING& name)
{
  int index = -1;

  NetworkInterfaces nics = get();
  NetworkInterfaces::iterator nic_pos = std::find_if(nics.begin(), nics.end(), NetworkInterfaceName(name));
  if (nic_pos != nics.end()) {
    index = nic_pos->index();
  }

  return index;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NETWORK_CONFIG_MONITOR
