/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGMONITOR_H
#define OPENDDS_DCPS_NETWORKCONFIGMONITOR_H

#include "dcps_export.h"

#include "PoolAllocator.h"
#include "RcObject.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export NetworkInterface : public virtual RcObject {
public:
  typedef OPENDDS_SET(ACE_INET_Addr) AddressSet;

  NetworkInterface()
    : index_(-1)
    , can_multicast_(false)
  {}
  NetworkInterface(const NetworkInterface& val) : RcObject() { *this = val; }
  NetworkInterface(int index,
                   const OPENDDS_STRING& name,
                   bool can_multicast)
    : index_(index)
    , name_(name)
    , can_multicast_(can_multicast)
  {}
  virtual ~NetworkInterface() {}

  int index() const { return index_; }
  void index(int index) { index_ = index; }
  const OPENDDS_STRING& name() const { return name_; }
  bool can_multicast() const { return can_multicast_; }
  bool has_ipv4() const
  {
    ACE_Guard<ACE_Thread_Mutex> g(mutex_);
    for (AddressSet::const_iterator pos = addresses_.begin(), limit = addresses_.end(); pos != limit; ++pos) {
      if (pos->get_type() == AF_INET) {
        return true;
      }
    }
    return false;
  }

  bool has_ipv6() const
  {
    ACE_Guard<ACE_Thread_Mutex> g(mutex_);
    for (AddressSet::const_iterator pos = addresses_.begin(), limit = addresses_.end(); pos != limit; ++pos) {
      if (pos->get_type() == AF_INET6) {
        return true;
      }
    }
    return false;
  }

  AddressSet get_addresses() const { ACE_Guard<ACE_Thread_Mutex> g(mutex_); return addresses_; }

  void add_default_addrs();
  bool has(const ACE_INET_Addr& addr) const;
  bool exclude_from_multicast(const char* configured_interface) const;

  bool add_address(const ACE_INET_Addr& addr);
  bool remove_address(const ACE_INET_Addr& addr);

  NetworkInterface& operator=(const NetworkInterface& rhs);

private:
  mutable ACE_Thread_Mutex mutex_;
  AddressSet addresses_;
  int index_;
  OPENDDS_STRING name_;
  bool can_multicast_;
};
typedef RcHandle<NetworkInterface> NetworkInterface_rch;

struct NetworkInterfaceIndex {
  explicit NetworkInterfaceIndex(int index) : index_(index) {}

  bool operator()(const NetworkInterface_rch& nic) const
  {
    return index_ == nic->index();
  }

  const int index_;
};

struct NetworkInterfaceName {
  explicit NetworkInterfaceName(const OPENDDS_STRING& name) : name_(name) {}

  bool operator()(const NetworkInterface_rch& nic)
  {
    return name_ == nic->name();
  }

  const OPENDDS_STRING name_;
};

class OpenDDS_Dcps_Export NetworkConfigListener : public virtual RcObject {
public:
  virtual ~NetworkConfigListener() {}

  virtual void add_interface(const NetworkInterface_rch& nic)
  {
    NetworkInterface::AddressSet addresses = nic->get_addresses();
    for (NetworkInterface::AddressSet::const_iterator pos = addresses.begin(), limit = addresses.end();
         pos != limit; ++pos) {
      add_address(nic, *pos);
    }
  }

  virtual void remove_interface(const NetworkInterface_rch& nic)
  {
    NetworkInterface::AddressSet addresses = nic->get_addresses();
    for (NetworkInterface::AddressSet::const_iterator pos = addresses.begin(), limit = addresses.end();
         pos != limit; ++pos) {
      remove_address(nic, *pos);
    }
  }

  virtual void add_address(const NetworkInterface_rch& /*interface*/,
                           const ACE_INET_Addr& /*address*/) = 0;
  virtual void remove_address(const NetworkInterface_rch& /*interface*/,
                              const ACE_INET_Addr& /*address*/) = 0;
};

typedef WeakRcHandle<NetworkConfigListener> NetworkConfigListener_wrch;

typedef OPENDDS_VECTOR(NetworkInterface_rch) NetworkInterfaces;

class OpenDDS_Dcps_Export NetworkConfigMonitor : public virtual RcObject {
public:
  virtual bool open() = 0;
  virtual bool close() = 0;

  void add_listener(NetworkConfigListener_wrch listener);
  void remove_listener(NetworkConfigListener_wrch listener);
  NetworkInterfaces get_interfaces() const;

protected:
  void add_interface(const NetworkInterface_rch& nic);
  void remove_interface(int index);
  void remove_interface(const OPENDDS_STRING& name);
  void publish_remove_interface(const NetworkInterface_rch& nic);
  void add_address(int index, const ACE_INET_Addr& address);
  void add_address(const OPENDDS_STRING& name, const ACE_INET_Addr& address);
  void publish_add_address(const NetworkInterface_rch& nic, const ACE_INET_Addr& address);
  void remove_address(int index, const ACE_INET_Addr& address);
  void remove_address(const OPENDDS_STRING& name, const ACE_INET_Addr& address);
  void publish_remove_address(const NetworkInterface_rch& nic, const ACE_INET_Addr& address);

private:
  typedef OPENDDS_SET(NetworkConfigListener_wrch) Listeners;
  Listeners listeners_;
  mutable ACE_Thread_Mutex listeners_mutex_;

  NetworkInterfaces network_interfaces_;
  mutable ACE_Thread_Mutex network_interfaces_mutex_;
};

typedef RcHandle<NetworkConfigMonitor> NetworkConfigMonitor_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_NETWORKCONFIGMONITOR_H
