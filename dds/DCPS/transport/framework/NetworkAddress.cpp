/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "NetworkAddress.h"
#include <dds/DCPS/LogAddr.h>
#include "dds/DCPS/TimeTypes.h"
#include "ace/OS_NS_netdb.h"
#include "ace/Sock_Connect.h"
#include "ace/OS_NS_sys_socket.h" // For setsockopt()
#include "ace/OS_NS_arpa_inet.h"

#include <cstdlib>
#include <cstring>

#if !defined (__ACE_INLINE__)
# include "NetworkAddress.inl"
#endif /* !__ACE_INLINE__ */

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  return (outCdr << ACE_OutputCDR::from_boolean(ACE_CDR_BYTE_ORDER)) &&
         (outCdr << ACE_OutputCDR::from_octet(value.reserved_)) &&
         (outCdr << value.addr_.c_str());
}

ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkAddress& value)
{
  CORBA::Boolean byte_order;

  if (!(inCdr >> ACE_InputCDR::to_boolean(byte_order)))
    return false;

  inCdr.reset_byte_order(byte_order);

  if (!(inCdr >> ACE_InputCDR::to_octet(value.reserved_)))
    return false;

  char* buf = 0;

  if (!(inCdr >> buf))
    return false;

  value.addr_ = buf;

  delete[] buf;

  return inCdr.good_bit();
}

ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

