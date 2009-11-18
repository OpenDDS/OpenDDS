/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendUnreliable.h"

#include "ace/SOCK_Dgram_Mcast.h"

namespace OpenDDS {
namespace DCPS {

MulticastSendUnreliable::MulticastSendUnreliable(MulticastDataLink* link)
  : MulticastSendStrategy(link)
{
}

void
MulticastSendUnreliable::stop_i()
{
}

ssize_t
MulticastSendUnreliable::send_bytes_i(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.send(iov, n);
}

} // namespace DCPS
} // namespace OpenDDS
