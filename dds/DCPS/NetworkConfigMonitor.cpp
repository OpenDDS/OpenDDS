/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigMonitor.h"

#ifdef ACE_HAS_GETIFADDRS

#include "ace/os_include/os_ifaddrs.h"

#include <net/if.h>

#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool NetworkConfigMonitor::open()
{
#ifndef ACE_HAS_GETIFADDRS
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::open(). This OS does not support ::getifaddrs().\n")));
  return false;
#else
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigMonitor::open() enumerating interfaces\n")));
  }

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0)
    return false;

  // Using logic from ACE::get_ip_interfaces_getifaddrs
  // but need ifa_name which is not returned by it
  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next)
  {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET) {
      struct sockaddr_in *addr =
        reinterpret_cast<sockaddr_in *> (p_if->ifa_addr);

      // Sometimes the kernel returns 0.0.0.0 as the interface
      // address, skip those...
      if (addr->sin_addr.s_addr != INADDR_ANY) {
        address.set((u_short) 0,
                     addr->sin_addr.s_addr,
                     0);

        NetworkInterface iface(count, p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST);
        iface.addresses.insert(address);

        NetworkConfigMonitor::add_interface(iface);
        NetworkConfigMonitor::add_address(count, address);

        ++count;
      }
    }
  }

  ::freeifaddrs (p_ifa);

  return true;
#endif /* ACE_HAS_GETIFADDRS */
}

bool NetworkConfigMonitor::close()
{
  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigMonitor::close()\n")));
  }

  return true;
}

NetworkInterfaces NetworkConfigMonitor::add_listener(NetworkConfigListener_wrch listener)
{
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, listeners_mutex_, NetworkInterfaces());
    listeners_.insert(listener);
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, network_interfaces_mutex_, NetworkInterfaces());
  return network_interfaces_;
}

void NetworkConfigMonitor::remove_listener(NetworkConfigListener_wrch listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
  listeners_.erase(listener);
}

NetworkInterfaces NetworkConfigMonitor::get() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, network_interfaces_mutex_, NetworkInterfaces());
  return network_interfaces_;
}

void NetworkConfigMonitor::add_interface(const OPENDDS_STRING &name)
{
#ifndef ACE_HAS_GETIFADDRS
  ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::add_interface(). This OS does not support ::getifaddrs().\n")));
  return;
#else

  NetworkInterface* p_nic = 0;

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::add_interface %p"),
        ACE_TEXT("getifaddrs error - ")));
      return;
  }

  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next)
  {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET) {
      if (name == p_if->ifa_name)
      {
        p_nic = new NetworkInterface (count, p_if->ifa_name, p_if->ifa_flags & IFF_MULTICAST);
        break;
      }

      ++count;
    }
  }

  ::freeifaddrs (p_ifa);

  if (p_nic) {
    add_interface(*p_nic);

    delete p_nic;
  }

  validate_interfaces_index();

#endif /* ACE_HAS_GETIFADDRS */
}

void NetworkConfigMonitor::remove_interface(const OPENDDS_STRING &name)
{
  int index = get_index(name);
  if (index != -1) {
    remove_interface(index);
  }
  else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::remove_interface. Interface '%s' not found"), name.c_str()));
  }
}

void NetworkConfigMonitor::add_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  int index = get_index(name);
  if (index != -1) {
    add_address(index, address);
  }
  else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::add_address. Interface '%s' not found"), name.c_str()));
  }
}

void NetworkConfigMonitor::remove_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address)
{
  int index = get_index(name);
  if (index != -1) {
    remove_address(index, address);
  }
  else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: NetworkConfigMonitor::remove_address. Interface '%s' not found"), name.c_str()));
  }
}

void NetworkConfigMonitor::validate_interfaces_index()
{
// if the OS has inserted a new nic, which happens when
// turning on/off wifi or cellular on iOS, then an
// interface could have a different index from the index
// stored in network_interfaces_
#ifdef ACE_HAS_GETIFADDRS

  ifaddrs* p_ifa = 0;
  ifaddrs* p_if = 0;

  ACE_INET_Addr address;

  if (::getifaddrs(&p_ifa) != 0)
  {
      ACE_ERROR((LM_ERROR, "NetworkConfigMonitor::add_interface getifaddrs error."));
      return;
  }

  int count = 0;

  // Pull the address out of each INET interface.
  for (p_if = p_ifa; p_if != 0; p_if = p_if->ifa_next)
  {
    if (p_if->ifa_addr == 0)
      continue;

    // Check to see if it's up.
    if ((p_if->ifa_flags & IFF_UP) != IFF_UP)
      continue;

    if (p_if->ifa_addr->sa_family == AF_INET) {
      const std::string name(p_if->ifa_name);
      {
        NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceName(name));
        if (nic_pos != network_interfaces_.end()) {
          nic_pos->index(count);
        }
      }

      ++count;
    }
  }

  ::freeifaddrs (p_ifa);

#endif /* ACE_HAS_GETIFADDRS */
}


void NetworkConfigMonitor::add_interface(const NetworkInterface& nic)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    network_interfaces_.push_back(nic);
  }

  Listeners listeners;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
    listeners = listeners_;
  }

  for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
    const RcHandle<NetworkConfigListener> listener(pos->lock());
    if (listener) {
      listener->add_interface(nic);
    }
  }
}

void NetworkConfigMonitor::remove_interface(int index)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceIndex(index));
    if (pos != network_interfaces_.end()) {
      nic = *pos;
      network_interfaces_.erase(pos);
      publish = true;
    }
  }

  if (!publish) {
    return;
  }

  Listeners listeners;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
    listeners = listeners_;
  }

  while (!nic.addresses.empty()) {
    const ACE_INET_Addr addr = *nic.addresses.begin();
    nic.addresses.erase(nic.addresses.begin());
    for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
      const RcHandle<NetworkConfigListener> listener(pos->lock());
      if (listener) {
        listener->remove_address(nic, addr);
      }
    }
  }

  for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
    const RcHandle<NetworkConfigListener> listener(pos->lock());
    if (listener) {
      listener->remove_interface(nic);
    }
  }
}

void NetworkConfigMonitor::add_address(int index, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceIndex(index));
    if (nic_pos != network_interfaces_.end()) {
      NetworkInterface::AddressSet::const_iterator addr_pos = nic_pos->addresses.find(address);
      if (addr_pos == nic_pos->addresses.end()) {
        nic_pos->addresses.insert(address);
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (!publish) {
    return;
  }

  Listeners listeners;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
    listeners = listeners_;
  }

  for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
    const RcHandle<NetworkConfigListener> listener(pos->lock());
    if (listener) {
      listener->add_address(nic, address);
    }
  }
}

void NetworkConfigMonitor::remove_address(int index, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceIndex(index));
    if (nic_pos != network_interfaces_.end()) {
      NetworkInterface::AddressSet::iterator addr_pos = nic_pos->addresses.find(address);
      if (addr_pos != nic_pos->addresses.end()) {
        nic_pos->addresses.erase(addr_pos);
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (!publish) {
    return;
  }

  Listeners listeners;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
    listeners = listeners_;
  }

  for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
    const RcHandle<NetworkConfigListener> listener(pos->lock());
    if (listener) {
      listener->remove_address(nic, address);
    }
  }
}

int NetworkConfigMonitor::get_index(const OPENDDS_STRING& name)
{
  int index = -1;
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, network_interfaces_mutex_, -1);
    NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceName(name));
    if (nic_pos != network_interfaces_.end()) {
      index = nic_pos->index();
    }
  }

  return index;
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
