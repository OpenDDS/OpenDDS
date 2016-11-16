/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORKADDRESS_H
#define OPENDDS_DCPS_NETWORKADDRESS_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/PoolAllocator.h"

#include "tao/Basic_Types.h"

#include "ace/INET_Addr.h"
#include "ace/CDR_Stream.h"
#include "ace/SString.h"
#include "ace/SOCK_Dgram.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct HostnameInfo {
  size_t index_;
  OPENDDS_STRING hostname_;
};

typedef OPENDDS_VECTOR(HostnameInfo) HostnameInfoVector;

/**
 * @struct NetworkAddress
 *
 * @brief Defines a wrapper around address info which is used for advertise.
 *
 *
 * This is used to send/receive an address information through transport.
 */
struct OpenDDS_Dcps_Export NetworkAddress {
  NetworkAddress();
  explicit NetworkAddress(const ACE_INET_Addr& addr, bool use_hostname = false);
  explicit NetworkAddress(const OPENDDS_STRING& addr);

  ~NetworkAddress();

  void dump();

  /// Accessor to populate the provided ACE_INET_Addr object from the
  /// address string received through transport.
  void to_addr(ACE_INET_Addr& addr) const;

  /// Reserve byte for some feature supports in the future.
  /// e.g. version support.
  CORBA::Octet reserved_;

  /// The address in string format. e.g. ip:port, hostname:port
  OPENDDS_STRING addr_;
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
OPENDDS_STRING get_fully_qualified_hostname(ACE_INET_Addr* addr = 0);

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
bool open_appropriate_socket_type(ACE_SOCK_Dgram& socket, const ACE_INET_Addr& local_address);
} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

/// Marshal into a buffer.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator<< (ACE_OutputCDR& outCdr, OpenDDS::DCPS::NetworkAddress& value);

/// Demarshal from a buffer.
extern OpenDDS_Dcps_Export
ACE_CDR::Boolean
operator>> (ACE_InputCDR& inCdr, OpenDDS::DCPS::NetworkAddress& value);

ACE_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "NetworkAddress.inl"
#endif  /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_NETWORKADDRESS_H */
