/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableSessionFactory.h"
#include "ReliableSession.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

int
ReliableSessionFactory::requires_send_buffer() const
{
  return 1; // require send buffer
}

MulticastSession_rch
ReliableSessionFactory::create(RcHandle<EventDispatcher> event_dispatcher,
                               MulticastDataLink* link,
                               MulticastPeer remote_peer)
{
  return make_rch<ReliableSession>(event_dispatcher, link, remote_peer);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
