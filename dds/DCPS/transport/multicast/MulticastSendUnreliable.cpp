/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSendUnreliable.h"

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

ACE_HANDLE
MulticastSendUnreliable::get_handle()
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->get_socket();
  return socket.get_handle();
}

ssize_t
MulticastSendUnreliable::send_bytes_i(const iovec iov[], int n)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->get_socket();
  return socket.send(iov, n);
}

} // namespace DCPS
} // namespace OpenDDS
