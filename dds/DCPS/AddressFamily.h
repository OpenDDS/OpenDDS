/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ADDRESSFAMILY_H
#define OPENDDS_DCPS_ADDRESSFAMILY_H

#include <dds/OpenDDSConfigWrapper.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

enum AddressFamily {
  ADDRESS_FAMILY_IPV4,
  ADDRESS_FAMILY_IPV6,
  ADDRESS_FAMILY_DUAL
};

inline bool use_ipv4(AddressFamily value)
{
  return value != ADDRESS_FAMILY_IPV6;
}

inline bool use_ipv6(AddressFamily value)
{
#ifdef ACE_HAS_IPV6
  return value != ADDRESS_FAMILY_IPV4;
#else
  (void) value;
  return false;
#endif
}

inline const char* address_family_name(AddressFamily value)
{
  switch (value) {
  case ADDRESS_FAMILY_IPV4: return "ipv4";
  case ADDRESS_FAMILY_IPV6: return "ipv6";
  case ADDRESS_FAMILY_DUAL: return "dual";
  }
  return "unknown";
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_ADDRESSFAMILY_H */
