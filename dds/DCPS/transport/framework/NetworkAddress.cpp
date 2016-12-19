/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "NetworkAddress.h"
#include "ace/OS_NS_netdb.h"
#include "ace/Sock_Connect.h"
#include "ace/OS_NS_sys_socket.h" // For setsockopt()

#if !defined (__ACE_INLINE__)
# include "NetworkAddress.inl"
#endif /* !__ACE_INLINE__ */

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  outCdr << ACE_OutputCDR::from_boolean(ACE_CDR_BYTE_ORDER);

  outCdr << ACE_OutputCDR::from_octet(value.reserved_);
  outCdr << value.addr_.c_str();

  return outCdr.good_bit();
}

ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  CORBA::Boolean byte_order;

  if ((inCdr >> ACE_InputCDR::to_boolean(byte_order)) == 0)
    return 0;

  inCdr.reset_byte_order(byte_order);

  if ((inCdr >> ACE_InputCDR::to_octet(value.reserved_)) == 0)
    return 0;

  char* buf = 0;

  if ((inCdr >> buf) == 0)
    return 0;

  value.addr_ = buf;

  delete[] buf;

  return inCdr.good_bit();
}

ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OPENDDS_STRING get_fully_qualified_hostname(ACE_INET_Addr* addr)
{
  // cache the determined fully qualified hostname and its
  // address to be used on subsequent calls
  static OPENDDS_STRING fullname;
  static ACE_INET_Addr selected_address;

  if (fullname.length() == 0) {
    size_t addr_count;
    ACE_INET_Addr *addr_array;
    OpenDDS::DCPS::HostnameInfoVector nonFQDN;

    int result = ACE::get_ip_interfaces(addr_count, addr_array);

    struct Array_Guard {
      Array_Guard(ACE_INET_Addr *ptr) : ptr_(ptr) {}
      ~Array_Guard() {
        delete [] ptr_;
      }
      ACE_INET_Addr* const ptr_;
    } guardObject(addr_array);

    if (result != 0 || addr_count < 1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: Unable to probe network. %p\n"),
                 ACE_TEXT("ACE::get_ip_interfaces")));

    } else {
#ifdef ACE_HAS_IPV6
        //front load IPV6 addresses to give preference to IPV6 interfaces
        size_t index_last_non_ipv6 = 0;
        for (size_t i = 0; i < addr_count; i++) {
          if (addr_array[i].get_type() == AF_INET6) {
            if (i == index_last_non_ipv6) {
              ++index_last_non_ipv6;
            } else {
              std::swap(addr_array[i], addr_array[index_last_non_ipv6]);
              ++index_last_non_ipv6;
            }
          }
        }
#endif
      for (size_t i = 0; i < addr_count; i++) {
        char hostname[MAXHOSTNAMELEN+1] = "";

        //Discover the fully qualified hostname

        if (ACE::get_fqdn(addr_array[i], hostname, MAXHOSTNAMELEN+1) == 0) {
          if (addr_array[i].is_loopback() == false && ACE_OS::strchr(hostname, '.') != 0) {
            VDBG_LVL((LM_DEBUG, "(%P|%t) found fqdn %C from %C:%d\n",
                      hostname, addr_array[i].get_host_addr(), addr_array[i].get_port_number()), 2);
            selected_address = addr_array[i];
            fullname = hostname;
            if (addr) {
              *addr = selected_address;
            }
            return fullname;

          } else {
            VDBG_LVL((LM_DEBUG, "(%P|%t) ip interface %C:%d maps to hostname %C\n",
                      addr_array[i].get_host_addr(), addr_array[i].get_port_number(), hostname), 2);

            if (ACE_OS::strncmp(hostname, "localhost", 9) == 0) {
              addr_array[i].get_host_addr(hostname, MAXHOSTNAMELEN);
            }

            OpenDDS::DCPS::HostnameInfo info;
            info.index_ = i;
            info.hostname_ = hostname;
            nonFQDN.push_back(info);
          }
        }
      }
    }

    OpenDDS::DCPS::HostnameInfoVector::iterator itBegin = nonFQDN.begin();
    OpenDDS::DCPS::HostnameInfoVector::iterator itEnd = nonFQDN.end();

    for (OpenDDS::DCPS::HostnameInfoVector::iterator it = itBegin; it != itEnd; ++it) {
      if (addr_array[it->index_].is_loopback() == false) {
        ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: Could not find FQDN. Using "
                   "\"%C\" as fully qualified hostname, please "
                   "correct system configuration.\n", it->hostname_.c_str()));
        selected_address = addr_array[it->index_];
        fullname = it->hostname_;
        if (addr) {
          *addr = selected_address;
        }
        return fullname;
      }
    }

    if (itBegin != itEnd) {
      ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: Could not find FQDN. Using "
                 "\"%C\" as fully qualified hostname, please "
                 "correct system configuration.\n", itBegin->hostname_.c_str()));
      selected_address = addr_array[itBegin->index_];
      fullname = itBegin->hostname_;
      if (addr) {
        *addr = selected_address;
      }
      return fullname;
    }

