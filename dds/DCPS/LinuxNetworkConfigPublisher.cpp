/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ace/config.h"

#ifdef ACE_LINUX

#include "LinuxNetworkConfigPublisher.h"

#include <ace/Netlink_Addr.h>

#include <linux/rtnetlink.h>
#include <net/if.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

LinuxNetworkConfigPublisher::LinuxNetworkConfigPublisher(ReactorInterceptor_rch interceptor)
{
  reactor(interceptor->reactor());
}

bool LinuxNetworkConfigPublisher::open()
{
  // Listener to changes in links and IPV4 addresses.
  ACE_Netlink_Addr addr;
  addr.set(getpid(), RTMGRP_NOTIFY | RTMGRP_LINK | RTMGRP_IPV4_IFADDR);
  if (socket_.open(addr, AF_NETLINK, NETLINK_ROUTE) != 0) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::open: could not open socket: %m\n"));
    return false;
  }

  if (socket_.enable(ACE_NONBLOCK) != 0) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::open: could not set non-blocking: %m\n"));
    return false;
  }

  struct {
    struct nlmsghdr header;
    struct rtgenmsg msg;
  } request = {};

  // Request a dump of the links.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETLINK;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = getpid();
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != request.header.nlmsg_len) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::open: could not send request for links: %m\n"));
    return false;
  }

  read_messages();

  // Request a dump of the addresses.
  request.header.nlmsg_len = NLMSG_LENGTH(sizeof(request.msg));
  request.header.nlmsg_type = RTM_GETADDR;
  request.header.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
  request.header.nlmsg_pid = getpid();
  request.msg.rtgen_family = AF_UNSPEC;

  if (socket_.send(&request, request.header.nlmsg_len, 0) != request.header.nlmsg_len) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::open: could not send request for addresses: %m\n"));
    return false;
  }

  read_messages();

  if (reactor()->register_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::open: could not register for input: %m\n"));
    return false;
  }

  return true;
}

bool LinuxNetworkConfigPublisher::close()
{
  bool retval = true;

  if (reactor()->remove_handler(this, READ_MASK) != 0) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::close: could not unregister for input: %m\n"));
    retval = false;
  }

  if (socket_.close() != 0) {
    ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::close: could not close socket: %m\n"));
    retval = false;
  }

  return retval;
}

ACE_HANDLE LinuxNetworkConfigPublisher::get_handle() const
{
  return socket_.get_handle();
}

int LinuxNetworkConfigPublisher::handle_input(ACE_HANDLE)
{
  read_messages();
  return 0;
}

void LinuxNetworkConfigPublisher::read_messages()
{
  char buffer[4096];

  for (;;) {
    ssize_t buffer_length = socket_.recv(buffer, 4096, 0);
    if (buffer_length <= 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      }
      ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::read_messages: could not recv: %m\n", errno));
      return;
    }

    for (const struct nlmsghdr* header = reinterpret_cast<const struct nlmsghdr*>(buffer);
         buffer_length >= 0 && NLMSG_OK(header, buffer_length);
         header = NLMSG_NEXT(header, buffer_length)) {
      process_message(header);
    }
  }
}

void LinuxNetworkConfigPublisher::process_message(const struct nlmsghdr* header)
{
  switch (header->nlmsg_type) {
  case NLMSG_ERROR:
    {
      const struct nlmsgerr* msg = reinterpret_cast<struct nlmsgerr*>(NLMSG_DATA(header));
      ACE_ERROR((LM_ERROR, "LinuxNetworkConfigPublisher::process_message: NETLINK error: %C\n", strerror(-msg->error)));
    }
    break;
  case RTM_NEWADDR:
    {
      const struct ifaddrmsg* msg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(header));
      size_t address_length = 0;
      switch(msg->ifa_family) {
      case AF_INET:
        address_length = 4;
        break;
      default:
        ACE_ERROR((LM_WARNING, "LinuxNetworkConfigPublisher::process_message: unhandled address family: %d\n", msg->ifa_family));
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const struct rtattr* attr = reinterpret_cast<const struct rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        switch (attr->rta_type) {
        case IFA_ADDRESS:
          {
            if (RTA_PAYLOAD(attr) != address_length) {
              ACE_ERROR((LM_WARNING, "LinuxNetworkConfigPublisher::process_message: incorrect address byte count\n"));
              return;
            }
            ACE_INET_Addr addr;
            addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
            add_address(msg->ifa_index, addr);
          }
          break;
        }
      }

    }
    break;
  case RTM_DELADDR:
    {
      const struct ifaddrmsg* msg = reinterpret_cast<struct ifaddrmsg*>(NLMSG_DATA(header));
      size_t address_length = 0;
      switch(msg->ifa_family) {
      case AF_INET:
        address_length = 4;
        break;
      default:
        ACE_ERROR((LM_WARNING, "LinuxNetworkConfigPublisher::process_message: unhandled address family: %d\n", msg->ifa_family));
        return;
      }
      int rta_length = IFA_PAYLOAD(header);
      for (const struct rtattr* attr = reinterpret_cast<const struct rtattr*>(IFA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        switch (attr->rta_type) {
        case IFA_ADDRESS:
          {
            if (RTA_PAYLOAD(attr) != address_length) {
              ACE_ERROR((LM_WARNING, "LinuxNetworkConfigPublisher::process_message: incorrect address byte count\n"));
              return;
            }
            ACE_INET_Addr addr;
            addr.set_address(reinterpret_cast<const char*>(RTA_DATA(attr)), address_length, 0);
            remove_address(msg->ifa_index, addr);
          }
          break;
        }
      }

    }
    break;
  case RTM_NEWLINK:
    {
      OPENDDS_STRING name;
      const struct ifinfomsg* msg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(header));
      int rta_length = IFLA_PAYLOAD(header);
      for (const struct rtattr* attr = reinterpret_cast<const struct rtattr*>(IFLA_RTA(msg));
           RTA_OK(attr, rta_length);
           attr = RTA_NEXT(attr, rta_length)) {
        switch (attr->rta_type) {
        case IFLA_IFNAME:
          name = reinterpret_cast<const char*>(RTA_DATA(attr));
          break;
        }
      }

      remove_interface(msg->ifi_index);
      add_interface(NetworkInterface(msg->ifi_index, name, msg->ifi_flags & IFF_MULTICAST));
    }
    break;
  case RTM_DELLINK:
    {
      const struct ifinfomsg* msg = reinterpret_cast<struct ifinfomsg*>(NLMSG_DATA(header));
      remove_interface(msg->ifi_index);
    }
    break;
  }
}

} // DCPS
} // OpenDDS

#endif // ACE_LINUX
