/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSession.h"

namespace OpenDDS {
namespace DCPS {

BestEffortSession::BestEffortSession(MulticastDataLink* link,
                                     MulticastPeer remote_peer)
  : MulticastSession(link, remote_peer)
{
}

bool
BestEffortSession::acked()
{
  // Assume remote peer is available; this does not prevent
  // data loss if the peer is initially unresponsive:
  return true;
}

bool
BestEffortSession::check_header(const TransportHeader& /*header*/)
{
  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
  return true;
}

bool
BestEffortSession::check_header(const DataSampleHeader& /*header*/)
{
  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
  return true;
}

void
BestEffortSession::control_received(char /*submessage_id*/,
                                    ACE_Message_Block* /*control*/)
{
  // Ignore all transport control samples; this permits a
  // best-effort session to share the same communication
  // channel as reliable sessions.
}

bool
BestEffortSession::start(bool /*active*/)
{
  return true;
}

void
BestEffortSession::stop()
{
}

} // namespace DCPS
} // namespace OpenDDS