#ifdef OPENDDS_SAFETY_PROFILE
    // address resolution may not be available due to safety profile,
    // return an address that should work for running tests
    if (addr) {
      static const char local[] = {1, 0, 0, 127};
      addr->set_address(local, sizeof local);
    }
    return "localhost";
#else
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: failed to discover the fully qualified hostname\n"));
#endif
  }

  if (addr) {
    *addr = selected_address;
  }
  return fullname;
}

void get_interface_addrs(OPENDDS_VECTOR(ACE_INET_Addr)& addrs)
{
  ACE_INET_Addr *if_addrs = 0;
  size_t if_cnt = 0;
  size_t endpoint_count = 0;

  int result =
#ifdef OPENDDS_SAFETY_PROFILE
    -1;
#else
    ACE::get_ip_interfaces(if_cnt, if_addrs);
#endif

  struct Array_Guard {
    Array_Guard(ACE_INET_Addr *ptr) : ptr_(ptr) {}
    ~Array_Guard() {
      delete[] ptr_;
    }
    ACE_INET_Addr* const ptr_;
  } guardObject(if_addrs);

  if (!result) {
    size_t lo_cnt = 0;  // Loopback interface count
#if defined (ACE_HAS_IPV6)
    size_t ipv4_cnt = 0;
    size_t ipv4_lo_cnt = 0;
    size_t ipv6_ll = 0;
    bool ipv6_non_ll = false;
#endif
    for (size_t j = 0; j < if_cnt; ++j) {
      // Scan for the loopback interface since it shouldn't be included in
      // the list of cached hostnames unless it is the only interface.
      if (if_addrs[j].is_loopback())
        ++lo_cnt;
#if defined (ACE_HAS_IPV6)
      // Scan for IPv4 interfaces since these should not be included
      // when IPv6-only is selected.
      if (if_addrs[j].get_type() != AF_INET6 ||
          if_addrs[j].is_ipv4_mapped_ipv6()) {
        ++ipv4_cnt;
        if (if_addrs[j].is_loopback())
          ++ipv4_lo_cnt;  // keep track of IPv4 loopback ifs
      } else if (!if_addrs[j].is_linklocal() &&
                 !if_addrs[j].is_loopback()) {
        ipv6_non_ll = true; // we have at least 1 non-local IPv6 if
      } else if (if_addrs[j].is_linklocal()) {
        ++ipv6_ll;  // count link local addrs to exclude them afterwards
      }
#endif /* ACE_HAS_IPV6 */
    }

    bool ipv4_only = ACE_INET_Addr().get_type() == AF_INET;

#if defined (ACE_HAS_IPV6)

    // If the loopback interface is the only interface then include it
    // in the list of interfaces to query for a hostname, otherwise
    // exclude it from the list.
    bool ignore_lo;
    if (ipv4_only) {
      ignore_lo = ipv4_cnt != ipv4_lo_cnt;
    } else {
      ignore_lo = if_cnt != lo_cnt;
    }

    // Adjust counts for IPv4 only if required
    size_t if_ok_cnt = if_cnt;
    if (ipv4_only) {
      if_ok_cnt = ipv4_cnt;
      lo_cnt = ipv4_lo_cnt;
      ipv6_ll = 0;
    }

    // In case there are no non-local IPv6 ifs in the list only exclude
    // IPv4 loopback.
    // IPv6 loopback will be needed to successfully connect IPv6 clients
    // in a localhost environment.
    if (!ipv4_only && !ipv6_non_ll)
      lo_cnt = ipv4_lo_cnt;

    if (!ignore_lo)
      endpoint_count = if_ok_cnt - ipv6_ll;
    else
      endpoint_count = if_ok_cnt - ipv6_ll - lo_cnt;
#else /* end ACE_HAS_IPV6 begin !ACE_HAS_IPV6*/
    // If the loopback interface is the only interface then include it
    // in the list of interfaces to query for a hostname, otherwise
    // exclude it from the list.
    bool ignore_lo;
    ignore_lo = if_cnt != lo_cnt;
    if (!ignore_lo)
      endpoint_count = if_cnt;
    else
      endpoint_count = if_cnt - lo_cnt;
#endif /* !ACE_HAS_IPV6 */
    if (endpoint_count == 0) {
      VDBG_LVL((LM_DEBUG,
        ACE_TEXT("(%P|%t) get_interface_addrs() - ")
        ACE_TEXT("found no usable addresses\n")), 2);
    }

    for (size_t i = 0; i < if_cnt; ++i) {
      // Ignore any non-IPv4 interfaces when so required.
      if (ipv4_only && (if_addrs[i].get_type() != AF_INET))
        continue;
#if defined (ACE_HAS_IPV6)
      // Ignore any loopback interface if there are other
      // non-loopback interfaces.
      if (ignore_lo &&
          if_addrs[i].is_loopback() &&
          (ipv4_only ||
          ipv6_non_ll ||
          if_addrs[i].get_type() != AF_INET6))
        continue;

      // Ignore all IPv6 link local interfaces when so required.
      if (ipv6_non_ll && if_addrs[i].is_linklocal())
        continue;
#else /* ACE_HAS_IPV6 */
      // Ignore any loopback interface if there are other
      // non-loopback interfaces.
      if (ignore_lo && if_addrs[i].is_loopback())
        continue;
#endif /* !ACE_HAS_IPV6 */
      addrs.push_back(if_addrs[i]);
    }
  }
#ifdef ACE_HAS_IPV6
  //front load IPV6 addresses to give preference to IPV6 interfaces
  size_t index_last_non_ipv6 = 0;
  for (size_t i = 0; i < addrs.size(); i++) {
    if (addrs.at(i).get_type() == AF_INET6) {
      if (i == index_last_non_ipv6) {
        ++index_last_non_ipv6;
      }
      else {
        std::swap(addrs.at(i), addrs.at(index_last_non_ipv6));
        ++index_last_non_ipv6;
      }
    }
  }
#endif
#ifdef OPENDDS_SAFETY_PROFILE
  // address resolution may not be available due to safety profile,
  // return an address that should work for running tests
  if (addrs.empty()) {
    ACE_INET_Addr addr;
    static const char local[] = { 1, 0, 0, 127 };
    addr.set_address(local, sizeof local);
    addrs.push_back(addr);
  }
#else
  if (addrs.empty()) {
    ACE_ERROR((LM_ERROR,
      "(%P|%t) ERROR: failed to find usable interface address\n"));
  }
#endif
}

