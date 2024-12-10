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

class OpenDDS_Dcps_Export LinuxNetworkConfigMonitor
  : public virtual RcEventHandler
  , public virtual NetworkConfigMonitor
{
public:
  explicit LinuxNetworkConfigMonitor(ReactorTask_rch reactor_task);
  bool open();
  bool close();

private:
  class OpenHandler : public ReactorTask::Command {
  public:
    OpenHandler(const RcHandle<LinuxNetworkConfigMonitor>& lncm)
      : lncm_(lncm)
      , condition_(mutex_)
      , done_(false)
      , retval_(false)
    {}

    virtual ~OpenHandler() {}

    bool wait() const;

  private:
    void execute(ACE_Reactor* reactor);

    WeakRcHandle<LinuxNetworkConfigMonitor> lncm_;
    mutable ACE_Thread_Mutex mutex_;
    mutable ConditionVariable<ACE_Thread_Mutex> condition_;
    bool done_;
    bool retval_;
  };

  bool open_i();

  class CloseHandler : public ReactorTask::Command {
  public:
    CloseHandler(const RcHandle<LinuxNetworkConfigMonitor>& lncm)
      : lncm_(lncm)
      , condition_(mutex_)
      , done_(false)
      , retval_(false)
    {}

    virtual ~CloseHandler() {}

    bool wait() const;

  private:
    void execute(ACE_Reactor* reactor);

    WeakRcHandle<LinuxNetworkConfigMonitor> lncm_;
    mutable ACE_Thread_Mutex mutex_;
    mutable ConditionVariable<ACE_Thread_Mutex> condition_;
    bool done_;
    bool retval_;
  };

  bool close_i();

  ACE_HANDLE get_handle() const;
  int handle_input(ACE_HANDLE);
  void read_messages();
  void process_message(const nlmsghdr* header);

  ACE_SOCK_Netlink socket_;
  ACE_Thread_Mutex socket_mutex_;
  ReactorTask_rch reactor_task_;

  struct NetworkInterface {
    OPENDDS_STRING name;
    bool can_multicast;

    NetworkInterface()
      : can_multicast(false)
    {}

    NetworkInterface(const OPENDDS_STRING& a_name,
                     bool a_can_multicast)
      : name(a_name)
      , can_multicast(a_can_multicast)
    {}
  };

  typedef OPENDDS_MAP(int, NetworkInterface) NetworkInterfaceMap;
  NetworkInterfaceMap network_interface_map_;
};

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_LINUX || ACE_ANDROID

#endif // OPENDDS_DCPS_LINUXNETWORKCONFIGPUBLISHER_H
