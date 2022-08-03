/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H

#include "Rtps_Udp_Export.h"

#include "ConstSharedRepoIdSet.h"

#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Rtps_Udp_Export MetaSubmessage {
  MetaSubmessage()
    : src_guid_(GUID_UNKNOWN), dst_guid_(GUID_UNKNOWN), redundant_(false) {}
  MetaSubmessage(const RepoId& src, const RepoId& dst)
    : src_guid_(src), dst_guid_(dst), redundant_(false) {}

  void reset_destination()
  {
    dst_guid_ = GUID_UNKNOWN;
  }

  RepoId src_guid_;
  RepoId dst_guid_;
  RTPS::Submessage sm_;
  bool redundant_;
};

typedef OPENDDS_VECTOR(MetaSubmessage) MetaSubmessageVec;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H */
