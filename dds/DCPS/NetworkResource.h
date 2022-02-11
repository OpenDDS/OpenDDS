/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORK_RESOURCE_H
#define OPENDDS_DCPS_NETWORK_RESOURCE_H

#include "dcps_export.h"
#include "PoolAllocator.h"

#include <dds/OpenddsDcpsExtC.h>

#include <tao/Basic_Types.h>

#include <ace/INET_Addr.h>
#include <ace/CDR_Stream.h>
#include <ace/SString.h>
#include <ace/SOCK_Dgram.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct HostnameInfo {
  size_t index_;
  String hostname_;
};

typedef OPENDDS_VECTOR(HostnameInfo) HostnameInfoVector;

/**
 * @struct NetworkResource
 *
 * @brief Defines a wrapper around address info which is used for advertise.
 *
 *
 * This is used to send/receive an address information through transport.
 */
struct OpenDDS_Dcps_Export NetworkResource {
  NetworkResource();
  explicit NetworkResource(const ACE_INET_Addr& addr);
  explicit NetworkResource(const String& addr);

  ~NetworkResource();

  void dump() const;

  /// Accessor to populate the provided ACE_INET_Addr object from the
  /// address string received through transport.
  void to_addr(ACE_INET_Addr& addr) const;

  /// Reserve byte for some feature supports in the future.
  /// e.g. version support.
  CORBA::Octet reserved_;

  /// The address in string format. e.g. ip:port, hostname:port
  String addr_;
};

/// Helper function to get the fully qualified hostname.
/// It attempts to discover the FQDN by the network interface addresses, however
/// the result is impacted by the network configuration, so it returns name in the
/// order whoever is found first - FQDN, short hostname, name resolved from loopback
/// address. In the case using short hostname or name resolved from loopback, a
/// warning is logged. If there is no any name discovered from network interfaces,
/// an error is logged.
/// If ACE_HAS_IPV6, will give priority to IPV6 interfaces
extern OpenDDS_Dcps_Export
String get_fully_qualified_hostname(ACE_INET_Addr* addr = 0);

/// Helper function to get the vector of addresses which should
/// be advertised to peers
extern OpenDDS_Dcps_Export
void get_interface_addrs(OPENDDS_VECTOR(ACE_INET_Addr)& addrs);

/// Helper function to set the ttl on a socket appropriately
/// given whether it is IPV4 or IPV6
extern OpenDDS_Dcps_Export
bool set_socket_multicast_ttl(const ACE_SOCK_Dgram& socket, const unsigned char& ttl);

/// Helper function to create dual stack socket to support IPV4 and IPV6,
/// for IPV6 builds allows for setting IPV6_V6ONLY socket option to 0 before binding
/// Otherwise defaults to opening a socket based on the type of local_address
extern OpenDDS_Dcps_Export
bool open_appropriate_socket_type(ACE_SOCK_Dgram& socket, const ACE_INET_Addr& local_address, int* proto_family = 0);

extern OpenDDS_Dcps_Export
ACE_INET_Addr choose_single_coherent_address(const OPENDDS_VECTOR(ACE_INET_Addr)& addrs, bool prefer_loopback = true, const String& name = String());

extern OpenDDS_Dcps_Export
ACE_INET_Addr choose_single_coherent_address(const ACE_INET_Addr& addr, bool prefer_loopback = true);

extern OpenDDS_Dcps_Export
ACE_INET_Addr choose_single_coherent_address(const String& hostname, bool prefer_loopback = true, bool allow_ipv4_fallback = true);

inline void assign(DDS::OctetArray16& dest,
                   ACE_CDR::ULong ipv4addr_be)
{
  std::memset(&dest[0], 0, 12);
  dest[12] = ipv4addr_be >> 24;
  dest[13] = ipv4addr_be >> 16;
  dest[14] = ipv4addr_be >> 8;
  dest[15] = ipv4addr_be;
}

inline void
address_to_bytes(DDS::OctetArray16& dest, const ACE_INET_Addr& addr)
{
  const void* raw = addr.get_addr();
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    const sockaddr_in6* in = static_cast<const sockaddr_in6*>(raw);
    std::memcpy(&dest[0], &in->sin6_addr, 16);
  } else {
#else
  {
#endif
    const sockaddr_in* in = static_cast<const sockaddr_in*>(raw);
    std::memset(&dest[0], 0, 12);
    std::memcpy(&dest[12], &in->sin_addr, 4);
  }
}

// FUTURE: Remove the map parameter.  Caller can deal with IPV6.
OpenDDS_Dcps_Export
int locator_to_address(ACE_INET_Addr& dest,
                       const Locator_t& locator,
                       bool map /*map IPV4 to IPV6 addr*/);

OpenDDS_Dcps_Export
void address_to_locator(Locator_t& locator,
                        const ACE_INET_Addr& dest);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

/// Marshal into a buffer.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkResource& value);

/// Demarshal from a buffer.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkResource& value);

ACE_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "NetworkResource.inl"
#endif  /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_NETWORK_RESOURCE_H */
