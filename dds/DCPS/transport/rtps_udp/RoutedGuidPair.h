/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_ROUTEDGUIDPAIR_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_ROUTEDGUIDPAIR_H

#include "Rtps_Udp_Export.h"
#include <dds/DCPS/GuidUtils.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#pragma pack(push, 1)
struct OpenDDS_Rtps_Udp_Export RoutedGuidPair {
  RoutedGuidPair(const GUID_t& src, const GUID_t dst) : src_(src), dst_(dst) {}

  bool operator<(const RoutedGuidPair& rhs) const {
    return std::memcmp(static_cast<const void*>(&src_), static_cast<const void*>(&rhs.src_), 2 * sizeof(GUID_t)) < 0;
  }
  bool operator==(const RoutedGuidPair& rhs) const {
    return std::memcmp(static_cast<const void*>(&src_), static_cast<const void*>(&rhs.src_), 2 * sizeof(GUID_t)) == 0;
  }

  // Note: The comparison operators for RoutedGuidPair assume a tightly packed pair of GUIDs
  // If you intend to modify or add to these members, you will also need to update the comparison operators defined above
  GUID_t src_;
  GUID_t dst_;
};
#pragma pack(pop)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
OPENDDS_OOAT_STD_HASH(OpenDDS::DCPS::RoutedGuidPair, OpenDDS_Rtps_Udp_Export);
#endif

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_ROUTEDGUIDPAIR_H */
