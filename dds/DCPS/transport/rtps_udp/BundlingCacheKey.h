/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H

#include "Rtps_Udp_Export.h"
#include "ConstSharedRepoIdSet.h"

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

struct OpenDDS_Rtps_Udp_Export BundlingCacheKey : public virtual RcObject {
  BundlingCacheKey(const GUID_t& dst_guid, const GUID_t& src_guid, RcHandle<ConstSharedRepoIdSet> addr_guids)
    : RcObject()
    , src_guid_(src_guid)
    , dst_guid_(dst_guid)
    , addr_guids_(addr_guids)
#if defined ACE_HAS_CPP11
    , hash_(calculate_hash())
#endif
  {
  }

  BundlingCacheKey(const BundlingCacheKey& val)
    : RcObject()
    , src_guid_(val.src_guid_)
    , dst_guid_(val.dst_guid_)
    , addr_guids_(val.addr_guids_)
#if defined ACE_HAS_CPP11
    , hash_(val.hash_)
#endif
  {
  }

  bool operator<(const BundlingCacheKey& rhs) const
  {
    int r = std::memcmp(static_cast<const void*>(&src_guid_), static_cast<const void*>(&rhs.src_guid_), 2 * sizeof (GUID_t));
    if (r < 0) {
      return true;
    } else if (r == 0) {
      return addr_guids_->guids_ < rhs.addr_guids_->guids_;
    }
    return false;
  }

  bool operator==(const BundlingCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(&src_guid_), static_cast<const void*>(&rhs.src_guid_), 2 * sizeof (GUID_t)) == 0 &&
#if defined ACE_HAS_CPP11
      addr_guids_->hash() == rhs.addr_guids_->hash() &&
#endif
      addr_guids_->guids_ == rhs.addr_guids_->guids_;
  }

  BundlingCacheKey& operator=(const BundlingCacheKey& rhs)
  {
    if (this != &rhs) {
      const_cast<GUID_t&>(src_guid_) = rhs.src_guid_;
      const_cast<GUID_t&>(dst_guid_) = rhs.dst_guid_;
      addr_guids_ = rhs.addr_guids_;
    }
    return *this;
  }

  void get_contained_guids(GuidSet& set) const
  {
    set = addr_guids_->guids_;
    set.insert(src_guid_);
    set.insert(dst_guid_);
  }

  const GUID_t src_guid_;
  const GUID_t dst_guid_;
  RcHandle<ConstSharedRepoIdSet> addr_guids_;
#if defined ACE_HAS_CPP11
  const size_t hash_;

  size_t calculate_hash()
  {
    uint32_t hash = addr_guids_->hash();
    hash = OpenDDS::DCPS::one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&src_guid_), 2 * sizeof (OpenDDS::DCPS::GUID_t), hash);
    return static_cast<size_t>(hash);
  }
#endif
};

#pragma pack(pop)

} // namespace DCPS
} // namespace OpenDDS

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

} // namespace std
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H */
