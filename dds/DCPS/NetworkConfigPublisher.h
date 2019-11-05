/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGPUBLISHER_H
#define OPENDDS_DCPS_NETWORKCONFIGPUBLISHER_H

#include "dcps_export.h"

#include "PoolAllocator.h"
#include "RcObject.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class NetworkInterface {
public:
  typedef OPENDDS_SET(ACE_INET_Addr) AddressSet;

  NetworkInterface() {}

  NetworkInterface(int index,
                   const OPENDDS_STRING& name,
                   bool can_multicast)
    : index_(index)
    , name_(name)
    , can_multicast_(can_multicast)
  {}
  int index() const { return index_; }
  const OPENDDS_STRING& name() const { return name_; }
  bool can_multicast() const { return can_multicast_; }

  AddressSet addresses;

  bool operator==(int index) const
  {
    return index_ == index;
  }

private:
  int index_;
  OPENDDS_STRING name_;
  bool can_multicast_;
};

class NetworkConfigListener : public virtual RcObject {
public:
  virtual void add_interface(const NetworkInterface& /*interface*/) {}
  virtual void remove_interface(const NetworkInterface& /*interface*/) {}
  virtual void add_address(const NetworkInterface& /*interface*/,
                           const ACE_INET_Addr& /*address*/) {}
  virtual void remove_address(const NetworkInterface& /*interface*/,
                              const ACE_INET_Addr& /*address*/) {}
};

typedef RcHandle<NetworkConfigListener> NetworkConfigListener_rch;

typedef OPENDDS_VECTOR(NetworkInterface) NetworkInterfaces;

class OpenDDS_Dcps_Export NetworkConfigPublisher : public virtual RcObject {
public:
  virtual bool open() = 0;
  virtual bool close() = 0;

  NetworkInterfaces add_listener(NetworkConfigListener_rch listener);
  void remove_listener(NetworkConfigListener_rch listener);
  NetworkInterfaces get() const;

protected:
  void add_interface(const NetworkInterface& nic);
  void remove_interface(int index);
  void add_address(int index, const ACE_INET_Addr& address);
  void remove_address(int index, const ACE_INET_Addr& address);

  mutable ACE_Thread_Mutex mutex_;

  typedef OPENDDS_SET(NetworkConfigListener_rch) Listeners;
  Listeners listeners_;
  NetworkInterfaces network_interfaces_;
};

typedef RcHandle<NetworkConfigPublisher> NetworkConfigPublisher_rch;

} // DCPS
} // OpenDDS

#endif // OPENDDS_DCPS_NETWORKCONFIGPUBLISHER_H
