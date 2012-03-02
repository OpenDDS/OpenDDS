/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"
#include "MulticastDataLink.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy),
    link_(link)
{
  // Multicast will send a SYN (TRANSPORT_CONTROL) before any reservations
  // are made on the DataLink, if the link is "release" it will be dropped.
  this->link_released(false);
}

void
MulticastSendStrategy::prepare_header_i()
{
  // Tag outgoing packets with our peer ID:
  this->header_.source_ = this->link_->local_peer();
}

ssize_t
MulticastSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.send(iov, n);
}

void
MulticastSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
