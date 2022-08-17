#include <ace/INET_Addr.h>
#include <ace/OS_NS_netdb.h>
#include <ace/Sock_Connect.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_string.h>
#include <ace/OS_NS_unistd.h>

#include <string>
#include <cstring>


void print_addr(const ACE_INET_Addr& addr, const char* prefix) {
  char buffer[256] = {'\0'};
  if (addr.get_host_addr(buffer, sizeof(buffer)) == 0) {
    ACE_DEBUG((LM_WARNING, "WARNING: print_addr: Failed to convert address to string\n"));
  } else {
    ACE_DEBUG((LM_DEBUG, "DEBUG: print_addr: %C %C\n", prefix, buffer));
  }
}

void hostname_to_ip(std::string address) {
  ACE_DEBUG((LM_DEBUG, "DEBUG: hostname_to_ip: Resolving IP addresses from hostname %C\n", address.c_str()));
  unsigned short port_number = 0;

  addrinfo hints;
  std::memset(&hints, 0, sizeof(hints));
#ifdef ACE_HAS_IPV6
  hints.ai_family = AF_UNSPEC;
#else
  hints.ai_family = AF_INET;
#endif

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
  const int error = ACE_OS::getaddrinfo(address.c_str(), 0, &hints, &res);
  if (error) {
    ACE_DEBUG((LM_WARNING, "WARNING: hostname_to_ip: Call to getaddrinfo() for hostname %C returned error: %d\n",
               address.c_str(), gai_strerror(error)));
    return;
  }

  for (addrinfo* curr = res; curr; curr = curr->ai_next) {
    if (curr->ai_family != AF_INET && curr->ai_family != AF_INET6) {
      ACE_DEBUG((LM_DEBUG, "DEBUG: hostname_to_ip: Encounter an address that is not AF_INET or AF_INET6\n"));
      continue;
    }

    union ip46 {
        sockaddr_in  in4_;
#ifdef ACE_HAS_IPV6
        sockaddr_in6 in6_;
#endif /* ACE_HAS_IPV6 */
    } addr;
    std::memset(&addr, 0, sizeof(addr));

#ifdef ACE_HAS_IPV6
    ACE_DEBUG((LM_DEBUG, "DEBUG: hostname_to_ip: ip46.in6_ size is %d\n", sizeof(addr.in6_)));
#endif
    ACE_DEBUG((LM_DEBUG, "DEBUG: hostname_to_ip: addr size is %d, curr addrinfo size is %d,"
               " curr->ai_addrlen is %d, curr->ai_addr size is %d, curr family is %d\n",
               sizeof(addr), sizeof(*curr), curr->ai_addrlen, sizeof(*(curr->ai_addr)), curr->ai_family));
    std::memcpy(&addr, curr->ai_addr, curr->ai_addrlen);

#ifdef ACE_HAS_IPV6
    if (curr->ai_family == AF_INET6) {
      addr.in6_.sin6_port = ACE_NTOHS(port_number);
    } else {
#endif /* ACE_HAS_IPV6 */
      addr.in4_.sin_port = ACE_NTOHS(port_number);
#ifdef ACE_HAS_IPV6
    }
#endif /* ACE_HAS_IPV6 */

    ACE_INET_Addr temp;
    temp.set_addr(&addr, sizeof(addr));
    temp.set_port_number(port_number, 1 /*encode*/);

    print_addr(temp, "==== IP address:");
  }
  ACE_OS::freeaddrinfo(res);
}

void address_info() {
  size_t addr_count;
  ACE_INET_Addr *addr_array = 0;
  const int result = ACE::get_ip_interfaces(addr_count, addr_array);

  struct Array_Guard {
    Array_Guard(ACE_INET_Addr *ptr) : ptr_(ptr) {}
    ~Array_Guard() {
      delete [] ptr_;
    }
    ACE_INET_Addr* const ptr_;
  } guardObject(addr_array);

  if (result != 0 || addr_count < 1) {
    ACE_ERROR((LM_ERROR, "ERROR: address_info: Unable to probe network interfaces\n"));
    return;
  }

  ACE_DEBUG((LM_DEBUG, "DEBUG: address_info: There are %d interfaces\n", addr_count));
  for (size_t i = 0; i < addr_count; ++i) {
    ACE_DEBUG((LM_DEBUG, "DEBUG: address_info: Considering interface %d\n", i));
    char buffer[256] = {'\0'};
    if (addr_array[i].get_host_addr(buffer, sizeof(buffer)) == 0) {
      ACE_DEBUG((LM_WARNING, "WARNING: address_info: Failed to convert address to string\n"));
    } else {
      ACE_DEBUG((LM_DEBUG, "DEBUG: address_info: Found IP interface %C\n", buffer));
    }

    // Find the hostname of the interface
    char hostname[MAXHOSTNAMELEN+1] = {'\0'};
    if (ACE::get_fqdn(addr_array[i], hostname, MAXHOSTNAMELEN+1) == 0) {
      ACE_DEBUG((LM_DEBUG, "DEBUG: address_info: IP address %C maps to hostname %C\n", buffer, hostname));
    } else {
      ACE_DEBUG((LM_WARNING, "WARNING: address_info: Failed to get FQDN\n\n"));
      continue;
    }

    // Resolve the hostname back to a list of IP addreses
    hostname_to_ip(hostname);
    ACE_DEBUG((LM_DEBUG, "\n"));
  }
  ACE_DEBUG((LM_DEBUG, "\n"));
}


int main(int argc, char* argv[]) {
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);
  const int attempts = 3;
  for (int i = 0; i < attempts; ++i) {
    ACE_DEBUG((LM_DEBUG, "========= Attempt %d....\n", i));
    address_info();
    ACE_OS::sleep((i+1)*2);
  }
  return 0;
}
