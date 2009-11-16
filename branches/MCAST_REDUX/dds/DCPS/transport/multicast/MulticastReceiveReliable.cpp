/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveReliable.h"

namespace OpenDDS {
namespace DCPS {

MulticastReceiveReliable::MulticastReceiveReliable(MulticastDataLink* link)
  : MulticastReceiveStrategy(link)
{
}

ssize_t
MulticastReceiveReliable::receive_bytes(iovec iov[],
                                        int n,
                                        ACE_INET_Addr& remote_address)
{
  return 0; // TODO implement
}

MulticastReceiveReliable::TransportHeaderStatus
MulticastReceiveReliable::check_transport_header(const TransportHeader& header)
{
  return INVALID_HEADER; // TODO implement
}

void
MulticastReceiveReliable::deliver_sample(ReceivedDataSample& sample,
                                         const ACE_INET_Addr& remote_address)
{
  // TODO implement
}

int
MulticastReceiveReliable::start_i()
{
  return 0; // TODO implement
}

void
MulticastReceiveReliable::stop_i()
{
  // TODO implement
}

} // namespace DCPS
} // namespace OpenDDS
