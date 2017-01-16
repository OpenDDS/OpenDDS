/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpSendStrategy.h"
#include "UdpDataLink.h"
#include "UdpInst.h"
#include "dds/DCPS/transport/framework/NullSynchStrategy.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

UdpSendStrategy::UdpSendStrategy(UdpDataLink* link, const TransportInst_rch& inst)
  : TransportSendStrategy(0, inst,
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>()),
    link_(link)
{
}

ssize_t
UdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.send(iov, n, this->link_->remote_address());
}

void
UdpSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
