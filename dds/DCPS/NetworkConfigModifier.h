/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H
#define OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H

#include "ace/config.h"

#if defined(ACE_HAS_GETIFADDRS) && !defined(OPENDDS_SAFETY_PROFILE)

#define OPENDDS_NETWORK_CONFIG_MODIFIER

#include "NetworkConfigMonitor.h"
#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct NetworkInterfaceName {
  explicit NetworkInterfaceName(const OPENDDS_STRING& name) : name_(name) {}

  bool operator()(const NetworkInterface& nic)
  {
    return name_ == nic.name();
  }

  const OPENDDS_STRING name_;
};

class OpenDDS_Dcps_Export NetworkConfigModifier : public NetworkConfigMonitor {
public:
  explicit NetworkConfigModifier();
  bool open();
  bool close();

  void add_interface(const OPENDDS_STRING &name);
  void remove_interface(const OPENDDS_STRING &name);
  void add_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address);
  void remove_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address);

protected:
  int get_index(const OPENDDS_STRING& name);

private:
  void validate_interfaces_index();
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_GETIFADDRS && ! OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_NETWORKCONFIGMODIFIER_H