bool set_socket_multicast_ttl(const ACE_SOCK_Dgram& socket, const unsigned char& ttl)
{
  ACE_HANDLE handle = socket.get_handle();
  const void* ttlp = &ttl;
#if defined(ACE_LINUX) || defined(__linux__)
  int ttl_2 = ttl;
  ttlp = &ttl_2;
#define TTL ttl_2
#else
#define TTL ttl
#endif
#if defined (ACE_HAS_IPV6)
  ACE_INET_Addr local_addr;
  if (0 != socket.get_local_addr(local_addr)) {
    VDBG((LM_WARNING, "(%P|%t) set_socket_ttl: "
          "ACE_SOCK_Dgram::get_local_addr %p\n", ACE_TEXT("")));
  }
  if (local_addr.get_type () == AF_INET6) {
    if (0 != ACE_OS::setsockopt(handle,
                                IPPROTO_IPV6,
                                IPV6_MULTICAST_HOPS,
                                static_cast<const char*>(ttlp),
                                sizeof(TTL))) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("set_socket_ttl: ")
                        ACE_TEXT("failed to set IPV6 TTL: %d %p\n"),
                        ttl,
                        ACE_TEXT("ACE_OS::setsockopt(TTL)")),
                       false);
    }
  } else
#endif  /* ACE_HAS_IPV6 */
  if (0 != ACE_OS::setsockopt(handle,
                              IPPROTO_IP,
                              IP_MULTICAST_TTL,
                              static_cast<const char*>(ttlp),
                              sizeof(TTL))) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("set_socket_ttl: ")
                      ACE_TEXT("failed to set TTL: %d %p\n"),
                      ttl,
                      ACE_TEXT("ACE_OS::setsockopt(TTL)")),
                     false);
  }
  return true;
}

