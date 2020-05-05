/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigMonitor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void NetworkConfigMonitor::add_listener(NetworkConfigListener_wrch listener)
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, listeners_mutex_);
    listeners_.insert(listener);
  }

  const RcHandle<NetworkConfigListener> list = listener.lock();

  ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
  for (NetworkInterfaces::const_iterator pos = network_interfaces_.begin(), limit = network_interfaces_.end(); pos != limit; ++pos) {
    list->add_interface(*pos);
  }
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

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
