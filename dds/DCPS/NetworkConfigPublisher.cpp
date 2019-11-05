/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigPublisher.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

NetworkInterfaces NetworkConfigPublisher::add_listener(NetworkConfigListener_rch listener)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, NetworkInterfaces());
  listeners_.insert(listener);
  return network_interfaces_;
}

void NetworkConfigPublisher::remove_listener(NetworkConfigListener_rch listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  listeners_.erase(listener);
}

NetworkInterfaces NetworkConfigPublisher::get() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, NetworkInterfaces());
  return network_interfaces_;
}

void NetworkConfigPublisher::add_interface(const NetworkInterface& nic)
{
  Listeners listeners;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    listeners = listeners_;
    network_interfaces_.push_back(nic);
  }
  for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
    (*pos)->add_interface(nic);
  }
}

void NetworkConfigPublisher::remove_interface(int index)
{
  NetworkInterface nic;
  Listeners listeners;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    listeners = listeners_;
    NetworkInterfaces::iterator pos = std::find(network_interfaces_.begin(), network_interfaces_.end(), index);
    if (pos != network_interfaces_.end()) {
      nic = *pos;
      network_interfaces_.erase(pos);
      publish = true;
    }
  }
  if (publish) {
    while (!nic.addresses.empty()) {
      const ACE_INET_Addr addr = *nic.addresses.begin();
      nic.addresses.erase(nic.addresses.begin());
      for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
        (*pos)->remove_address(nic, addr);
      }
    }

    for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
      (*pos)->remove_interface(nic);
    }
  }
}

void NetworkConfigPublisher::add_address(int index, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  Listeners listeners;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    listeners = listeners_;
    NetworkInterfaces::iterator nic_pos = std::find(network_interfaces_.begin(), network_interfaces_.end(), index);
    if (nic_pos != network_interfaces_.end()) {
      NetworkInterface::AddressSet::const_iterator addr_pos = nic_pos->addresses.find(address);
      if (addr_pos == nic_pos->addresses.end()) {
        nic_pos->addresses.insert(address);
        nic = *nic_pos;
        publish = true;
      }
    }
  }
  if (publish) {
    for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
      (*pos)->add_address(nic, address);
    }
  }
}

void NetworkConfigPublisher::remove_address(int index, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  Listeners listeners;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    listeners = listeners_;
    NetworkInterfaces::iterator nic_pos = std::find(network_interfaces_.begin(), network_interfaces_.end(), index);
    if (nic_pos != network_interfaces_.end()) {
      NetworkInterface::AddressSet::const_iterator addr_pos = nic_pos->addresses.find(address);
      if (addr_pos != nic_pos->addresses.end()) {
        nic_pos->addresses.erase(addr_pos);
        nic = *nic_pos;
        publish = true;
      }
    }
  }
  if (publish) {
    for (Listeners::const_iterator pos = listeners.begin(), limit = listeners.end(); pos != limit; ++pos) {
      (*pos)->remove_address(nic, address);
    }
  }
}

} // DCPS
} // OpenDDS
