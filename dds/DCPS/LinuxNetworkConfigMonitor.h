/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
#define OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H

#include "ace/config.h"

#if (defined(ACE_LINUX) || defined(ACE_ANDROID)) && !defined(OPENDDS_SAFETY_PROFILE)

#define OPENDDS_LINUX_NETWORK_CONFIG_MONITOR

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
  class RegisterHandler : public ReactorInterceptor::Command {
  public:
    RegisterHandler(LinuxNetworkConfigMonitor* lncm)
      : lncm_(lncm)
    {}

  private:
    void execute()
    {
      if (reactor()->register_handler(lncm_, READ_MASK) != 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not register for input: %m\n")));
      }
    }

    LinuxNetworkConfigMonitor* lncm_;
  };

  class RemoveHandler : public ReactorInterceptor::Command {
  public:
    RemoveHandler(LinuxNetworkConfigMonitor* lncm)
      : lncm_(lncm)
    {}

    void execute()
    {
      reactor()->remove_handler(lncm_, READ_MASK);
    }

    LinuxNetworkConfigMonitor* lncm_;
  };

  ACE_HANDLE get_handle() const;
  int handle_input(ACE_HANDLE);
  void read_messages();
  void process_message(const nlmsghdr* header);

  ACE_SOCK_Netlink socket_;
  ReactorInterceptor_wrch interceptor_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_LINUX || ACE_ANDROID

#endif // OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