bool open_appropriate_socket_type(ACE_SOCK_Dgram& socket, const ACE_INET_Addr& local_address)
{
#if defined (ACE_HAS_IPV6) && defined (IPV6_V6ONLY)
  int protocol_family = ACE_PROTOCOL_FAMILY_INET;
  int protocol = 0;
  int reuse_addr = 0;
  if (static_cast<ACE_Addr>(local_address) != ACE_Addr::sap_any) {
    protocol_family = local_address.get_type();
  } else if (protocol_family == PF_UNSPEC) {
    protocol_family = ACE::ipv6_enabled() ? PF_INET6 : PF_INET;
  }

  int one = 1;
  socket.set_handle(ACE_OS::socket(protocol_family,
    SOCK_DGRAM,
    protocol));

  if (socket.get_handle() == ACE_INVALID_HANDLE) {
    ACE_ERROR_RETURN((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING:")
      ACE_TEXT("open_appropriate_socket_type: ")
      ACE_TEXT("failed to set socket handle\n")),
      false);
  } else if (protocol_family != PF_UNIX &&
             reuse_addr &&
             socket.set_option(SOL_SOCKET,
                               SO_REUSEADDR,
                               &one,
                               sizeof one) == -1) {
    socket.close();
    ACE_ERROR_RETURN((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("open_appropriate_socket_type: ")
      ACE_TEXT("failed to set socket SO_REUSEADDR option\n")),
      false);
  }
  ACE_HANDLE handle = socket.get_handle();
  int ipv6_only = 0;
  if (protocol_family == PF_INET6 &&
      0 != ACE_OS::setsockopt(handle,
                              IPPROTO_IPV6,
                              IPV6_V6ONLY,
                              (char*)&ipv6_only,
                              sizeof(ipv6_only))) {
    ACE_ERROR_RETURN((LM_WARNING,
      ACE_TEXT("(%P|%t) WARNING: ")
      ACE_TEXT("open_appropriate_socket_type: ")
      ACE_TEXT("failed to set IPV6_V6ONLY to 0: %p\n"),
      ACE_TEXT("ACE_OS::setsockopt(IPV6_V6ONLY)")),
      false);
  }
  bool error = false;

  if (static_cast<ACE_Addr>(local_address) == ACE_Addr::sap_any) {
    if (protocol_family == PF_INET || protocol_family == PF_INET6) {
      if (ACE::bind_port(socket.get_handle(),
                         INADDR_ANY,
                         protocol_family) == -1) {
        error = true;
      }
    }
  } else if (ACE_OS::bind(socket.get_handle(),
                          reinterpret_cast<sockaddr *> (local_address.get_addr()),
                          local_address.get_size()) == -1) {
    error = true;
  }

  if (error) {
    socket.close();
    VDBG_LVL((LM_WARNING, "(%P|%t) WARNING: open_appropriate_socket_type: "
                          "failed to bind address to socket\n"), 2);
    return false;
  }
  return true;
#else
  return socket.open(local_address) == 0;
#endif
}
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
