/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_LOCATORCACHEKEY_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_LOCATORCACHEKEY_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/AddressCache.h"
#include "dds/DCPS/Hash.h"

#if defined ACE_HAS_CPP11
#include <functional>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#pragma pack(push, 1)

struct OpenDDS_Rtps_Udp_Export LocatorCacheKey {
  LocatorCacheKey(const GUID_t& remote, const GUID_t& local, bool prefer_unicast)
    : remote_(remote)
    , local_(local)
    , prefer_unicast_(prefer_unicast)
  {
  }

  LocatorCacheKey(const LocatorCacheKey& val)
    : remote_(val.remote_)
    , local_(val.local_)
    , prefer_unicast_(val.prefer_unicast_)
  {
  }

  bool operator<(const LocatorCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(this), static_cast<const void*>(&rhs), sizeof (LocatorCacheKey)) < 0;
  }

  bool operator==(const LocatorCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(this), static_cast<const void*>(&rhs), sizeof (LocatorCacheKey)) == 0;
  }

  void get_contained_guids(GuidSet& set) const
  {
    set.clear();
    set.insert(remote_);
    set.insert(local_);
  }

  GUID_t remote_;
  GUID_t local_;
  bool prefer_unicast_;
};

#pragma pack(pop)

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
OPENDDS_OOAT_STD_HASH(OpenDDS::DCPS::LocatorCacheKey, OpenDDS_Rtps_Udp_Export);
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_LOCATORCACHEKEY_H */
