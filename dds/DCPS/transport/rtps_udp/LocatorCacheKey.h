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

struct OpenDDS_Rtps_Udp_Export LocatorCacheKey : public RcObject {
  LocatorCacheKey(const GUID_t& remote, const GUID_t& local, bool prefer_unicast)
    : remote_(remote)
    , local_(local)
    , prefer_unicast_(prefer_unicast)
  {
  }

  LocatorCacheKey(const LocatorCacheKey& val)
    : RcObject()
    , remote_(val.remote_)
    , local_(val.local_)
    , prefer_unicast_(val.prefer_unicast_)
  {
  }

  bool operator<(const LocatorCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(&remote_), static_cast<const void*>(&rhs.remote_), 2 * sizeof (OpenDDS::DCPS::GUID_t) + sizeof (bool)) < 0;
  }

  bool operator==(const LocatorCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(&remote_), static_cast<const void*>(&rhs.remote_), 2 * sizeof (OpenDDS::DCPS::GUID_t) + sizeof (bool)) == 0;
  }

  LocatorCacheKey& operator=(const LocatorCacheKey& rhs)
  {
    if (this != &rhs) {
      const_cast<GUID_t&>(remote_) = rhs.remote_;
      const_cast<GUID_t&>(local_) = rhs.local_;
      const_cast<bool&>(prefer_unicast_) = rhs.prefer_unicast_;
    }
    return *this;
  }

  void get_contained_guids(GuidSet& set) const
  {
    set.clear();
    set.insert(remote_);
    set.insert(local_);
  }

  const GUID_t remote_;
  const GUID_t local_;
  const bool prefer_unicast_;
};

#pragma pack(pop)

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
namespace std
{

template<> struct OpenDDS_Rtps_Udp_Export hash<OpenDDS::DCPS::LocatorCacheKey>
{
  std::size_t operator()(const OpenDDS::DCPS::LocatorCacheKey& val) const noexcept
  {
    uint32_t hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&val.remote_), 2 * sizeof (OpenDDS::DCPS::GUID_t) + sizeof (bool));
    return static_cast<size_t>(hash);
  }
};

}
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_LOCATORCACHEKEY_H */
