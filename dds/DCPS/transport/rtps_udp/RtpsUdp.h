/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDP_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDP_H

#include "Rtps_Udp_Export.h"
#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Rtps_Udp_Export RtpsUdpInitializer {
public:
  RtpsUdpInitializer();
};

static RtpsUdpInitializer rtps_udp_init;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDP_H */
