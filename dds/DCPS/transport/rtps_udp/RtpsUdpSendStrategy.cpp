/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"

namespace OpenDDS {
namespace DCPS {

RtpsUdpSendStrategy::RtpsUdpSendStrategy(RtpsUdpDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy),
    link_(link)
{
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  return n;
}

void
RtpsUdpSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
