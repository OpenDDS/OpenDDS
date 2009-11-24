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
                                         MulticastPeer local_peer,
                                         MulticastPeer remote_peer)
  : MulticastDataLink(transport,
                      local_peer,
                      remote_peer)
{
}

bool
BestEffortMulticast::header_received(const TransportHeader& header)
{
  return true;
}

void
BestEffortMulticast::sample_received(ReceivedDataSample& sample)
{
  switch(sample.header_.message_id_) {
  case SAMPLE_ACK:
    ack_received(sample);
    break;

  default:
    data_received(sample);
  }
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
