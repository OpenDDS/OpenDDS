/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "NetworkConfigMonitor.h"
#include "Service_Participant.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

void NetworkInterface::add_default_addrs()
{
  const ACE_INET_Addr sp_default = TheServiceParticipant->default_address();
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);
  if (sp_default != ACE_INET_Addr()) {
    addresses_.insert(sp_default);
    return;
  }
  static const u_short port_zero = 0;
  ACE_INET_Addr addr(port_zero, "0.0.0.0");
  addresses_.insert(addr);
#ifdef ACE_HAS_IPV6
  ACE_INET_Addr addr2(port_zero, "::");
  addresses_.insert(addr2);
#endif
}

bool NetworkInterface::has(const ACE_INET_Addr& addr) const
{
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);
  return addresses_.count(addr);
}

bool NetworkInterface::exclude_from_multicast(const char* configured_interface) const
{
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);

  if (addresses_.empty() || !can_multicast_) {
    return true;
  }

  if (configured_interface && *configured_interface && name_ != configured_interface) {
    OPENDDS_STRING ci_semi(configured_interface);
    if (ci_semi.find_first_of(':') == OPENDDS_STRING::npos) {
      ci_semi += ':';
    }
    ACE_INET_Addr as_addr(ci_semi.c_str());
    if (as_addr == ACE_INET_Addr() || addresses_.count(as_addr) == 0) {
      return true;
    }
  }

  const ACE_INET_Addr sp_default = TheServiceParticipant->default_address();
  if (sp_default != ACE_INET_Addr() && addresses_.count(sp_default) == 0) {
    return true;
  }

  return false;
}

bool NetworkInterface::add_address(const ACE_INET_Addr& addr)
{
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);
  return addresses_.insert(addr).second;
}

bool NetworkInterface::remove_address(const ACE_INET_Addr& addr)
{
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);
  return addresses_.erase(addr) != 0;
}

NetworkInterface& NetworkInterface::operator=(const NetworkInterface& rhs)
{
  if (this != &rhs) {
    const bool me_first = this < &rhs;
    ACE_Thread_Mutex* const first = me_first ? &mutex_ : &(rhs.mutex_);
    ACE_Thread_Mutex* const second = me_first ? &(rhs.mutex_) : &mutex_;

    ACE_Guard<ACE_Thread_Mutex> g1(*first);
    ACE_Guard<ACE_Thread_Mutex> g2(*second);

    addresses_ = rhs.addresses_;
    index_ = rhs.index_;
    name_ = rhs.name_;
    can_multicast_ = rhs.can_multicast_;
  }
  return *this;
}

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

NetworkInterfaces NetworkConfigMonitor::get_interfaces() const
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
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigMontior::add_interface(%s)\n"), nic.name().c_str()));
      }
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
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigMontior::remove_interface(%s)\n"), nic.name().c_str()));
      }
      network_interfaces_.erase(pos);
      publish = true;
    }
  }

  if (publish) {
    publish_remove_interface(nic);
  }
}

void NetworkConfigMonitor::remove_interface(const OPENDDS_STRING& name)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceName(name));
    if (pos != network_interfaces_.end()) {
      nic = *pos;
      if (DCPS_debug_level > 4) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) NetworkConfigMontior::remove_interface(%s)\n"), nic.name().c_str()));
      }
      network_interfaces_.erase(pos);
      publish = true;
    }
  }

  if (publish) {
    publish_remove_interface(nic);
  }
}

void NetworkConfigMonitor::publish_remove_interface(const NetworkInterface& nic)
{
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
      if (nic_pos->add_address(address)) {
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (publish) {
    publish_add_address(nic, address);
  }
}

void NetworkConfigMonitor::add_address(const OPENDDS_STRING& name, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceName(name));
    if (nic_pos != network_interfaces_.end()) {
      if (nic_pos->add_address(address)) {
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (publish) {
    publish_add_address(nic, address);
  }
}

void NetworkConfigMonitor::publish_add_address(const NetworkInterface& nic, const ACE_INET_Addr& address)
{
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
      if (nic_pos->remove_address(address)) {
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (publish) {
    publish_remove_address(nic, address);
  }
}

void NetworkConfigMonitor::remove_address(const OPENDDS_STRING& name, const ACE_INET_Addr& address)
{
  NetworkInterface nic;
  bool publish = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, network_interfaces_mutex_);
    NetworkInterfaces::iterator nic_pos = std::find_if(network_interfaces_.begin(), network_interfaces_.end(), NetworkInterfaceName(name));
    if (nic_pos != network_interfaces_.end()) {
      if (nic_pos->remove_address(address)) {
        nic = *nic_pos;
        publish = true;
      }
    }
  }

  if (publish) {
    publish_remove_address(nic, address);
  }
}

void NetworkConfigMonitor::publish_remove_address(const NetworkInterface& nic, const ACE_INET_Addr& address)
{
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
