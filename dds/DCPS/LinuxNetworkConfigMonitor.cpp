/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ace/config.h"

#include "LinuxNetworkConfigMonitor.h"

#ifdef OPENDDS_LINUX_NETWORK_CONFIG_MONITOR

#include "ThreadStatusManager.h"
#include "Service_Participant.h"

#include <ace/Netlink_Addr.h>

#include <linux/rtnetlink.h>
#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const size_t MAX_NETLINK_MESSAGE_SIZE = 4096;

LinuxNetworkConfigMonitor::LinuxNetworkConfigMonitor(ReactorInterceptor_rch interceptor)
  : interceptor_(interceptor)
{
  reactor(interceptor->reactor());
}

bool LinuxNetworkConfigMonitor::open()
{
  // Listen to changes in links and IPV4 and IPV6 addresses.
  const pid_t pid = 0;
  ACE_Netlink_Addr addr;
  addr.set(pid, RTMGRP_NOTIFY | RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR);
  if (socket_.open(addr, AF_NETLINK, NETLINK_ROUTE) != 0) {
#ifdef ACE_ANDROID
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LinuxNetworkConfigMonitor::open: could not open socket"
                 " this is expected for API30+\n"));
    }
#else
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not open socket: %m\n"));
    }
#endif
    return false;
  }

  if (socket_.enable(ACE_NONBLOCK) != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not set non-blocking: %m\n"));
    }
    return false;
  }

  struct {
    nlmsghdr header;
    rtgenmsg msg;
  } request;
  memset(&request, 0, sizeof(request));

  // Request a dump of the links.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETLINK;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = pid;
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != static_cast<ssize_t>(request.header.nlmsg_len)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not send request for links: %m\n"));
    }
    return false;
  }

  read_messages();

  // Request a dump of the addresses.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETADDR;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = pid;
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != static_cast<ssize_t>(request.header.nlmsg_len)) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not send request for addresses: %m\n"));
    }
    return false;
  }

  read_messages();

  ReactorInterceptor_rch interceptor = interceptor_.lock();
  if (interceptor) {
    interceptor->execute_or_enqueue(make_rch<RegisterHandler>(this));
  }

  return true;
}

bool LinuxNetworkConfigMonitor::close()
{
  bool retval = true;

  ReactorInterceptor_rch interceptor = interceptor_.lock();
  if (interceptor) {
    ReactorInterceptor::CommandPtr command = interceptor->execute_or_enqueue(make_rch<RemoveHandler>(this));
    command->wait();
  }

  if (socket_.close() != 0) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::close: could not close socket: %m\n"));
    }
    retval = false;
  }

  return retval;
}

ACE_HANDLE LinuxNetworkConfigMonitor::get_handle() const
{
  return socket_.get_handle();
}

int LinuxNetworkConfigMonitor::handle_input(ACE_HANDLE)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  read_messages();
  return 0;
}

void LinuxNetworkConfigMonitor::read_messages()
{
  char buffer[MAX_NETLINK_MESSAGE_SIZE];

  for (;;) {
    ssize_t buffer_length = socket_.recv(buffer, 4096, 0);
    if (buffer_length < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::read_messages: could not recv: %m\n"));
      }
      return;
    } else if (buffer_length == 0) {
      return;
    }

    for (const nlmsghdr* header = reinterpret_cast<const nlmsghdr*>(buffer);
         buffer_length >= 0 && NLMSG_OK(header, static_cast<size_t>(buffer_length));
         header = NLMSG_NEXT(header, buffer_length)) {
      process_message(header);
    }
  }
}

void LinuxNetworkConfigMonitor::process_message(const nlmsghdr* header)
{
  switch (header->nlmsg_type) {
  case NLMSG_ERROR:
    {
      const nlmsgerr* msg = reinterpret_cast<nlmsgerr*>(NLMSG_DATA(header));
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: LinuxNetworkConfigMonitor::process_message: NETLINK error: %C\n", strerror(-msg->error)));
      }
    }
    break;
  case RTM_NEWADDR:
    {
      const ifaddrmsg* msg = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(header));
      size_t address_length = 0;
      switch (msg->ifa_family) {
      case AF_INET:
        address_length = 4;
        break;
      case AF_INET6:
        address_length = 16;
        break;
      default:
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const rtattr* attr = reinterpret_cast<const rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        if (attr->rta_type == IFA_ADDRESS) {
          if (RTA_PAYLOAD(attr) != address_length) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LinuxNetworkConfigMonitor::process_message: incorrect address byte count\n"));
            }
            return;
          }
          ACE_INET_Addr addr;
          addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
          NetworkInterfaceMap::const_iterator pos = network_interface_map_.find(msg->ifa_index);
          if (pos != network_interface_map_.end()) {
            set(NetworkInterfaceAddress(pos->second.name, pos->second.can_multicast, NetworkAddress(addr)));
          } else if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LinuxNetworkConfigMonitor::process_message: cannot find interface for address\n"));
          }
        }
      }
    }
    break;
  case RTM_DELADDR:
    {
      const ifaddrmsg* msg = reinterpret_cast<ifaddrmsg*>(NLMSG_DATA(header));
      size_t address_length = 0;
      switch (msg->ifa_family) {
      case AF_INET:
        address_length = 4;
        break;
      case AF_INET6:
        address_length = 16;
        break;
      default:
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const rtattr* attr = reinterpret_cast<const rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        if (attr->rta_type == IFA_ADDRESS) {
          if (RTA_PAYLOAD(attr) != address_length) {
            if (log_level >= LogLevel::Warning) {
              ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: LinuxNetworkConfigMonitor::process_message: incorrect address byte count\n"));
            }
            return;
          }

          NetworkInterfaceMap::iterator pos = network_interface_map_.find(msg->ifa_index);
          if (pos != network_interface_map_.end()) {
            ACE_INET_Addr addr;
            addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
            remove_address(pos->second.name, NetworkAddress(addr));
          }
        }
      }
    }
    break;
  case RTM_NEWLINK:
    {
      OPENDDS_STRING name;
      const ifinfomsg* msg = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(header));
      int rta_length = IFLA_PAYLOAD(header);
      for (const rtattr* attr = reinterpret_cast<const rtattr*>(IFLA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        if (attr->rta_type == IFLA_IFNAME) {
          name = reinterpret_cast<const char*>(RTA_DATA(attr));
        }
      }

      // Clean up the old if necessary.
      NetworkInterfaceMap::iterator pos = network_interface_map_.find(msg->ifi_index);
      if (pos != network_interface_map_.end()) {
        remove_interface(pos->second.name);
        network_interface_map_.erase(pos);
      }
      network_interface_map_[msg->ifi_index] = NetworkInterface(name, msg->ifi_flags & (IFF_MULTICAST | IFF_LOOPBACK));
    }
    break;
  case RTM_DELLINK:
    {
      const ifinfomsg* msg = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(header));
      NetworkInterfaceMap::iterator pos = network_interface_map_.find(msg->ifi_index);
      if (pos != network_interface_map_.end()) {
        remove_interface(pos->second.name);
        network_interface_map_.erase(pos);
      }
    }
    break;
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_LINUX_NETWORK_CONFIG_MONITOR
