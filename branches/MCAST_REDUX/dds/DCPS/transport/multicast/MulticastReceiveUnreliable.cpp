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
  return 0; // TODO implement
}

void
MulticastReceiveUnreliable::deliver_sample(ReceivedDataSample& sample,
                                           const ACE_INET_Addr& remote_address)
{
  // TODO implement
}

int
MulticastReceiveUnreliable::start_i()
{
  return 0; // TODO implement
}

void
MulticastReceiveUnreliable::stop_i()
{
  // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
