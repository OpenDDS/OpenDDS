/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveUnreliable.h"

namespace OpenDDS {
namespace DCPS {

MulticastReceiveUnreliable::MulticastReceiveUnreliable(MulticastDataLink* link)
  : MulticastReceiveStrategy(link)
{
}

ssize_t
MulticastReceiveUnreliable::receive_bytes(iovec iov[],
                                          int n,
                                          ACE_INET_Addr& remote_address)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->get_socket();
  return socket.recv(iov, n, remote_address);
}

void
MulticastReceiveUnreliable::deliver_sample(ReceivedDataSample& sample,
                                           const ACE_INET_Addr& /*remote_address*/)
{
  // No pre-processing; deliver data as-is:
  this->link_->data_received(sample);
}

int
MulticastReceiveUnreliable::start_i()
{
  return 0; // do nothing
}

void
MulticastReceiveUnreliable::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
