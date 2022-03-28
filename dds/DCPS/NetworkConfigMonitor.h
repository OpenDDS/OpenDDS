/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGMONITOR_H
#define OPENDDS_DCPS_NETWORKCONFIGMONITOR_H

#include "dcps_export.h"

#include "InternalTopic.h"
#include "NetworkAddress.h"
#include "PoolAllocator.h"
#include "RcObject.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Dcps_Export NetworkInterfaceAddress {
  NetworkInterfaceAddress()
    : can_multicast(false)
  {}
  NetworkInterfaceAddress(const OPENDDS_STRING& a_name,
                          bool a_can_multicast,
                          const NetworkAddress& a_address)
    : name(a_name)
    , can_multicast(a_can_multicast)
    , address(a_address)
  {}

  bool is_ipv4() const { return address.get_type() == AF_INET; }
  bool is_ipv6() const { return address.get_type() == AF_INET6; }
  bool exclude_from_multicast(const char* configured_interface) const;

  bool operator==(const NetworkInterfaceAddress& other) const
  {
    return name == other.name &&
      can_multicast == other.can_multicast &&
      address == other.address;
  }

  bool operator!=(const NetworkInterfaceAddress& other) const
  {
    return !(*this == other);
  }

  bool operator<(const NetworkInterfaceAddress& other) const
  {
    if (name != other.name) {
      return name < other.name;
    }

    return address < other.address;
  }

  OPENDDS_STRING name;
  bool can_multicast;
  NetworkAddress address;
};

struct NetworkInterfaceAddressKeyEqual {
  explicit NetworkInterfaceAddressKeyEqual(const NetworkInterfaceAddress& nia)
    : nia_(nia)
  {}

  bool operator()(const NetworkInterfaceAddress& other)
  {
    return nia_.name == other.name && nia_.address == other.address;
  }

  const NetworkInterfaceAddress& nia_;
};

class OpenDDS_Dcps_Export NetworkConfigMonitor : public virtual RcObject {
public:
  typedef OPENDDS_LIST(NetworkInterfaceAddress) List;

  NetworkConfigMonitor();

  void connect(RcHandle<InternalTopic<NetworkInterfaceAddress> > topic);
  virtual bool open() = 0;
  virtual bool close() = 0;
  void disconnect(RcHandle<InternalTopic<NetworkInterfaceAddress> > topic);

 protected:
  void set(const List& list);
  void clear();

  void set(const NetworkInterfaceAddress& nia);
  void remove_interface(const OPENDDS_STRING& name);
  void remove_address(const OPENDDS_STRING& name, const NetworkAddress& address);

private:
  RcHandle<InternalDataWriter<NetworkInterfaceAddress> > writer_;

  List list_;

  mutable ACE_Thread_Mutex mutex_;
};

typedef RcHandle<NetworkConfigMonitor> NetworkConfigMonitor_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_NETWORKCONFIGMONITOR_H
