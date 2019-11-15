/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
#define OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H

#include "ace/config.h"

#ifdef ACE_LINUX

#include "NetworkConfigMonitor.h"
#include "RcEventHandler.h"
#include "ReactorInterceptor.h"
#include "dcps_export.h"

#include <ace/SOCK_Netlink.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export LinuxNetworkConfigMonitor : public RcEventHandler, public NetworkConfigMonitor {
public:
  explicit LinuxNetworkConfigMonitor(ReactorInterceptor_rch interceptor);
  bool open();
  bool close();

private:
  ACE_HANDLE get_handle() const;
  int handle_input(ACE_HANDLE);
  void read_messages();
  void process_message(const nlmsghdr* header);

  ACE_SOCK_Netlink socket_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_LINUX

#endif // OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
