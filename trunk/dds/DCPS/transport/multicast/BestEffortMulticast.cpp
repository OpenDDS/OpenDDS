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
BestEffortMulticast::acked()
{
  // Assume remote peer is available; this does not prevent
  // data loss if the remote peer is initially unresponsive:
  return true;
}

bool
BestEffortMulticast::header_received(const TransportHeader& /*header*/)
{
  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
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

} // namespace DCPS
} // namespace OpenDDS
