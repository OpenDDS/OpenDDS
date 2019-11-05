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

#include "NetworkConfigPublisher.h"
#include "RcEventHandler.h"
#include "ReactorInterceptor.h"
#include "dcps_export.h"

#include <ace/SOCK_Netlink.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export LinuxNetworkConfigPublisher : public RcEventHandler, public NetworkConfigPublisher {
public:
  LinuxNetworkConfigPublisher(ReactorInterceptor_rch interceptor);
  bool open() override;
  bool close() override;

private:
  ACE_HANDLE get_handle() const override;
  int handle_input(ACE_HANDLE) override;
  void read_messages();
  void process_message(const struct nlmsghdr* header);

  ACE_SOCK_Netlink socket_;
};

} // DCPS
} // OpenDDS

#endif // ACE_LINUX

#endif // OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
