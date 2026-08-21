/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ADDRESSFAMILY_H
#define OPENDDS_DCPS_ADDRESSFAMILY_H

#include "dcps_export.h"

#include <dds/OpenDDSConfigWrapper.h>

#include <cstring>

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

inline AddressFamily default_address_family()
{
#ifdef ACE_HAS_IPV6
  return ADDRESS_FAMILY_DUAL;
#else
  return ADDRESS_FAMILY_IPV4;
#endif
}

inline bool parse_address_family(const char* value, AddressFamily& result)
{
  if (std::strcmp(value, "ipv4") == 0) {
    result = ADDRESS_FAMILY_IPV4;
    return true;
  } else if (std::strcmp(value, "ipv6") == 0) {
    result = ADDRESS_FAMILY_IPV6;
    return true;
  } else if (std::strcmp(value, "dual") == 0) {
    result = ADDRESS_FAMILY_DUAL;
    return true;
  }
  return false;
}

inline bool address_family_supported(AddressFamily value)
{
#ifdef ACE_HAS_IPV6
  (void) value;
  return true;
#else
  return value != ADDRESS_FAMILY_IPV6;
#endif
}

OpenDDS_Dcps_Export AddressFamily get_address_family(const char* config_key);

OpenDDS_Dcps_Export bool set_address_family(const char* config_key,
                                            AddressFamily value);

OpenDDS_Dcps_Export bool set_address_family(const char* config_key,
                                            const char* value);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_ADDRESSFAMILY_H */
