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

#include <ace/Netlink_Addr.h>

#include <linux/rtnetlink.h>
#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const size_t MAX_NETLINK_MESSAGE_SIZE = 4096;

LinuxNetworkConfigMonitor::LinuxNetworkConfigMonitor(ReactorInterceptor_rch interceptor)
{
  reactor(interceptor->reactor());
}

bool LinuxNetworkConfigMonitor::open()
{
  // Listener to changes in links and IPV4 addresses.
  const pid_t pid = getpid();
  ACE_Netlink_Addr addr;
  addr.set(pid, RTMGRP_NOTIFY | RTMGRP_LINK | RTMGRP_IPV4_IFADDR);
  if (socket_.open(addr, AF_NETLINK, NETLINK_ROUTE) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not open socket: %m\n")));
    return false;
  }

  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not set non-blocking: %m\n")));
    return false;
  }

  struct {
    nlmsghdr header;
    rtgenmsg msg;
  } request = {};

  // Request a dump of the links.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETLINK;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = pid;
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != request.header.nlmsg_len) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not send request for links: %m\n")));
    return false;
  }

  read_messages();

  // Request a dump of the addresses.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETADDR;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = pid;
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != request.header.nlmsg_len) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not send request for addresses: %m\n")));
    return false;
  }

  read_messages();

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::open: could not register for input: %m\n")));
    return false;
  }

  return true;
}

bool LinuxNetworkConfigMonitor::close()
{
  bool retval = true;

  reactor()->remove_handler(this, READ_MASK);

  if (socket_.close() != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::close: could not close socket: %m\n")));
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::read_messages: could not recv: %m\n")));
      return;
    } else if (buffer_length == 0) {
      return;
    }

    for (const nlmsghdr* header = reinterpret_cast<const nlmsghdr*>(buffer);
         buffer_length >= 0 && NLMSG_OK(header, buffer_length);
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: LinuxNetworkConfigMonitor::process_message: NETLINK error: %C\n"), strerror(-msg->error)));
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
      default:
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const rtattr* attr = reinterpret_cast<const rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        if (attr->rta_type == IFA_ADDRESS) {
          if (RTA_PAYLOAD(attr) != address_length) {
            ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: LinuxNetworkConfigMonitor::process_message: incorrect address byte count\n")));
            return;
          }
          ACE_INET_Addr addr;
          addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
          add_address(msg->ifa_index, addr);
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
      default:
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const rtattr* attr = reinterpret_cast<const rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        if (attr->rta_type == IFA_ADDRESS) {
          if (RTA_PAYLOAD(attr) != address_length) {
            ACE_ERROR((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: LinuxNetworkConfigMonitor::process_message: incorrect address byte count\n")));
            return;
          }
          ACE_INET_Addr addr;
          addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
          remove_address(msg->ifa_index, addr);
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

      remove_interface(msg->ifi_index);
      add_interface(NetworkInterface(msg->ifi_index, name, msg->ifi_flags & IFF_MULTICAST));
    }
    break;
  case RTM_DELLINK:
    {
      const ifinfomsg* msg = reinterpret_cast<ifinfomsg*>(NLMSG_DATA(header));
      remove_interface(msg->ifi_index);
    }
    break;
  }
}

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_LINUX_NETWORK_CONFIG_MONITOR
