/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_LINUXNETWORKCONFIGMONITOR_H
#define OPENDDS_DCPS_LINUXNETWORKCONFIGMONITOR_H

#include "ace/config.h"

#if (defined(ACE_LINUX) || defined(ACE_ANDROID)) && !defined(OPENDDS_SAFETY_PROFILE)

#define OPENDDS_LINUX_NETWORK_CONFIG_MONITOR

#include "NetworkConfigMonitor.h"
#include "RcEventHandler.h"
#include "ReactorTask.h"
#include "ReactorTask_rch.h"
#include "dcps_export.h"

#include <ace/SOCK_Netlink.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export LinuxNetworkConfigMonitor : public RcEventHandler, public NetworkConfigMonitor {
public:
  explicit LinuxNetworkConfigMonitor(ReactorTask_rch reactor_task);
  bool open();
  bool close();

private:
  class OpenHandler : public ReactorTask::Command {
  public:
    RegisterHandler(WeakRcHandle<LinuxNetworkConfigMonitor> lncm)
      : lncm_(lncm)
    {}

  private:
    void execute(ReactorWrapper& reactor_wrapper);
    {
      RcHandle<LinuxNetworkConfigMonitor> lncm = lncm_.lock();
      if (!lncm) {
        return;
      }
      ACE_GUARD(ACE_Thread_Mutex, g, lncm->socket_mutex_);
      if (reactor()->register_handler(lncm.get(), READ_MASK) != 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not register for input: %m\n")));
      }
    }

    WeakRcHandle<LinuxNetworkConfigMonitor> lncm_;
  };

  bool open_i(ReactorWrapper& reactor_wrapper);
  class CloseHandler : public ReactorTask::Command {
  public:
    RemoveHandler(WeakRcHandle<LinuxNetworkConfigMonitor> lncm)
      : lncm_(lncm)
    {}

    void execute()
    {
      RcHandle<LinuxNetworkConfigMonitor> lncm = lncm_.lock();
      if (!lncm) {
        return;
    void execute(ReactorWrapper& reactor_wrapper);
      ACE_GUARD(ACE_Thread_Mutex, g, lncm->socket_mutex_);
      reactor()->remove_handler(lncm.get(), READ_MASK);
    }

    WeakRcHandle<LinuxNetworkConfigMonitor> lncm_;
  };

  bool close_i(ReactorWrapper& reactor_wrapper);
  ACE_HANDLE get_handle() const;
  int handle_input(ACE_HANDLE);
  void read_messages();
  void process_message(const nlmsghdr* header);

  ACE_SOCK_Netlink socket_;
  ACE_Thread_Mutex socket_mutex_;
  ReactorTask_rch reactor_task_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_LINUX || ACE_ANDROID

#endif // OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
