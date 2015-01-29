/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableSessionFactory.h"
#include "ReliableSession.h"

namespace OpenDDS {
namespace DCPS {

int
ReliableSessionFactory::requires_send_buffer() const
{
  return 1; // require send buffer
}

MulticastSession*
ReliableSessionFactory::create(ACE_Reactor* reactor,
                               ACE_thread_t owner,
                               MulticastDataLink* link,
                               MulticastPeer remote_peer)
{
  ReliableSession* session;
  ACE_NEW_RETURN(session, ReliableSession(reactor, owner, link, remote_peer), 0);
  return session;
}

} // namespace DCPS
} // namespace OpenDDS
