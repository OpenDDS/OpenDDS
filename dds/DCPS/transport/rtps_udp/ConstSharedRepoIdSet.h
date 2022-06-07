/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/RcObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct ConstSharedRepoIdSet : public virtual RcObject {
  ConstSharedRepoIdSet()
    : guids_()
#if defined ACE_HAS_CPP11
    , hash_(0)
    , valid_hash_(false)
#endif
  {}

  ConstSharedRepoIdSet(const RepoIdSet& guids)
    : guids_(guids)
#if defined ACE_HAS_CPP11
    , hash_(0)
    , valid_hash_(false)
#endif
  {}

  const RepoIdSet guids_;

#if defined ACE_HAS_CPP11
  uint32_t hash()
  {
    if (guids_.empty()) {
      return 0;
    }

    if (!valid_hash_) {
      uint32_t hash = 0;
      for (RepoIdSet::const_iterator it = guids_.begin(), limit = guids_.end(); it != limit; ++it) {
        hash = one_at_a_time_hash(reinterpret_cast<const uint8_t*>(&(*it)), sizeof (OpenDDS::DCPS::GUID_t), hash);
      }
      hash_ = hash;
      valid_hash_ = true;
    }
    return hash_;
  }

  uint32_t hash_;
  bool valid_hash_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H */
