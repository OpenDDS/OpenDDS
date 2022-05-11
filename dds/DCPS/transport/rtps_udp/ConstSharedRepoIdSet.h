/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H

#include "Rtps_Udp_Export.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/RcObject.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct ConstSharedRepoIdSet : public RcObject {
  ConstSharedRepoIdSet() : guids_() {}
  ConstSharedRepoIdSet(const RepoIdSet& guids) : guids_(guids) {}

  const RepoIdSet guids_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_CONSTSHAREDREPOIDSET_H */
