/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortMulticast.h"

namespace OpenDDS {
namespace DCPS {

BestEffortMulticast::BestEffortMulticast(MulticastTransport* transport,
                                         ACE_INT32 local_peer,
                                         ACE_INT32 remote_peer)
  : MulticastDataLink(transport, local_peer, remote_peer)
{
}

void
BestEffortMulticast::sample_received(ReceivedDataSample& sample)
{
  data_received(sample);
}

bool
BestEffortMulticast::acked()
{
  return true;
}

bool
BestEffortMulticast::join_i(const ACE_INET_Addr& /*group_address*/, bool /*active*/)
{
  return true;
}

void
BestEffortMulticast::leave_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
