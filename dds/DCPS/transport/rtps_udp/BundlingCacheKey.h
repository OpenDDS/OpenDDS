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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#pragma pack(push, 1)

struct OpenDDS_Rtps_Udp_Export BundlingCacheKey : public RcObject {
  BundlingCacheKey(const GUID_t& dst_guid, const GUID_t& from_guid, const GuidSet& to_guids)
    : dst_guid_(dst_guid)
    , from_guid_(from_guid)
    , to_guids_(to_guids)
#if defined ACE_HAS_CPP11
    , hash_(calculate_hash())
#endif
  {
  }

  BundlingCacheKey(const GUID_t& dst_guid, const GUID_t& from_guid, GuidSet& to_guids)
    : dst_guid_(dst_guid)
    , from_guid_(from_guid)
    , to_guids_()
#if defined ACE_HAS_CPP11
    , hash_(0)
#endif
  {
    const_cast<GuidSet&>(to_guids_).swap(to_guids);
#if defined ACE_HAS_CPP11
    const_cast<size_t&>(hash_) = calculate_hash();
#endif
  }

  BundlingCacheKey(const BundlingCacheKey& val)
    : RcObject()
    , dst_guid_(val.dst_guid_)
    , from_guid_(val.from_guid_)
    , to_guids_(val.to_guids_)
#if defined ACE_HAS_CPP11
    , hash_(val.hash_)
#endif
  {
  }

  bool operator<(const BundlingCacheKey& rhs) const
  {
    int r = std::memcmp(static_cast<const void*>(&dst_guid_), static_cast<const void*>(&rhs.dst_guid_), 2 * sizeof (GUID_t));
    if (r < 0) {
      return true;
    } else if (r == 0) {
      return to_guids_ < rhs.to_guids_;
    }
    return false;
  }

  bool operator==(const BundlingCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(&dst_guid_), static_cast<const void*>(&rhs.dst_guid_), 2 * sizeof (GUID_t)) == 0 && to_guids_ == rhs.to_guids_;
  }

  BundlingCacheKey& operator=(const BundlingCacheKey& rhs)
  {
    if (this != &rhs) {
      const_cast<GUID_t&>(dst_guid_) = rhs.dst_guid_;
      const_cast<GUID_t&>(from_guid_) = rhs.from_guid_;
      const_cast<GuidSet&>(to_guids_) = rhs.to_guids_;
    }
    return *this;
  }

  void get_contained_guids(GuidSet& set) const
  {
    set = to_guids_;
    set.insert(dst_guid_);
    set.insert(from_guid_);
  }

  const GUID_t dst_guid_;
  const GUID_t from_guid_;
  const GuidSet to_guids_;
#if defined ACE_HAS_CPP11
  const size_t hash_;

  size_t calculate_hash()
  {
    uint32_t hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&dst_guid_), 2 * sizeof (OpenDDS::DCPS::GUID_t));
    for (auto it = to_guids_.begin(); it != to_guids_.end(); ++it) {
      hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&(*it)), sizeof (OpenDDS::DCPS::GUID_t), hash);
    }
    return static_cast<size_t>(hash);
  }
#endif
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
    return val.hash_;
  }
};

}
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H */
