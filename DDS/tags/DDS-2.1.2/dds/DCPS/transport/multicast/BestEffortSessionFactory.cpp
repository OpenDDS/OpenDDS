/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSessionFactory.h"
#include "BestEffortSession.h"

namespace OpenDDS {
namespace DCPS {

int
BestEffortSessionFactory::requires_send_buffer() const
{
  return 0;
}

MulticastSession*
BestEffortSessionFactory::create(MulticastDataLink* link,
                                 MulticastPeer remote_peer)
{
  BestEffortSession* session;
  ACE_NEW_RETURN(session, BestEffortSession(link, remote_peer), 0);
  return session;
}

} // namespace DCPS
} // namespace OpenDDS
