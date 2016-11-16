/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSessionFactory.h"
#include "BestEffortSession.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

int
BestEffortSessionFactory::requires_send_buffer() const
{
  return 0;
}

MulticastSession*
BestEffortSessionFactory::create(ACE_Reactor* reactor,
                                 ACE_thread_t owner,
                                 MulticastDataLink* link,
                                 MulticastPeer remote_peer)
{
  BestEffortSession* session;
  ACE_NEW_RETURN(session, BestEffortSession(reactor, owner, link, remote_peer), 0);
  return session;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
