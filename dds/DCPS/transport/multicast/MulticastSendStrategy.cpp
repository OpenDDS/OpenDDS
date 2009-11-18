/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendStrategy.h"
#include "MulticastDataLink.h"
#include "MulticastConfiguration.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendStrategy::MulticastSendStrategy(MulticastDataLink* link)
  : TransportSendStrategy(link->config(),
                          0,  // synch_resource
                          link->transport_priority()),
    link_(link)
{
}

void
MulticastSendStrategy::stop_i()
{
}

ssize_t
MulticastSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.send(iov, n);
}

} // namespace DCPS
} // namespace OpenDDS