String get_fully_qualified_hostname(ACE_INET_Addr* addr)
{
  // cache the determined fully qualified hostname and its
  // address to be used on subsequent calls
  static String fullname;
  static ACE_INET_Addr selected_address;

  if (fullname.length() == 0) {
    size_t addr_count;
    ACE_INET_Addr *addr_array = 0;
    OpenDDS::DCPS::HostnameInfoVector nonFQDN;

    const int result = ACE::get_ip_interfaces(addr_count, addr_array);

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
      for (size_t i = 0; i < addr_count; i++) {
        VDBG_LVL((LM_DEBUG, "(%P|%t) NetworkAddress: found IP interface %C\n", LogAddr::ip(addr_array[i]).c_str()), 4);
      }

#ifdef ACE_HAS_IPV6
        // Front load IPV6 addresses to give preference to IPV6 interfaces
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

        // Discover the fully qualified hostname
        if (ACE::get_fqdn(addr_array[i], hostname, MAXHOSTNAMELEN+1) == 0) {
          VDBG_LVL((LM_DEBUG, "(%P|%t) considering fqdn %C\n", hostname), 4);
          if (!addr_array[i].is_loopback() && ACE_OS::strchr(hostname, '.') != 0 && choose_single_coherent_address(hostname, false, false) != ACE_INET_Addr()) {
            VDBG_LVL((LM_DEBUG, "(%P|%t) found fqdn %C from %C\n", hostname, LogAddr(addr_array[i]).c_str()), 2);
            selected_address = addr_array[i];
            fullname = hostname;
            if (addr) {
              *addr = selected_address;
            }
            return fullname;

          } else {
            VDBG_LVL((LM_DEBUG, "(%P|%t) ip interface %C maps to hostname %C\n",
                      LogAddr(addr_array[i]).c_str(), hostname), 2);

            if (ACE_OS::strncmp(hostname, "localhost", 9) == 0) {
              addr_array[i].get_host_addr(hostname, MAXHOSTNAMELEN);
            }

            OpenDDS::DCPS::HostnameInfo info;
            info.index_ = i;
            info.hostname_ = hostname;
            if (choose_single_coherent_address(info.hostname_, false) != ACE_INET_Addr()) {
              nonFQDN.push_back(info);
            }
          }
        }
      }
    }

    OpenDDS::DCPS::HostnameInfoVector::iterator itBegin = nonFQDN.begin();
    OpenDDS::DCPS::HostnameInfoVector::iterator itEnd = nonFQDN.end();

    for (OpenDDS::DCPS::HostnameInfoVector::iterator it = itBegin; it != itEnd; ++it) {
      if (!addr_array[it->index_].is_loopback()) {
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
#if defined(ACE_LINUX) || defined(__linux__) || defined(ACE_HAS_MAC_OSX)
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

bool open_appropriate_socket_type(ACE_SOCK_Dgram& socket, const ACE_INET_Addr& local_address, int* proto_family)
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
  if (proto_family) {
    *proto_family = protocol_family;
  }
  return true;
#else
  if (proto_family) {
    *proto_family = PF_INET;
  }
  return socket.open(local_address) == 0;
#endif
}

ACE_INET_Addr choose_single_coherent_address(const ACE_INET_Addr& address, bool prefer_loopback)
{
// Check that ACE_INET_Addr supports next()
#if !(ACE_MAJOR_VERSION < 6 || (ACE_MAJOR_VERSION == 6 && (ACE_MINOR_VERSION < 3 || (ACE_MINOR_VERSION == 3 && ACE_MICRO_VERSION < 1))))
  ACE_INET_Addr copy(address);
  OPENDDS_VECTOR(ACE_INET_Addr) addresses;
  do {
    ACE_INET_Addr temp;
    temp.set_addr(copy.get_addr(), copy.get_addr_size());
    addresses.push_back(temp);
  } while (copy.next());
  return choose_single_coherent_address(addresses, prefer_loopback);
#else
  ACE_UNUSED_ARG(prefer_loopback);
  return address;
#endif // !(ACE_MAJOR_VERSION < 6 || (ACE_MAJOR_VERSION == 6 && (ACE_MINOR_VERSION < 3 || (ACE_MINOR_VERSION == 3 && ACE_MICRO_VERSION < 1))))
}

namespace {

template <typename T>
ACE_INET_Addr tie_breaker(const T& addrs, const String& name)
{
  if (!name.empty()) {
    for (typename T::const_iterator it = addrs.begin(); it != addrs.end(); ++it) {
      if (name.compare(LogAddr::host(*it)) == 0) {
        VDBG((LM_DEBUG, "(%P|%t) tie_breaker - Choosing Address %C\n",
          LogAddr(*it, LogAddr::IpPortHost).c_str()));
        return *it;
      }
    }
  }
  VDBG((LM_DEBUG, "(%P|%t) tie_breaker - Choosing Address %C\n", LogAddr(*(addrs.begin())).c_str()));
  return *addrs.begin();
}

}

ACE_INET_Addr choose_single_coherent_address(const OPENDDS_VECTOR(ACE_INET_Addr)& addresses, bool prefer_loopback, const String& name)
{
#ifdef ACE_HAS_IPV6
  OPENDDS_SET(ACE_INET_Addr) set6_loopback;
  OPENDDS_SET(ACE_INET_Addr) set6_linklocal;
  OPENDDS_SET(ACE_INET_Addr) set6_mapped_v4;
  OPENDDS_SET(ACE_INET_Addr) set6;
#endif // ACE_HAS_IPV6
  OPENDDS_SET(ACE_INET_Addr) set4_loopback;
  OPENDDS_SET(ACE_INET_Addr) set4;

  for (OPENDDS_VECTOR(ACE_INET_Addr)::const_iterator it = addresses.begin(); it != addresses.end(); ++it) {
#ifdef ACE_HAS_IPV6
    if (it->get_type() == AF_INET6 && !it->is_multicast()) {
      if (it->is_loopback()) {
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv6 LOOPBACK LIST\n", LogAddr(*it).c_str()));
        set6_loopback.insert(*it);
      } else if (it->is_ipv4_mapped_ipv6() || it->is_ipv4_compat_ipv6()) {
#ifndef IPV6_V6ONLY
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv6 MAPPED / COMPATIBLE IPv4 LIST\n", LogAddr(*it).c_str()));
        set6_mapped_v4.insert(*it);
#endif  // ! IPV6_V6ONLY
      } else if (it->is_linklocal()) {
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv6 LINK-LOCAL LIST\n", LogAddr(*it).c_str()));
        set6_linklocal.insert(*it);
      } else {
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv6 NORMAL LIST\n", LogAddr(*it).c_str()));
        set6.insert(*it);
      }
    }
#endif // ACE_HAS_IPV6
    if (it->get_type() == AF_INET && !it->is_multicast()) {
      if (it->is_loopback()) {
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv4 LOOPBACK LIST\n", LogAddr(*it).c_str()));
        set4_loopback.insert(*it);
      } else {
        VDBG((LM_DEBUG, "(%P|%t) choose_single_coherent_address(list) - "
          "Considering Address %C - ADDING TO IPv4 NORMAL LIST\n", LogAddr(*it).c_str()));
        set4.insert(*it);
      }
    }
  }

#ifdef ACE_HAS_IPV6
  if (prefer_loopback && !set6_loopback.empty()) {
    return tie_breaker(set6_loopback, name);
  }
#endif // ACE_HAS_IPV6

  if (prefer_loopback && !set4_loopback.empty()) {
    return tie_breaker(set4_loopback, name);
  }

#ifdef ACE_HAS_IPV6
  if (prefer_loopback && !set6_linklocal.empty()) {
    return tie_breaker(set6_linklocal, name);
  }
  if (!set6.empty()) {
    return tie_breaker(set6, name);
  }
  if (!set6_mapped_v4.empty()) {
    return tie_breaker(set6_mapped_v4, name);
  }
#endif // ACE_HAS_IPV6

  if (!set4.empty()) {
    return tie_breaker(set4, name);
  }

#ifdef ACE_HAS_IPV6
  if (!set6_linklocal.empty()) {
    return tie_breaker(set6_linklocal, name);
  }
  if (!set6_loopback.empty()) {
    return tie_breaker(set6_loopback, name);
  }
#endif // ACE_HAS_IPV6

  if (!set4_loopback.empty()) {
    return tie_breaker(set4_loopback, name);
  }

  if (!addresses.empty()) {
    return tie_breaker(addresses, name);
  }

  return ACE_INET_Addr();
}

ACE_INET_Addr choose_single_coherent_address(const String& address, bool prefer_loopback, bool allow_ipv4_fallback)
{
  ACE_INET_Addr result;

  if (address.empty()) {
    return ACE_INET_Addr();
  }

  String host_name_str;
  unsigned short port_number = 0;

#ifdef ACE_HAS_IPV6
  const String::size_type openb = address.find_first_of('[');
  const String::size_type closeb = address.find_first_of(']', openb);
  const String::size_type last_double = address.rfind("::", closeb);
  const String::size_type port_div = closeb != String::npos ?
                                       address.find_first_of(':', closeb + 1u) :
                                       (last_double != String::npos ?
                                         address.find_first_of(':', last_double + 2u) :
                                         address.find_last_of(':'));
#else
  const String::size_type port_div = address.find_last_of(':');
#endif

  if (port_div != String::npos) {
#ifdef ACE_HAS_IPV6
    if (openb != String::npos && closeb != String::npos) {
      host_name_str = address.substr(openb + 1u, closeb - 1u);
    } else
#endif /* ACE_HAS_IPV6 */
    {
      host_name_str = address.substr(0, port_div);
    }
    port_number = static_cast<unsigned short>(std::strtoul(address.substr(port_div + 1u).c_str(), 0, 10));
  } else {
#ifdef ACE_HAS_IPV6
    if (openb != String::npos && closeb != String::npos) {
      host_name_str = address.substr(openb + 1u, closeb - 1u);
    } else
#endif /* ACE_HAS_IPV6 */
    {
      host_name_str = address;
    }
  }

  if (host_name_str.empty()) {
    return ACE_INET_Addr();
  }

  const char* host_name = host_name_str.c_str();

  union ip46
  {
    sockaddr_in  in4_;
#ifdef ACE_HAS_IPV6
    sockaddr_in6 in6_;
#endif /* ACE_HAS_IPV6 */
  } inet_addr;
  std::memset(&inet_addr, 0, sizeof inet_addr);

  int address_family = AF_UNSPEC;

#if defined ACE_HAS_IPV6 && defined ACE_USES_IPV4_IPV6_MIGRATION
  if (address_family == AF_UNSPEC && !ACE::ipv6_enabled()) {
    address_family = AF_INET;
  }
#endif /* ACE_HAS_IPV6 && ACE_USES_IPV4_IPV6_MIGRATION */

#ifdef ACE_HAS_IPV6
  if (address_family == AF_UNSPEC && ACE::ipv6_enabled() && !allow_ipv4_fallback) {
    address_family = AF_INET6;
  }

  if (address_family != AF_INET && ACE_OS::inet_pton(AF_INET6, host_name, &inet_addr.in6_.sin6_addr) == 1) {
#ifdef ACE_HAS_SOCKADDR_IN6_SIN6_LEN
    inet_addr.in6_.sin6_len = sizeof inet_addr.in6_;
#endif /* ACE_HAS_SOCKADDR_IN6_SIN6_LEN */
    inet_addr.in6_.sin6_family = AF_INET6;
    result.set_addr(&inet_addr, sizeof inet_addr);
    result.set_port_number(port_number, 1 /*encode*/);
    return result;
  }
#else
  ACE_UNUSED_ARG(allow_ipv4_fallback);
  address_family = AF_INET;
#endif /* ACE_HAS_IPV6 */

  if (ACE_OS::inet_pton(AF_INET, host_name, &inet_addr.in4_.sin_addr) == 1) {
#ifdef ACE_HAS_SOCKADDR_IN_SIN_LEN
    inet_addr.in4_.sin_len = sizeof inet_addr.in4_;
#endif /* ACE_HAS_SOCKADDR_IN_SIN_LEN */
    inet_addr.in4_.sin_family = AF_INET;
    result.set_addr(&inet_addr, sizeof inet_addr);
    result.set_port_number(port_number, 1 /*encode*/);
    return result;
  }

  addrinfo hints;
  std::memset(&hints, 0, sizeof hints);
  hints.ai_family = address_family;

  // The ai_flags used to contain AI_ADDRCONFIG as well but that prevented
  // lookups from completing if there is no, or only a loopback, IPv6
  // interface configured. See Bugzilla 4211 for more info.

#if defined ACE_HAS_IPV6 && !defined IPV6_V6ONLY
  hints.ai_flags |= AI_V4MAPPED;
#endif

#if defined ACE_HAS_IPV6 && defined AI_ALL
  // Without AI_ALL, Windows machines exhibit inconsistent behaviors on
  // difference machines we have tested.
  hints.ai_flags |= AI_ALL;
#endif

  // Note - specify the socktype here to avoid getting multiple entries
  // returned with the same address for different socket types or
  // protocols. If this causes a problem for some reason (an address that's
  // available for TCP but not UDP, or vice-versa) this will need to change
  // back to unrestricted hints and weed out the duplicate addresses by
  // searching this->inet_addrs_ which would slow things down.
  hints.ai_socktype = SOCK_STREAM;

  addrinfo *res = 0;
  const int error = ACE_OS::getaddrinfo(host_name, 0, &hints, &res);

  if (error) {
    VDBG((LM_WARNING, "(%P|%t) choose_single_coherent_address() - Call to getaddrinfo() for hostname %C returned error: %d\n", host_name, error));
    return ACE_INET_Addr();
  }

  OPENDDS_VECTOR(ACE_INET_Addr) addresses;

#ifdef ACE_WIN32
  static ACE_Thread_Mutex addr_cache_map_mutex_;
  typedef std::pair<SystemTimePoint, OPENDDS_SET(ACE_INET_Addr)> AddrCachePair;
  typedef OPENDDS_MAP(String, AddrCachePair) AddrCacheMap;
  static AddrCacheMap addr_cache_map_;
  ACE_Guard<ACE_Thread_Mutex> g(addr_cache_map_mutex_);
  const SystemTimePoint now = SystemTimePoint::now();
  for (AddrCacheMap::iterator it = addr_cache_map_.begin(); it != addr_cache_map_.end(); /* inc in loop */) {
    if (it->second.first + TimeDuration(3, 0) < now) {
      addr_cache_map_.erase(it++);
    } else {
      ++it;
    }
  }
  AddrCacheMap::iterator it = addr_cache_map_.find(host_name);
  if (it != addr_cache_map_.end()) {
    addresses.insert(addresses.end(), it->second.second.begin(), it->second.second.end());
    it->second.first = now;
  }
#endif /* ACE_WIN32 */

  for (addrinfo* curr = res; curr; curr = curr->ai_next) {
    if (curr->ai_family != AF_INET && curr->ai_family != AF_INET6) {
      continue;
    }
    ip46 addr;
    std::memset(&addr, 0, sizeof addr);
    std::memcpy(&addr, curr->ai_addr, curr->ai_addrlen);
#ifdef ACE_HAS_IPV6
    if (curr->ai_family == AF_INET6) {
      addr.in6_.sin6_port = ACE_NTOHS(port_number);
    } else {
#endif /* ACE_HAS_IPV6 */
      addr.in4_.sin_port = ACE_NTOHS(port_number);;
#ifdef ACE_HAS_IPV6
    }
#endif /* ACE_HAS_IPV6 */

    ACE_INET_Addr temp;
    temp.set_addr(&addr, sizeof addr);
    temp.set_port_number(port_number, 1 /*encode*/);
    addresses.push_back(temp);
#ifdef ACE_WIN32
    if (it != addr_cache_map_.end()) {
      it->second.second.insert(temp);
    }
#endif /* ACE_WIN32 */
  }

#ifdef ACE_WIN32
  g.release();
#endif /* ACE_WIN32 */

  ACE_OS::freeaddrinfo(res);

  return choose_single_coherent_address(addresses, prefer_loopback, host_name);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
