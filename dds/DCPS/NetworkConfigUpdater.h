/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKCONFIGUPDATER_H
#define OPENDDS_DCPS_NETWORKCONFIGUPDATER_H

#ifdef ACE_HAS_IOS 
#define OPENDDS_NETWORK_CONFIG_UPDATER

#include "ace/os_include/os_ifaddrs.h"
#include "NetworkConfigMonitor.h"
#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export NetworkConfigUpdater : public NetworkConfigMonitor {
public:
  explicit NetworkConfigUpdater();
  bool open();
  bool close();

  void add_interface(const OPENDDS_STRING &name);
  void remove_interface(const OPENDDS_STRING &name);
  void add_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address);
  void remove_address(const OPENDDS_STRING &name, const ACE_INET_Addr& address);
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_IOS / OPENDDS_HAS_NETWORK_CONFIG_UPDATER

#endif // OPENDDS_DCPS_NETWORKCONFIGUPDATER_H
