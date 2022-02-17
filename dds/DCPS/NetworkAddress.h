/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_NETWORK_ADDRESS_H
#define OPENDDS_DCPS_NETWORK_ADDRESS_H

#include "dcps_export.h"

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Definitions.h"
#include "PoolAllocator.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export NetworkAddress {
public:
  NetworkAddress();
  NetworkAddress(const NetworkAddress& addr);

  explicit NetworkAddress(const char*);
  NetworkAddress(ACE_UINT16 port, const char*);

#if defined (ACE_HAS_WCHAR)
  explicit NetworkAddress(const wchar_t*);
  NetworkAddress(ACE_UINT16 port, const wchar_t*);
#endif

  explicit NetworkAddress(const ACE_INET_Addr& addr);

  NetworkAddress& operator=(const NetworkAddress& rhs);
  NetworkAddress& operator=(const ACE_INET_Addr& rhs);

  bool operator==(const NetworkAddress& rhs) const;
  bool operator!=(const NetworkAddress& rhs) const;

  bool operator<(const NetworkAddress& rhs) const;

  bool addr_bytes_equal(const NetworkAddress& rhs) const;

  ACE_INET_Addr to_addr() const;
  void to_addr(ACE_INET_Addr&) const;

  ACE_INT16 get_type() const;
  void set_type(ACE_INT16 type);

  ACE_UINT16 get_port_number() const;
  void set_port_number(ACE_UINT16 port);

  bool is_any() const;
  bool is_loopback() const;

  bool is_private() const; // IPv4 only

  bool is_linklocal() const; // IPv6 only (can't be routed)
  bool is_uniquelocal() const; // IPv6 only (only one routing domain)
  bool is_sitelocal() const; // IPv6 only (deprecated)

private:
  union ip46
  {
    sockaddr_in  in4_;
#if defined (ACE_HAS_IPV6)
    sockaddr_in6 in6_;
#endif /* ACE_HAS_IPV6 */
  } inet_addr_;
};

typedef OPENDDS_SET(NetworkAddress) AddrSet;

OpenDDS_Dcps_Export
bool is_more_local(const NetworkAddress& current, const NetworkAddress& incoming);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /*OPENDDS_DCPS_NETWORK_ADDRESS_H*/
