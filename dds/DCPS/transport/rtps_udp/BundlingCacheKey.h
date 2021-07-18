/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/AddressCache.h"
#include "dds/DCPS/Hash.h"

#if defined ACE_HAS_CPP11
#include <functional>
#endif

namespace OpenDDS {
namespace DCPS {

#pragma pack(push, 1)

struct OpenDDS_Rtps_Udp_Export BundlingCacheKey {
  BundlingCacheKey(const RepoId& dst_guid, const RepoId& from_guid, const RepoIdSet& to_guids)
    : dst_guid_(dst_guid)
    , from_guid_(from_guid)
    , to_guids_(to_guids)
  {
  }

  bool operator<(const BundlingCacheKey& rhs) const
  {
    int r = std::memcmp(&dst_guid_, &rhs.dst_guid_, 2 * sizeof (RepoId));
    if (r < 0) {
      return true;
    } else if (r == 0) {
      return to_guids_ < rhs.to_guids_;
    }
    return false;
  }

  bool operator==(const BundlingCacheKey& rhs) const
  {
    return std::memcmp(this, &rhs, 2 * sizeof (RepoId)) == 0 && to_guids_ == rhs.to_guids_;
  }

  void contains(RepoIdSet& set) const
  {
    set = to_guids_;
    set.insert(dst_guid_);
    set.insert(from_guid_);
  }

  const RepoId dst_guid_;
  const RepoId from_guid_;
  const RepoIdSet to_guids_;
};

#pragma pack(pop)

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
namespace std
{

template<> struct OpenDDS_Rtps_Udp_Export hash<OpenDDS::DCPS::BundlingCacheKey>
{
  std::size_t operator()(const OpenDDS::DCPS::BundlingCacheKey& val) const noexcept
  {
    uint32_t hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&val), 2 * sizeof (OpenDDS::DCPS::RepoId));
    for (auto it = val.to_guids_.begin(); it != val.to_guids_.end(); ++it) {
      hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&(*it)), sizeof (OpenDDS::DCPS::RepoId), hash);
    }
    return static_cast<size_t>(hash);
  }
};

}
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H */
