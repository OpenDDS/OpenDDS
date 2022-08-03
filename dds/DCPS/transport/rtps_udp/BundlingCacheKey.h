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

struct OpenDDS_Rtps_Udp_Export BundlingCacheKey {
  BundlingCacheKey(const GUID_t& dst_guid, const GUID_t& src_guid)
    : src_guid_(src_guid)
    , dst_guid_(dst_guid)
  {
  }

  BundlingCacheKey(const BundlingCacheKey& val)
    : src_guid_(val.src_guid_)
    , dst_guid_(val.dst_guid_)
  {
  }

  bool operator<(const BundlingCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(this), static_cast<const void*>(&rhs), sizeof (BundlingCacheKey)) < 0;
  }

  bool operator==(const BundlingCacheKey& rhs) const
  {
    return std::memcmp(static_cast<const void*>(this), static_cast<const void*>(&rhs), sizeof (BundlingCacheKey)) == 0;
  }

  void get_contained_guids(GuidSet& set) const
  {
    set.clear();
    set.insert(src_guid_);
    set.insert(dst_guid_);
  }

  GUID_t src_guid_;
  GUID_t dst_guid_;
};

#pragma pack(pop)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
OPENDDS_OOAT_STD_HASH(OpenDDS::DCPS::BundlingCacheKey, OpenDDS_Rtps_Udp_Export);
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_BUNDLINGCACHEKEY_H */
