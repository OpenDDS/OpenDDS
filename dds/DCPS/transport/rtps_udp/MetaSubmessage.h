/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H

#include "Rtps_Udp_Export.h"

#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Rtps_Udp_Export MetaSubmessage {
  MetaSubmessage()
    : src_guid_(GUID_UNKNOWN), dst_guid_(GUID_UNKNOWN), ignore_(false) {}
  MetaSubmessage(const RepoId& src, const RepoId& dst)
    : src_guid_(src), dst_guid_(dst), ignore_(false) {}

  void reset_destination()
  {
    dst_guid_ = GUID_UNKNOWN;
  }

  RepoId src_guid_;
  RepoId dst_guid_;
  RTPS::Submessage sm_;
  bool ignore_;
};

typedef OPENDDS_VECTOR(MetaSubmessage) MetaSubmessageVec;

/// Mark submessages that are superseded as ignored.
/// Returns the number of messages that are marked as ignored.
OpenDDS_Rtps_Udp_Export
size_t dedup(MetaSubmessageVec& vec);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_METASUBMESSAGE_H */
