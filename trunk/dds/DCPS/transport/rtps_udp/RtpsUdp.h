/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPS_UDP_H
#define DCPS_RTPS_UDP_H

#include "Rtps_Udp_Export.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Rtps_Udp_Export RtpsUdpInitializer {
public:
  RtpsUdpInitializer();
};

static RtpsUdpInitializer rtps_udp_init;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDP_H */
